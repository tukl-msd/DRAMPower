#ifndef DRAMPOWER_UTIL_BUS_H
#define DRAMPOWER_UTIL_BUS_H

#include <DRAMPower/util/binary_ops.h>
#include <DRAMPower/util/burst_storage.h>
#include <DRAMPower/util/bus_types.h>
#include <DRAMPower/util/pending_stats.h>
#include <DRAMPower/util/Serialize.h>
#include <DRAMPower/util/Deserialize.h>
#include <DRAMPower/Types.h>

#include <DRAMUtils/util/types.h>

#include <algorithm>

#include <cmath>
#include <cstdint>
#include <limits>
#include <cassert>
#include <limits.h>

#include <iostream>

namespace DRAMPower::util
{

template <std::size_t max_bitset_size = 0>
class Bus : public Serialize, public Deserialize {

public:
	
	using burst_storage_t = util::burst_storage<BusContainer<max_bitset_size>>;
	using burst_t = typename burst_storage_t::burst_t;
	using stats_t = bus_stats_t;

	stats_t stats;

private:
	burst_storage_t burst_storage;
	
	timestamp_t last_load = 0;
	bool enableflag = true;
	timestamp_t virtual_disable_timestamp = 0;
	std::size_t width = 0;
	uint64_t datarate = 1;
	
	PendingStats<stats_t> pending_stats;
	
	burst_t last_pattern;

	stats_t idle_stats;
	burst_t idle_pattern_burst;
	
	BusIdlePatternSpec idle_pattern;
public:
	Bus(std::size_t width, uint64_t datarate, BusIdlePatternSpec idle_pattern,
		bool enableflag = true
	) 
		: burst_storage(width)
		, enableflag(enableflag)
		, width(width)
		, datarate(datarate)
		, idle_pattern(idle_pattern)
	{
		
		// Initialize idle pattern
		switch(idle_pattern)
		{
			case BusIdlePatternSpec::L:
				this->idle_pattern_burst = burst_t();
				this->idle_pattern_burst.reset();
				break;
			case BusIdlePatternSpec::H:
				this->idle_pattern_burst = burst_t();
				this->idle_pattern_burst.reset();
				for (std::size_t i = 0; i < width; i++)
				{
					this->idle_pattern_burst.set(i, true);
				}
				break;
		}
		this->idle_stats = diff(idle_pattern_burst, idle_pattern_burst);
		// Init stats
		if (enableflag) {
			this->pending_stats.setPendingStats(0, this->idle_stats);
		}
		this->last_pattern = this->idle_pattern_burst;
	};

private:

	void advance_stats(timestamp_t virtual_timestamp, stats_t& stats) const {
		// Advance counters to new timestamp
		timestamp_t burstStorageEndTime = this->burst_storage.endTime();
		// Burst storage
		timestamp_t lastburstEnd = std::min(burstStorageEndTime, virtual_timestamp);
		for (timestamp_t t = this->last_load; 0 != lastburstEnd && t < lastburstEnd - 1; ++t) {
			stats += diff(this->at(t), this->at(t + 1)); // Last: (virtual_timestamp - 2, virtual_timestamp - 1)
		}
		// Burst to idle
		if (virtual_timestamp > burstStorageEndTime && burstStorageEndTime > 0) {
			// Burst to idle
			stats += diff(this->at(burstStorageEndTime - 1), this->at(burstStorageEndTime));
		}
		// Idle
		if (virtual_timestamp > burstStorageEndTime) {
			stats += (virtual_timestamp - burstStorageEndTime - 1) * idle_stats;
		}
	}

	void add_previous_stats(timestamp_t virtual_timestamp)
	{
		assert(this->pending_stats.getTimestamp() <= virtual_timestamp); // No interleaved commands
		// Add pending stats from last load
		if(this->pending_stats.isPending() && this->pending_stats.getTimestamp() < virtual_timestamp)
		{
			this->stats += this->pending_stats.getStats();
			this->pending_stats.clear();
		}
		advance_stats(virtual_timestamp, stats);
		// Pattern for pending stats (virtual_timestamp - 1, virtual_timestamp)
		if(virtual_timestamp > 0)
		{
			this->last_pattern = this->at(virtual_timestamp - 1);
		}
	}

	void add_data(timestamp_t virtual_timestamp, const uint8_t * data, std::size_t n_bits)
	{
		// Add new burst to storage
		BurstStorageInsertHelper::insert_data(this->burst_storage, virtual_timestamp, width, data, n_bits);

		// Adjust statistics for new data
		this->pending_stats.setPendingStats(virtual_timestamp, diff(
			this->last_pattern,
			this->at(this->last_load)
		));

		// last pattern for idle pattern
		if (this->burst_storage.size() != 0) {
			this->last_pattern = this->burst_storage.get_burst(this->burst_storage.size()-1);
		}
	}

public:
	void load(timestamp_t timestamp, const uint8_t * data, std::size_t n_bits) {
		if (!this->enableflag) {
			return;
		}
		
		// check timestamp * this->datarate no overflow
		assert(timestamp <= std::numeric_limits<timestamp_t>::max() / this->datarate);
		timestamp_t virtual_timestamp = timestamp * this->datarate;

		assert(this->last_load + burst_storage.size() <= virtual_timestamp); // No interleaved commands
		
		add_previous_stats(virtual_timestamp);
		this->last_load = virtual_timestamp;

		// Assumption: A load can only be executed with a virtual_timestamp > last_load + burst_storage.size()
		// given the assumption the burst_storage can safely be cleared
		this->burst_storage.clear();

		add_data(virtual_timestamp, data, n_bits);
	};
	
