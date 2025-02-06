#ifndef DRAMPOWER_DDR_RANK_H
#define DRAMPOWER_DDR_RANK_H

#include <DRAMPower/util/command_counter.h>
#include <DRAMPower/command/CmdType.h>
#include <DRAMPower/dram/Bank.h>
#include <DRAMPower/Types.h>

#include <vector>

namespace DRAMPower {

// Power-Down and Self-refresh related memory states
enum class MemState {
	NOT_IN_PD = 0,
	PDN_ACT,
	PDN_PRE,
	SREF,
	DSM,
};


struct Rank {

// Type aliases
	using commandCounter_t = util::CommandCounter<CmdType>;

public:
// Variables
	MemState memState = MemState::NOT_IN_PD;
	commandCounter_t commandCounter;
	struct {
		interval_t pre; // useful ???
		interval_t act;
		interval_t ref;
		interval_t sref;
		interval_t powerDownAct;
		interval_t powerDownPre;
		interval_t deepSleepMode;
	} cycles;
	struct {
		uint64_t selfRefresh = 0;
		uint64_t deepSleepMode = 0;
	} counter = { 0 };
	timestamp_t endRefreshTime = 0;
	std::vector<Bank> banks;

	uint64_t 	seamlessPrePostambleCounter_read	= 0;
	uint64_t 	seamlessPrePostambleCounter_write	= 0;
	uint64_t	mergedPrePostambleCounter_read		= 0;
	uint64_t	mergedPrePostambleCounter_write		= 0;
	timestamp_t	mergedPrePostambleTime_read			= 0;
	timestamp_t	mergedPrePostambleTime_write		= 0;
	timestamp_t lastReadEnd = 0;
	timestamp_t lastWriteEnd = 0;

public:
// Constructors
	Rank(std::size_t numBanks);

// Functions
public:
	bool isActive(timestamp_t timestamp);
	std::size_t countActiveBanks() const;

};

} // namespace DRAMPower

#endif /* DRAMPOWER_DDR_RANK_H */
