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

energy_info_t DRAMPower::energy_t::total_energy()
{
    energy_info_t total;

    for (const auto& bank_e : this->bank_energy)
        total += bank_e;

    total.E_bg_act += this->E_bg_act_shared;

    return total;
}
