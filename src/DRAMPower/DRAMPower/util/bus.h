#ifndef DRAMPOWER_UTIL_BUS_H
#define DRAMPOWER_UTIL_BUS_H

#include <DRAMPower/util/binary_ops.h>
#include <DRAMPower/util/burst_storage.h>
#include <DRAMPower/Types.h>

#include <optional>

#include <bitset>
#include <cmath>
#include <cstdint>
#include <limits>
#include <cassert>

#include <iostream>

namespace DRAMPower::util 
{

struct bus_stats_t {
	uint64_t ones = 0;
	uint64_t zeroes = 0;
	uint64_t bit_changes = 0;
	uint64_t ones_to_zeroes = 0;
	uint64_t zeroes_to_ones = 0;

	bus_stats_t& operator+=(const bus_stats_t& rhs) {
		this->bit_changes += rhs.bit_changes;
		this->ones += rhs.ones;
		this->zeroes += rhs.zeroes;
		this->ones_to_zeroes += rhs.ones_to_zeroes;
		this->zeroes_to_ones += rhs.zeroes_to_ones;
		return *this;
	};

	bus_stats_t& operator*=(const uint64_t rhs) {
		this->bit_changes *= rhs;
		this->ones *= rhs;
		this->zeroes *= rhs;
		this->ones_to_zeroes *= rhs;
		this->zeroes_to_ones *= rhs;
		return *this;
	};

	friend bus_stats_t operator+(bus_stats_t lhs, const bus_stats_t& rhs) {
		return lhs += rhs;
	}

	friend bus_stats_t operator*(bus_stats_t lhs, const uint64_t rhs) {
		return lhs *= rhs;
	}

	friend bus_stats_t operator*(const uint64_t lhs, bus_stats_t rhs) {
		return rhs *= lhs;
	}
};

class Bus {

private:
	enum class BusInitPatternSpec_
	{
		L = 0,
		H = 1,
		CUSTOM = 2
	};

public:
	enum class BusIdlePatternSpec
	{
		L = 0,
		H = 1,
		LAST_PATTERN = 2
	};
	enum class BusInitPatternSpec
	{
		L = 0,
		H = 1,
	};
	using burst_storage_t = util::burst_storage;
	using burst_t = typename burst_storage_t::burst_t;
	const std::size_t width;
	bus_stats_t stats;

private:
	burst_storage_t burst_storage;
	
	timestamp_t last_load = 0;
	bool init_load = false;
	
	timestamp_t pending_timestamp = 0;
	bool pending = false;
	bus_stats_t pending_stats;
	
	burst_t last_pattern;
	burst_t zero_pattern;
	burst_t one_pattern;
	
	BusIdlePatternSpec idle_pattern;
	BusInitPatternSpec_ init_pattern;
	std::optional<burst_t> custom_init_pattern;

private:
	Bus(std::size_t width, BusIdlePatternSpec idle_pattern, BusInitPatternSpec_ init_pattern,
		std::optional<burst_t> custom_init_pattern = std::nullopt
	) :
		width(width), burst_storage(width), 
		idle_pattern(idle_pattern), init_pattern(init_pattern), custom_init_pattern (custom_init_pattern)
	{
			
		assert(width >= 0);
		
		// Initialize zero and one patterns
		this->zero_pattern = burst_t();
		this->one_pattern = burst_t();
		for(std::size_t i = 0; i < width; i++)
		{
			this->zero_pattern.push_back(false);
			this->one_pattern.push_back(true);
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
			case BusInitPatternSpec_::CUSTOM:
				assert(custom_init_pattern.has_value());
				assert(custom_init_pattern.value().size() == width);
				this->last_pattern = custom_init_pattern.value();
				break;
			default:
				assert(false);
		}
	};
public: // Ensure type safety for init_pattern with 2 seperate constructors
	Bus(std::size_t width, BusIdlePatternSpec idle_pattern, BusInitPatternSpec init_pattern)
		: Bus(width, idle_pattern,
		  init_pattern ==  BusInitPatternSpec::H ? BusInitPatternSpec_::H : BusInitPatternSpec_::L) {}
	
	Bus(std::size_t width, BusIdlePatternSpec idle_pattern, burst_t custom_init_pattern)
		: Bus(width, idle_pattern, BusInitPatternSpec_::CUSTOM, custom_init_pattern) {}

