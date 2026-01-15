#include "PseudoChannel.h"
#include "DRAMPower/Types.h"

namespace DRAMPower {

PseudoChannel::PseudoChannel(std::size_t numBanks)
    : banks(numBanks)
{}

bool PseudoChannel::isActive(timestamp_t timestamp) {
    return Rank::isActive_impl(timestamp, endRefreshTime, banks);
}

std::size_t PseudoChannel::countActiveBanks() const {
    return Rank::countActiveBanks_impl(banks);
}

void PseudoChannel::serialize(std::ostream& stream) const {
    commandCounter.serialize(stream);
    stream.write(reinterpret_cast<const char*>(&memState), sizeof(memState));
    stream.write(reinterpret_cast<const char* >(&counter.selfRefresh), sizeof(counter.selfRefresh));
    stream.write(reinterpret_cast<const char* >(&counter.deepSleepMode), sizeof(counter.deepSleepMode));
    stream.write(reinterpret_cast<const char* >(&endRefreshTime), sizeof(endRefreshTime));
    cycles.act.serialize(stream);
    cycles.ref.serialize(stream);

    for (const auto& bank : banks) {
        bank.serialize(stream);
    }
};

void PseudoChannel::deserialize(std::istream& stream) {
    commandCounter.deserialize(stream);
    stream.read(reinterpret_cast<char *>(&memState), sizeof(memState));
    stream.read(reinterpret_cast<char* >(&counter.selfRefresh), sizeof(counter.selfRefresh));
    stream.read(reinterpret_cast<char* >(&counter.deepSleepMode), sizeof(counter.deepSleepMode));
    stream.read(reinterpret_cast<char* >(&endRefreshTime), sizeof(endRefreshTime));
    cycles.act.deserialize(stream);
    cycles.ref.deserialize(stream);

    for (auto & bank : banks) {
        bank.deserialize(stream);
    }
};

} // namespace DRAMPower