#ifndef DRAMPOWER_STANDARDS_HBM2_HBM2CORE_H
#define DRAMPOWER_STANDARDS_HBM2_HBM2CORE_H

#include <DRAMPower/Types.h>
#include <DRAMPower/dram/Rank.h>
#include <DRAMPower/command/Command.h>
#include <DRAMPower/data/stats.h>
#include <DRAMPower/util/ImplicitCommandHandler.h>
#include <DRAMPower/util/RegisterHelper.h>

#include <DRAMPower/memspec/MemSpecHBM2.h>

#include <vector>
#include <cstddef>
#include <cassert>

namespace DRAMPower {

class HBM2Core : public util::Serialize, public util::Deserialize {
// Public type definitions
public:
    using implicitCommandInserter_t = ImplicitCommandHandler::Inserter_t;
    using coreRegisterHelper_t = util::CoreRegisterHelper<HBM2Core>;
    using Stack_t = Rank;

// Public constructors and assignment operators
public:
    HBM2Core() = delete; // No default constructor
    HBM2Core(const HBM2Core&) = default; // copy constructor
    HBM2Core& operator=(const HBM2Core&) = delete; // copy assignment operator
    HBM2Core(HBM2Core&&) = default; // move constructor
    HBM2Core& operator=(HBM2Core&&) = delete; // move assignment operator
    HBM2Core(const MemSpecHBM2& memSpec, implicitCommandInserter_t&& implicitCommandInserter)
        : m_stacks(memSpec.numberOfStacks, {static_cast<std::size_t>(memSpec.numberOfBanks)}) 
        , m_memSpec(memSpec)
        , m_implicitCommandInserter(std::move(implicitCommandInserter))
    {}

// Private member functions
private:
    void handleRefreshOnBank(Stack_t & stack, Bank & bank, timestamp_t timestamp, uint64_t timing, uint64_t& counter);
    static inline void handlePre_impl(Stack_t & stack, Bank & bank, timestamp_t timestamp, uint64_t& counter);

// Public member functions
public:
    coreRegisterHelper_t getRegisterHelper() {
        return coreRegisterHelper_t{this, m_stacks};
    }

    void handleAct(Stack_t & stack, Bank & bank, timestamp_t timestamp);
    void handlePre(Stack_t & stack, Bank & bank, timestamp_t timestamp);
    void handlePreAll(Stack_t & stack, timestamp_t timestamp); 
    void handleRefAll(Stack_t & stack, timestamp_t timestamp);
    void handleRefSingleBank(Stack_t & stack, Bank & bank, timestamp_t timestamp);
    void handleSelfRefreshEntry(Stack_t & stack, timestamp_t timestamp);
    void handleSelfRefreshExit(Stack_t & stack, timestamp_t timestamp);
    void handleRead(Stack_t & stack, Bank & bank, timestamp_t timestamp);
    void handleWrite(Stack_t & stack, Bank & bank, timestamp_t timestamp);
    void handleReadAuto(Stack_t & stack, Bank & bank, timestamp_t timestamp);
    void handleWriteAuto(Stack_t & stack, Bank & bank, timestamp_t timestamp);
    void handlePowerDownActEntry(Stack_t & stack, timestamp_t timestamp);
    void handlePowerDownActExit(Stack_t & stack, timestamp_t timestamp);
    void handlePowerDownPreEntry(Stack_t & stack, timestamp_t timestamp);
    void handlePowerDownPreExit(Stack_t & stack, timestamp_t timestamp);

    timestamp_t earliestPossiblePowerDownEntryTime(Stack_t & stack) const;

    void getWindowStats(timestamp_t timestamp, SimulationStats &stats) const;

// Overrides
public:
    void serialize(std::ostream& stream) const override;
    void deserialize(std::istream& stream) override;

// Public member variables
public:
    std::vector<Stack_t> m_stacks;

// Private members variables
private:
    const MemSpecHBM2& m_memSpec;
    implicitCommandInserter_t m_implicitCommandInserter;
};

} // namespace DRAMPower


#endif /* DRAMPOWER_STANDARDS_HBM2_HBM2CORE_H */
