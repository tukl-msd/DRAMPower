#ifndef DRAMPOWER_UTIL_DBI_H
#define DRAMPOWER_UTIL_DBI_H

#include "DRAMPower/Types.h"
#include "DRAMPower/util/pin_types.h"
#include "DRAMPower/util/Serialize.h"
#include "DRAMPower/util/Deserialize.h"

#include "DRAMPower/util/dbialgos.h"
#include "DRAMPower/util/dbitypes.h"
#include "DRAMPower/util/dbihelpers.h"

#include <algorithm>
#include <bitset>
#include <climits>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <utility>
#include <vector>
#include <cstring>
#include <cassert>
#include <tuple>
#include <type_traits>

namespace DRAMPower::util {


// Valid types for DataType_t are -> integral types f.e. uint8_t, uint16_t, uint32_t or bitset<N>
template<typename DataType_t, std::size_t SUBCHUNKS, PinState IDLEPATTERN, typename Algorithm>
class DBI : public Serialize, public Deserialize {
static_assert(std::is_integral_v<DataType_t> || types::is_bitset_v<DataType_t>, "DataType_t must be an integral type or a std::bitset.");
// Public type definitions
public:
    using Self_t = DBI;

    using DataBuffer_t = std::vector<DataType_t>;
    using DataBufferIterator_t = typename DataBuffer_t::iterator;

    static constexpr std::size_t DATATYPEDIGITS = types::digit_count_v<DataType_t>;
    static constexpr std::size_t ChunkSize = SUBCHUNKS * DATATYPEDIGITS;

    // This limitation allows future algorithm improvements
    static_assert(types::is_contiguous_container_v<DataBuffer_t>, "DataBuffer_t must contain a buffer with contiguous data.");
    static_assert(DATATYPEDIGITS > 0, "The BufferType must contain at least one digit.");

    // Algorithm
    using Algorithm_t = Algorithm;
    using SelectedAlgorithmType_t = typename Algorithm_t::iterator_type_t;
    using IteratorType_t = typename IteratorSelector<DBI, SelectedAlgorithmType_t>::type;
    using AlgorithmPreviousConcat_t = ConcatIterator<IteratorType_t, IteratorType_t>;
    using AlgorithmPrevious_t = std::tuple<AlgorithmPreviousConcat_t, AlgorithmPreviousConcat_t>;
    using AlgorithmPreviousOptional_t = std::optional<AlgorithmPrevious_t>;


    using State_t = bool; // true for inverted, false for normal

    // Callback signature void(timestamp_t load_time, timestamp_t chunk_time, std::size_t chunk_idx, bool inversion_state, bool read)
    // chunk_time equals load_time if no bus width is provided. The chunk_idx is then absolute to the beginning of the request
    using ChangeCallback_t = std::function<void(timestamp_t, timestamp_t, std::size_t, bool, bool)>;
    
    using IdlePattern_t = PinState;

// Private type definitions
private:
    class LastBurst_t : public Serialize, public Deserialize {
    public:
        std::size_t beats() const {
            return m_end - m_start;
        }
        std::size_t chunks() const {
            return m_n_chunks;
        }
        std::size_t entries() const {
            return m_n_chunks * SUBCHUNKS;
        }
        timestamp_t end() const {
            return m_end;
        }
        bool isInitialized() const {
            return m_init;
        }
        const std::vector<bool>& getInversionState() const {
            return m_inversions;
        }
        std::vector<bool>& getInversionState() {
            return m_inversions;
        }

        void update(timestamp_t timestamp, const std::optional<std::size_t>& m_width, std::size_t m_burstLength, std::size_t n_bits) {
            m_start = timestamp;
            if (m_width.has_value()) {
                m_end = timestamp + (n_bits / m_width.value());
            } else {
                m_end = timestamp + m_burstLength;
            }
            m_n_chunks = n_bits / ChunkSize;
            m_init = true;
        }

