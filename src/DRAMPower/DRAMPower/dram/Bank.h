#ifndef DRAMPOWER_DDR_BANK_H
#define DRAMPOWER_DDR_BANK_H

#include <DRAMPower/data/stats.h>
#include <DRAMPower/Types.h>

#include <stdint.h>

namespace DRAMPower {

struct Bank {
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
};

}

#endif /* DRAMPOWER_DDR_BANK_H */
