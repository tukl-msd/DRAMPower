#include "DRAMPower/util/dbi.h"
#include "DRAMPower/Types.h"
#include <optional>

namespace DRAMPower::util {

void DBI::invertChunk(std::size_t chunk_start_bit, std::size_t chunk_end_bit) {
    for (std::size_t bit = chunk_start_bit; bit < chunk_end_bit; ++bit) {
        std::size_t byte_idx = bit / 8;
        std::size_t bit_pos = bit % 8;
        
        // Toggle the bit in buffer
        m_invertedData[byte_idx] ^= (1u << bit_pos);
    }
}

void DBI::enable(bool enable)
{
    m_enable = enable;
}

bool DBI::isEnabled() const
{
    return m_enable;
}

const std::vector<DBI::State_t>& DBI::getInversionStateRead() const
{
    return m_lastInvert_read;
}

const std::vector<DBI::State_t>& DBI::getInversionStateWrite() const
{
    return m_lastInvert_write;
}

std::size_t DBI::getChunksPerWidth() const
{
    return m_width / m_chunkSize;
}

void DBI::setIdlePattern(IdlePattern_t pattern)
{
    m_idlePattern = pattern;
}
DBI::IdlePattern_t DBI::getIdlePattern() const
{
    return m_idlePattern;
}

void DBI::resetDBI() {
    m_lastInvert_read.clear();
    m_lastInvert_write.clear();
}

void DBI::dispatchResetCallback(timestamp_t timestamp) const {
    dispatchResetCallback(timestamp, true);
    dispatchResetCallback(timestamp, false);
}

void DBI::dispatchResetCallback(timestamp_t timestamp, bool read) const {
    auto &m_lastBurst = read ? m_lastBurst_read : m_lastBurst_write;
    auto &m_lastInvert = read ? m_lastInvert_read : m_lastInvert_write;
    assert(m_lastInversionSize % getChunksPerWidth() == 0); // Ensure last inversion size is a multiple of chunks per width
    assert(!m_lastBurst.init || m_lastInversionSize >= getChunksPerWidth()); // Ensure there is at least one chunk in the last inversion
    if (!m_lastBurst.init) {
        // Call before last burst -> nothing to reset
        return;
    } else if (timestamp < m_lastBurst.end) {
        // previous burst and timestamp is before the last burst end
        return;
    } else if (m_lastBurst.end == timestamp && m_lastBurst.read == read) {
        // Seamless burst
        return;
    }

    // The last burst initialized
    //    and no seamleass burst
    //    and timestamp >= m_lastBurst.end

    // Dispatch callback for the inversion back to the idle pattern
    if (nullptr != m_changeCallback) {
        // First timestamp after the last chunk
        timestamp_t t = m_lastBurst.end;
        std::size_t start = m_lastInversionSize - getChunksPerWidth();
        // Loop over the chunks in the last pattern
        for (std::size_t chunk = start; chunk < m_lastInversionSize; ++chunk) {
            std::size_t chunk_idx = chunk % getChunksPerWidth(); // Pin index
            if (m_lastInvert[chunk]) {
                // Reset the inversion state
                m_changeCallback(timestamp, t, chunk_idx, false, read);
            }
        }
    }
}

std::optional<const uint8_t *> DBI::updateDBI(timestamp_t timestamp, std::size_t n_bits, const uint8_t* data, bool read)
{
    if (!data || 0 == n_bits || !m_enable || IdlePattern_t::Z == m_idlePattern) return std::nullopt;

    auto &m_lastInvert = read ? m_lastInvert_read : m_lastInvert_write;
    auto &m_lastBurst = read ? m_lastBurst_read : m_lastBurst_write;

    // Reset to Idle Pattern if this and the last burst are not seamless
    dispatchResetCallback(timestamp, read);
    
    // Calculate number of bytes needed to store the data
    std::size_t bytes_needed = (n_bits + 7) / 8;
    
    // Resize output buffer if needed
    if (m_invertedData.size() < bytes_needed) {
        m_invertedData.resize(bytes_needed, 0);
    }
    
    // copy data to output buffer
    std::memcpy(m_invertedData.data(), data, bytes_needed);
    
    // Process each chunk independently
    assert(n_bits % m_chunkSize == 0); // Ensure n_bits is a multiple of chunk size
    assert(m_width % m_chunkSize == 0); // Ensure burst size is a multiple of chunk size
    std::size_t n_chunks = n_bits / m_chunkSize;
    
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
        
        // Invert the chunk if needed
        if (invert_chunk) {
            invertChunk(chunk_start_bit, chunk_end_bit);
        }

        // Callback
        std::size_t chunks_per_width = getChunksPerWidth();
        std::size_t chunk_idx = chunk % chunks_per_width;
        
        // NOTE: Assumption: m_lastInversionSize = n * chunks_per_width
        bool previous_state = (chunk >= chunks_per_width) // Check for current burst
            ? m_lastInvert[chunk - chunks_per_width] // Compare in current burst
            : chunk < m_lastInversionSize // Check if last burst was big enough
                ? m_lastInvert[m_lastInversionSize - chunks_per_width + chunk] // Compare with last burst
                : false; // Default to false if no comparison is possible
        
        
        // Dispatch callback if inversion state changed
        if (nullptr != m_changeCallback && previous_state != invert_chunk) {
            timestamp_t t = timestamp + (chunk / chunks_per_width);
            m_changeCallback(timestamp, t, chunk_idx, invert_chunk, read);
        }

        // Update the last inversion state
        m_lastInvert[chunk] = invert_chunk;
    }

    timestamp_t m_lastBurstEnd = timestamp + (n_chunks / getChunksPerWidth());
    m_lastBurst.start = timestamp;
    m_lastBurst.end = m_lastBurstEnd;
    m_lastBurst.read = read;
    m_lastBurst.init = true; // Mark the last burst as initialized

    m_lastInversionSize = n_chunks;
    return m_invertedData.data();
}

std::tuple<const uint8_t *, std::size_t> DBI::getInvertedData() const {
    return std::make_tuple(m_invertedData.data(), m_invertedData.size());
}

} // namespace DRAMPower::util