        void serialize(std::ostream &stream) const override {
            stream.write(reinterpret_cast<const char*>(&m_start), sizeof(m_start));
            stream.write(reinterpret_cast<const char*>(&m_end), sizeof(m_end));
            stream.write(reinterpret_cast<const char*>(&m_n_chunks), sizeof(m_n_chunks));
            stream.write(reinterpret_cast<const char*>(&m_n_bits), sizeof(m_n_bits));
            stream.write(reinterpret_cast<const char*>(&m_init), sizeof(m_init));
            std::size_t m_inversionsSize = m_inversions.size();
            stream.write(reinterpret_cast<const char*>(&m_inversionsSize), sizeof(m_inversionsSize));
            for (const auto& entry : m_inversions) {
                stream.write(reinterpret_cast<const char*>(&entry), sizeof(entry));
            }
            std::size_t m_lastbeatSize = m_lastbeat.size();
            stream.write(reinterpret_cast<const char*>(&m_lastbeatSize), sizeof(m_lastbeatSize));
            for (const auto& entry : m_lastbeat) {
                stream.write(reinterpret_cast<const char*>(&entry), sizeof(entry));
            }
        }

        void deserialize(std::istream& stream) override {
            stream.read(reinterpret_cast<char*>(&m_start), sizeof(m_start));
            stream.read(reinterpret_cast<char*>(&m_end), sizeof(m_end));
            stream.read(reinterpret_cast<char*>(&m_n_chunks), sizeof(m_n_chunks));
            stream.read(reinterpret_cast<char*>(&m_n_bits), sizeof(m_n_bits));
            stream.read(reinterpret_cast<char*>(&m_init), sizeof(m_init));
            std::size_t m_inversionsSize;
            stream.read(reinterpret_cast<char*>(&m_inversionsSize), sizeof(m_inversionsSize));
            m_inversions.clear();
            m_inversions.reserve(m_inversionsSize);
            for (std::size_t i = 0; i < m_inversionsSize; ++i) {
                bool entry;
                stream.read(reinterpret_cast<char*>(&entry), sizeof(entry));
                m_inversions.push_back(entry);
            }
            std::size_t m_lastbeatSize;
            stream.read(reinterpret_cast<char*>(&m_lastbeatSize), sizeof(m_lastbeatSize));
            m_lastbeat.clear();
            m_lastbeat.reserve(m_lastbeatSize);
            for (std::size_t i = 0; i < m_lastbeatSize; ++i) {
                bool entry;
                stream.read(reinterpret_cast<char*>(&entry), sizeof(entry));
                m_lastbeat.push_back(entry);
            }
        }
    public:
        DataBuffer_t m_lastbeat{};
        bool m_init = false;
    private:
        timestamp_t m_start = 0; // Start timestamp of the last burst
        timestamp_t m_end = 0; // End timestamp of the last burst
        std::size_t m_n_chunks = 0;
        std::size_t m_n_bits = 0;
        std::vector<bool> m_inversions;
    };

// Public contructors, destructors and assignment operators
public:
    DBI() = delete;
    DBI(const DBI&) = default;
    DBI(DBI&&) = default;
    DBI& operator=(const DBI&) = delete;
    DBI& operator=(DBI&&) = delete;
    ~DBI() = default;

    template<typename T>
    constexpr std::enable_if_t<std::is_integral_v<T>> set_all_bits(bool ones, T& out_val) const {
        out_val = ones ? ~T{0} : T{0};
    }

    template<std::size_t N>
    constexpr void set_all_bits(bool ones, std::bitset<N>& out_val) const {
        out_val = ones ? std::bitset<N>{}.set() : std::bitset<N>{};
    }

    DataBuffer_t getIdlePattern(std::size_t busWidth) const {
        DataType_t data{};
        if constexpr (PinState::H == IDLEPATTERN) {
            set_all_bits(true, data);
        } else {
            // (PinState::L == IDLEPATTERN) || (PinState::Z == IDLEPATTERN)
            set_all_bits(false, data);
        }
        // Size: SubChunks per busWidth
        return DataBuffer_t(static_cast<std::size_t>(busWidth / DATATYPEDIGITS), data);
    }

    template<typename Func = std::nullptr_t>
    DBI (std::optional<std::size_t> busWidth, std::size_t burstLength, Func&& func = nullptr, bool enable = false)
        : m_width(busWidth)
        , m_burstLength(burstLength)
        , m_idlePattern(IDLEPATTERN)
        , m_idleData(busWidth.has_value() ? std::optional<DataBuffer_t>{getIdlePattern(busWidth.value())} : std::nullopt)
        , m_changeCallback(std::forward<Func>(func))
        , m_enable(enable)
    {
        assert(!busWidth.has_value() || 0 == (busWidth.value() % ChunkSize) && "busWidth must be a multiple of CHUNKSIZE");
    }

// Private member functions
private:
    void invertChunk(std::size_t chunk_start) {
        std::size_t subchunk_start = chunk_start * SUBCHUNKS;
        for (std::size_t subchunk = subchunk_start; subchunk < subchunk_start + SUBCHUNKS; ++subchunk) {
            m_invertedData[subchunk] = ~m_invertedData[subchunk];
        }
    }

// Public member functions
public:
    void enable(bool enable) {
        m_enable = enable;
    }
    bool isEnabled() const {
        return m_enable;
    }

