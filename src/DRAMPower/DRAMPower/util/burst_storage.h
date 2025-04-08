#ifndef DRAMPOWER_UTIL_BURST_STORAGE_H
#define DRAMPOWER_UTIL_BURST_STORAGE_H

#include <bitset>
#include <vector>
#include <array>
#include <optional>
#include <type_traits>
#include <cassert>
#include <cstdint>


#include <DRAMPower/util/dynamic_bitset.h>
#include <DRAMPower/util/sub_bitset.h>

namespace DRAMPower::util
{

/** This class implements a burst storage with the help of a vector of bursts.
 *  The burst storage is used to store bursts of bits.
 *  The burst is modeled with a std::bitset of size `bitset_size`.
 */
template<std::size_t bitset_size = 64>
class burst_storage
{
public:
	using burst_t = util::sub_bitset<bitset_size>;
	using burst_storage_t = std::vector<burst_t>;

private:
	std::size_t count = 0;
	std::size_t width = 0;
	burst_storage_t bursts;
public:
	explicit burst_storage(std::size_t width) : width(width) 
	{
		assert(width > 0);
		bursts.resize(width, createBitset());
	}
public:

	inline burst_t createBitset() {
		burst_t burst{width};
		burst.reset();
		return burst;
	}

	void push_back(burst_t bits) {
		bursts.push_back(bits);
		count++;
	}

	inline burst_t& get_or_add(std::size_t index) {
		if (index >= bursts.size()) {
			bursts.resize(index + 1, createBitset());
		}
		return bursts[index];
	}

	void insert_data(const uint8_t* data, std::size_t n_bits, bool invert = false) {
		size_t n_bursts = n_bits / width;

		size_t burst_offset = 0;
		size_t byte_index = 0;
		size_t bit_index = 0;
		for (std::size_t i = 0; i < n_bursts; ++i) {
			// Extract bursts
			burst_t &bits = get_or_add(i);
			burst_offset = i * width;
			for (std::size_t j = 0; j < width && (burst_offset + j) < n_bits; ++j) {
				// Extract bit
				std::size_t bit_position = bit_index % 8;
				bool bit_value = (data[byte_index] >> bit_position) & 1;
				bits.set(j, invert ? !bit_value : bit_value);
				bit_index++;
				if (bit_index % 8 == 0) {
					++byte_index;
				}
			}
		}
		count = n_bursts;
	}

	bool empty() const { return 0 == count; };
	std::size_t size() const { return count; };

	burst_t get_burst(std::size_t n) const { return this->bursts[size() - 1 - n]; };

	void clear() {
		this->count = 0;
	}
};

};

#endif /* DRAMPOWER_UTIL_BURST_STORAGE_H */
