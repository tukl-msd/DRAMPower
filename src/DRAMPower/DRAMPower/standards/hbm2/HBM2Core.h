#ifndef DRAMPOWER_STANDARDS_HBM2_HBM2CORE_H
#define DRAMPOWER_STANDARDS_HBM2_HBM2CORE_H

#include <DRAMPower/Types.h>
#include <DRAMPower/dram/PseudoChannel.h>
#include <DRAMPower/command/Command.h>
#include <DRAMPower/data/stats.h>
#include <DRAMPower/util/ImplicitCommandHandler.h>
#include <DRAMPower/util/RegisterHelper.h>

#include <DRAMPower/memspec/MemSpecHBM2.h>

#include <cassert>

namespace DRAMPower {

struct HBM2BankExtractor {
    HBM2BankExtractor (uint64_t numberOfStacks)
        : m_numberOfStacks(numberOfStacks)
    {}

    inline std::size_t operator()(const TargetCoordinate &coordinate) {
        return coordinate.bank + coordinate.stack * m_numberOfStacks;
    };

    const uint64_t m_numberOfStacks;
};

class HBM2Core : public util::Serialize, public util::Deserialize {
// Public type definitions
public:
    using implicitCommandInserter_t = ImplicitCommandHandler::Inserter_t;
    using coreRegisterHelper_t = util::CoreRegisterHelperPseudoChannel<HBM2Core, HBM2BankExtractor>;
    using commandCounter_t = typename coreRegisterHelper_t::commandCounter_t;

// Public constructors and assignment operators
public:
    HBM2Core() = delete; // No default constructor
    HBM2Core(const HBM2Core&) = default; // copy constructor
    HBM2Core& operator=(const HBM2Core&) = delete; // copy assignment operator
    HBM2Core(HBM2Core&&) = default; // move constructor
    HBM2Core& operator=(HBM2Core&&) = delete; // move assignment operator
    HBM2Core(const MemSpecHBM2& memSpec, implicitCommandInserter_t&& implicitCommandInserter);

// Private member functions
private:
    void handleRefreshOnBank(PseudoChannel &pseudoChannel, Bank & bank, timestamp_t timestamp, uint64_t timing, uint64_t& counter);
    inline void handlePre_impl(PseudoChannel &pseudoChannel, Bank & bank, timestamp_t timestamp, uint64_t& counter);

// Public member functions
public:

    coreRegisterHelper_t getRegisterHelper() {
        return coreRegisterHelper_t{this, m_pseudoChannels, m_memSpec.numberOfStacks};
    }

    void handleAct(PseudoChannel & pseudoChannel, Bank & bank, timestamp_t timestamp);
    void handlePre(PseudoChannel & pseudoChannel, Bank & bank, timestamp_t timestamp);
    void handleRefSingleBank(PseudoChannel & pseudoChannel, Bank & bank, timestamp_t timestamp);
    void handleRead(PseudoChannel & pseudoChannel, Bank & bank, timestamp_t timestamp);
    void handleWrite(PseudoChannel & pseudoChannel, Bank & bank, timestamp_t timestamp);
    void handleReadAuto(PseudoChannel & pseudoChannel, Bank & bank, timestamp_t timestamp);
    void handleWriteAuto(PseudoChannel & pseudoChannel, Bank & bank, timestamp_t timestamp);
    void handlePreAll(PseudoChannel & pseudoChannel, timestamp_t timestamp); 
    void handleRefAll(PseudoChannel & pseudoChannel, timestamp_t timestamp);
    void handleSelfRefreshEntry(timestamp_t timestamp);
    void handleSelfRefreshExit(timestamp_t timestamp);
    void handlePowerDownActEntry(timestamp_t timestamp);
    void handlePowerDownActExit(timestamp_t timestamp);
    void handlePowerDownPreEntry(timestamp_t timestamp);
    void handlePowerDownPreExit(timestamp_t timestamp);

    timestamp_t earliestPossiblePowerDownEntryTime() const;

    void getWindowStats(timestamp_t timestamp, SimulationStats &stats) const;

// Overrides
public:
    void serialize(std::ostream& stream) const override;
    void deserialize(std::istream& stream) override;

// Public member variables
public:
    std::vector<PseudoChannel> m_pseudoChannels;

// Private members variables
private:
	struct {
		interval_t sref;
		interval_t powerDownAct;
		interval_t powerDownPre;
		interval_t deepSleepMode;
	} m_cycles;
	struct {
		uint64_t selfRefresh = 0;
	} m_counter = { 0 };
    const MemSpecHBM2& m_memSpec;
    implicitCommandInserter_t m_implicitCommandInserter;
};

} // namespace DRAMPower


#endif /* DRAMPOWER_STANDARDS_HBM2_HBM2CORE_H */