    const std::vector<bool>& getInversionStateRead() const {
        return m_lastBurst_read.getInversionState();
    }
    std::size_t getInversionSizeRead() const {
        return m_lastBurst_read.entries();
    }
    const std::vector<bool>& getInversionStateWrite() const {
        return m_lastBurst_write.getInversionState();
    }
    
    std::size_t getInversionSizeWrite() const {
        return m_lastBurst_write.entries();
    }

    std::optional<std::size_t> getChunksPerWidth() const {
        return m_width.has_value() ? std::optional<std::size_t>(m_width.value() / ChunkSize) : std::nullopt;
    }

    IdlePattern_t getIdlePattern() const {
        return m_idlePattern;
    }

    template<typename Func>
    void setChangeCallback(Func&& func) {
        m_changeCallback = std::forward<Func>(func);
    }

    std::optional<const DataType_t*> updateDBI(timestamp_t timestamp, std::size_t n_bits, const DataType_t* data, bool read) {
        if (!data || 0 == n_bits || !m_enable || IdlePattern_t::Z == m_idlePattern) return std::nullopt;

        // Select read or write members
        auto &lastInvert = read ? m_lastBurst_read.getInversionState() : m_lastBurst_write.getInversionState();
        auto &m_lastBurst = read ? m_lastBurst_read : m_lastBurst_write;

        // Reset to Idle Pattern if this burst and the last burst are not seamless
        dispatchResetCallback(timestamp, read);
        
        // Calculate number of bytes needed to store the data
        assert(0 == (n_bits % DATATYPEDIGITS) && "n_bits must be a multiple of DATATYPEDIGITS");
        std::size_t entries_needed = (n_bits + DATATYPEDIGITS - 1) / DATATYPEDIGITS;
        
        // SAFETY: the resizing is necessary so there are no data copies during dbi calculation
        // Resize output buffer if needed
        if (m_invertedData.capacity() < entries_needed) {
            m_invertedData.resize(entries_needed, 0);
        }

        // copy data to output buffer
        std::memcpy(m_invertedData.data(), data, entries_needed * sizeof(DataType_t));
        
        // Process each chunk independently
        assert(n_bits % ChunkSize == 0); // Ensure n_bits is a multiple of chunk size
        std::size_t n_chunks = n_bits / ChunkSize;
        
        // Resize lastInvert vector if needed
        if (lastInvert.capacity() < n_chunks) {
            lastInvert.resize(n_chunks, false);
        }
        
        // SAFETY: inverting the chunk while iterating over the data is safe because the invertedData is resized to the maximum size
        auto invert_visitor = [this, &lastInvert, timestamp, read] (bool invert_chunk, const std::size_t chunk) {
            dispatchCallback(timestamp, chunk, invert_chunk, read);

            // Update the last inversion state
            lastInvert[chunk] = invert_chunk;

            // Invert Chunk
            if (invert_chunk) {
                invertChunk(chunk);
            }
        };

        // Compute DBI
        m_algorithm.computeDBI(
            std::make_tuple<IteratorType_t, IteratorType_t>(m_invertedData.begin(), m_invertedData.end()),
            createPreviousIterator(timestamp, m_lastBurst),
        m_idlePattern, std::move(invert_visitor));

        // Store burst information
        m_lastBurst.update(timestamp, m_width, m_burstLength, n_bits);
        lastBurstRead = read;
        return m_invertedData.data();
    }

    std::tuple<const uint8_t *, std::size_t> getInvertedData() const {
        auto &m_lastBurst = lastBurstRead ? m_lastBurst_read : m_lastBurst_write;
        assert(m_invertedData.size() >= m_lastBurst.chunks() * SUBCHUNKS && "invalid inverted Data container size");
        return std::make_tuple(m_invertedData.data(), m_lastBurst.chunks() * SUBCHUNKS);
    }
    
    inline std::size_t getInvertedDataSize() const {
        auto &m_lastBurst = lastBurstRead ? m_lastBurst_read : m_lastBurst_write;
        return m_lastBurst.chunks() * SUBCHUNKS;
    }