	void load(timestamp_t timestamp, const uint8_t * data, std::size_t n_bits) {

		// assume no interleaved commands appear, but the old one already finishes cleanly before the next
		//assert(this->last_load + burst_storage.size() <= timestamp);
		
		// Init pattern consumed on first load
		if(!this->init_load && timestamp == 0)
		{
			this->init_load = true;
			// Add new burst to storage to calculate pending stats and last pattern
			this->burst_storage.clear();
			this->burst_storage.insert_data(data, n_bits);

			// Pending stats
			this->pending_stats = diff(this->last_pattern, this->burst_storage.get_burst(0));
			this->pending_timestamp = 0;
			this->pending = true;

			this->last_load = timestamp;

			// last pattern for idle pattern	
			this->last_pattern = this->burst_storage.get_burst(this->burst_storage.size()-1);
			return;
		}
		else if(!this->init_load)
		{
			this->init_load = true;
			// timestamp > 0
			// Use idle pattern for stats
			this->stats += diff(this->last_pattern, this->at(0));
		}
		
		
		// Add pending stats from last load
		if(this->pending && this->pending_timestamp < timestamp)
		{
			this->stats += this->pending_stats;
			this->pending = false;
		}

		// Advance counters to new timestamp
		for (auto n = this->last_load; timestamp != 0 && n < timestamp - 1; n++) {
			this->stats += diff(this->at(n), this->at(n + 1)); // Last: (timestamp - 2, timestamp - 1)
		};

		// adjust new timestamp
		if(timestamp > 0) // TODO implicit
			this->last_pattern = this->at(timestamp - 1);

		this->last_load = timestamp;

		// Add new burst to storage
		this->burst_storage.clear();
		this->burst_storage.insert_data(data, n_bits);

		// Adjust statistics for new data
		this->pending_stats = diff(this->last_pattern, this->burst_storage.get_burst(0));
		this->pending_timestamp = timestamp;
		this->pending = true;

		// last pattern for idle pattern
		this->last_pattern = this->burst_storage.get_burst(this->burst_storage.size()-1);
	};

	void load(timestamp_t timestamp, uint64_t data, std::size_t length) {
		this->load(timestamp, (uint8_t*)&data, (width * length));
	};

	burst_t at(timestamp_t n) const
	{
		// Assert timestamp does not lie in past
		assert(n - last_load >= 0);

		if (n - this->last_load >= burst_storage.size()) {
			switch(this->idle_pattern)
			{
				case BusIdlePatternSpec::L:
					return this->zero_pattern;
				case BusIdlePatternSpec::H:
					return this->one_pattern;
				case BusIdlePatternSpec::LAST_PATTERN:
					return this->last_pattern;
				default:
					assert(false);
			}
		}

		auto burst = this->burst_storage.get_burst(std::size_t(n - this->last_load));

		return burst;
	};

	auto get_width() const { return width; };

	// Get stats not including timestamp t
	auto get_stats(timestamp_t t) const 
	{
		assert(t >= this->last_load);
		// Return empty stats for t = 0
		if(t == 0)
		{
			return this->stats;
		}

		auto stats = this->stats;

		if(!this->init_load)
		{
			// Add transition from init pattern to idle pattern
			stats += diff(this->last_pattern, this->at(0));
		}

		// Add pending stats from last load
		if(this->pending && this->pending_timestamp < t)
		{
			stats += this->pending_stats;
		}

		// Advance stats to new timestamp
		for (auto n = this->last_load; n < t - 1; n++) {
			stats += diff(this->at(n), this->at(n + 1)); // Last: (timestamp - 2, timestamp - 1)
		}

		return stats;
	};

	bus_stats_t diff(burst_t high, burst_t low) const {
		bus_stats_t stats;
		stats.ones += util::BinaryOps::popcount(low);
		stats.zeroes += width - stats.ones;
		stats.bit_changes += util::BinaryOps::bit_changes(high, low);
		stats.ones_to_zeroes += util::BinaryOps::one_to_zeroes(high, low);
		stats.zeroes_to_ones += util::BinaryOps::zero_to_ones(high, low);
		return stats;
	};
};

}

#endif /* DRAMPOWER_UTIL_BUS_H */
