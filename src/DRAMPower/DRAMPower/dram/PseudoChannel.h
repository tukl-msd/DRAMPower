#ifndef DRAMPOWER_DRAM_PSEUDOCHANNEL_H
#define DRAMPOWER_DRAM_PSEUDOCHANNEL_H

#include <DRAMPower/command/CmdType.h>
#include <DRAMPower/dram/Bank.h>
#include <DRAMPower/Types.h>

#include <DRAMPower/dram/Rank.h>

#include <vector>

namespace DRAMPower {

struct PseudoChannel {
public:
// Variables
	MemState memState = MemState::NOT_IN_PD;
	struct {
		interval_t act;
		interval_t ref;
	} cycles;
	struct {
		uint64_t selfRefresh = 0;
		uint64_t deepSleepMode = 0;
	} counter = { 0 };
	timestamp_t endRefreshTime = 0;
	std::vector<Bank> banks;

public:
// Constructors
	PseudoChannel(std::size_t numBanks);

// Functions
public:
	bool isActive();
	std::size_t countActiveBanks() const;
};

} // namespace DRAMPower

#endif /* DRAMPOWER_DRAM_PSEUDOCHANNEL_H */
