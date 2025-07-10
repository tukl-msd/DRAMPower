#include "Rank.h"

#include <algorithm>

namespace DRAMPower {

Rank::Rank(std::size_t numBanks)
    : banks(numBanks)
{};

bool Rank::isActive(timestamp_t timestamp) {
    if ( timestamp < this->endRefreshTime ) {
        std::cout << "[WARN] Rank::isActive() -> timestamp (" << timestamp <<") < "  << "endRefreshTime (" << this->endRefreshTime << ")"  << std::endl;
    }
    return countActiveBanks() > 0 || timestamp < this->endRefreshTime;
}

std::size_t Rank::countActiveBanks() const {
    return (unsigned)std::count_if(banks.begin(), banks.end(),
        [](const auto& bank) {
            return (bank.bankState == Bank::BankState::BANK_ACTIVE);
    });
}

} // namespace DRAMPower