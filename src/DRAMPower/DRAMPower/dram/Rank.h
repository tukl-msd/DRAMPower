#ifndef DRAMPOWER_DDR_RANK_H
#define DRAMPOWER_DDR_RANK_H

#include <DRAMPower/util/command_counter.h>
#include <DRAMPower/util/cycle_stats.h>
#include <DRAMPower/dram/Bank.h>
#include <DRAMPower/Types.h>

#include <algorithm>
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
public:
	using commandCounter_t = util::CommandCounter<CmdType>;
public:
	enum class RankState {
		ACTIVE = 0,
		ACTIVE_POWERDOWN = 1,
		PRECHARGED_POWERDOWN = 2,
		SELF_REFRESH_DEEP_SLEEP_MODE = 3,
		DEEP_SLEEP_MODE = 4,
	};
public:
	MemState memState = MemState::NOT_IN_PD;
	commandCounter_t commandCounter;
public:
	struct {
		interval_t act;
		interval_t ref;
		interval_t sref;
		interval_t powerDownAct;
		interval_t powerDownPre;
		interval_t deepSleepMode;
	} cycles;
public:
	timestamp_t endRefreshTime = 0;

	struct {
		uint64_t selfRefresh = 0;
		uint64_t deepSleepMode = 0;
	} counter = { 0 };

	std::vector<Bank> banks;
public:
	Rank(std::size_t numBanks)
		: banks(numBanks)
	{};
public:
	bool isActive(timestamp_t timestamp) {
		return countActiveBanks() > 0 || timestamp < this->endRefreshTime;
	};

	std::size_t countActiveBanks() const {
		return (unsigned)std::count_if(banks.begin(), banks.end(),
			[](const auto& bank) { 
				return (bank.bankState == Bank::BankState::BANK_ACTIVE); 
		});
	};
};

}

#endif /* DRAMPOWER_DDR_RANK_H */
