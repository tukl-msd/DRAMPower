#ifndef DRAMPOWER_STANDARDS_LPDDR5_LPDDR5CORE_H
#define DRAMPOWER_STANDARDS_LPDDR5_LPDDR5CORE_H

#include "DRAMPower/util/Deserialize.h"
#include "DRAMPower/util/Serialize.h"
#include <DRAMPower/Types.h>
#include "DRAMPower/dram/Rank.h"
#include "DRAMPower/command/Command.h"
#include <DRAMPower/data/stats.h>
#include "DRAMPower/util/ImplicitCommandHandler.h"

#include "DRAMPower/memspec/MemSpecLPDDR5.h"

#include <vector>

namespace DRAMPower {

namespace internal {
    template<typename Core>
    class TestAccessor;
}

class LPDDR5Core : public util::Serialize, public util::Deserialize {
// Friend classes
friend class internal::TestAccessor<LPDDR5Core>;

// Public constructors
public:
    LPDDR5Core() = delete; // No default constructor
    LPDDR5Core(const LPDDR5Core&) = default; // copy constructor
    LPDDR5Core& operator=(const LPDDR5Core&) = delete; // copy assignment operator
    LPDDR5Core(LPDDR5Core&&) = default; // move constructor
    LPDDR5Core& operator=(LPDDR5Core&&) = delete; // move assignment operator
    LPDDR5Core(const MemSpecLPDDR5& memSpec)
        : m_memSpec(memSpec)
        , m_ranks(memSpec.numberOfRanks, {static_cast<std::size_t>(memSpec.numberOfBanks)})
    {}

// Public member functions
public:
// Member functions
    void doCommand(const Command& cmd);
    timestamp_t getLastCommandTime() const;
    bool isSerializable() const;
    void getWindowStats(timestamp_t timestamp, SimulationStats &stats);
// Overrides
    void serialize(std::ostream& stream) const override;
    void deserialize(std::istream& stream) override;

// Private member functions
private:
    void handleAct(Rank& rank, Bank& bank, timestamp_t timestamp);
    void handlePre(Rank& rank, Bank& bank, timestamp_t timestamp);
    void handlePreAll(Rank& rank, timestamp_t timestamp);
    void handleRead(Rank& rank, Bank& bank, timestamp_t timestamp);
    void handleWrite(Rank& rank, Bank& bank, timestamp_t timestamp);
    void handleReadAuto(std::size_t rank_idx, std::size_t bank_idx, timestamp_t timestamp);
    void handleWriteAuto(std::size_t rank_idx, std::size_t bank_idx, timestamp_t timestamp);
    void handleRefAll(std::size_t rank_idx, timestamp_t timestamp);
    void handleRefPerBank(std::size_t rank_idx, std::size_t bank_idx, timestamp_t timestamp);
    void handleRefPerTwoBanks(std::size_t rank_idx, std::size_t bank_id, timestamp_t timestamp);
    void handleRefreshOnBank(std::size_t rank_idx, std::size_t bank_idx, timestamp_t timestamp, uint64_t timing,
                             uint64_t& counter);
    void handleSelfRefreshEntry(std::size_t rank_idx, timestamp_t timestamp);
    void handleSelfRefreshExit(Rank& rank, timestamp_t timestamp);
    void handlePowerDownActEntry(std::size_t rank_idx, timestamp_t timestamp);
    void handlePowerDownActExit(std::size_t rank_idx, timestamp_t timestamp);
    void handlePowerDownPreEntry(std::size_t rank_idx, timestamp_t timestamp);
    void handlePowerDownPreExit(std::size_t rank_idx, timestamp_t timestamp);
    void handleDSMEntry(Rank& rank, timestamp_t timestamp);
    void handleDSMExit(Rank& rank, timestamp_t timestamp);

    timestamp_t earliestPossiblePowerDownEntryTime(Rank & rank) const;

// Private member variables
private:
    const MemSpecLPDDR5& m_memSpec;
    std::vector<Rank> m_ranks;
    ImplicitCommandHandler<LPDDR5Core> m_implicitCommandHandler;
    timestamp_t m_last_command_time = 0;
};

} // namespace DRAMPower

#endif /* DRAMPOWER_STANDARDS_LPDDR5_LPDDR5CORE_H */
