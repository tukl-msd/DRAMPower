#ifndef DRAMPOWER_UTIL_BUS_EXTENSIONS_H
#define DRAMPOWER_UTIL_BUS_EXTENSIONS_H

#include <cstddef>
#include <cassert>
#include <vector>
#include <cstdint>
#include <limits>
#include <type_traits>
#include <functional>
#include <cstring>

#include <DRAMPower/Types.h>
#include <DRAMPower/util/bus_types.h>
#include <DRAMPower/util/binary_ops.h>

namespace DRAMPower::util::bus_extensions {

enum class BusHook : uint64_t {
    None        = 0,
    onLoad      = 1 << 0,
};
constexpr BusHook operator|(BusHook lhs, BusHook rhs) {
    return static_cast<BusHook>(static_cast<std::underlying_type_t<BusHook>>(lhs) |
        static_cast<std::underlying_type_t<BusHook>>(rhs));
}
constexpr BusHook operator&(BusHook lhs, BusHook rhs) {
    return static_cast<BusHook>(static_cast<std::underlying_type_t<BusHook>>(lhs) &
        static_cast<std::underlying_type_t<BusHook>>(rhs));
}
constexpr bool operator!=(BusHook lhs, size_t rhs) {
    return static_cast<std::underlying_type_t<BusHook>>(lhs) != rhs;
}

class BusExtensionDBI {
public:
    using IdlePattern_t = util::BusIdlePatternSpec;
    using PinNumber_t = std::size_t;
    // Note: The timestamp is relative to the internal bus time and datarate
    using InvertChangeCallback_t = std::function<void(timestamp_t, PinNumber_t, bool)>;

public:
    explicit BusExtensionDBI() = default;

    // supported hooks
    static constexpr BusHook getSupportedHooks() {
        return BusHook::onLoad;
    }

// Public member functions
public:
    void setWidth(std::size_t width) {
        m_width = width;
    }

    void enable(bool enable) {
        m_enable = enable;
    }

    bool isEnabled() const {
        return m_enable;
    }

    void setNumberOfDevices(std::size_t devices) {
        m_devices = devices;
    }

    void setChunkSize(std::size_t chunkSize) {
        m_chunkSize = chunkSize;
    }

    void setDataRate(std::size_t dataRate) {
        m_datarate = dataRate;
    }

    std::size_t getDBIPinNumber() const {
        return (m_devices * m_width) / m_chunkSize;
    }

    void setIdlePattern(IdlePattern_t pattern) {
        m_idlePattern = pattern;
    }

    template <typename Func>
    void setChangeCallback(Func&& callback) {
        m_invertChangeCallback = std::forward<Func>(callback);
    }

// Hook functions
public:

    // The timestamp is relative to the datarate of the bus (timestamp = t_clock * datarate_bus)
    void onLoad(timestamp_t timestamp, std::size_t n_bits, const uint8_t *datain, uint8_t *&dataout, bool &invert) {
        if (!datain || n_bits == 0 || !m_enable) return;
        if (IdlePattern_t::Z == m_idlePattern) {
            // No inversion needed because there is no termination
            invert = false;
            return;
        }
        
        // Calculate number of bytes needed to store the data
        std::size_t bytes_needed = (n_bits + 7) / 8;
        
        // Resize output buffer if needed
        if (m_invertedData.size() < bytes_needed) {
            m_invertedData.resize(bytes_needed, 0);
        }
        
        // copy data to output buffer
        std::memcpy(m_invertedData.data(), datain, bytes_needed);
        
        // Set the output pointer to the vector data (safe because the vector was resized)
        dataout = m_invertedData.data();
        
        // Initialize invert flag
        invert = false;

        // Process each chunk independently
        assert(n_bits % m_chunkSize == 0); // Ensure n_bits is a multiple of chunk size
        assert((m_devices * m_width) % m_chunkSize == 0); // Ensure burst size is a multiple of chunk size
        std::size_t n_chunks = n_bits / m_chunkSize;
        std::size_t chunksPerCompleteBus = getDBIPinNumber();
        
        // Resize lastInvert vector if needed
        if (m_lastInvert.size() < n_chunks) {
            m_lastInvert.resize(n_chunks, false);
        }
        
        // Loop through each chunk
        for (std::size_t chunk = 0; chunk < n_chunks; ++chunk) {
            std::size_t chunk_start_bit = chunk * m_chunkSize;
            std::size_t chunk_end_bit = std::min(chunk_start_bit + m_chunkSize, n_bits);
            
            // Count 0s and 1s in this chunk
            uint64_t chunk_count_0 = 0;
            uint64_t chunk_count_1 = 0;
            
            for (std::size_t bit = chunk_start_bit; bit < chunk_end_bit; ++bit) {
                // Calculate byte index and bit position within byte
                std::size_t byte_idx = bit / 8;
                std::size_t bit_pos = bit % 8;
                
                // Check if bit is set
                bool bit_value = (datain[byte_idx] & (1u << bit_pos)) != 0;
                if (bit_value) {
                    chunk_count_1++;
                } else {
                    chunk_count_0++;
                }
            }
            
            // Decide whether to invert this chunk
            bool invert_chunk = false;
            switch(m_idlePattern) {
                case IdlePattern_t::L:
                    if (chunk_count_0 < chunk_count_1) {
                        invert_chunk = true;
                    }
                    break;
                case IdlePattern_t::H:
                    if (chunk_count_1 < chunk_count_0) {
                        invert_chunk = true;
                    }
                    break;
                default:
                    assert(false);
            }
            
            // Invert the chunk if needed
            if (invert_chunk) {
                for (std::size_t bit = chunk_start_bit; bit < chunk_end_bit; ++bit) {
                    std::size_t byte_idx = bit / 8;
                    std::size_t bit_pos = bit % 8;
                    
                    // Toggle the bit in our buffer
                    m_invertedData[byte_idx] ^= (1u << bit_pos);
                }
                
                // Set the global invert flag
                invert = true;
            }
            
            // Call the callback if inversion state changed and callback is set
            if (m_lastInvert[chunk] != invert_chunk && nullptr != m_invertChangeCallback) {
                timestamp_t t = timestamp + (chunk / chunksPerCompleteBus);
                PinNumber_t pin = chunk % chunksPerCompleteBus;
                m_invertChangeCallback(t, pin, invert_chunk);
            }
            
            // Update the last inversion state
            m_lastInvert[chunk] = invert_chunk;
        }
    }
private:
    bool m_enable = false;
    std::vector<bool> m_lastInvert;
    std::vector<uint8_t> m_invertedData;
    std::size_t m_devices = 1;
    std::size_t m_datarate = 1;
    std::size_t m_width = 0;
    std::size_t m_chunkSize = 8;

    InvertChangeCallback_t m_invertChangeCallback = nullptr;

    IdlePattern_t m_idlePattern = IdlePattern_t::Z;
};

} // namespace DRAMPower::util::bus_extensions

#endif /* DRAMPOWER_UTIL_BUS_EXTENSIONS_H */
