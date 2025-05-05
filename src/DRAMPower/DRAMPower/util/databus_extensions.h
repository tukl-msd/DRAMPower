#ifndef DRAMPOWER_UTIL_DATABUS_EXTENSIONS
#define DRAMPOWER_UTIL_DATABUS_EXTENSIONS

#include <cstddef>
#include <vector>
#include <cstdint>
#include <limits>
#include <type_traits>
#include <functional>

#include <DRAMPower/util/databus_types.h>
#include <DRAMPower/Types.h>
#include <DRAMPower/util/bus_types.h>
#include <DRAMPower/util/binary_ops.h>

namespace DRAMPower::util::databus_extensions {

enum class DataBusHook : uint64_t {
    None        = 0,
    onInit      = 1 << 0,
    onLoad      = 1 << 1,
};
constexpr DataBusHook operator|(DataBusHook lhs, DataBusHook rhs) {
    return static_cast<DataBusHook>(static_cast<std::underlying_type_t<DataBusHook>>(lhs) |
        static_cast<std::underlying_type_t<DataBusHook>>(rhs));
}
constexpr DataBusHook operator&(DataBusHook lhs, DataBusHook rhs) {
    return static_cast<DataBusHook>(static_cast<std::underlying_type_t<DataBusHook>>(lhs) &
        static_cast<std::underlying_type_t<DataBusHook>>(rhs));
}
constexpr bool operator!=(DataBusHook lhs, size_t rhs) {
    return static_cast<std::underlying_type_t<DataBusHook>>(lhs) != rhs;
}

class DataBusExtensionDBI {
public:
    using IdlePattern_t = util::BusIdlePatternSpec;
    using PinNumber_t = std::size_t;
    using InvertChangeCallback_t = std::function<void(timestamp_t, PinNumber_t, bool)>;

public:
    explicit DataBusExtensionDBI() = default;

    // supported hooks
    static constexpr DataBusHook getSupportedHooks() {
        return DataBusHook::onLoad
            | DataBusHook::onInit;
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
    void onInit(std::size_t width, std::size_t devices, std::size_t datarate, std::size_t chunkSize, IdlePattern_t idlePattern, bool enable) {
        m_width = width;
        m_devices = devices;
        m_chunkSize = chunkSize;
        m_idlePattern = idlePattern;
        m_datarate = datarate;
        m_enable = enable;
        m_lastInvert.resize(getDBIPinNumber(), false);
    }

    void onLoad(timestamp_t timestamp, util::DataBusMode mode, std::size_t n_bits, const uint8_t *data, bool &invert) {
        if (!data || n_bits == 0 || util::DataBusMode::Bus != mode || !m_enable) return;
        if (IdlePattern_t::Z == m_idlePattern) {
            // No inversion needed because there is no termination
            invert = false;
            return;
        }
        invert = false;

        // Process each chunk independently
        assert(n_bits % m_chunkSize == 0);
        assert((m_devices * m_width) % m_chunkSize == 0);
        std::size_t n_chunks = n_bits / m_chunkSize;
        std::size_t chunksPerCompleteBus = getDBIPinNumber();
        
        // Make sure m_lastInvert is properly sized
        if (m_lastInvert.size() < n_chunks) {
            m_lastInvert.resize(n_chunks, false);
        }
        
        for (std::size_t chunk = 0; chunk < n_chunks; ++chunk) {
            std::size_t chunk_start_bit = chunk * m_chunkSize;
            std::size_t chunk_end_bit = std::min(chunk_start_bit + m_chunkSize, n_bits);
            //std::size_t chunk_bits = chunk_end_bit - chunk_start_bit;
            
            // Count 0s and 1s in this chunk
            uint64_t chunk_count_0 = 0;
            uint64_t chunk_count_1 = 0;
            
            for (std::size_t bit = chunk_start_bit; bit < chunk_end_bit; ++bit) {
                // Calculate byte index and bit position within byte
                std::size_t byte_idx = bit / 8;
                std::size_t bit_pos = bit % 8;
                
                // Check if bit is set
                bool bit_value = (data[byte_idx] & (1u << bit_pos)) != 0;
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
            
            m_lastInvert[chunk] = invert_chunk;
            if (m_lastInvert[chunk] != invert_chunk && nullptr != m_invertChangeCallback) {
                if (nullptr != m_invertChangeCallback) {
                    // timestamp is relative to the complete bus size and the chunk size
                    timestamp_t t = timestamp + ((chunk / chunksPerCompleteBus) * m_datarate);
                    m_invertChangeCallback(t, chunk / chunksPerCompleteBus, invert_chunk);
                }
            }
        }

        // Return if any chunk is inverted
        invert = std::any_of(m_lastInvert.begin(), m_lastInvert.end(), [](bool b) { return b; });
    }
private:
    bool m_enable = false;
    std::vector<bool> m_lastInvert;
    std::size_t m_devices = 1;
    std::size_t m_datarate = 1;
    std::size_t m_width = 0;
    std::size_t m_chunkSize = 8;

    InvertChangeCallback_t m_invertChangeCallback = nullptr;

    IdlePattern_t m_idlePattern = IdlePattern_t::Z;
};

} // namespace DRAMPower::util::databus_extensions

#endif /* DRAMPOWER_UTIL_DATABUS_EXTENSIONS */
