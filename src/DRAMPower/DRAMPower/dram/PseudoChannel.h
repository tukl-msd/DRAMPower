#ifndef DRAMPOWER_DRAM_PSEUDOCHANNEL_H
#define DRAMPOWER_DRAM_PSEUDOCHANNEL_H

#include <DRAMPower/util/command_counter.h>
#include <DRAMPower/command/CmdType.h>
#include <DRAMPower/dram/Bank.h>
#include <DRAMPower/Types.h>

#include <DRAMPower/dram/Rank.h>

#include <DRAMPower/util/Serialize.h>
#include <DRAMPower/util/Deserialize.h>

#include <vector>

namespace DRAMPower {

struct PseudoChannel : public util::Serialize, public util::Deserialize {

// Type aliases
	using commandCounter_t = util::CommandCounter<CmdType>;

public:
// Variables
	MemState memState = MemState::NOT_IN_PD;
	commandCounter_t commandCounter;
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
	bool isActive(timestamp_t timestamp);
	std::size_t countActiveBanks() const;
// Overrides
	void serialize(std::ostream& stream) const override;
	void deserialize(std::istream& stream) override;
};

} // namespace DRAMPower

#endif /* DRAMPOWER_DRAM_PSEUDOCHANNEL_H */
