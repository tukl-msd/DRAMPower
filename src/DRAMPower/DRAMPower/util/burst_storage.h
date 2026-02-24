#ifndef DRAMPOWER_UTIL_BURST_STORAGE_H
#define DRAMPOWER_UTIL_BURST_STORAGE_H

#include "DRAMUtils/util/types.h"

#include "DRAMPower/Types.h"
#include "DRAMPower/util/binary_ops.h"
#include "DRAMPower/util/bus_types.h"
#include "DRAMPower/util/Serialize.h"
#include "DRAMPower/util/Deserialize.h"

#include <bitset>
#include <cstddef>
#include <iostream>
#include <ostream>
#include <vector>
#include <array>
#include <cassert>
#include <cstdint>


namespace DRAMPower::util
{

template <std::size_t bitset_size>
struct BitsetContainer {
    using bitset_t = std::bitset<bitset_size>;
    bitset_t bitset{};
    std::size_t count = 0;
    timestamp_t load_time = 0;
};

template <std::size_t bitset_size>
struct BusContainer {
    using bitset_t = std::bitset<bitset_size>;
    std::vector<bitset_t> bursts{};
    std::size_t width = 0;
    std::size_t count = 0;
    timestamp_t load_time = 0;
};

// Declaration
template <typename Burst>
struct burst_storage_impl {
    static_assert(DRAMUtils::util::always_false<Burst>::value, "No specialization available for type Burst");
};


// Specializations
template <std::size_t bitset_size>
struct burst_storage_impl <BitsetContainer<bitset_size>>{
    using burst_t = bool;
    using data_t = BitsetContainer<bitset_size>;
    using bitset_t = typename data_t::bitset_t;
    using stats_t = util::bus_stats_t;

    static inline void init(data_t&) { /* No initialization necessary */}

    static inline stats_t count(const data_t& data, timestamp_t start, timestamp_t end, std::optional<burst_t> prev = std::nullopt) {
        assert(start >= data.load_time && "start timestamp must be greater or equal to load timestamp");
        std::size_t length = end - start;
        // Clamp length to count
        if (length > data.count) {
            length = data.count;
        }
        const std::size_t offset = start - data.load_time;
        assert(length + offset <= data.count && "Invalid bitset access");
        bitset_t val = data.bitset >> offset;

        // Mask calculation
        bitset_t mask;
        mask.set();
        if (length < bitset_size) {
            mask >>= (bitset_size - length);
        }

        if (0 == length) {
            mask.reset();
        }

        // Extract window depending on load, start, end
        val &= mask;

        // Stats
        bus_stats_t stats{};

        // Prev to offset toggles
        if (prev.has_value() && length != 0 && 0 == offset) {
            stats.ones_to_zeroes = prev.value() && ~val[0];
            stats.zeroes_to_ones = !prev.value() && val[0];
            stats.bit_changes += (0 != stats.ones_to_zeroes && 0 != stats.zeroes_to_ones) ? 1 : 0;
        }

        // Count ones, zeroes
        stats.ones = val.count();
        stats.zeroes = length - stats.ones;

        // Extract toggles
        bitset_t shifted = val >> 1;
        bitset_t trans_mask = mask >> 1;
        bitset_t toggles = (val ^ shifted) & trans_mask;

        // Calculate transitions
        stats.bit_changes = util::BinaryOps::popcount(toggles);
        bitset_t z2o = toggles & shifted;
        stats.zeroes_to_ones = util::BinaryOps::popcount(z2o);
        stats.ones_to_zeroes = stats.bit_changes - stats.zeroes_to_ones;

        return stats;
    }

    static inline void resize(data_t&, std::size_t size) {
        assert(size <= bitset_size && "bitset_size not enough");
    }

    static inline void push_back(data_t& data, timestamp_t load_time, burst_t bit) {
        if (load_time != data.load_time) {
            data.count = 0;
            data.load_time = load_time;
        }
        resize(data, data.count + 1);
        data.bitset.set(data.count, bit);
        data.count++;
    }

    static inline burst_t& get(data_t& data, std::size_t index) {
        assert((data.count > index) && "Invalid access");
        return data.bitset[index];
    }

    static inline burst_t get_const(const data_t& data, std::size_t n) {
        assert((data.count > n) && "Invalid access");
        return data.bitset[data.count - 1 - n];
    }

    static inline std::size_t size(const data_t& data) {
        return data.count;
    }

    static inline void clear(data_t& data) {
        data.count = 0;
        data.load_time = 0;
    }

    static inline std::size_t getCount(const data_t& data) {
        return data.count;
    }

