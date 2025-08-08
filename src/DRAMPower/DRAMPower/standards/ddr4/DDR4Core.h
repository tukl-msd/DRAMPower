#ifndef DRAMPOWER_STANDARDS_DDR4_DDR4CORE_H
#define DRAMPOWER_STANDARDS_DDR4_DDR4CORE_H

#include <DRAMPower/Types.h>
#include <DRAMPower/dram/Rank.h>
#include <DRAMPower/command/Command.h>
#include <DRAMPower/data/stats.h>
#include <DRAMPower/util/ImplicitCommandHandler.h>
#include <DRAMPower/util/RegisterHelper.h>

#include <DRAMPower/memspec/MemSpecDDR4.h>

#include <memory>
#include <vector>
#include <cstddef>
#include <cassert>

namespace DRAMPower {

class DDR4Core {
// Public type definitions
public:
    using implicitCommandInserter_t = ImplicitCommandHandler::Inserter_t;
    using coreRegisterHelper_t = util::CoreRegisterHelper<DDR4Core>;

// Public constructors and assignment operators
public:
    DDR4Core() = delete; // No default constructor
    DDR4Core(const DDR4Core&) = default; // copy constructor
    DDR4Core& operator=(const DDR4Core&) = delete; // copy assignment operator
    DDR4Core(DDR4Core&&) = default; // move constructor
    DDR4Core& operator=(DDR4Core&&) = delete; // move assignment operator
    DDR4Core(const MemSpecDDR4& memSpec, implicitCommandInserter_t&& implicitCommandInserter)
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
    void handleRefAll(Rank & rank, timestamp_t timestamp);
    void handleSelfRefreshEntry(Rank & rank, timestamp_t timestamp);
    void handleSelfRefreshExit(Rank & rank, timestamp_t timestamp);
    void handleRead(Rank & rank, Bank & bank, timestamp_t timestamp);
    void handleWrite(Rank & rank, Bank & bank, timestamp_t timestamp);
    void handleReadAuto(Rank & rank, Bank & bank, timestamp_t timestamp);
    void handleWriteAuto(Rank & rank, Bank & bank, timestamp_t timestamp);
    void handlePowerDownActEntry(Rank & rank, timestamp_t timestamp);
    void handlePowerDownActExit(Rank & rank, timestamp_t timestamp);
    void handlePowerDownPreEntry(Rank & rank, timestamp_t timestamp);
    void handlePowerDownPreExit(Rank & rank, timestamp_t timestamp);

    timestamp_t earliestPossiblePowerDownEntryTime(Rank & rank) const;

    void getWindowStats(timestamp_t timestamp, SimulationStats &stats) const;

// Public member variables
public:
    std::vector<Rank> m_ranks;

// Private members variables
private:
    const MemSpecDDR4& m_memSpec;
    implicitCommandInserter_t m_implicitCommandInserter;
};

} // namespace DRAMPower


#endif /* DRAMPOWER_STANDARDS_DDR4_DDR4CORE_H */
