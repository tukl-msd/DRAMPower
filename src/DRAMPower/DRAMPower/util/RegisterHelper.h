#ifndef DRAMPOWER_STANDARDS_REGISTERHELPER_H
#define DRAMPOWER_STANDARDS_REGISTERHELPER_H

#include <vector>

#include <DRAMPower/command/Command.h>
#include <DRAMPower/dram/Rank.h>
#include <DRAMPower/dram/PseudoChannel.h>

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
struct CoreRegisterHelperPseudoChannel {
// Public constructors and assignment operators
public:
    using commandCounter_t = util::CommandCounter<CmdType>;

    template<typename... Args>
    CoreRegisterHelperPseudoChannel(Core *core, std::vector<PseudoChannel> &pseudoChannel, Args&&... args)
        : m_core(core)
        , m_pseudoChannel(pseudoChannel)
        , m_bankExtractor(std::forward<Args>(args)...)
    {}
    CoreRegisterHelperPseudoChannel(const CoreRegisterHelperPseudoChannel&) = delete; // No copy constructor
    CoreRegisterHelperPseudoChannel& operator=(const CoreRegisterHelperPseudoChannel&) = delete; // No copy assignment operator
    CoreRegisterHelperPseudoChannel(CoreRegisterHelperPseudoChannel&&) = default; // Move constructor
    CoreRegisterHelperPseudoChannel& operator=(CoreRegisterHelperPseudoChannel&&) = default; // Move assignment operator
    ~CoreRegisterHelperPseudoChannel() = default; // Destructor

// Public member functions
public:
    template<typename Func>
    decltype(auto) registerBankHandler(Func &&member_func) {
        Core* this_ptr = m_core;
        return [this, this_ptr, pseudoChannel_ref = std::ref(m_pseudoChannel), f = std::forward<Func>(member_func)](const Command & command) {
            auto &this_pseudoChannel = pseudoChannel_ref.get();

            assert(this_pseudoChannel.size() > command.targetCoordinate.pseudoChannel);
            auto & pseudoChannel = this_pseudoChannel.at(command.targetCoordinate.pseudoChannel);

            std::size_t bank_id = m_bankExtractor(command.targetCoordinate);
            assert(pseudoChannel.banks.size() > bank_id);
            auto & bank = pseudoChannel.banks.at(bank_id);

            (this_ptr->*f)(pseudoChannel, bank, command.timestamp);
            pseudoChannel.commandCounter.inc(command.type);
        };
    }

    template<typename Func>
    decltype(auto) registerPseudoChannelHandler(Func &&member_func) {
        Core* this_ptr = m_core;
        return [this_ptr, pseudoChannel_ref = std::ref(m_pseudoChannel), f = std::forward<Func>(member_func)](const Command & command) mutable {
            auto &this_pseudoChannel = pseudoChannel_ref.get();

            assert(this_pseudoChannel.size() > command.targetCoordinate.pseudoChannel);
            auto & pseudoChannel = this_pseudoChannel.at(command.targetCoordinate.pseudoChannel);

            (this_ptr->*f)(pseudoChannel, command.timestamp);
            pseudoChannel.commandCounter.inc(command.type);
        };
    }

    template <typename Func>
    decltype(auto) registerBankGroupHandler(Func &&member_func) {
        Core *this_ptr = m_core;
        return [this, this_ptr, pseudoChannel_ref = std::ref(m_pseudoChannel), f = std::forward<Func>(member_func)](const Command & command) {
            auto &this_pseudoChannel = pseudoChannel_ref.get();

            assert(this_pseudoChannel.size() > command.targetCoordinate.pseudoChannel);
            auto & pseudoChannel = this_pseudoChannel.at(command.targetCoordinate.pseudoChannel);

            std::size_t bank_id = m_bankExtractor(command.targetCoordinate);
            assert(pseudoChannel.banks.size() > bank_id);
            if (command.targetCoordinate.bank >= pseudoChannel.banks.size()) {
                throw std::invalid_argument("Invalid bank targetcoordinate");
            }
            (this_ptr->*f)(pseudoChannel, bank_id, command.timestamp);
            pseudoChannel.commandCounter.inc(command.type);
        };
    }

    template<typename Func>
    decltype(auto) registerHandler(Func &&member_func) {
        Core* this_ptr = m_core;
        return [this_ptr, f = std::forward<Func>(member_func)](const Command & command) {
            (this_ptr->*f)(command.timestamp);
        };
    }

// Private member variables
private:
    Core *m_core;
    std::vector<PseudoChannel>& m_pseudoChannel;
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
