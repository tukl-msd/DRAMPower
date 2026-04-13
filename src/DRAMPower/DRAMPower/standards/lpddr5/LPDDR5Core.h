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

struct LPDDR5CoreMemSpec {
    LPDDR5CoreMemSpec(const MemSpecLPDDR5& memSpec)
        : numberOfBanks(memSpec.numberOfBanks)
        , numberOfRanks(memSpec.numberOfRanks)
        , tRFC(memSpec.memTimingSpec.tRFC)
        , tRFCPB(memSpec.memTimingSpec.tRFCPB)
        , tRAS(memSpec.memTimingSpec.tRAS)
        , tRCD(memSpec.memTimingSpec.tRCD)
        , tRP(memSpec.memTimingSpec.tRP)
        , bank_arch(memSpec.bank_arch)
        , perTwoBankOffset(memSpec.perTwoBankOffset)
        , prechargeOffsetRD(memSpec.prechargeOffsetRD)
        , prechargeOffsetWR(memSpec.prechargeOffsetWR)
    {}

    uint64_t numberOfBanks;
    uint64_t numberOfRanks;

    uint64_t tRFC;
    uint64_t tRFCPB;
    uint64_t tRAS;
    uint64_t tRCD;
    uint64_t tRP;
    MemSpecLPDDR5::BankArchitectureMode bank_arch;
    std::size_t perTwoBankOffset;
    uint64_t prechargeOffsetRD;
    uint64_t prechargeOffsetWR;
};

class LPDDR5Core : public util::Serialize, public util::Deserialize {
// Friend classes
friend class internal::TestAccessor<LPDDR5Core>;

// Public constructors
public:
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
    LPDDR5CoreMemSpec m_memSpec;
    std::vector<Rank> m_ranks;
    ImplicitCommandHandler<LPDDR5Core> m_implicitCommandHandler;
    timestamp_t m_last_command_time = 0;
};

} // namespace DRAMPower

#endif /* DRAMPOWER_STANDARDS_LPDDR5_LPDDR5CORE_H */
