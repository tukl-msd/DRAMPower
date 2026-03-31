#ifndef DRAMPOWER_STANDARDS_LPDDR4_LPDDR4CORE_H
#define DRAMPOWER_STANDARDS_LPDDR4_LPDDR4CORE_H

#include "DRAMPower/util/Deserialize.h"
#include "DRAMPower/util/Serialize.h"
#include <DRAMPower/Types.h>
#include "DRAMPower/dram/Rank.h"
#include "DRAMPower/command/Command.h"
#include <DRAMPower/data/stats.h>
#include <DRAMPower/util/ImplicitCommandHandler.h>

#include "DRAMPower/memspec/MemSpecLPDDR4.h"

#include <vector>

namespace DRAMPower {

namespace internal {
    template<typename Core>
    class TestAccessor;
}

class LPDDR4Core : public util::Serialize, public util::Deserialize {
// Friend classes
friend class internal::TestAccessor<LPDDR4Core>;

// Public constructors amd assignment operators
public:
    LPDDR4Core() = delete; // No default constructor
    LPDDR4Core(const LPDDR4Core&) = default; // copy constructor
    LPDDR4Core& operator=(const LPDDR4Core&) = delete; // copy assignment operator
    LPDDR4Core(LPDDR4Core&&) = default; // move constructor
    LPDDR4Core& operator=(LPDDR4Core&&) = delete; // move assignment operator
    LPDDR4Core(const MemSpecLPDDR4& memSpec)
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

// Private member variables
private:
    const MemSpecLPDDR4& m_memSpec;
    std::vector<Rank> m_ranks;
    ImplicitCommandHandler<> m_implicitCommandHandler;
    timestamp_t m_last_command_time = 0;
};

} // namespace DRAMPower

#endif /* DRAMPOWER_STANDARDS_LPDDR4_LPDDR4CORE_H */
