#ifndef DRAMPOWER_DDR_BANK_H
#define DRAMPOWER_DDR_BANK_H

#include <DRAMPower/data/stats.h>
#include <DRAMPower/Types.h>
#include <DRAMPower/util/Serialize.h>
#include <DRAMPower/util/Deserialize.h>

#include <stdint.h>

namespace DRAMPower {

struct Bank : public util::Serialize, public util::Deserialize {
public:
    enum class BankState {
        BANK_PRECHARGED = 0,
        BANK_ACTIVE = 1,
    };
public:
    CycleStats::command_stats_t counter;

    struct {
        interval_t act;
        interval_t powerDownAct;
        interval_t powerDownPre;
    } cycles;

    BankState bankState;
public:
    timestamp_t latestPre = 0;
    timestamp_t refreshEndTime = 0;

    void serialize(std::ostream& stream) const override {
        stream.write(reinterpret_cast<const char *>(&bankState), sizeof(bankState));
        stream.write(reinterpret_cast<const char *>(&latestPre), sizeof(latestPre));
        stream.write(reinterpret_cast<const char *>(&refreshEndTime), sizeof(refreshEndTime));
        cycles.act.serialize(stream);
        cycles.powerDownAct.serialize(stream);
        cycles.powerDownPre.serialize(stream);
        counter.serialize(stream);
    }
    void deserialize(std::istream& stream) override {
        stream.read(reinterpret_cast<char *>(&bankState), sizeof(bankState));
        stream.read(reinterpret_cast<char *>(&latestPre), sizeof(latestPre));
        stream.read(reinterpret_cast<char *>(&refreshEndTime), sizeof(refreshEndTime));
        cycles.act.deserialize(stream);
        cycles.powerDownAct.deserialize(stream);
        cycles.powerDownPre.deserialize(stream);
        counter.deserialize(stream);
    }
};

}

#endif /* DRAMPOWER_DDR_BANK_H */
