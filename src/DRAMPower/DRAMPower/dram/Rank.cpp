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

bool Rank::isActive_impl(const timestamp_t& timestamp, const timestamp_t& endRefreshTime, const std::vector<Bank>& banks) {
    if ( timestamp < endRefreshTime ) {
        std::cout << "[WARN] Rank::isActive() -> timestamp (" << timestamp <<") < "  << "endRefreshTime (" << endRefreshTime << ")"  << std::endl;
    }
    return countActiveBanks_impl(banks) > 0 || timestamp < endRefreshTime;
}

bool Rank::isActive(timestamp_t timestamp) {
    return isActive_impl(timestamp, endRefreshTime, banks);
}

std::size_t Rank::countActiveBanks() const {
    return countActiveBanks_impl(banks);
}

void Rank::serialize(std::ostream& stream) const {
    commandCounter.serialize(stream);
    stream.write(reinterpret_cast<const char*>(&memState), sizeof(memState));
    stream.write(reinterpret_cast<const char* >(&counter.selfRefresh), sizeof(counter.selfRefresh));
    stream.write(reinterpret_cast<const char* >(&counter.deepSleepMode), sizeof(counter.deepSleepMode));
    stream.write(reinterpret_cast<const char* >(&endRefreshTime), sizeof(endRefreshTime));
    cycles.pre.serialize(stream);
    cycles.act.serialize(stream);
    cycles.ref.serialize(stream);
    cycles.sref.serialize(stream);
    cycles.powerDownAct.serialize(stream);
    cycles.powerDownPre.serialize(stream);
    cycles.deepSleepMode.serialize(stream);

    for (const auto& bank : banks) {
        bank.serialize(stream);
    }
};

void Rank::deserialize(std::istream& stream) {
    commandCounter.deserialize(stream);
    stream.read(reinterpret_cast<char *>(&memState), sizeof(memState));
    stream.read(reinterpret_cast<char* >(&counter.selfRefresh), sizeof(counter.selfRefresh));
    stream.read(reinterpret_cast<char* >(&counter.deepSleepMode), sizeof(counter.deepSleepMode));
    stream.read(reinterpret_cast<char* >(&endRefreshTime), sizeof(endRefreshTime));
    cycles.pre.deserialize(stream);
    cycles.act.deserialize(stream);
    cycles.ref.deserialize(stream);
    cycles.sref.deserialize(stream);
    cycles.powerDownAct.deserialize(stream);
    cycles.powerDownPre.deserialize(stream);
    cycles.deepSleepMode.deserialize(stream);

    for (auto & bank : banks) {
        bank.deserialize(stream);
    }
};


void RankInterface::serialize(std::ostream& stream) const {
    stream.write(reinterpret_cast<const char*>(&seamlessPrePostambleCounter_read), sizeof(seamlessPrePostambleCounter_read));
    stream.write(reinterpret_cast<const char*>(&seamlessPrePostambleCounter_write), sizeof(seamlessPrePostambleCounter_write));
    stream.write(reinterpret_cast<const char*>(&mergedPrePostambleCounter_read), sizeof(mergedPrePostambleCounter_read));
    stream.write(reinterpret_cast<const char*>(&mergedPrePostambleCounter_write), sizeof(mergedPrePostambleCounter_write));
    stream.write(reinterpret_cast<const char*>(&mergedPrePostambleTime_read), sizeof(mergedPrePostambleTime_read));
    stream.write(reinterpret_cast<const char*>(&mergedPrePostambleTime_write), sizeof(mergedPrePostambleTime_write));
    stream.write(reinterpret_cast<const char*>(&lastReadEnd), sizeof(lastReadEnd));
    stream.write(reinterpret_cast<const char*>(&lastWriteEnd), sizeof(lastWriteEnd));
}
void RankInterface::deserialize(std::istream& stream) {
    stream.read(reinterpret_cast<char*>(&seamlessPrePostambleCounter_read), sizeof(seamlessPrePostambleCounter_read));
    stream.read(reinterpret_cast<char*>(&seamlessPrePostambleCounter_write), sizeof(seamlessPrePostambleCounter_write));
    stream.read(reinterpret_cast<char*>(&mergedPrePostambleCounter_read), sizeof(mergedPrePostambleCounter_read));
    stream.read(reinterpret_cast<char*>(&mergedPrePostambleCounter_write), sizeof(mergedPrePostambleCounter_write));
    stream.read(reinterpret_cast<char*>(&mergedPrePostambleTime_read), sizeof(mergedPrePostambleTime_read));
    stream.read(reinterpret_cast<char*>(&mergedPrePostambleTime_write), sizeof(mergedPrePostambleTime_write));
    stream.read(reinterpret_cast<char*>(&lastReadEnd), sizeof(lastReadEnd));
    stream.read(reinterpret_cast<char*>(&lastWriteEnd), sizeof(lastWriteEnd));
}

} // namespace DRAMPower