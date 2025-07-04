#ifndef DRAMPOWER_STANDARDS_DDR5_DDR5CORE_H
#define DRAMPOWER_STANDARDS_DDR5_DDR5CORE_H

#include "DRAMPower/dram/Rank.h"
#include "DRAMPower/util/RegisterHelper.h"
#include "DRAMPower/util/RegisterHelper.h"
#include "DRAMPower/util/ImplicitCommandHandler.h"

#include "DRAMPower/memspec/MemSpecDDR5.h"

#include <vector>

namespace DRAMPower {

class DDR5Core {
// Public type definitions
public:
    using implicitCommandInserter_t = ImplicitCommandHandler::Inserter_t;
    using coreRegisterHelper_t = util::CoreRegisterHelper<DDR5Core>;
// Public constructors
public:
    DDR5Core() = delete; // No default constructor
    DDR5Core(const DDR5Core&) = default; // copy constructor
    DDR5Core& operator=(const DDR5Core&) = default; // copy assignment operator
    DDR5Core(DDR5Core&&) = default; // move constructor
    DDR5Core& operator=(DDR5Core&&) = default; // move assignment operator
    DDR5Core(const std::shared_ptr<const MemSpecDDR5>& memSpec, implicitCommandInserter_t&& implicitCommandInserter)
        : m_ranks(memSpec->numberOfRanks, {static_cast<std::size_t>(memSpec->numberOfBanks)})
        , m_memSpec(memSpec)
        , m_implicitCommandInserter(std::move(implicitCommandInserter))
    {}

// Public member functions
public:
    coreRegisterHelper_t getRegisterHelper() {
        return coreRegisterHelper_t{this, m_ranks};
    }

    void handleAct(Rank& rank, Bank& bank, timestamp_t timestamp);
    void handlePre(Rank& rank, Bank& bank, timestamp_t timestamp);
    void handlePreSameBank(Rank& rank, std::size_t bank_id, timestamp_t timestamp);
    void handlePreAll(Rank& rank, timestamp_t timestamp);
    void handleRefSameBank(Rank& rank, std::size_t bank_id, timestamp_t timestamp);
    void handleRefAll(Rank& rank, timestamp_t timestamp);
    void handleRefreshOnBank(Rank& rank, Bank& bank, timestamp_t timestamp, uint64_t timing, uint64_t& counter);
    void handleSelfRefreshEntry(Rank& rank, timestamp_t timestamp);
    void handleSelfRefreshExit(Rank& rank, timestamp_t timestamp);
    void handleRead(Rank& rank, Bank& bank, timestamp_t timestamp);
    void handleWrite(Rank& rank, Bank& bank, timestamp_t timestamp);
    void handleReadAuto(Rank& rank, Bank& bank, timestamp_t timestamp);
    void handleWriteAuto(Rank& rank, Bank& bank, timestamp_t timestamp);
    void handlePowerDownActEntry(Rank& rank, timestamp_t timestamp);
    void handlePowerDownActExit(Rank& rank, timestamp_t timestamp);
    void handlePowerDownPreEntry(Rank& rank, timestamp_t timestamp);
    void handlePowerDownPreExit(Rank& rank, timestamp_t timestamp);

    timestamp_t earliestPossiblePowerDownEntryTime(Rank& rank);

    void getWindowStats(timestamp_t timestamp, SimulationStats &stats) const;

// Publie member variables:
public:
    std::vector<Rank> m_ranks;

// Private member variables
private:
    std::shared_ptr<const MemSpecDDR5> m_memSpec;
    implicitCommandInserter_t m_implicitCommandInserter;
};

} // namespace DRAMPower

#endif /* DRAMPOWER_STANDARDS_DDR5_DDR5CORE_H */
