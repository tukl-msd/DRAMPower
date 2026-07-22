#include "DRAMPower/standards/ddr4/DDR4ImplicitCommands.h"

#include "DRAMPower/standards/ddr4/DDR4Core.h"


namespace DRAMPower {

void DDR4ImplicitCommandsBridge::handlePreAferRef(DDR4Core& core, Container_rbt data) {
    auto& rank = core.m_ranks[data.r];
    auto& bank = rank.banks[data.b];
    bank.bankState = Bank::BankState::BANK_PRECHARGED;
    bank.cycles.act.close_interval(data.t);
    // stop rank active interval if no more banks active
    if (!rank.isActive(data.t))                                  // stop rank active interval if no more banks active
    {
        rank.cycles.act.close_interval(data.t);
        //rank.cycles.pre.start_interval(timestamp_end);
    }
}

void DDR4ImplicitCommandsBridge::handlePre(DDR4Core& core, Container_rbt data) {
    core.handlePre(core.m_ranks[data.r], core.m_ranks[data.r].banks[data.b], data.t);
}

void DDR4ImplicitCommandsBridge::handleRefEntry(DDR4Core& core, Container_rt data) {
    auto& rank = core.m_ranks[data.r];
    rank.counter.selfRefresh++;
    rank.cycles.sref.start_interval(data.t);
    rank.memState = MemState::SREF;
}

void DDR4ImplicitCommandsBridge::handlePDActEntry(DDR4Core& core, Container_rt data) {
    auto& rank = core.m_ranks[data.r];
    rank.memState = MemState::PDN_ACT;
    rank.cycles.powerDownAct.start_interval(data.t);
    rank.cycles.act.close_interval(data.t);
    //rank.cycles.pre.close_interval(entryTime);
    for (auto & bank : rank.banks) {
        bank.cycles.act.close_interval(data.t);
    }
}

void DDR4ImplicitCommandsBridge::handlePDPreEntry(DDR4Core& core, Container_rt data) {
    auto& rank = core.m_ranks[data.r];
    for (auto &bank : rank.banks)
        bank.cycles.act.close_interval(data.t);
    rank.memState = MemState::PDN_PRE;
    rank.cycles.powerDownPre.start_interval(data.t);
    //rank.cycles.pre.close_interval(data.t);
    rank.cycles.act.close_interval(data.t);
}

void DDR4ImplicitCommandsBridge::handlePDExit(DDR4Core& core, Container_rt data) {
    auto& rank = core.m_ranks[data.r];
    rank.memState = MemState::NOT_IN_PD;
    rank.cycles.powerDownAct.close_interval(data.t);
    rank.cycles.powerDownPre.close_interval(data.t);

    // Activate banks that were active prior to PDA 
    for (auto & bank : rank.banks)
    {
        if (bank.bankState==Bank::BankState::BANK_ACTIVE)
        {
            bank.cycles.act.start_interval(data.t);
        }
    }
    // Activate rank if at least one bank is active
    // At least one bank must be active for PDA -> remove if statement?
    if(rank.isActive(data.t))
        rank.cycles.act.start_interval(data.t); 
}

} // namespace DRAMPower