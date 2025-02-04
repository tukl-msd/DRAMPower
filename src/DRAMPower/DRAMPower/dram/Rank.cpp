#include "Rank.h"

#include <algorithm>

namespace DRAMPower {

Rank::Rank(std::size_t numBanks)
    : banks(numBanks)
{};

bool Rank::isActive(timestamp_t timestamp) {
    if ( timestamp < this->endRefreshTime ) {
        // TODO Invalid in test cases
        // Implicit given in the dram implementation
        std::cout << "[WARN] Rank::isActive() -> timestamp (" << timestamp <<") < "  << "endRefreshTime (" << this->endRefreshTime << ")"  << std::endl;
    }
    return countActiveBanks() > 0 || timestamp < this->endRefreshTime;    // TODO check for endRefreshTime not necessary -> implicit
    /*
        TODO theoretisch check für timestamp < endRefreshTime nicht notwending,
        wenn der Memory Zustand vor dem Check gesetzt wird
    */
}

std::size_t Rank::countActiveBanks() const {
    return (unsigned)std::count_if(banks.begin(), banks.end(),
        [](const auto& bank) {
            return (bank.bankState == Bank::BankState::BANK_ACTIVE);
    });
}

} // namespace DRAMPower