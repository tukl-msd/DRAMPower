#ifndef DRAMPOWER_STANDARDS_REGISTERHELPER_H
#define DRAMPOWER_STANDARDS_REGISTERHELPER_H

#include <vector>

#include <DRAMPower/command/Command.h>
#include <DRAMPower/dram/Rank.h>

namespace DRAMPower::util {

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

    template <typename Func>
    decltype(auto) registerBankGroupHandler(Func &&member_func) {
        Core *this_ptr = m_core;
        return [this_ptr, ranks_ref = std::ref(m_ranks), member_func](const Command & command) {
            auto &this_ranks = ranks_ref.get();
            assert(this_ranks.size()>command.targetCoordinate.rank);
            auto& rank = this_ranks.at(command.targetCoordinate.rank);

            assert(rank.banks.size()>command.targetCoordinate.bank);
            if (command.targetCoordinate.bank >= rank.banks.size()) {
                throw std::invalid_argument("Invalid bank targetcoordinate");
            }
            auto bank_id = command.targetCoordinate.bank;
            
            rank.commandCounter.inc(command.type);
            (this_ptr->*member_func)(rank, bank_id, command.timestamp);
        };
    }

    template<typename Func>
    decltype(auto) registerHandler(Func &&member_func) {
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

template<typename Core, typename BankExtractor>
struct CoreRegisterHelperNoRank {
// Public constructors and assignment operators
public:
    using commandCounter_t = util::CommandCounter<CmdType>;

    template<typename... Args>
    CoreRegisterHelperNoRank(Core *core, std::vector<Bank> &banks, commandCounter_t &commandCounter, Args&&... args)
        : m_core(core)
        , m_banks(banks)
        , m_commandCounter(commandCounter)
        , m_bankExtractor(std::forward<Args>(args)...)
    {}
    CoreRegisterHelperNoRank(const CoreRegisterHelperNoRank&) = delete; // No copy constructor
    CoreRegisterHelperNoRank& operator=(const CoreRegisterHelperNoRank&) = delete; // No copy assignment operator
    CoreRegisterHelperNoRank(CoreRegisterHelperNoRank&&) = default; // Move constructor
    CoreRegisterHelperNoRank& operator=(CoreRegisterHelperNoRank&&) = default; // Move assignment operator
    ~CoreRegisterHelperNoRank() = default; // Destructor

// Public member functions
public:
    template<typename Func>
    decltype(auto) registerBankHandler(Func &&member_func) {
        Core* this_ptr = m_core;
        return [this, this_ptr, banks_ref = std::ref(m_banks), commandCounter_ref = std::ref(m_commandCounter), f = std::forward<Func>(member_func)](const Command & command) {
            auto &this_banks = banks_ref.get();
            auto &this_commandCounter = commandCounter_ref.get();

            std::size_t bank_id = m_bankExtractor(command.targetCoordinate);
            assert(this_banks.size()>bank_id);
            auto & bank = this_banks.at(bank_id);

            (this_ptr->*f)(bank, command.timestamp);
            this_commandCounter.inc(command.type);
        };
    }

    template<typename Func>
    decltype(auto) registerRankHandler(Func &&member_func) {
        Core* this_ptr = m_core;
        return [this_ptr, commandCounter_ref = std::ref(m_commandCounter), f = std::forward<Func>(member_func)](const Command & command) mutable {
            auto &this_commandCounter = commandCounter_ref.get();
            (this_ptr->*f)(command.timestamp);
            this_commandCounter.inc(command.type);
        };
    }

    template <typename Func>
    decltype(auto) registerBankGroupHandler(Func &&member_func) {
        Core *this_ptr = m_core;
        return [this, this_ptr, banks_ref = std::ref(m_banks), commandCounter_ref = std::ref(m_commandCounter), f = std::forward<Func>(member_func)](const Command & command) {
            auto &this_banks = banks_ref.get();
            auto &this_commandCounter = commandCounter_ref.get();

            std::size_t bank_id = m_bankExtractor(command.targetCoordinate);
            assert(this_banks.size()>bank_id);
            if (command.targetCoordinate.bank >= this_banks.size()) {
                throw std::invalid_argument("Invalid bank targetcoordinate");
            }
            (this_ptr->*f)(bank_id, command.timestamp);
            this_commandCounter.inc(command.type);
        };
    }

    template<typename Func>
    decltype(auto) registerHandler(Func &&member_func) {
        Core* this_ptr = m_core;
        return [commandCounter_ref = std::ref(m_commandCounter), this_ptr, f = std::forward<Func>(member_func)](const Command & command) {
            auto &this_commandCounter = commandCounter_ref.get();
            (this_ptr->*f)(command.timestamp);
            this_commandCounter.inc(command.type);
        };
    }

// Private member variables
private:
    Core *m_core;
    std::vector<Bank> &m_banks;
    commandCounter_t &m_commandCounter;
    BankExtractor m_bankExtractor;
};

template<typename Interface>
struct InterfaceRegisterHelper {
// Public constructors and assignment operators
public:
    InterfaceRegisterHelper(Interface *interface)
        : m_interface(interface)
    {}
    InterfaceRegisterHelper(const InterfaceRegisterHelper&) = default; // copy constructor
    InterfaceRegisterHelper& operator=(const InterfaceRegisterHelper&) = default; // copy assignment operator
    InterfaceRegisterHelper(InterfaceRegisterHelper&&) = default; // Move constructor
    InterfaceRegisterHelper& operator=(InterfaceRegisterHelper&&) = default; // Move assignment operator
    ~InterfaceRegisterHelper() = default; // Destructor

// Public member functions
    template<typename Func>
    decltype(auto) registerHandler(Func &&member_func) {
        Interface *this_ptr = m_interface;
        return [this_ptr, member_func](const Command & command) {
            (this_ptr->*member_func)(command);
        };
    }

// Private member variables
private:
    Interface *m_interface;
};

} // namespace DRAMPower::util

#endif /* DRAMPOWER_STANDARDS_REGISTERHELPER_H */
