#ifndef DRAMPOWER_UTIL_DBI_H
#define DRAMPOWER_UTIL_DBI_H

#include "DRAMPower/Types.h"
#include "DRAMPower/util/pin_types.h"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <utility>
#include <vector>
#include <cstring>
#include <cassert>
#include <tuple>

namespace DRAMPower::util {

class DBI {
// Public type definitions
public:
    using DataBuffer_t = std::vector<uint8_t>;
    using State_t = bool; // true for inverted, false for normal
    // Callback signature void(timestamp_t load_time, timestamp_t chunk_time, std::size_t chunk_idx, bool inversion_state, bool read)
    using ChangeCallback_t = std::function<void(timestamp_t, timestamp_t, std::size_t, bool, bool)>;
    using IdlePattern_t = PinState;

// Private type definitions
private:
    using Self = DBI;
    struct LastBurst_t {
        timestamp_t start; // Start timestamp of the last burst
        timestamp_t end; // End timestamp of the last burst
        bool read; // True if the last burst was a read operation

        bool init = false;
    };

// Public contructors, destructors and assignment operators
public:
    DBI() = delete;
    DBI(const DBI&) = default;
    DBI(DBI&&) = default;
    DBI& operator=(const DBI&) = default;
    DBI& operator=(DBI&&) = default;
    ~DBI() = default;

    template<typename Func = std::nullptr_t>
    DBI(std::size_t width, IdlePattern_t idlePattern = IdlePattern_t::Z, std::size_t chunkSize = 8, Func&& func = nullptr, bool enable = false)
        : m_width(width)
        , m_idlePattern(idlePattern)
        , m_chunkSize(chunkSize)
        , m_changeCallback(std::forward<Func>(func))
        , m_enable(enable)
    {}

// Private member functions
private:
    void invertChunk(std::size_t chunk_start_bit, std::size_t chunk_end_bit);

// Public member functions
public:
    void enable(bool enable);
    bool isEnabled() const;

    const std::vector<State_t>& getInversionStateRead() const;
    const std::vector<State_t>& getInversionStateWrite() const;

    std::size_t getChunksPerWidth() const;

    void setIdlePattern(IdlePattern_t pattern);
    IdlePattern_t getIdlePattern() const;

    void resetDBI();

    template<typename Func>
    void setStateCallback(Func&& func) {
        m_changeCallback = std::forward<Func>(func);
    }

    std::optional<const uint8_t *> updateDBI(timestamp_t timestamp, std::size_t n_bits, const uint8_t* data, bool read);
    std::tuple<const uint8_t *, std::size_t> getInvertedData() const;
    void dispatchResetCallback(timestamp_t timestamp, bool read) const;
    void dispatchResetCallback(timestamp_t timestamp) const;

// Private member variables
private:
    std::size_t m_width = 0;   // Width of the complete bus
    IdlePattern_t m_idlePattern = IdlePattern_t::Z; // Default idle pattern is High-Z
    std::size_t m_lastInversionSize = 0; // Store last inversion length for toggle detection

    LastBurst_t m_lastBurst_read;  // Last burst end timestamp
    LastBurst_t m_lastBurst_write; // Last burst end timestamp

    std::size_t m_chunkSize = 8; // Size of each chunk in bits
    std::vector<State_t> m_lastInvert_read;  // Last inversion state for each pin
    std::vector<State_t> m_lastInvert_write; // Last inversion state for each pin
    DataBuffer_t m_invertedData; // Buffer for inverted data

    ChangeCallback_t m_changeCallback = nullptr; // Callback called in updateDBI
    bool m_enable = false;
};

} // namespace DRAMPower::util

#endif /* DRAMPOWER_UTIL_DBI_H */
