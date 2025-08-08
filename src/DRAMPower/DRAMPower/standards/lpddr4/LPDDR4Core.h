#ifndef DRAMPOWER_STANDARDS_LPDDR4_LPDDR4CORE_H
#define DRAMPOWER_STANDARDS_LPDDR4_LPDDR4CORE_H

#include "DRAMPower/dram/Rank.h"
#include "DRAMPower/util/RegisterHelper.h"
#include "DRAMPower/util/RegisterHelper.h"
#include "DRAMPower/util/ImplicitCommandHandler.h"

#include "DRAMPower/memspec/MemSpecLPDDR4.h"

#include <vector>

namespace DRAMPower {

class LPDDR4Core {
// Public type definitions
public:
    using implicitCommandInserter_t = ImplicitCommandHandler::Inserter_t;
    using coreRegisterHelper_t = util::CoreRegisterHelper<LPDDR4Core>;
// Public constructors
public:
    LPDDR4Core() = delete; // No default constructor
    LPDDR4Core(const LPDDR4Core&) = default; // copy constructor
    LPDDR4Core& operator=(const LPDDR4Core&) = delete; // copy assignment operator
    LPDDR4Core(LPDDR4Core&&) = default; // move constructor
    LPDDR4Core& operator=(LPDDR4Core&&) = delete; // move assignment operator
    LPDDR4Core(const MemSpecLPDDR4& memSpec, implicitCommandInserter_t&& implicitCommandInserter)
        : m_ranks(memSpec.numberOfRanks, {static_cast<std::size_t>(memSpec.numberOfBanks)})
        , m_memSpec(memSpec)
        , m_implicitCommandInserter(std::move(implicitCommandInserter))
    {}

// Public member functions
public:
    coreRegisterHelper_t getRegisterHelper() {
        return coreRegisterHelper_t{this, m_ranks};
    }

    void handleAct(Rank & rank, Bank & bank, timestamp_t timestamp);
    void handlePre(Rank & rank, Bank & bank, timestamp_t timestamp);
    void handlePreAll(Rank & rank, timestamp_t timestamp);
    void handleRead(Rank & rank, Bank & bank, timestamp_t timestamp);
    void handleWrite(Rank & rank, Bank & bank, timestamp_t timestamp);
    void handleReadAuto(Rank & rank, Bank & bank, timestamp_t timestamp);
    void handleWriteAuto(Rank & rank, Bank & bank, timestamp_t timestamp);
    void handleRefAll(Rank & rank, timestamp_t timestamp);
    void handleRefPerBank(Rank & rank, Bank & bank, timestamp_t timestamp);
    void handleRefreshOnBank(Rank & rank, Bank & bank, timestamp_t timestamp, uint64_t timing, uint64_t & counter);
    void handleSelfRefreshEntry(Rank & rank, timestamp_t timestamp);
    void handleSelfRefreshExit(Rank & rank, timestamp_t timestamp);
    void handlePowerDownActEntry(Rank & rank, timestamp_t timestamp);
    void handlePowerDownActExit(Rank & rank, timestamp_t timestamp);
    void handlePowerDownPreEntry(Rank & rank, timestamp_t timestamp);
    void handlePowerDownPreExit(Rank & rank, timestamp_t timestamp);

    timestamp_t earliestPossiblePowerDownEntryTime(Rank & rank) const;
    void getWindowStats(timestamp_t timestamp, SimulationStats &stats) const;


// Publie member variables:
public:
    std::vector<Rank> m_ranks;

// Private member variables
private:
    const MemSpecLPDDR4& m_memSpec;
    implicitCommandInserter_t m_implicitCommandInserter;
};

} // namespace DRAMPower

#endif /* DRAMPOWER_STANDARDS_LPDDR4_LPDDR4CORE_H */
