#ifndef DRAMPOWER_STANDARDS_LPDDR6_LPDDR6CORE_H
#define DRAMPOWER_STANDARDS_LPDDR6_LPDDR6CORE_H

#include "DRAMPower/dram/Rank.h"
#include "DRAMPower/command/Command.h"
#include "DRAMPower/util/ImplicitCommandHandler.h"
#include "DRAMPower/util/Serialize.h"
#include "DRAMPower/util/Deserialize.h"

#include "DRAMPower/memspec/MemSpecLPDDR6.h"

#include <vector>

namespace DRAMPower {

namespace internal {
    template<typename Core>
    class TestAccessor;
}

class LPDDR6Core : public util::Serialize, public util::Deserialize {
// Public type definitions
public:
    using implicitCommandInserter_t = ImplicitCommandHandler::Inserter_t;
// Public constructors
public:
    LPDDR6Core() = delete; // No default constructor
    LPDDR6Core(const LPDDR6Core&) = default; // copy constructor
    LPDDR6Core& operator=(const LPDDR6Core&) = delete; // copy assignment operator
    LPDDR6Core(LPDDR6Core&&) = default; // move constructor
    LPDDR6Core& operator=(LPDDR6Core&&) = delete; // move assignment operator
    LPDDR6Core(implicitCommandInserter_t&& implicitCommandInserter, const MemSpecLPDDR6& memSpec)
        : m_memSpec(memSpec)
        , m_ranks(memSpec.numberOfRanks, {static_cast<std::size_t>(memSpec.numberOfBanks)})
        , m_implicitCommandInserter(std::move(implicitCommandInserter))
    {}

// Public member functions
public:
// Member functions
    void doCommand(const Command& cmd);
    void getWindowStats(timestamp_t timestamp, SimulationStats &stats) const;
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
    void handleReadAuto(Rank& rank, Bank& bank, timestamp_t timestamp);
    void handleWriteAuto(Rank& rank, Bank& bank, timestamp_t timestamp);
    void handleRefAll(Rank& rank, timestamp_t timestamp);
    void handleRefPerBank(Rank& rank, Bank& bank, timestamp_t timestamp);
    void handleRefPerTwoBanks(Rank& rank, std::size_t bank_id, timestamp_t timestamp);
    void handleRefreshOnBank(Rank& rank, Bank& bank, timestamp_t timestamp, uint64_t timing,
                             uint64_t& counter);
    void handleSelfRefreshEntry(Rank& rank, timestamp_t timestamp);
    void handleSelfRefreshExit(Rank& rank, timestamp_t timestamp);
    void handlePowerDownActEntry(Rank& rank, timestamp_t timestamp);
    void handlePowerDownActExit(Rank& rank, timestamp_t timestamp);
    void handlePowerDownPreEntry(Rank& rank, timestamp_t timestamp);
    void handlePowerDownPreExit(Rank& rank, timestamp_t timestamp);
    void handleDSMEntry(Rank& rank, timestamp_t timestamp);
    void handleDSMExit(Rank& rank, timestamp_t timestamp);

    timestamp_t earliestPossiblePowerDownEntryTime(Rank & rank) const;

// Private member variables
private:
    const MemSpecLPDDR6& m_memSpec;
    std::vector<Rank> m_ranks;
    implicitCommandInserter_t m_implicitCommandInserter;
};

} // namespace DRAMPower

#endif /* DRAMPOWER_STANDARDS_LPDDR6_LPDDR6CORE_H */