    void dispatchResetCallback(timestamp_t timestamp) const {
        dispatchResetCallback(timestamp, true);
        dispatchResetCallback(timestamp, false);
    }


    void serialize(std::ostream& stream) const override {
        stream.write(reinterpret_cast<const char*>(&m_enable), sizeof(m_enable));
        stream.write(reinterpret_cast<const char*>(&m_idlePattern), sizeof(m_idlePattern));
        stream.write(reinterpret_cast<const char*>(&lastBurstRead), sizeof(lastBurstRead));

        m_lastBurst_read.serialize(stream);
        m_lastBurst_write.serialize(stream);

        std::size_t invertedDataSize = m_invertedData.size();
        stream.write(reinterpret_cast<const char*>(&invertedDataSize), sizeof(invertedDataSize));
        for (const auto& byte : m_invertedData) {
            stream.write(reinterpret_cast<const char*>(&byte), sizeof(byte));
        }
    }
    void deserialize(std::istream& stream) override {
        stream.read(reinterpret_cast<char*>(&m_enable), sizeof(m_enable));
        stream.read(reinterpret_cast<char*>(&m_idlePattern), sizeof(m_idlePattern));
        stream.read(reinterpret_cast<char*>(&lastBurstRead), sizeof(lastBurstRead));

        m_lastBurst_read.deserialize(stream);
        m_lastBurst_write.deserialize(stream);

        std::size_t invertedDataSize;
        stream.read(reinterpret_cast<char*>(&invertedDataSize), sizeof(invertedDataSize));
        m_invertedData.clear();
        m_invertedData.reserve(invertedDataSize);
        for (std::size_t i = 0; i < invertedDataSize; ++i) {
            uint8_t byte;
            stream.read(reinterpret_cast<char*>(&byte), sizeof(byte));
            m_invertedData.push_back(byte);
        }
    }


// Private member functions
private:
    AlgorithmPreviousOptional_t createPreviousIterator(timestamp_t timestamp, LastBurst_t& m_lastBurst) {
        if (m_lastBurst.m_init && m_lastBurst.end() == timestamp && m_width.has_value() // seamless burst and busWidth provided
            && m_lastBurst.beats() != 0 && m_invertedData.size() > m_width.value() / DATATYPEDIGITS) // at least one beat is required
        {
            assert(0 <= (m_invertedData.size() - (m_width.value() / DATATYPEDIGITS)) && "Invalid m_invertedData size");
            m_lastBurst.m_lastbeat.clear();
            std::size_t n_subChunks = m_width.value() / DATATYPEDIGITS;
            m_lastBurst.m_lastbeat.resize(n_subChunks);
            std::copy(m_invertedData.end() - n_subChunks, m_invertedData.end(), m_lastBurst.m_lastbeat.begin());
            // Create concat iterator of lastburst and current burst
            return std::make_optional<AlgorithmPrevious_t>(std::make_tuple<AlgorithmPreviousConcat_t, AlgorithmPreviousConcat_t>(
                AlgorithmPreviousConcat_t{m_lastBurst.m_lastbeat.begin(), m_lastBurst.m_lastbeat.end(), m_invertedData.begin()},
                AlgorithmPreviousConcat_t{m_lastBurst.m_lastbeat.end(), m_lastBurst.m_lastbeat.end(), m_invertedData.end() - m_width.value() / DATATYPEDIGITS}
            ));
        } else if (m_width.has_value() && m_idleData.has_value()) {
            assert(0 <= (m_invertedData.size() - (m_width.value() / DATATYPEDIGITS)) && "Invalid m_invertedData size");
            // Create concat iterator of idleData and current burst
            return std::make_optional<AlgorithmPrevious_t>(std::make_tuple<AlgorithmPreviousConcat_t, AlgorithmPreviousConcat_t>(
                AlgorithmPreviousConcat_t{m_idleData.value().begin(), m_idleData.value().end(), m_invertedData.begin()},
                AlgorithmPreviousConcat_t{m_idleData.value().end(), m_idleData.value().end(), m_invertedData.end() - m_width.value() / DATATYPEDIGITS}
            ));
        } else {
            return std::nullopt;
        }
    }

