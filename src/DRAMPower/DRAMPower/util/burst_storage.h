#ifndef DRAMPOWER_UTIL_BURST_STORAGE_H
#define DRAMPOWER_UTIL_BURST_STORAGE_H

#include <bitset>
#include <cstddef>
#include <vector>
#include <array>
#include <optional>
#include <type_traits>
#include <cassert>
#include <cstdint>

#include <DRAMPower/util/Serialize.h>
#include <DRAMPower/util/Deserialize.h>


namespace DRAMPower::util
{

/** This class implements a burst storage with the help of a vector of bursts.
 *  The burst storage is used to store bursts of bits.
 *  The burst is modeled with a std::bitset of size `bitset_size`.
 */
template<std::size_t bitset_size = 64>
class burst_storage : public Serialize, public Deserialize
{
public:
	using burst_t = std::bitset<bitset_size>;
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
		burst_t burst;
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

	void serializeBurst(std::ostream& stream, const burst_t& burst) const {
		// TODO think about shift direction
		std::array<uint8_t, (bitset_size + 7) / 8> burst_data;
		for (std::size_t i = 0; i < burst_data.size(); ++i) {
			burst_data[i] = 0;
			for (std::size_t j = 0; j < 8 && (i * 8 + j) < bitset_size; ++j) {
				if (burst.test(i * 8 + j)) {
					burst_data[i] |= (1 << j);
				}
			}
		}
		stream.write(reinterpret_cast<const char*>(burst_data.data()), burst_data.size());
	}
	void deserializeBurst(std::istream& stream, burst_t& burst) {
		// TODO think about shift direction
		std::array<uint8_t, (bitset_size +7) / 8> burst_data{};
		stream.read(reinterpret_cast<char*>(burst_data.data()), burst_data.size());
		for (std::size_t i = 0; i < burst_data.size(); ++i) {
			for (std::size_t j = 0; j < 8 && (i * 8 + j) < bitset_size; ++j) {
				burst.set(i * 8 + j, (burst_data[i] >> j) & 1);
			}
		}
	}

	void serialize(std::ostream& stream) const override {
		stream.write(reinterpret_cast<const char*>(&count), sizeof(count));
		std::size_t totalBursts = bursts.size();
		stream.write(reinterpret_cast<const char*>(&totalBursts), sizeof(totalBursts));
		for (const auto& burst : bursts) {
			serializeBurst(stream, burst);
		}
	}
	void deserialize(std::istream& stream) override {
		stream.read(reinterpret_cast<char*>(&count), sizeof(count));
		std::size_t totalBursts = 0;
		stream.read(reinterpret_cast<char*>(&totalBursts), sizeof(totalBursts));
		bursts.clear();
		bursts.resize(totalBursts);
		for (std::size_t i = 0; i < totalBursts; ++i) {
			deserializeBurst(stream, bursts[i]);
		}
	}
};

};

#endif /* DRAMPOWER_UTIL_BURST_STORAGE_H */
