#ifndef DRAMPOWER_STANDARDS_DDR4_DDR4CORE_H
#define DRAMPOWER_STANDARDS_DDR4_DDR4CORE_H

#include <DRAMPower/Types.h>
#include <DRAMPower/dram/Rank.h>
#include <DRAMPower/command/Command.h>
#include <DRAMPower/data/stats.h>
#include <DRAMPower/util/ImplicitCommandHandler.h>

#include <DRAMPower/memspec/MemSpecDDR4.h>

#include <vector>
#include <cstddef>
#include <functional>
#include <cassert>

namespace DRAMPower {

template<typename Core>
struct CoreRegisterHelper {
// Public constructors and assignment operators
public:
    CoreRegisterHelper(Core *core, std::vector<Rank> &ranks)
        : m_core(core)
        , m_ranks(ranks)
    {}
    CoreRegisterHelper(const CoreRegisterHelper&) = delete; // No copy constructor
    CoreRegisterHelper& operator=(const CoreRegisterHelper&) = delete; // No copy assignment operator
    CoreRegisterHelper(CoreRegisterHelper&&) = default; // Move constructor
    CoreRegisterHelper& operator=(CoreRegisterHelper&&) = default; // Move assignment operator
    ~CoreRegisterHelper() = default; // Destructor

// Public member functions
public:
    template<typename Func>
    decltype(auto) registerBankHandler(Func &&member_func) {
        Core* this_ptr = m_core;
        return [this_ptr, ranks_ref = std::ref(m_ranks), member_func](const Command & command) {
            auto &this_ranks = ranks_ref.get();
            assert(this_ranks.size()>command.targetCoordinate.rank);
            auto & rank = this_ranks.at(command.targetCoordinate.rank);

            assert(rank.banks.size()>command.targetCoordinate.bank);
            auto & bank = rank.banks.at(command.targetCoordinate.bank);

            rank.commandCounter.inc(command.type);
            (this_ptr->*member_func)(rank, bank, command.timestamp);
        };
    }

    template<typename Func>
    decltype(auto) registerRankHandler(Func &&member_func) {
        Core* this_ptr = m_core;
        return [this_ptr, ranks_ref = std::ref(m_ranks), member_func](const Command & command) {
            auto &this_ranks = ranks_ref.get();
            assert(this_ranks.size()>command.targetCoordinate.rank);
            auto & rank = this_ranks.at(command.targetCoordinate.rank);

            rank.commandCounter.inc(command.type);
            (this_ptr->*member_func)(rank, command.timestamp);
        };
    }

    template<typename Func>
    decltype(auto) registerHandler(Func && member_func) {
        Core* this_ptr = m_core;
        return [this_ptr, member_func](const Command & command) {
            (this_ptr->*member_func)(command.timestamp);
        };
    }

// Private member variables
private:
    Core *m_core;
    std::vector<Rank> &m_ranks;
};

class DDR4Core {
// Public type definitions
public:
    using implicitCommandInserter_t = ImplicitCommandHandler::Inserter_t;
    using coreRegisterHelper_t = CoreRegisterHelper<DDR4Core>;

// Public constructors
public:
    DDR4Core(const MemSpecDDR4 &memSpec, implicitCommandInserter_t&& implicitCommandInserter)
        : m_ranks(memSpec.numberOfRanks, {static_cast<std::size_t>(memSpec.numberOfBanks)}) 
        , m_memSpec(memSpec)
        , m_implicitCommandInserter(std::move(implicitCommandInserter))
    {}

// Public member functions
public:
    coreRegisterHelper_t getRegisterHelper() {
        return coreRegisterHelper_t{this, m_ranks};
    }

// Private member functions
private:
    template<typename Func>
    void addImplicitCommand(timestamp_t timestamp, Func && func) {
        m_implicitCommandInserter.addImplicitCommand(timestamp, std::forward<Func>(func));
    }

#ifdef DRAMPOWER_TESTING
public:
    const std::vector<Rank>& getRanks() const {
        return m_ranks;
    }
    std::vector<Rank>& getRanks() {
        return m_ranks;
    }
#endif

// Public member functions
public:
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

// Public members
public:
    std::vector<Rank> m_ranks;

// Private members
private:
    const MemSpecDDR4& m_memSpec;
    implicitCommandInserter_t m_implicitCommandInserter;
};

} // namespace DRAMPower


#endif /* DRAMPOWER_STANDARDS_DDR4_DDR4CORE_H */
