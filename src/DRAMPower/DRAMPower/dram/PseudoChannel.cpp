#include "PseudoChannel.h"
#include "DRAMPower/Types.h"

namespace DRAMPower {

PseudoChannel::PseudoChannel(std::size_t numBanks)
    : banks(numBanks)
{}

void PseudoChannel::reset() {
    memState = MemState::NOT_IN_PD;
    cycles = {};
    counter = {};
    for(auto& entry : banks) {
        entry.reset();
    }
}

bool PseudoChannel::isActive() {
    return Rank::isActive_impl(banks);
}

std::size_t PseudoChannel::countActiveBanks() const {
    return Rank::countActiveBanks_impl(banks);
}

} // namespace DRAMPower