#include "PseudoChannel.h"
#include "DRAMPower/Types.h"

namespace DRAMPower {

PseudoChannel::PseudoChannel(std::size_t numBanks)
    : banks(numBanks)
{}

bool PseudoChannel::isActive() {
    return Rank::isActive_impl(banks);
}

std::size_t PseudoChannel::countActiveBanks() const {
    return Rank::countActiveBanks_impl(banks);
}

} // namespace DRAMPower