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

#include <optional>

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

private:
	enum class BusInitPatternSpec_
	{
		L = 0,
		H = 1,
		Z = 2,
		CUSTOM = 3
	};

public:
	
	using burst_storage_t = util::burst_storage<max_bitset_size>;
	using burst_t = typename burst_storage_t::burst_t;

	bus_stats_t stats;

private:
	burst_storage_t burst_storage;
	
	timestamp_t last_load = 0;
	bool enableflag = true;
	timestamp_t virtual_disable_timestamp = 0;
	std::size_t width = 0;
	uint64_t datarate = 1;
	bool init_load = false;
	
	PendingStats<bus_stats_t> pending_stats;
	
	std::optional<burst_t> last_pattern;

	burst_t zero_pattern;
	burst_t one_pattern;
	
	BusIdlePatternSpec idle_pattern;
	BusInitPatternSpec_ init_pattern;
	std::optional<burst_t> custom_init_pattern;

private:
	Bus(std::size_t width, uint64_t datarate, BusIdlePatternSpec idle_pattern, BusInitPatternSpec_ init_pattern,
		std::optional<burst_t> custom_init_pattern = std::nullopt
	) 
		: burst_storage(width)
		, width(width)
		, datarate(datarate)
		, idle_pattern(idle_pattern)
		, init_pattern(init_pattern)
		, custom_init_pattern (custom_init_pattern)
	{
		
		// Initialize zero and one patterns
		this->zero_pattern = burst_t();
		this->one_pattern = burst_t();
		this->zero_pattern.reset();
		for (std::size_t i = 0; i < width; i++)
		{
			this->one_pattern.set(i, true);
		}

		// Initialize last pattern and init stats
		switch(init_pattern)
		{
			case BusInitPatternSpec_::L:
				this->last_pattern = zero_pattern;
				break;
			case BusInitPatternSpec_::H:
				this->last_pattern = one_pattern;
				break;
			case BusInitPatternSpec_::Z:
				this->last_pattern = std::nullopt;
				break;
			case BusInitPatternSpec_::CUSTOM:
				// Custom init pattern with no value is equivalent to Z
				this->last_pattern = custom_init_pattern;
				if(custom_init_pattern.has_value())
				{
					assert(custom_init_pattern.value().size() == width);
				}
				else
				{
					this->init_pattern = BusInitPatternSpec_::Z;
				}
				break;
			default:
				assert(false);
				this->last_pattern = std::nullopt;
		}
	};

	static BusInitPatternSpec_ convertInitPattern(BusInitPatternSpec pattern) {
		switch(pattern) {
			case BusInitPatternSpec::L: return BusInitPatternSpec_::L;
			case BusInitPatternSpec::H: return BusInitPatternSpec_::H;
			case BusInitPatternSpec::Z: return BusInitPatternSpec_::Z;
			default: 
				assert(false); // Invalid init pattern
				return BusInitPatternSpec_::Z;
		}
	}

	void add_previous_stats(timestamp_t virtual_timestamp)
	{
		// Add pending stats from last load
		assert(this->pending_stats.getTimestamp() <= virtual_timestamp); // No interleaved commands
		if(this->pending_stats.isPending() && this->pending_stats.getTimestamp() < virtual_timestamp)
		{
			this->stats += this->pending_stats.getStats();
			this->pending_stats.clear();
		}

		// Advance counters to new timestamp
		for (auto n = this->last_load; virtual_timestamp != 0 && n < virtual_timestamp - 1; n++) {
			this->stats += diff(this->at(n), this->at(n + 1)); // Last: (virtual_timestamp - 2, virtual_timestamp - 1)
		};

		// Pattern for pending stats (virtual_timestamp - 1, virtual_timestamp)
		if(virtual_timestamp > 0)
		{
			this->last_pattern = this->at(virtual_timestamp - 1);
		}
	}

	void add_data(timestamp_t virtual_timestamp, const uint8_t * data, std::size_t n_bits)
	{
		// Add new burst to storage
		this->burst_storage.insert_data(data, n_bits);

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

public: // Ensure type safety for init_pattern with 2 seperate constructors
	Bus(std::size_t width, uint64_t datarate, BusIdlePatternSpec idle_pattern, BusInitPatternSpec init_pattern)
		: Bus(width, datarate, idle_pattern, convertInitPattern(init_pattern), std::nullopt) {}
	
	Bus(std::size_t width, uint64_t datarate, BusIdlePatternSpec idle_pattern, burst_t custom_init_pattern)
		: Bus(width, datarate, idle_pattern, BusInitPatternSpec_::CUSTOM, custom_init_pattern) {}

	void set_idle_pattern(BusIdlePatternSpec idle_pattern)
	{
		this->idle_pattern = idle_pattern;
	}

	void load(timestamp_t timestamp, const uint8_t * data, std::size_t n_bits) {
		if (!this->enableflag) {
			return;
		}
		
		// check timestamp * this->datarate no overflow
		assert(timestamp <= std::numeric_limits<timestamp_t>::max() / this->datarate);
		if(timestamp > std::numeric_limits<timestamp_t>::max() / this->datarate)
		{
			std::cout << "[Error] timestamp * datarate overflows" << std::endl;
		}
		timestamp_t virtual_timestamp = timestamp * this->datarate;

		assert(this->last_load + burst_storage.size() <= virtual_timestamp); // No interleaved commands
		
		// Init stats
		if(!this->init_load && virtual_timestamp == 0) {
			// stats added as pending_stats
			this->init_load = true;
		} else if(!this->init_load) {
			// virtual_timestamp > 0
			this->init_load = true;
			this->stats += diff(this->last_pattern, this->at(0));
		}

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

	// Returns optional burst (std::nullopt if idle pattern is Z)
	std::optional<burst_t> at(timestamp_t n) const
	{
		// Assert timestamp does not lie in past
		assert(n >= last_load);

		if (n - this->last_load >= burst_storage.size()) {
			switch(this->idle_pattern)
			{
				case BusIdlePatternSpec::L:
					return std::make_optional(this->zero_pattern);
				case BusIdlePatternSpec::H:
					return std::make_optional(this->one_pattern);
				case BusIdlePatternSpec::Z:
					return std::nullopt;
				case BusIdlePatternSpec::LAST_PATTERN:
					return this->last_pattern;
				default:
					assert(false);
					return std::nullopt;
			}
		}

		auto burst = this->burst_storage.get_burst(std::size_t(n - this->last_load));

		return std::make_optional(burst);
	};

	// Returns timestamp of last burst not including burst to idle_pattern transition
	timestamp_t get_lastburst_timestamp(bool relative_to_clock = true) const
	{
		timestamp_t lastburst = this->last_load;
		// an init load with burst can be checked with the burst_storage size
		// if there is no init_load the toggles are computed in get_stats
		if (this->burst_storage.size() != 0) {
			// for each load there is at least one burst
			// get last burst timestamp
			lastburst += this->burst_storage.size();
		}

		// no burst or pending_stats present -> no load of the bus (last_load == 0)
		if (relative_to_clock) {
			auto remainder = lastburst % this->datarate;
			if (remainder != 0) {
				lastburst += this->datarate - remainder;
			}
			return lastburst / this->datarate;
		}
		return lastburst;
	}

	// Returns the timestamp when the bus is disabled
	timestamp_t disable(timestamp_t timestamp) {
		// Already disabled
		if (!this->enableflag) {
			assert(timestamp * this->datarate >= this->virtual_disable_timestamp);
			return timestamp;
		}
		this->virtual_disable_timestamp = timestamp * this->datarate;
		assert(this->virtual_disable_timestamp  >= this->last_load);

		// Init stats
		if(!this->init_load && this->virtual_disable_timestamp == 0) {
			// tranition from init_pattern cannot be computed
			this->init_load = true;
		} else if(!this->init_load) {
			// virtual_disable_timestamp > 0
			this->init_load = true;
			this->stats += diff(this->last_pattern, this->at(0));
		}

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
		this->pending_stats.setPendingStats(this->last_load, this->diff(std::nullopt, this->at(this->last_load)));
		this->enableflag = true;
	}

	size_t get_width() const { return width; };

	// Get stats not including timestamp t
	bus_stats_t get_stats(timestamp_t timestamp) const 
	{

		timestamp_t t_virtual = timestamp * this->datarate;
		assert(t_virtual >= this->last_load);
		// Return empty stats for t_virtual = 0
		if(t_virtual == 0)
		{
			return this->stats;
		}

		auto stats = this->stats;

		if(!this->init_load)
		{
			// t > 0 and no load on bus
			// only add init_transition if the bus was enabled at t=0
			// Add transition from init pattern to idle pattern
			stats += diff(this->last_pattern, this->at(0));
		}
		
		// Add pending stats from last load
		if(this->pending_stats.isPending() && this->pending_stats.getTimestamp() < t_virtual)
		{
			stats += this->pending_stats.getStats();
		}

		// Advance stats to new timestamp if enabled
		if (this->enableflag) {
			for (auto n = this->last_load; n < t_virtual - 1; n++) {
				stats += diff(this->at(n), this->at(n + 1)); // Last: (timestamp - 2, timestamp - 1)
			}
		}

		return stats;
	};

	bus_stats_t diff(std::optional<burst_t> high, std::optional<burst_t> low) const {
		bus_stats_t stats;
		if(low.has_value())
		{
			stats.ones += util::BinaryOps::popcount(low.value());
			stats.zeroes += width - stats.ones;
		}

		if(high.has_value() && low.has_value())
		{
			stats.bit_changes += util::BinaryOps::bit_changes(high.value(), low.value());
			stats.ones_to_zeroes += util::BinaryOps::one_to_zeroes(high.value(), low.value());
			stats.zeroes_to_ones += util::BinaryOps::zero_to_ones(high.value(), low.value());
		}
		return stats;
	};

	bus_stats_t diff(std::optional<burst_t> high, burst_t low) const {
		return diff(high, std::make_optional(low));
	};

	void serialize(std::ostream& stream) const override {
		this->stats.serialize(stream);
		this->burst_storage.serialize(stream);
		stream.write(reinterpret_cast<const char*>(&this->last_load), sizeof(this->last_load));
		stream.write(reinterpret_cast<const char*>(&this->enableflag), sizeof(this->enableflag));
		stream.write(reinterpret_cast<const char*>(&this->virtual_disable_timestamp), sizeof(this->virtual_disable_timestamp));
		stream.write(reinterpret_cast<const char*>(&this->width), sizeof(this->width));
		stream.write(reinterpret_cast<const char*>(&this->datarate), sizeof(this->datarate));
		stream.write(reinterpret_cast<const char*>(&this->init_load), sizeof(this->init_load));
		this->pending_stats.serialize(stream);
		stream.write(reinterpret_cast<const char*>(&this->idle_pattern), sizeof(this->idle_pattern));
		stream.write(reinterpret_cast<const char*>(&this->init_pattern), sizeof(this->init_pattern));
		bool hasValue = this->custom_init_pattern.has_value();
		stream.write(reinterpret_cast<const char*>(&hasValue), sizeof(hasValue));
		if (hasValue) {
			this->burst_storage.serializeBurst(stream, this->custom_init_pattern.value());
		}
	};

	void deserialize(std::istream& stream) override {
		this->stats.deserialize(stream);
		this->burst_storage.deserialize(stream);
		stream.read(reinterpret_cast<char*>(&this->last_load), sizeof(this->last_load));
		stream.read(reinterpret_cast<char*>(&this->enableflag), sizeof(this->enableflag));
		stream.read(reinterpret_cast<char*>(&this->virtual_disable_timestamp), sizeof(this->virtual_disable_timestamp));
		stream.read(reinterpret_cast<char*>(&this->width), sizeof(this->width));
		stream.read(reinterpret_cast<char*>(&this->datarate), sizeof(this->datarate));
		stream.read(reinterpret_cast<char*>(&this->init_load), sizeof(this->init_load));
		this->pending_stats.deserialize(stream);
		stream.read(reinterpret_cast<char*>(&this->idle_pattern), sizeof(this->idle_pattern));
		stream.read(reinterpret_cast<char*>(&this->init_pattern), sizeof(this->init_pattern));
		bool hasValue = false;
		stream.read(reinterpret_cast<char*>(&hasValue), sizeof(hasValue));
		if (hasValue) {
			this->custom_init_pattern = burst_t();
			this->burst_storage.deserializeBurst(stream, this->custom_init_pattern.value());
		} else {
			this->custom_init_pattern = std::nullopt;
		}
	};
};

}

#endif /* DRAMPOWER_UTIL_BUS_H */
