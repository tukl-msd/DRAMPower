#ifndef DRAMPOWER_UTIL_BUS_TYPES
#define DRAMPOWER_UTIL_BUS_TYPES

#include <cstdint>

#include <DRAMPower/util/Serialize.h>
#include <DRAMPower/util/Deserialize.h>

namespace DRAMPower::util {

enum class BusIdlePatternSpec
{
    L = 0,
    H = 1,
    Z = 2,
    LAST_PATTERN
};

enum class BusInitPatternSpec
{
    L = 0,
    H = 1,
    Z = 2,
};

struct bus_stats_t :public Serialize, public Deserialize {
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

	void serialize(std::ostream& stream) const override {
		stream.write(reinterpret_cast<const char*>(&ones), sizeof(ones));
		stream.write(reinterpret_cast<const char*>(&zeroes), sizeof(zeroes));
		stream.write(reinterpret_cast<const char*>(&bit_changes), sizeof(bit_changes));
		stream.write(reinterpret_cast<const char*>(&ones_to_zeroes), sizeof(ones_to_zeroes));
		stream.write(reinterpret_cast<const char*>(&zeroes_to_ones), sizeof(zeroes_to_ones));
	}
	void deserialize(std::istream& stream) override {
		stream.read(reinterpret_cast<char*>(&ones), sizeof(ones));
		stream.read(reinterpret_cast<char*>(&zeroes), sizeof(zeroes));
		stream.read(reinterpret_cast<char*>(&bit_changes), sizeof(bit_changes));
		stream.read(reinterpret_cast<char*>(&ones_to_zeroes), sizeof(ones_to_zeroes));
		stream.read(reinterpret_cast<char*>(&zeroes_to_ones), sizeof(zeroes_to_ones));
	}

	// Operator ==
	bool operator==(const bus_stats_t& rhs) const {
		return ones == rhs.ones &&
		       zeroes == rhs.zeroes &&
		       bit_changes == rhs.bit_changes &&
		       ones_to_zeroes == rhs.ones_to_zeroes &&
		       zeroes_to_ones == rhs.zeroes_to_ones;
	}
	
};

} // namespace DRAMPower::util

#endif /* DRAMPOWER_UTIL_BUS_TYPES */