	void load(timestamp_t timestamp, uint64_t data, std::size_t burst_length) {
		const std::size_t n_bits = burst_length * width;
		
		assert(n_bits <= std::numeric_limits<uint64_t>::digits); // Ensure data fits in a uint64_t
		
		const std::size_t n_bytes = (n_bits + 7) / 8; // Round up to nearest byte
		std::array<uint8_t, 8> bytes;

		// Extract bytes in little-endian order for consistency across platforms
		for (std::size_t i = 0; i < n_bytes; ++i) {
			bytes[i] = static_cast<uint8_t>((data >> (i * 8)) & 0xFF);
		}
		this->load(timestamp, bytes.data(), n_bits);
	};

	burst_t at(timestamp_t n) const
	{
		// Assert timestamp does not lie in past
		assert(n >= last_load);
		if (n - this->last_load >= burst_storage.size()) {
			return this->idle_pattern_burst;
		}
		return this->burst_storage.get_burst(std::size_t(n - this->last_load));
	};

	// Returns the timestamp when the bus is disabled
	timestamp_t disable(timestamp_t timestamp) {
		// Already disabled
		if (!this->enableflag) {
			assert(timestamp * this->datarate >= this->virtual_disable_timestamp);
			return timestamp;
		}
		this->virtual_disable_timestamp = timestamp * this->datarate;
		assert(this->virtual_disable_timestamp  >= this->last_load);

		add_previous_stats(this->virtual_disable_timestamp);
		// Clear pending stats
		this->burst_storage.clear();

		this->last_load = this->virtual_disable_timestamp;
		this->enableflag = false;
		return this->virtual_disable_timestamp / this->datarate;
	}

	void enable(timestamp_t timestamp) {
		if (this->enableflag) {
			return;
		}
		assert(timestamp * datarate > this->virtual_disable_timestamp);
		// Shift the counters to the enabled timestamp
		this->last_load = timestamp * this->datarate;
		// Add pending stats at enable timestamp
		this->pending_stats.setPendingStats(this->last_load, idle_stats);
		this->enableflag = true;
	}

	size_t get_width() const { return width; };

	// Get stats not including timestamp t
	stats_t get_stats(timestamp_t timestamp) const 
	{

		timestamp_t t_virtual = timestamp * this->datarate;
		assert(t_virtual >= this->last_load);
		// Return empty stats for t_virtual = 0
		if(0 == t_virtual)
		{
			return this->stats;
		}

		auto stats = this->stats;

		// Add pending stats from last load
		if(this->pending_stats.isPending() && this->pending_stats.getTimestamp() < t_virtual)
		{
			stats += this->pending_stats.getStats();
		}
		if (this->enableflag) {
			advance_stats(t_virtual, stats);
		}
		return stats;
	};

	stats_t diff(burst_t high, burst_t low) const {
		stats_t stats;
		stats.ones += util::BinaryOps::popcount(low);
		stats.zeroes += width - stats.ones;
		stats.bit_changes += util::BinaryOps::bit_changes(high, low);
		stats.ones_to_zeroes += util::BinaryOps::one_to_zeroes(high, low);
		stats.zeroes_to_ones += util::BinaryOps::zero_to_ones(high, low);
		return stats;
	};

	void serialize(std::ostream& stream) const override {
		this->stats.serialize(stream);
		this->burst_storage.serialize(stream);
		stream.write(reinterpret_cast<const char*>(&this->last_load), sizeof(this->last_load));
		stream.write(reinterpret_cast<const char*>(&this->enableflag), sizeof(this->enableflag));
		stream.write(reinterpret_cast<const char*>(&this->virtual_disable_timestamp), sizeof(this->virtual_disable_timestamp));
		stream.write(reinterpret_cast<const char*>(&this->last_pattern), sizeof(this->last_pattern));
		this->pending_stats.serialize(stream);
	};

	void deserialize(std::istream& stream) override {
		this->stats.deserialize(stream);
		this->burst_storage.deserialize(stream);
		stream.read(reinterpret_cast<char*>(&this->last_load), sizeof(this->last_load));
		stream.read(reinterpret_cast<char*>(&this->enableflag), sizeof(this->enableflag));
		stream.read(reinterpret_cast<char*>(&this->virtual_disable_timestamp), sizeof(this->virtual_disable_timestamp));
		stream.read(reinterpret_cast<char*>(&this->last_pattern), sizeof(this->last_pattern));
		this->pending_stats.deserialize(stream);
	};
};

}

#endif /* DRAMPOWER_UTIL_BUS_H */
