#ifndef DRAMPOWER_UTIL_REGISTERHELPER_H
#define DRAMPOWER_UTIL_REGISTERHELPER_H

#include <vector>

#include <DRAMPower/command/Command.h>
#include <DRAMPower/dram/Rank.h>

namespace DRAMPower::util {

namespace coreHelpers {
    template<typename Func, typename Owner, typename Command_t = Command>
    decltype(auto) bankHandler(const Command_t& cmd, std::vector<Rank>& ranks, Owner owner, Func &&member_func) {
        assert(ranks.size()>cmd.targetCoordinate.rank);
        auto & rank = ranks.at(cmd.targetCoordinate.rank);

        assert(rank.banks.size()>cmd.targetCoordinate.bank);
        auto & bank = rank.banks.at(cmd.targetCoordinate.bank);

        rank.commandCounter.inc(cmd.type);
        return (owner->*member_func)(rank, bank, cmd.timestamp);
    }

    template<typename Func, typename Owner, typename Command_t = Command>
    decltype(auto) bankHandlerIdx(const Command_t& cmd, std::vector<Rank>& ranks, Owner owner, Func &&member_func) {
        assert(ranks.size()>cmd.targetCoordinate.rank);
        assert(ranks.at(cmd.targetCoordinate.rank).banks.size()>cmd.targetCoordinate.bank);

        ranks.at(cmd.targetCoordinate.rank).commandCounter.inc(cmd.type);
        return (owner->*member_func)(cmd.targetCoordinate.rank, cmd.targetCoordinate.bank, cmd.timestamp);
    }

    template<typename Func, typename Owner, typename Command_t = Command>
    decltype(auto) rankHandler(const Command_t& cmd, std::vector<Rank>& ranks, Owner owner, Func &&member_func) {
        assert(ranks.size()>cmd.targetCoordinate.rank);
        auto & rank = ranks.at(cmd.targetCoordinate.rank);

        rank.commandCounter.inc(cmd.type);
        return (owner->*member_func)(rank, cmd.timestamp);
    }

    template<typename Func, typename Owner, typename Command_t = Command>
    decltype(auto) rankHandlerIdx(const Command_t& cmd, std::vector<Rank>& ranks, Owner owner, Func &&member_func) {
        assert(ranks.size()>cmd.targetCoordinate.rank);
        ranks.at(cmd.targetCoordinate.rank).commandCounter.inc(cmd.type);
        return (owner->*member_func)(cmd.targetCoordinate.rank, cmd.timestamp);
    }

    template<typename Func, typename Owner, typename Command_t = Command>
    decltype(auto) bankGroupHandler(const Command_t& cmd, std::vector<Rank>& ranks, Owner owner, Func &&member_func) {
        assert(ranks.size()>cmd.targetCoordinate.rank);
        auto& rank = ranks.at(cmd.targetCoordinate.rank);

        assert(rank.banks.size()>cmd.targetCoordinate.bank);
        if (cmd.targetCoordinate.bank >= rank.banks.size()) {
            throw std::invalid_argument("Invalid bank targetcoordinate");
        }
        auto bank_id = cmd.targetCoordinate.bank;

        rank.commandCounter.inc(cmd.type);
        return (owner->*member_func)(rank, bank_id, cmd.timestamp);
    }

    template<typename Func, typename Owner, typename Command_t = Command>
    decltype(auto) bankGroupHandlerIdx(const Command_t& cmd, std::vector<Rank>& ranks, Owner owner, Func &&member_func) {
        assert(ranks.size()>cmd.targetCoordinate.rank);
        auto& rank = ranks.at(cmd.targetCoordinate.rank);

        assert(rank.banks.size()>cmd.targetCoordinate.bank);
        if (cmd.targetCoordinate.bank >= rank.banks.size()) {
            throw std::invalid_argument("Invalid bank targetcoordinate");
        }
        auto bank_id = cmd.targetCoordinate.bank;

        rank.commandCounter.inc(cmd.type);
        return (owner->*member_func)(cmd.targetCoordinate.rank, bank_id, cmd.timestamp);
    }
} // namespace coreHelpers

} // namespace DRAMPower::util

#endif /* DRAMPOWER_UTIL_REGISTERHELPER_H */