    static inline std::size_t setCount(data_t& data, std::size_t count) {
        return data.count = count;
    }

    static inline timestamp_t endTime(const data_t& data) {
        return data.load_time + data.count;
    }

    static inline void setLoadTime(data_t& data, timestamp_t timestamp) {
        data.load_time = timestamp;
    }


    static inline void serialize(const data_t& data, std::ostream& stream) {
		stream.write(reinterpret_cast<const char*>(&data.count), sizeof(data.count));
		stream.write(reinterpret_cast<const char*>(&data.load_time), sizeof(data.load_time));
		std::array<uint8_t, (bitset_size + 7) / 8> burst_data;
		for (std::size_t i = 0; i < burst_data.size(); ++i) {
			burst_data[i] = 0;
			for (std::size_t j = 0; j < 8 && (i * 8 + j) < bitset_size; ++j) {
				if (data.bitset.test(i * 8 + j)) {
					burst_data[i] |= (1 << j);
				}
			}
		}
		stream.write(reinterpret_cast<const char*>(burst_data.data()), burst_data.size());
    }

    static inline void deserialize(data_t& data, std::istream& stream) {
		stream.read(reinterpret_cast<char*>(&data.count), sizeof(data.count));
		stream.read(reinterpret_cast<char*>(&data.load_time), sizeof(data.load_time));
        std::array<uint8_t, (bitset_size +7) / 8> burst_data{};
		stream.read(reinterpret_cast<char*>(burst_data.data()), burst_data.size());
		for (std::size_t i = 0; i < burst_data.size(); ++i) {
			for (std::size_t j = 0; j < 8 && (i * 8 + j) < bitset_size; ++j) {
				data.bitset.set(i * 8 + j, (burst_data[i] >> j) & 1);
			}
		}
    }
};

template <std::size_t bitset_size>
struct burst_storage_impl <BusContainer<bitset_size>> {
    using data_t = BusContainer<bitset_size>;
    // using data_t = BusContainer<32>;
    using bitset_t = typename data_t::bitset_t;
    using burst_t = bitset_t;
    // using bitset_t = std::bitset<32>;
    using stats_t = util::bus_stats_t;

    static inline void init(data_t& data, std::size_t width) {
        data.width = width;
    }

    static inline stats_t count(const data_t& data, timestamp_t start, timestamp_t end, std::optional<burst_t> prev = std::nullopt) {
        assert(start >= data.load_time && "start timestamp must be greater or equal to load timestamp");
        std::size_t length = end - start;
        // Clamp length to count
        if (length > data.count) {
            length = data.count;
        }

        const std::size_t offset = start - data.load_time;
        assert(length + offset <= data.bursts.size() && "Invalid access");

        // Stats
        bus_stats_t stats{};

        for (std::size_t i = offset; i < length; i++) {
            const auto& entry = data.bursts[i];
            if (0 == offset) {
                // Last Burst to this burst
                stats += diff(data, prev, entry);
            } else {
                auto ones = util::BinaryOps::popcount(entry);
                stats.ones += ones;
                stats.zeroes += data.width - ones;
                const auto& entry_prev = data.bursts[i - 1];
                stats.bit_changes += util::BinaryOps::bit_changes(entry_prev, entry);
                stats.ones_to_zeroes += util::BinaryOps::one_to_zeroes(entry_prev, entry);
                stats.zeroes_to_ones += stats.bit_changes - stats.ones_to_zeroes;
            }
        }
        return stats;
    }

    static inline void resize(data_t& data, std::size_t size) {
        if (data.bursts.size() < size) {
            data.bursts.resize(size);
        }
    }

    static inline void push_back(data_t& data, timestamp_t load_time, burst_t burst) {
        if (load_time != data.load_time) {
            data.count = 0;
            data.load_time = load_time;
        }
        resize(data, data.count + 1);
        data.bursts[data.count] = burst;
        data.count++;
    }

    static inline burst_t& get(data_t& data, std::size_t index) {
        assert((data.bursts.size() > index) && "Invalid access");
        return data.bursts[index];
    }

    static inline const burst_t& get_const(const data_t& data, std::size_t n) {
        assert((data.count > n) && "Invalid access");
        return data.bursts[data.count - 1 - n];
    }

    static inline std::size_t size(const data_t& data) {
        return data.bursts.size();
    }

    static inline void clear(data_t& data) {
        data.count = 0;
        data.load_time = 0;
    }

    static inline std::size_t getCount(const data_t& data) {
        return data.count;
    }

