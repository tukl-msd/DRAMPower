#ifndef DRAMPOWER_UTIL_REGISTERMAPPING_H
#define DRAMPOWER_UTIL_REGISTERMAPPING_H

#include <cstddef>
#include <cstdint>


namespace DRAMPower::util::coreHelpers {

struct RankMapping {
    template<typename Cmd>
    static constexpr std::size_t group(const Cmd& cmd) noexcept {
        return cmd.targetCoordinate.rank;
    }

    template<typename Cmd>
    static constexpr std::size_t bank(const Cmd& cmd) noexcept {
        return cmd.targetCoordinate.bank;
    }
};

struct PseudoChannelMapping {
    uint64_t banksPerStack = 0;

    template<typename Cmd>
    constexpr std::size_t group(const Cmd& cmd) const noexcept {
        return cmd.targetCoordinate.pseudoChannel;
    }

    template<typename Cmd>
    constexpr std::size_t bank(const Cmd& cmd) const noexcept {
        return cmd.targetCoordinate.bank
             + cmd.targetCoordinate.stack * banksPerStack;
    }
};

} // namespace DRAMPower::util::coreHelpers

#endif /* DRAMPOWER_UTIL_REGISTERMAPPING_H */
