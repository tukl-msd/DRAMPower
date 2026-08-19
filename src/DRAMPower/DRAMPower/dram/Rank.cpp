#include "Rank.h"
#include "DRAMPower/Types.h"

#include <algorithm>

namespace DRAMPower {

Rank::Rank(std::size_t numBanks)
    : banks(numBanks)
{}

std::size_t Rank::countActiveBanks_impl(const std::vector<Bank>& banks) {
    return static_cast<unsigned>(std::count_if(banks.begin(), banks.end(),
        [](const auto& bank) {
            return (bank.bankState == Bank::BankState::BANK_ACTIVE);
    }));
}

bool Rank::isActive_impl(const std::vector<Bank>& banks) {
    return countActiveBanks_impl(banks) > 0;
}

bool Rank::isActive() {
    return isActive_impl(banks);
}

std::size_t Rank::countActiveBanks() const {
    return countActiveBanks_impl(banks);
}

} // namespace DRAMPower