    static inline std::size_t setCount(data_t& data, std::size_t count) {
        return data.count = count;
    }

    static inline timestamp_t endTime(const data_t& data) {
        return data.load_time + data.count;
    }

    static inline void setLoadTime(data_t& data, timestamp_t timestamp) {
        data.load_time = timestamp;
    }

	static inline void serializeBurst(std::ostream& stream, const burst_t& burst) {
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
	static inline void deserializeBurst(std::istream& stream, burst_t& burst) {
		std::array<uint8_t, (bitset_size +7) / 8> burst_data{};
		stream.read(reinterpret_cast<char*>(burst_data.data()), burst_data.size());
		for (std::size_t i = 0; i < burst_data.size(); ++i) {
			for (std::size_t j = 0; j < 8 && (i * 8 + j) < bitset_size; ++j) {
				burst.set(i * 8 + j, (burst_data[i] >> j) & 1);
			}
		}
	}

    static inline void serialize(const data_t& data, std::ostream& stream) {
        std::size_t totalBursts = data.bursts.size();
		stream.write(reinterpret_cast<const char*>(&data.count), sizeof(data.count));
		stream.write(reinterpret_cast<const char*>(&data.load_time), sizeof(data.load_time));
        stream.write(reinterpret_cast<const char*>(&totalBursts), sizeof(totalBursts));
        for (const auto& burst : data.bursts) {
            serializeBurst(stream, burst);
        }
    }

    static inline void deserialize(data_t& data, std::istream& stream) {
        std::size_t totalBursts = 0;
		stream.read(reinterpret_cast<char*>(&data.count), sizeof(data.count));
		stream.read(reinterpret_cast<char*>(&data.load_time), sizeof(data.load_time));
		stream.read(reinterpret_cast<char*>(&totalBursts), sizeof(totalBursts));
		data.bursts.clear();
		data.bursts.resize(totalBursts);
		for (std::size_t i = 0; i < totalBursts; ++i) {
			deserializeBurst(stream, data.bursts[i]);
		}
    }
};

template<typename T>
class burst_storage : public Serialize, public Deserialize
{
public:
    using impl_data_t = T;
    using impl_t = burst_storage_impl<T>;

    using burst_t = typename impl_t::burst_t;

private:
	impl_data_t m_bursts;
public:
    template<typename... Args>
	explicit burst_storage(Args&&... args)
	{
        impl_t::init(m_bursts, std::forward<Args>(args)...);
	}
public:

	void push_back(timestamp_t load_time, burst_t burst) {
        impl_t::push_back(m_bursts, load_time, burst);
	}

    inline decltype(auto) count(timestamp_t start, timestamp_t end, std::optional<burst_t> prev = std::nullopt) const {
        return impl_t::count(m_bursts, start, end, prev);
    }

	inline burst_t& get_or_add(std::size_t index) {
        impl_t::resize(m_bursts, index + 1);
		return impl_t::get(m_bursts, index);
	}

	void setCount(std::size_t count) {
        impl_t::setCount(m_bursts, count);
	}

	bool empty() const { return 0 == impl_t::getCount(m_bursts); };
	std::size_t size() const { return impl_t::getCount(m_bursts); };

	burst_t get_burst(std::size_t n) const {
        return impl_t::get_const(m_bursts, n);
    };

    void clear() {
        impl_t::clear(m_bursts);
	}

    void setLoadTime(timestamp_t timestamp) {
        impl_t::setLoadTime(m_bursts, timestamp);
    }

    timestamp_t endTime() const {
        return impl_t::endTime(m_bursts);
    }

	void serialize(std::ostream& stream) const override {
        impl_t::serialize(m_bursts, stream);
	}
	void deserialize(std::istream& stream) override {
        impl_t::deserialize(m_bursts, stream);
	}
};



struct BurstStorageInsertHelper {

    template <std::size_t bitset_width>
    static inline void insert_data(burst_storage<BusContainer<bitset_width>>& burst_storage, timestamp_t timestamp, std::size_t width, const uint8_t* data, std::size_t n_bits, bool invert = false) {
        size_t n_bursts = n_bits / width;
        size_t burst_offset = 0;
        size_t byte_index = 0;
        size_t bit_index = 0;
        for (std::size_t i = 0; i < n_bursts; ++i) {
            // Extract bursts
            std::bitset<bitset_width> &bits = burst_storage.get_or_add(i);
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
        burst_storage.setLoadTime(timestamp);
        burst_storage.setCount(n_bursts);
    }

};

} // namespace DRAMPower::util

#endif /* DRAMPOWER_UTIL_BURST_STORAGE_H */
