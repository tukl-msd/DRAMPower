#ifndef DRAMPOWER_STANDARDS_REGISTERHELPER_H
#define DRAMPOWER_STANDARDS_REGISTERHELPER_H

#include <vector>

#include <DRAMPower/command/Command.h>
#include <DRAMPower/dram/Rank.h>

namespace DRAMPower::util {

namespace coreHelpers {
    template<typename Func, typename Owner>
    decltype(auto) bankHandler(const Command& cmd, std::vector<Rank>& ranks, Owner owner, Func &&member_func) {
        assert(ranks.size()>cmd.targetCoordinate.rank);
        auto & rank = ranks.at(cmd.targetCoordinate.rank);

        assert(rank.banks.size()>cmd.targetCoordinate.bank);
        auto & bank = rank.banks.at(cmd.targetCoordinate.bank);

        rank.commandCounter.inc(cmd.type);
        return (owner->*member_func)(rank, bank, cmd.timestamp);
    }

    template<typename Func, typename Owner>
    decltype(auto) rankHandler(const Command& cmd, std::vector<Rank>& ranks, Owner owner, Func &&member_func) {
        assert(ranks.size()>cmd.targetCoordinate.rank);
        auto & rank = ranks.at(cmd.targetCoordinate.rank);

        rank.commandCounter.inc(cmd.type);
        return (owner->*member_func)(rank, cmd.timestamp);
    }

    template<typename Func, typename Owner>
    decltype(auto) bankGroupHandler(const Command& cmd, std::vector<Rank>& ranks, Owner owner, Func &&member_func) {
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
} // namespace coreHelpers

} // namespace DRAMPower::util

#endif /* DRAMPOWER_STANDARDS_REGISTERHELPER_H */
