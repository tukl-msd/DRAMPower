#include "energy.h"

using namespace DRAMPower;

double energy_info_t::total() const
{
    auto total = E_act
        + E_pre
        + E_bg_act
        + E_bg_pre

        + E_RD
        + E_WR
        + E_RDA
        + E_WRA
        //+ E_pre_RDA
        //+ E_pre_WRA

        + E_ref_AB
        + E_ref_PB
        + E_ref_SB
        + E_ref_2B;

    return total;
};

void energy_info_t::to_json(json_t &j) const
{
    j = nlohmann::json{
        {"ACT", E_act},
        {"PRE", E_pre},
        {"BG_ACT", E_bg_act},
        {"BG_PRE", E_bg_pre},
        {"RD", E_RD},
        {"WR", E_WR},
        {"RDA", E_RDA},
        {"WRA", E_WRA},
        {"PRE_RDA", E_pre_RDA},
        {"PRE_WRA", E_pre_WRA},
        {"REF_AB", E_ref_AB},
        {"REF_PB", E_ref_PB},
        {"REF_SB", E_ref_SB},
        {"REF_2B", E_ref_2B}
    };

}

energy_info_t& energy_info_t::operator+=(const DRAMPower::energy_info_t& other)
{
    this->E_act += other.E_act;
    this->E_pre += other.E_pre;
    this->E_bg_act += other.E_bg_act;
    this->E_bg_pre += other.E_bg_pre;

    this->E_RD += other.E_RD;
    this->E_WR += other.E_WR;
    this->E_RDA += other.E_RDA;
    this->E_WRA += other.E_WRA;
    this->E_pre_RDA += other.E_pre_RDA;
    this->E_pre_WRA += other.E_pre_WRA;

    this->E_ref_AB += other.E_ref_AB;
    this->E_ref_PB += other.E_ref_PB;
    this->E_ref_SB += other.E_ref_SB;
    this->E_ref_2B += other.E_ref_2B;

    return *this;
}

double DRAMPower::energy_t::total() const
{
    double total = 0.0;

    energy_info_t bank_energy_total;
    for (const auto& bank_e : this->bank_energy)
        bank_energy_total += bank_e;

    total += bank_energy_total.total() + E_bg_act_shared + E_PDNA + E_PDNP + E_sref + E_dsm + E_refab;

    return total;
}

void DRAMPower::interface_energy_info_t::to_json(json_t &j) const
{
    j = nlohmann::json{};
    j["controller"]["dynamicEnergy"] = controller.dynamicEnergy;
    j["controller"]["staticEnergy"] = controller.staticEnergy;
    j["dram"]["dynamicEnergy"] = dram.dynamicEnergy;
    j["dram"]["staticEnergy"] = dram.staticEnergy;
}

void DRAMPower::energy_t::to_json(json_t &j) const
{
    j = nlohmann::json{
        {"E_bg_act_shared", E_bg_act_shared},
        {"E_PDNA", E_PDNA},
        {"E_PDNP", E_PDNP},
        {"E_sref", E_sref},
        {"E_dsm", E_dsm},
        {"E_refab", E_refab}
    };
    // Bank energy
    auto energy_arr = nlohmann::json::array();
    for (const energy_info_t& energy : bank_energy)
    {
        json_t bank_energy_json;
        energy.to_json(bank_energy_json);
        energy_arr.push_back(bank_energy_json);
    }
    j[this->get_Bank_energy_keyword()] = energy_arr;
}

energy_info_t DRAMPower::energy_t::aggregated_bank_energy() const
{
    energy_info_t total;

    for (const auto& bank_e : this->bank_energy)
        total += bank_e;

    total.E_bg_act += this->E_bg_act_shared;

    return total;
}
