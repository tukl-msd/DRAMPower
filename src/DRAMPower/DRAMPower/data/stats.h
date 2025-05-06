#ifndef DRAMPOWER_DATA_STATS_H
#define DRAMPOWER_DATA_STATS_H

#include <DRAMPower/util/bus.h>
#include <DRAMPower/dram/Interface.h>

#include <cstdint>

namespace DRAMPower
{
	struct CycleStats
	{
		struct command_stats_t {
			uint64_t act = 0;
			uint64_t pre = 0;
			uint64_t preSameBank = 0;
			uint64_t reads = 0;
			uint64_t writes = 0;
			uint64_t refAllBank = 0;
			uint64_t refPerBank = 0;
			uint64_t refPerTwoBanks = 0;
			uint64_t refSameBank = 0;
			uint64_t readAuto = 0;
			uint64_t writeAuto = 0;
		} counter;

		struct {
			uint64_t act = 0;
			uint64_t pre = 0;
			uint64_t ref = 0;		// TODO not used in DDR4 -> Used in other standards ???
			uint64_t powerDownAct = 0;
			uint64_t powerDownPre = 0;
			uint64_t selfRefresh = 0;
			uint64_t deepSleepMode = 0;
			uint64_t activeTime() { return act; };
			//uint64_t prechargeTime;
		} cycles;

		struct {
			uint64_t readMerged = 0;
			uint64_t readMergedTime = 0;
			uint64_t writeMerged = 0;
			uint64_t writeMergedTime = 0;
			uint64_t readSeamless = 0;
			uint64_t writeSeamless = 0;
		} prepos;
	};

    struct TogglingStats
    {
        util::bus_stats_t read;
        util::bus_stats_t write;
    };

	struct SimulationStats
	{
		util::bus_stats_t commandBus;
		util::bus_stats_t readBus;
		util::bus_stats_t writeBus;
		util::bus_stats_t clockStats;
		util::bus_stats_t wClockStats;

		util::bus_stats_t readdbiStats;
		util::bus_stats_t writedbiStats;

        TogglingStats togglingStats;

		util::bus_stats_t readDQSStats;
		util::bus_stats_t writeDQSStats;

		std::vector<CycleStats> bank;
		std::vector<CycleStats> rank_total;
	};
};


#endif /* DRAMPOWER_DATA_STATS_H */
