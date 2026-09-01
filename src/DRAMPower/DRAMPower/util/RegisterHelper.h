#ifndef DRAMPOWER_UTIL_REGISTERHELPER_H
#define DRAMPOWER_UTIL_REGISTERHELPER_H

#include <vector>

#include <DRAMPower/command/Command.h>
#include <DRAMPower/dram/Rank.h>
#include <DRAMPower/dram/PseudoChannel.h>

namespace DRAMPower::util {

namespace coreHelpers {

namespace detail {

struct ByRef {};
struct ByIdx {};

template<typename Sel, typename T>
constexpr decltype(auto) pick(T& obj, std::size_t idx) noexcept {
    if constexpr (std::is_same_v<Sel, ByIdx>) {
        return idx;
    } else {
        return obj;
    }
}

inline std::size_t checked(std::size_t idx, std::size_t size, const char* errorDescription) {
    assert(idx < size && errorDescription);
    (void)size; // not used
    (void)errorDescription; // not used
    return idx;
}

template<typename GroupSel, typename BankSel, typename Cmd, typename Group, typename Mapping, typename Func, typename... Ctx>
decltype(auto) dispatchBank(const Cmd& cmd, std::vector<Group>& groups, const Mapping& map, Func&& func, Ctx&&... ctx) {
    const std::size_t groupIdx = checked(map.group(cmd), groups.size(), "Invalid group coordinate");
    auto& group = groups[groupIdx];
    const std::size_t bankIdx = checked(map.bank(cmd), group.banks.size(), "Invalid bank coordinate");
    auto& bank = group.banks[bankIdx];
    return std::invoke(std::forward<Func>(func), std::forward<Ctx>(ctx)...,
        pick<GroupSel>(group, groupIdx),
        pick<BankSel>(bank, bankIdx),
        cmd.timestamp);
}

template<typename GroupSel, typename Cmd, typename Group, typename Mapping, typename Func, typename... Ctx>
decltype(auto) dispatchGroup(const Cmd& cmd, std::vector<Group>& groups, const Mapping& map, Func&& func, Ctx&&... ctx) {
    const std::size_t groupIdx = checked(map.group(cmd), groups.size(), "Invalid group coordinate");
    auto& group = groups[groupIdx];
    return std::invoke(std::forward<Func>(func), std::forward<Ctx>(ctx)...,
        pick<GroupSel>(group, groupIdx),
        cmd.timestamp);
}

} // namespace detail

template<typename Cmd, typename Group, typename Mapping, typename Func, typename... Ctx>
decltype(auto) bankHandler(const Cmd& cmd, std::vector<Group>& groups,
                           const Mapping& map, Func&& func, Ctx&&... ctx)
{   // (Group&, Bank&, timestamp)
    return detail::dispatchBank<detail::ByRef, detail::ByRef>(
        cmd, groups, map, std::forward<Func>(func), std::forward<Ctx>(ctx)...);
}

template<typename Cmd, typename Group, typename Mapping, typename Func, typename... Ctx>
decltype(auto) bankHandlerIdx(const Cmd& cmd, std::vector<Group>& groups,
                              const Mapping& map, Func&& func, Ctx&&... ctx)
{   // (groupIdx, bankIdx, timestamp)
    return detail::dispatchBank<detail::ByIdx, detail::ByIdx>(
        cmd, groups, map, std::forward<Func>(func), std::forward<Ctx>(ctx)...);
}

template<typename Cmd, typename Group, typename Mapping, typename Func, typename... Ctx>
decltype(auto) bankGroupHandler(const Cmd& cmd, std::vector<Group>& groups,
                                const Mapping& map, Func&& func, Ctx&&... ctx)
{   // (Group&, bankIdx, timestamp)
    return detail::dispatchBank<detail::ByRef, detail::ByIdx>(
        cmd, groups, map, std::forward<Func>(func), std::forward<Ctx>(ctx)...);
}

template<typename Cmd, typename Group, typename Mapping, typename Func, typename... Ctx>
decltype(auto) groupHandler(const Cmd& cmd, std::vector<Group>& groups,
                            const Mapping& map, Func&& func, Ctx&&... ctx)
{
    // (Group&, timestamp)
    return detail::dispatchGroup<detail::ByRef>(
        cmd, groups, map, std::forward<Func>(func), std::forward<Ctx>(ctx)...);
}

template<typename Cmd, typename Group, typename Mapping, typename Func, typename... Ctx>
decltype(auto) groupHandlerIdx(const Cmd& cmd, std::vector<Group>& groups,
                               const Mapping& map, Func&& func, Ctx&&... ctx)
{
    // (groupIdx, timestamp)
    return detail::dispatchGroup<detail::ByIdx>(
        cmd, groups, map, std::forward<Func>(func), std::forward<Ctx>(ctx)...);
}

} // namespace coreHelpers

} // namespace DRAMPower::util

#endif /* DRAMPOWER_UTIL_REGISTERHELPER_H */
