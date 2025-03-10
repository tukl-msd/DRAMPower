#ifndef DRAMPOWER_UTIL_BURST_STORAGE_H
#define DRAMPOWER_UTIL_BURST_STORAGE_H

#include <bitset>
#include <vector>
#include <array>
#include <optional>
#include <type_traits>

#include <DRAMPower/util/dynamic_bitset.h>
#include <DRAMPower/util/sub_bitset.h>

namespace DRAMPower::util
{

/** This class implements a burst storage with the help of
 *  a vector of bursts or an array of bursts depending on the maxburst_length template
 *  parameter. If `maxburst_length = 0` the burst storage is modeled with a vector of bursts.
 *  If `maxburst_length > 0` the burst storage is modeled with an array of bursts.
 *  The burst storage is used to store bursts of bits. The burst can either be a std::bitset 
 *  or a dynamic_bitset depending on the template parameter `bitset_size`.
 *  If `bitset_size = 0` the burst is modeled with a dynamic_bitset. The blocksize of the dynamic_bitset
 *  is given by the template parameter `blocksize`.
 *  If `bitset_size > 0` the burst is modeled with a std::bitset of size `bitset_size`. The blocksize template
 *  parameter is ignored in this case.
 */
template<std::size_t blocksize, std::size_t max_bitset_size = 0, std::size_t maxburst_length = 0>
class burst_storage
{
public:
	// Select burst type based on bitset_size
	// using static_bitset_t = std::bitset<max_bitset_size>;
	using sub_bitset_t = util::sub_bitset<max_bitset_size>;
	using dynamic_bitset_t = util::dynamic_bitset<blocksize>;
	using burst_t = std::conditional_t<
		max_bitset_size != 0,
		sub_bitset_t,
		dynamic_bitset_t
	>;

	// Select storage type based on maxburst_length
	using burst_vector_t = std::vector<burst_t>;
	using burst_array_t = std::array<burst_t, maxburst_length>;
	using burst_storage_t = std::conditional_t<
		maxburst_length != 0,
		burst_array_t,
		burst_vector_t
	>;
private:
	std::size_t count = 0;
	std::size_t width = 0;
	burst_storage_t bursts;
public:
	explicit burst_storage(std::size_t width) : width(width) {}
public:

	inline burst_t createBitset() {
		return burst_t{width};
	}

	void push_back(burst_t bits) {
		if constexpr (std::is_same_v<burst_storage_t, burst_vector_t>) {
			bursts.push_back(bits);
		} else {
			// std::is_same_v<burst_storage_t, burst_array_t>
			bursts.at(count) = bits;
		}
		count++;
	}

	void insert_data(const uint8_t* data, std::size_t n_bits) {
		size_t n_bursts = n_bits / width;

		size_t burst_offset = 0;
		size_t byte_index = 0;
		size_t bit_index = 0;
		for (std::size_t i = 0; i < n_bursts; ++i) {
			// Extract bursts
			burst_t bits = createBitset();
			burst_offset = i * width;
			for (std::size_t j = 0; j < width && (burst_offset + j) < n_bits; ++j) {
				// Extract bit
				std::size_t bit_position = bit_index % 8;
				bits.set(j, (data[byte_index] >> bit_position) & 1);
				bit_index++;
				if (bit_index % 8 == 0) {
					++byte_index;
				}
			}
			push_back(bits);
		}
	}

	bool empty() const { return 0 == count; };
	std::size_t size() const { return count; };

	burst_t get_burst(std::size_t n) const { return this->bursts[size() - 1 - n]; };

	void clear() {
		this->count = 0;
		if constexpr (std::is_same_v<burst_storage_t, burst_vector_t>) {
			this->bursts.clear();
		}
	}
};

};

#endif /* DRAMPOWER_UTIL_BURST_STORAGE_H */
