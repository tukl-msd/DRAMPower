#ifndef DRAMPOWER_STANDARDS_DDR5_DDR5CORE_H
#define DRAMPOWER_STANDARDS_DDR5_DDR5CORE_H

#include "DRAMPower/command/Command.h"
#include "DRAMPower/dram/Rank.h"
#include "DRAMPower/util/ImplicitCommandHandler.h"
#include "DRAMPower/util/Deserialize.h"
#include "DRAMPower/util/Serialize.h"

#include "DRAMPower/memspec/MemSpecDDR5.h"

#include <cstddef>
#include <vector>

namespace DRAMPower {

namespace internal {
    template<typename Core>
    class TestAccessor;
}

class DDR5Core : public util::Serialize, public util::Deserialize {
// Friend classes
friend class internal::TestAccessor<DDR5Core>;

// Public type definitions
public:
    using implicitCommandInserter_t = ImplicitCommandHandler::Inserter_t;

// Public constructors and assignment operators
public:
    DDR5Core() = delete; // No default constructor
    DDR5Core(const DDR5Core&) = default; // copy constructor
    DDR5Core& operator=(const DDR5Core&) = delete; // copy assignment operator
    DDR5Core(DDR5Core&&) = default; // move constructor
    DDR5Core& operator=(DDR5Core&&) = delete; // move assignment operator
    DDR5Core(implicitCommandInserter_t&& implicitCommandInserter, const MemSpecDDR5& memSpec)
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

// Private member variables
private:
    const MemSpecDDR5& m_memSpec;
    std::vector<Rank> m_ranks;
    implicitCommandInserter_t m_implicitCommandInserter;
};

} // namespace DRAMPower

#endif /* DRAMPOWER_STANDARDS_DDR5_DDR5CORE_H */
