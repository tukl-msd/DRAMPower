#ifndef DRAMPOWER_STANDARDS_HBM2_HBM2CORE_H
#define DRAMPOWER_STANDARDS_HBM2_HBM2CORE_H

#include "DRAMPower/standards/hbm2/HBM2Command.h"
#include "DRAMPower/util/RegisterMapping.h"
#include <DRAMPower/Types.h>
#include <DRAMPower/dram/PseudoChannel.h>
#include <DRAMPower/command/Command.h>
#include <DRAMPower/data/stats.h>
#include <DRAMPower/util/ImplicitCommandHandler.h>
#include <DRAMPower/util/RegisterHelper.h>

#include <DRAMPower/memspec/MemSpecHBM2.h>

#include <cassert>

namespace DRAMPower {

namespace internal {
    template<typename Core>
    class TestAccessor;
}

struct HBM2CoreMemSpec {
    HBM2CoreMemSpec(const MemSpecHBM2& memSpec)
        : numberOfBanks(memSpec.numberOfBanks)
        , numberOfPseudoChannels(memSpec.numberOfPseudoChannels)
        , numberOfStacks(memSpec.numberOfStacks)
        , tRFC(memSpec.memTimingSpec.tRFC)
        , tRFCSB(memSpec.memTimingSpec.tRFCSB)
        , tRAS(memSpec.memTimingSpec.tRAS)
        , tRCDRD(memSpec.memTimingSpec.tRCDRD)
        , tRCDWR(memSpec.memTimingSpec.tRCDWR)
        , tRP(memSpec.memTimingSpec.tRP)
        , prechargeOffsetRD(memSpec.prechargeOffsetRD)
        , prechargeOffsetWR(memSpec.prechargeOffsetWR)
    {}

    uint64_t numberOfBanks;
    uint64_t numberOfPseudoChannels;
    uint64_t numberOfStacks;

    uint64_t tRFC;
    uint64_t tRFCSB;
    uint64_t tRAS;
    uint64_t tRCDRD;
    uint64_t tRCDWR;
    uint64_t tRP;
    uint64_t prechargeOffsetRD;
    uint64_t prechargeOffsetWR;
};

class HBM2Core : public util::Serialize, public util::Deserialize {
// Friend classes
friend class internal::TestAccessor<HBM2Core>;

// Public type definitions
public:

// Public constructors and assignment operators
public:
    HBM2Core(const MemSpecHBM2& memSpec);

// Public member functions
public:
// Member functions
    void doCommand(const HBM2Command& cmd);
    timestamp_t getLastCommandTime() const;
    bool isSerializable() const;
    void getWindowStats(timestamp_t timestamp, SimulationStats &stats);
    void setSimulationTime(timestamp_t timestamp);
    void reset();
// Overrides
    void serialize(std::ostream& stream) const override;
    void deserialize(std::istream& stream) override;

// Private member functions
private:
    void handleRefreshOnBank(std::size_t pc_idx, std::size_t bank_idx, timestamp_t timestamp, uint64_t timing, uint64_t& counter);
    inline void handlePre_impl(PseudoChannel &pseudoChannel, Bank & bank, timestamp_t timestamp);
    
    void handleAct(PseudoChannel & pseudoChannel, Bank & bank, timestamp_t timestamp);
    void handlePre(PseudoChannel & pseudoChannel, Bank & bank, timestamp_t timestamp);
    void handleRefSingleBank(std::size_t pc_idx, std::size_t bank_idx, timestamp_t timestamp);
    void handleRead(PseudoChannel & pseudoChannel, Bank & bank, timestamp_t timestamp);
    void handleWrite(PseudoChannel & pseudoChannel, Bank & bank, timestamp_t timestamp);
    void handleReadAuto(std::size_t pc_idx, std::size_t bank_idx, timestamp_t timestamp);
    void handleWriteAuto(std::size_t pc_idx, std::size_t bank_idx, timestamp_t timestamp);
    void handlePreAll(PseudoChannel & pseudoChannel, timestamp_t timestamp); 
    void handleRefAll(std::size_t pc_idx, timestamp_t timestamp);
    void handleSelfRefreshEntry(timestamp_t timestamp);
    void handleSelfRefreshExit(timestamp_t timestamp);
    void handlePowerDownActEntry(timestamp_t timestamp);
    void handlePowerDownActExit(timestamp_t timestamp);
    void handlePowerDownPreEntry(timestamp_t timestamp);
    void handlePowerDownPreExit(timestamp_t timestamp);

    timestamp_t earliestPossiblePowerDownEntryTime() const;

// Private members variables
private:
    HBM2CoreMemSpec m_memSpec;
    util::coreHelpers::PseudoChannelMapping m_helperMapping;
    std::vector<PseudoChannel> m_pseudoChannels;
	struct {
		interval_t sref;
		interval_t powerDownAct;
		interval_t powerDownPre;
		interval_t deepSleepMode;
	} m_cycles;
	struct {
		uint64_t selfRefresh = 0;
	} m_counter = { 0 };
    ImplicitCommandHandler<HBM2Core> m_implicitCommandHandler;
    timestamp_t m_last_command_time = 0;
    timestamp_t m_offset = 0;
};

} // namespace DRAMPower


#endif /* DRAMPOWER_STANDARDS_HBM2_HBM2CORE_H */
