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
	enum class _BusInitPatternSpec
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
	burst_t last_pattern;
	burst_t zero_pattern;
	burst_t one_pattern;
	BusIdlePatternSpec idle_pattern;
	_BusInitPatternSpec init_pattern;
	std::optional<burst_t> custom_init_pattern;

private:
	Bus(std::size_t width, BusIdlePatternSpec idle_pattern, _BusInitPatternSpec init_pattern,
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

			// Initialize last pattern
			switch(init_pattern)
			{
				case _BusInitPatternSpec::L:
					this->last_pattern = this->zero_pattern;
					break;
				case _BusInitPatternSpec::H:
					this->last_pattern = this->one_pattern;
					break;
				case _BusInitPatternSpec::CUSTOM:
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
		  init_pattern ==  BusInitPatternSpec::H ? _BusInitPatternSpec::H : _BusInitPatternSpec::L) {}
	
	Bus(std::size_t width, BusIdlePatternSpec idle_pattern, burst_t custom_init_pattern)
		: Bus(width, idle_pattern, _BusInitPatternSpec::CUSTOM, custom_init_pattern) {}

	void load(timestamp_t timestamp, const uint8_t * data, std::size_t n_bits) {

		// assume no interleaved commands appear, but the old one already finishes cleanly before the next
		//assert(this->last_load + burst_storage.size() <= timestamp);

		if(timestamp == 0)
			this->init_load = true;

		// Advance counters to new timestamp
		for (auto n = this->last_load; timestamp != 0 && n < timestamp - 1; n++) {
			auto stat_at = this->at(n);
			this->stats += diff(this->at(n), this->at(n + 1));
		};

		// adjust new timestamp
		if(timestamp > 0)
			this->last_pattern = this->at(timestamp - 1);

		this->last_load = timestamp;

		this->burst_storage.clear();
		this->burst_storage.insert_data(data, n_bits);

		// Adjust statistics for new data
		this->stats += diff(this->last_pattern, this->burst_storage.get_burst(0));
	};

	void load(timestamp_t timestamp, uint64_t data, std::size_t length) {
		this->load(timestamp, (uint8_t*)&data, (width * length));
	};

	burst_t at(timestamp_t n) const
	{
		// Assert timestamp does not lie in past
		assert(n - last_load >= 0);

		// Init load overrides init pattern
		if(n == 0 && !this->init_load)
			return this->last_pattern; 

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

	auto get_stats(timestamp_t t) const 
	{
		assert(t >= this->last_load);

		auto stats = this->stats;

		auto steps = t - this->last_load;

		if( steps != 0)
		for (std::size_t i = 0; i < steps; ++i) {
			auto low = this->at(last_load + i);
			auto high = this->at(last_load + i + 1);

			stats += diff(low, high);
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
