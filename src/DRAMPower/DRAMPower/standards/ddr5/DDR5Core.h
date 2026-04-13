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

struct DDR5CoreMemSpec {
    DDR5CoreMemSpec(const MemSpecDDR5& memSpec)
        : numberOfBanks(memSpec.numberOfBanks)
        , numberOfRanks(memSpec.numberOfRanks)
        , banksPerGroup(memSpec.banksPerGroup)
        , numberOfBankGroups(memSpec.numberOfBankGroups)
        , tRFC(memSpec.memTimingSpec.tRFC)
        , tRFCsb(memSpec.memTimingSpec.tRFCsb)
        , tRAS(memSpec.memTimingSpec.tRAS)
        , tRCD(memSpec.memTimingSpec.tRCD)
        , tRP(memSpec.memTimingSpec.tRP)
        , prechargeOffsetRD(memSpec.prechargeOffsetRD)
        , prechargeOffsetWR(memSpec.prechargeOffsetWR)
    {}

    uint64_t numberOfBanks;
    uint64_t numberOfRanks;
    uint64_t banksPerGroup;
    uint64_t numberOfBankGroups;

    uint64_t tRFC;
    uint64_t tRFCsb;
    uint64_t tRAS;
    uint64_t tRCD;
    uint64_t tRP;
    uint64_t prechargeOffsetRD;
    uint64_t prechargeOffsetWR;
};

class DDR5Core : public util::Serialize, public util::Deserialize {
// Friend classes
friend class internal::TestAccessor<DDR5Core>;

// Public constructors and assignment operators
public:
    DDR5Core(const MemSpecDDR5& memSpec)
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
    void handlePreSameBank(Rank& rank, std::size_t bank_id, timestamp_t timestamp);
    void handlePreAll(Rank& rank, timestamp_t timestamp);
    void handleRefSameBank(std::size_t rank_idx, std::size_t bank_id, timestamp_t timestamp);
    void handleRefAll(std::size_t rank_idx, timestamp_t timestamp);
    void handleRefreshOnBank(std::size_t rank_idx, std::size_t bank_idx, timestamp_t timestamp, uint64_t timing, uint64_t& counter);
    void handleSelfRefreshEntry(std::size_t rank_idx, timestamp_t timestamp);
    void handleSelfRefreshExit(Rank& rank, timestamp_t timestamp);
    void handleRead(Rank& rank, Bank& bank, timestamp_t timestamp);
    void handleWrite(Rank& rank, Bank& bank, timestamp_t timestamp);
    void handleReadAuto(std::size_t rank_idx, std::size_t bank_idx, timestamp_t timestamp);
    void handleWriteAuto(std::size_t rank_idx, std::size_t bank_idx, timestamp_t timestamp);
    void handlePowerDownActEntry(std::size_t rank_idx, timestamp_t timestamp);
    void handlePowerDownActExit(std::size_t rank_idx, timestamp_t timestamp);
    void handlePowerDownPreEntry(std::size_t rank_idx, timestamp_t timestamp);
    void handlePowerDownPreExit(std::size_t rank_idx, timestamp_t timestamp);

    timestamp_t earliestPossiblePowerDownEntryTime(Rank& rank);

// Private member variables
private:
    DDR5CoreMemSpec m_memSpec;
    std::vector<Rank> m_ranks;
    ImplicitCommandHandler<DDR5Core> m_implicitCommandHandler;
    timestamp_t m_last_command_time = 0;
};

} // namespace DRAMPower

#endif /* DRAMPOWER_STANDARDS_DDR5_DDR5CORE_H */
