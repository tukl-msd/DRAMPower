#ifndef UTIL_HPP
#define UTIL_HPP


#include "csv.hpp"

#include <DRAMPower/data/energy.h>
#include <spdlog/fmt/ostr.h>

#include <stdint.h>

namespace DRAMPower::DRAMPowerCLI::util {

    // Util function to get the memory
    std::unique_ptr<uint8_t[]> hexStringToUint8Array(const csv::string_view data, size_t &size);

} // namespace DRAMPower::DRAMPowerCLI::util

template <> struct fmt::formatter<DRAMPower::energy_t>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

    template <typename FormatContext>
    auto format(const DRAMPower::energy_t& e, FormatContext& ctx) const
    {
        return fmt::format_to(ctx.out(),
                              "E_bg_act_shared: {} "
                              "E_PDNA: {} "
                              "E_PDNP: {} "
                              "E_sref: {} "
                              "E_dsm: {} "
                              "E_refab: {}",
                              e.E_bg_act_shared,
                              e.E_PDNA,
                              e.E_PDNP,
                              e.E_sref,
                              e.E_dsm,
                              e.E_refab);
    }
};

template <> struct fmt::formatter<DRAMPower::interface_energy_info_t>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

    template <typename FormatContext>
    auto format(const DRAMPower::interface_energy_info_t& e, FormatContext& ctx) const
    {
        return fmt::format_to(ctx.out(),
                              "Controller: dynamicEnergy: {} "
                              "staticEnergy: {}\n"
                              "DRAM: dynamicEnergy: {} "
                              "staticEnergy: {}\n"
                              "Total: {}",
                              e.controller.dynamicEnergy,
                              e.controller.staticEnergy,
                              e.dram.dynamicEnergy,
                              e.dram.staticEnergy,
                              e.total());
    }
};

template <> struct fmt::formatter<DRAMPower::energy_info_t>
{
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

    template <typename FormatContext>
    auto format(const DRAMPower::energy_info_t& ei, FormatContext& ctx) const
    {
        return fmt::format_to(ctx.out(),
                              "ACT: {} "
                              "PRE: {} "
                              "BG_ACT: {} "
                              "BG_PRE: {} "
                              "RD: {} "
                              "WR: {} "
                              "RDA: {} "
                              "WRA: {} "
                              "PRE_RDA: {} "
                              "PRE_WRA: {} "
                              "REF_AB: {} "
                              "REF_PB: {} "
                              "REF_SB: {} "
                              "REF_2B: {}",
                              ei.E_act,
                              ei.E_pre,
                              ei.E_bg_act,
                              ei.E_bg_pre,
                              ei.E_RD,
                              ei.E_WR,
                              ei.E_RDA,
                              ei.E_WRA,
                              ei.E_pre_RDA,
                              ei.E_pre_WRA,
                              ei.E_ref_AB,
                              ei.E_ref_PB,
                              ei.E_ref_SB,
                              ei.E_ref_2B);
    }
};

#endif