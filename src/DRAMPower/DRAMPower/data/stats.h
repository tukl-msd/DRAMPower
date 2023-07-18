#ifndef DRAMPOWER_DATA_STATS_H
#define DRAMPOWER_DATA_STATS_H

#include <DRAMPower/util/bus.h>

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
			uint64_t ref = 0;
			uint64_t powerDownAct = 0;
			uint64_t powerDownPre = 0;
			uint64_t selfRefresh = 0;
			uint64_t deepSleepMode = 0;
			uint64_t activeTime() { return act; };
			//uint64_t prechargeTime;
		}cycles;
	};

	struct SimulationStats
	{
		util::bus_stats_t commandBus;
		util::bus_stats_t readBus;
		util::bus_stats_t writeBus;
		util::bus_stats_t clockStats;
		util::bus_stats_t WClockStats;

		util::bus_stats_t readDQSStats;
		util::bus_stats_t writeDQSStats;

		std::vector<CycleStats> bank;
		CycleStats total;
	};
};


#endif /* DRAMPOWER_DATA_STATS_H */