    void dispatchCallback(timestamp_t timestamp, std::size_t chunk, bool invert_chunk, bool read) {
        auto &lastInvert = read ? m_lastBurst_read.getInversionState() : m_lastBurst_write.getInversionState();
        auto &m_lastBurst = read ? m_lastBurst_read : m_lastBurst_write;
        std::optional<std::size_t> chunks_per_width = getChunksPerWidth();
        if (nullptr != m_changeCallback) {
            if (!chunks_per_width.has_value()) {
                m_changeCallback(timestamp, timestamp, chunk, invert_chunk, read);
            } else {
                std::size_t beat_idx = chunk % chunks_per_width.value();
            
                assert(0 == (m_lastBurst.chunks() % chunks_per_width.value()) && "Last inversion size is no multiple of chunks per width");
                bool previous_state = (chunk >= chunks_per_width.value()) // Check for current burst
                    ? lastInvert[chunk - chunks_per_width.value()] // Compare in current burst
                    : chunk < m_lastBurst.chunks() // Check if last burst was big enough
                        ? lastInvert[m_lastBurst.chunks() - chunks_per_width.value() + chunk] // Compare with last burst
                        : false; // Default to false if no comparison is possible

                // Dispatch callback if inversion state changed
                if (previous_state != invert_chunk) {
                    timestamp_t t = timestamp + (chunk / chunks_per_width.value());
                    m_changeCallback(timestamp, t, beat_idx, invert_chunk, read);
                }
            }
        }
    }

    void dispatchResetCallback(timestamp_t timestamp, bool read) const {
        const auto &m_lastBurst = read ? m_lastBurst_read : m_lastBurst_write;
        const auto &lastInvert = read ? m_lastBurst_read.getInversionState() : m_lastBurst_write.getInversionState();
         // Ensure there is at least one chunk in the last inversion
        assert(!m_lastBurst.m_init || (!getChunksPerWidth().has_value() || m_lastBurst.chunks() >= getChunksPerWidth().value()));
        if (!m_lastBurst.m_init) {
            return;
        } else if (timestamp < m_lastBurst.end()) {
            // previous burst present and timestamp is before the last burst end
            return;
        } else if (m_lastBurst.end() == timestamp) {
            // Seamless burst
            // if new burstlength is smaller then last burstlength not all pins are reset for a seamless burst in absolute chunk mode (m_width not set)
            // In absolute mode the callback is called for every chunk
            return;
        }

        // The last burst is initialized
        //    and no seamleass burst
        //    and timestamp >= m_lastBurst.end

        // Dispatch callback for the inversion back to the idle pattern
        if (nullptr != m_changeCallback) {
            timestamp_t t = m_lastBurst.end();
            if (getChunksPerWidth().has_value()) {
                // First timestamp after the last chunk
                std::size_t start = m_lastBurst.chunks() - getChunksPerWidth().value();
                // Loop over the chunks/dbi_pins in the last pattern
                for (std::size_t chunk = start; chunk < m_lastBurst.chunks(); ++chunk) {
                    std::size_t chunk_idx = chunk % getChunksPerWidth().value(); // Pin index
                    if (lastInvert[chunk]) {
                        // Reset the inversion state
                        m_changeCallback(timestamp, t, chunk_idx, false, read);
                    }
                }
            } else {
                // TODO split the callbacks in dedicated update / reset callbacks?
                // no bus width is defined. The dbi executes in absolute chunk mode.
                // The number of dbi pins cannot be calculated
                // Looping over all chunks of the lastBurst
                for (std::size_t chunk = 0; chunk < m_lastBurst.chunks(); chunk++) {
                    if (lastInvert[chunk]) {
                        m_changeCallback(timestamp, t, chunk, false, read);
                    }
                }
            }
        }
    }


// Private member variables
private:
    Algorithm_t m_algorithm{}; // Use default constructor of algorithm

    const std::optional<std::size_t> m_width = std::nullopt; // Width of the complete bus
    const std::size_t m_burstLength = 0; // Width of the complete bus
    IdlePattern_t m_idlePattern = IdlePattern_t::Z; // Default idle pattern is High-Z

    bool lastBurstRead = false;
    LastBurst_t m_lastBurst_read;  // Last burst end timestamp
    LastBurst_t m_lastBurst_write; // Last burst end timestamp

    DataBuffer_t m_invertedData; // Buffer for inverted data
    std::optional<DataBuffer_t> m_idleData; // Buffer for idle data

    ChangeCallback_t m_changeCallback = nullptr; // Callback called in updateDBI
    bool m_enable = false;
};

} // namespace DRAMPower::util

#endif /* DRAMPOWER_UTIL_DBI_H */
