#ifndef DRAMPOWER_UTIL_BURST_STORAGE_H
#define DRAMPOWER_UTIL_BURST_STORAGE_H

#include <bitset>
#include <vector>
#include <optional>

#include <DRAMPower/util/dynamic_bitset.h>

namespace DRAMPower::util
{

template<::std::size_t blocksize>
class burst_storage
{
public:
	using burst_t = util::dynamic_bitset<blocksize>;
	using burst_vector_t = std::vector<burst_t>;
private:
	std::size_t count = 0;
	std::size_t width = 0;
	burst_vector_t bursts;
public:
	burst_storage(std::size_t width) : width(width) {};
public:

	void insert_data(const uint8_t* data, std::size_t n_bits) {
		size_t n_bursts = n_bits / width;

		size_t burst_offset = 0;
		size_t byte_index = 0;
		size_t bit_index = 0;
		for (std::size_t i = 0; i < n_bursts; ++i) {
			// Extract bursts
			burst_t bits{width};
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
			bursts.push_back(bits);
		}
	};

	bool empty() const { return this->bursts.empty(); };
	std::size_t size() const { return this->bursts.size(); };

	burst_t get_burst(std::size_t n) const { return this->bursts[size() - 1 - n]; };

	void clear() {
		this->count = 0;
		this->bursts.clear();
	};
};

};

#endif /* DRAMPOWER_UTIL_BURST_STORAGE_H */
