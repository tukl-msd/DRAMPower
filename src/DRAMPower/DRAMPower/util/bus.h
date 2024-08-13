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
#include <limits.h>

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
		Z = 2,
		CUSTOM = 3
	};

	class PendingStats
	{
		private:
		timestamp_t timestamp;
		bus_stats_t stats;
		bool 		pending;

		public:
		PendingStats() : timestamp(0), stats(), pending(false) {}
		void setPendingStats(timestamp_t timestamp, bus_stats_t stats)
		{
			this->timestamp = timestamp;
			this->stats = stats;
			this->pending = true;
		}

		bool isPending() const
		{
			return this->pending;
		}

		void clear()
		{
			this->pending = false;
		}

		timestamp_t getTimestamp() const
		{
			return this->timestamp;
		}

		bus_stats_t getStats() const
		{
			return this->stats;
		}
	};

public:
	enum class BusIdlePatternSpec
	{
		L = 0,
		H = 1,
		Z = 2,
		LAST_PATTERN = 3
	};
	enum class BusInitPatternSpec
	{
		L = 0,
		H = 1,
		Z = 2,
	};
	using burst_storage_t = util::burst_storage;
	using burst_t = typename burst_storage_t::burst_t;
	const std::size_t width;
	bus_stats_t stats;

private:
	burst_storage_t burst_storage;
	
	timestamp_t last_load = 0;
	uint64_t datarate = 1;
	bool init_load = false;
	
	PendingStats pending_stats;
	
	std::optional<burst_t> last_pattern;

	burst_t zero_pattern;
	burst_t one_pattern;
	
	BusIdlePatternSpec idle_pattern;
	BusInitPatternSpec_ init_pattern;
	std::optional<burst_t> custom_init_pattern;

private:
	Bus(std::size_t width, uint64_t datarate, BusIdlePatternSpec idle_pattern, BusInitPatternSpec_ init_pattern,
		std::optional<burst_t> custom_init_pattern = std::nullopt
	) :
		width(width), burst_storage(width), datarate(datarate),
		idle_pattern(idle_pattern), init_pattern(init_pattern), custom_init_pattern (custom_init_pattern)
	{
		static_assert(std::numeric_limits<decltype(width)>::is_signed == false, "std::size_t must be unsigned");
		
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
public: // Ensure type safety for init_pattern with 2 seperate constructors
	Bus(std::size_t width, uint64_t datarate, BusIdlePatternSpec idle_pattern, BusInitPatternSpec init_pattern)
		: Bus(width, datarate, idle_pattern, static_cast<BusInitPatternSpec_>(init_pattern)) {} // TODO alternative to static_cast ??
	
	Bus(std::size_t width, uint64_t datarate, BusIdlePatternSpec idle_pattern, burst_t custom_init_pattern)
		: Bus(width, datarate, idle_pattern, BusInitPatternSpec_::CUSTOM, custom_init_pattern) {}

	void set_idle_pattern(BusIdlePatternSpec idle_pattern)
	{
		this->idle_pattern = idle_pattern;
	}

	void load(timestamp_t timestamp, const uint8_t * data, std::size_t n_bits) {

		// assume no interleaved commands appear, but the old one already finishes cleanly before the next
		//assert(this->last_load + burst_storage.size() <= timestamp); // TODO add this assert??
		
		// check timestamp * this->datarate no overflow
		assert(timestamp <= std::numeric_limits<timestamp_t>::max() / this->datarate);
		if(timestamp > std::numeric_limits<timestamp_t>::max() / this->datarate)
		{
			std::cout << "[Error] timestamp * datarate overflows" << std::endl;
		}
		timestamp_t virtual_timestamp = timestamp * this->datarate;
		
		// Init pattern consumed on first load
		if(!this->init_load && virtual_timestamp == 0)
		{
			// init load processed
			this->init_load = true;

			// Add new burst to storage to calculate pending stats and last pattern
			this->burst_storage.clear();
			this->burst_storage.insert_data(data, n_bits);

			// Add pending init stats if not high impedance
			this->pending_stats.setPendingStats(virtual_timestamp, 
				diff(
					this->last_pattern,
					this->burst_storage.get_burst(0)
				)
			);
			
			this->last_load = virtual_timestamp;

			// last pattern for idle pattern
			this->last_pattern = this->burst_storage.get_burst(this->burst_storage.size()-1);
			return;
		}
		else if(!this->init_load)
		{
			// virtual_timestamp > 0
			// Init load then idle pattern

			// Init load processed
			this->init_load = true;

			// Use init pattern and idle pattern at t=0 for stats
			this->stats += diff(this->last_pattern, this->at(0));
		}
		
		
		// Add pending stats from last load
		// TODO implicit this->pending.getTimestamp() > virtual_timestamp -> see assume no interleaved commands appear
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
		if(virtual_timestamp > 0) // TODO implicit
		{
			this->last_pattern = this->at(virtual_timestamp - 1);
		}

		this->last_load = virtual_timestamp;

		// Add new burst to storage
		this->burst_storage.clear();
		this->burst_storage.insert_data(data, n_bits);

		// Adjust statistics for new data
		this->pending_stats.setPendingStats(virtual_timestamp, diff(
			this->last_pattern,
			this->burst_storage.get_burst(0)
		));

		// last pattern for idle pattern
		this->last_pattern = this->burst_storage.get_burst(this->burst_storage.size()-1);
	};

	void load(timestamp_t timestamp, uint64_t data, std::size_t length) {
		this->load(timestamp, (uint8_t*)&data, (width * length));
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

	auto get_width() const { return width; };

	// Get stats not including timestamp t
	auto get_stats(timestamp_t t) const 
	{
		timestamp_t virtual_t = t * this->datarate;
		assert(virtual_t >= this->last_load);
		// Return empty stats for virtual_t = 0
		if(virtual_t == 0)
		{
			return this->stats;
		}

		auto stats = this->stats;

		if(!this->init_load)
		{
			// t > 0 and no load on bus
			// Add transition from init pattern to idle pattern
			stats += diff(this->last_pattern, this->at(0));
		}
		
		// Add pending stats from last load
		if(this->pending_stats.isPending() && this->pending_stats.getTimestamp() < virtual_t)
		{
			stats += this->pending_stats.getStats();
		}

		// Advance stats to new timestamp
		for (auto n = this->last_load; n < virtual_t - 1; n++) {
			stats += diff(this->at(n), this->at(n + 1)); // Last: (timestamp - 2, timestamp - 1)
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
};

}

#endif /* DRAMPOWER_UTIL_BUS_H */
