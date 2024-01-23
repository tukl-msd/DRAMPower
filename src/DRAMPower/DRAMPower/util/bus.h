#ifndef DRAMPOWER_UTIL_BUS_H
#define DRAMPOWER_UTIL_BUS_H

#include <DRAMPower/util/binary_ops.h>
#include <DRAMPower/util/burst_storage.h>
#include <DRAMPower/Types.h>

#include <optional>

#include <bitset>
#include <cmath>
#include <cstdint>
#include <cassert>

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

// TODO: Idle state einbauen wenn Kommando fertig (done?)
class Bus {
public:
	using burst_storage_t = util::burst_storage;
	using burst_t = typename burst_storage_t::burst_t;
public:
	const std::size_t width;
public:
	bus_stats_t stats;
private:
	burst_storage_t burst_storage;
private:
	timestamp_t last_load = 0;
public:
	Bus(std::size_t width) : width(width), burst_storage(width) {};
public:
	void load(timestamp_t timestamp, const uint8_t * data, std::size_t n_bits) {

		// assume no interleaved commands appear, but the old one already finishes cleanly before the next
		//assert(this->last_load + burst_storage.size() <= timestamp);

		// Advance counters to new timestamp
		for (auto n = this->last_load; timestamp != 0 && n < timestamp - 1; n++) {
			auto stat_at = this->at(n);
			this->stats += diff(this->at(n), this->at(n + 1));
		};

		// adjust new timestamp
		burst_t old_high = burst_t(width, 0x00000000);
		if(timestamp > 0)
			old_high = this->at(timestamp - 1);

		this->last_load = timestamp;

		this->burst_storage.clear();
		this->burst_storage.insert_data(data, n_bits);

		// Adjust statistics for new data
		this->stats += diff(old_high, this->burst_storage.get_burst(0));
	};

	void load(timestamp_t timestamp, uint64_t data, std::size_t length) {
		this->load(timestamp, (uint8_t*)&data, (width * length));
	};
public:
	burst_t at(timestamp_t n) const
	{
		// Assert timestamp does not lie in past
		assert(n - last_load >= 0);

		if (n - last_load >= burst_storage.size()) {
			return burst_t(width, 0x0000); // ToDO: Configurable idle value
		}

		auto burst = this->burst_storage.get_burst(std::size_t(n - last_load));

		return burst;
	};
public:
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
public:
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
