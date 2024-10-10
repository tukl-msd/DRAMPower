#ifndef DRAMPOWER_UTIL_BURST_STORAGE_H
#define DRAMPOWER_UTIL_BURST_STORAGE_H

#include <bitset>
#include <vector>
#include <optional>

#include <DRAMPower/util/dynamic_bitset.h>

namespace DRAMPower::util
{

class burst_storage
{
public:
	using burst_t = util::dynamic_bitset;
	using burst_vector_t = std::vector<burst_t>;
public:
	class bitset_inserter
	{
	public:
		using bitset_t = burst_t;
	private:
		std::size_t pos = 0;
		bitset_t& bitset;
		const std::size_t N;
	public:
		bitset_inserter(bitset_t& bitset, std::size_t N)
			: bitset(bitset) , N(N) {};
	public:
		void insert(bool bit) {
			if (full())
				return;

			bitset.push_back(bit);
		};

		bool full() {
			return bitset.size() == N;
		};
	};
private:
	const std::size_t N;
	std::size_t count = 0;
	burst_vector_t bursts;
	std::optional<bitset_inserter> inserter;
public:
	burst_storage(std::size_t N) : N(N) { };
public:
	void insert_bit(bool bit) {
		if (!inserter || inserter->full()) {
			bursts.emplace_back();
			inserter.emplace(bursts.back(), N);
		};

		inserter->insert(bit);
		++count;
	};

	void insert_byte(uint8_t byte, std::size_t n_bits) {
		std::bitset<8> bitset = byte;
		for (std::size_t i = 0; i < 8 && i < n_bits; ++i) {
			this->insert_bit(bitset[i]);
		};
	};

	void insert_data(const uint8_t* data, std::size_t n_bits) {
		std::size_t bits_left = n_bits;
		for (std::size_t i = 0; i < n_bits && count < n_bits; i += 8) {
			this->insert_byte(data[i/8], bits_left >= 8 ? 8 : bits_left );
			bits_left -= 8;
		};
	};

	bool empty() const { return this->bursts.empty(); };
	std::size_t size() const { return this->bursts.size(); };

	burst_t get_burst(std::size_t n) const { return this->bursts[size() - 1 - n]; };

	void clear() {
		this->count = 0;
		this->inserter.reset();
		this->bursts.clear();
	};
};

};

#endif /* DRAMPOWER_UTIL_BURST_STORAGE_H */
