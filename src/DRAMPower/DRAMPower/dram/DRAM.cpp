//#include "DRAM.h"
//
//#include <DRAMPower/command/Pattern.h>
//#include <DRAMPower/calculation/calculation.h>
//
//
// namespace DRAMPower {
//
// DRAM::DRAM(const MemSpecDDR4& memSpec)
//    : memSpec(memSpec)
//	, ranks(memSpec.numberOfRanks, { memSpec.numberOfBanks })
//	, commandBus{ 6 }
//	, readBus{ 6 }
//	, writeBus{ 6 }
//{
//	this->registerPatterns();
//
//	this->registerBankHandler<CmdType::ACT>(&DRAM::handleAct);
//	this->registerBankHandler<CmdType::PRE>(&DRAM::handlePre);
//	this->registerRankHandler<CmdType::PREA>(&DRAM::handlePreAll);
//
//	this->registerRankHandler<CmdType::REFA>(&DRAM::handleRefAll);
//	this->registerBankHandler<CmdType::REFB>(&DRAM::handleRefPerBank);
//
//	this->registerBankHandler<CmdType::RD>(&DRAM::handleRead);
//	this->registerBankHandler<CmdType::RDA>(&DRAM::handleReadAuto);
//	this->registerBankHandler<CmdType::WR>(&DRAM::handleWrite);
//	this->registerBankHandler<CmdType::WRA>(&DRAM::handleWriteAuto);
//
//	this->registerRankHandler<CmdType::PDEA>(&DRAM::handlePowerDownActEntry);
//	this->registerRankHandler<CmdType::PDXA>(&DRAM::handlePowerDownActExit);
//	this->registerRankHandler<CmdType::PDEP>(&DRAM::handlePowerDownPreEntry);
//	this->registerRankHandler<CmdType::PDXP>(&DRAM::handlePowerDownPreExit);
//
//	this->registerRankHandler<CmdType::SREFEN>(&DRAM::handleSelfRefreshEntry);
//	this->registerRankHandler<CmdType::SREFEX>(&DRAM::handleSelfRefreshExit);
//
//	this->registerRankHandler<CmdType::DSMEN>(&DRAM::handleDSMEntry);
//	this->registerRankHandler<CmdType::DSMEX>(&DRAM::handleDSMExit);
//
//    routeCommand<CmdType::REFSB>([this](const Command& cmd) {
//		Rank & rank = this->ranks[cmd.targetCoordinate.rank];
//		this->handleRefSameBank(rank, cmd.targetCoordinate.bank, cmd.timestamp);
//	});
//
//	routeCommand<CmdType::REFP2B>([this](const Command& cmd) {
//		Rank & rank = this->ranks[cmd.targetCoordinate.rank];
//		this->handleRefPerTwoBanks(rank, cmd.targetCoordinate.bank, cmd.timestamp);
//	});
//
//	routeCommand<CmdType::PRESB>([this](const Command& cmd) {
//		Rank & rank = this->ranks[cmd.targetCoordinate.rank];
//		this->handlePreSameBank(rank, cmd.targetCoordinate.bank, cmd.timestamp);
//	});
//
//	routeCommand<CmdType::END_OF_SIMULATION>([this](const Command& cmd) { this->endOfSimulation(cmd.timestamp); });
//};
//
// void DRAM::registerPatterns()
//{
//	using namespace pattern_descriptor;
//
//	// LPDDR4
//	// ---------------------------------:
//	this->registerPattern<CmdType::ACT>({
//		H,   L,   R12, R13, R14, R15,
//		BA0, BA1, BA2, R16, R10, R11,
//		R17, R18, R6,  R7,  R8,  R9,
//		R0,  R1,  R2,  R3,  R4,  R5,
//	});
//	this->registerPattern<CmdType::PRE>({
//		L,   L,   L,   L,   H,   L,
//		BA0, BA1, BA2, V,   V,   V,
//	});
//	this->registerPattern<CmdType::PREA>({
//		L,   L,   L,   L,   H,   H,
//		V,   V,   V,   V,   V,   V,
//	});
//	this->registerPattern<CmdType::REFB>({
//		L,   L,   L,   H,   L,   L,
//		BA0, BA1, BA2, V,   V,   V,
//	});
//	this->registerPattern<CmdType::REFA>({
//		L,   L,   L,   H,   L,   H,
//		V,   V,   V,   V,   V,   V,
//	});
//	this->registerPattern<CmdType::SREFEN>({
//		L,   L,   L,   H,   H,   V,
//		V,   V,   V,   V,   V,   V,
//	});
//	this->registerPattern<CmdType::SREFEX>({
//		L,   L,   H,   L,   H,   V,
//		V,   V,   V,   V,   V,   V,
//	});
//	this->registerPattern<CmdType::WR>({
//		L,   L,   H,   L,   L,   BL,
//		BA0, BA1, BA2, V,   C9,  AP,
//		L,   H,   L,   L,   H,   C8,
//		C2,  C3,  C4,  C5,  C6,  C7,
//	});
//	this->registerPattern<CmdType::RD>({
//		L,   H,   L,   L,   L,   BL,
//		BA0, BA1, BA2, V,   C9,  AP,
//		L,   H,   L,   L,   H,   C8,
//		C2,  C3,  C4,  C5,  C6,  C7
//	});
//};
//
// void DRAM::handle_interface(const Command& cmd)
//{
//	/*
//	0b0000000000000000000000000000000000000000
//	100'000
//	100'000
//	000'000
//	010'000
//	000'000
//	*/
//
//	/* 0b0000000000000000000000000000000000000000000000
//	000'000
//	000'010
//	100'000
//	000'000
//	*/
//
//
//	auto pattern = this->getCommandPattern(cmd);
//	auto length = this->getPattern(cmd.type).size() / commandBus.get_width();
//	this->commandBus.load(cmd.timestamp, pattern, length);
//
//	//auto stats = this->commandBus.get_stats(cmd.timestamp + length - 1);
//	auto stats_2= this->commandBus.get_stats(cmd.timestamp + length);
//
//};
//
// void DRAM::handleAct(Rank & rank, Bank & bank, timestamp_t timestamp)
//{
//    bank.counter.act++;
//	bank.cycles.act.start_interval(timestamp); // DDR5: + timing.irgendwas
//
//    if ( !rank.isActive(timestamp) ) {
//		rank.cycles.act.start_interval(timestamp);
//    };
//
//    bank.bankState = Bank::BankState::BANK_ACTIVE;
//}
//
// void DRAM::handlePre(Rank & rank, Bank & bank, timestamp_t timestamp)
//{
//	if (bank.bankState == Bank::BankState::BANK_PRECHARGED)
//		return;
//
//	bank.counter.pre++;
//	bank.cycles.act.close_interval(timestamp);
//	bank.latestPre = timestamp;
//	bank.bankState = Bank::BankState::BANK_PRECHARGED;
//
//    if ( !rank.isActive(timestamp) ) {
//		rank.cycles.act.close_interval(timestamp);
//    };
//};
//
// void DRAM::handlePreAll(Rank & rank, timestamp_t timestamp)
//{
//    for (auto& bank : rank.banks) {
//		handlePre(rank, bank, timestamp);
//    }
//};
//
// void DRAM::handlePreSameBank(Rank & rank, std::size_t relative_bank_id, timestamp_t timestamp) // Only in DDR5!
//{
//	for (std::size_t bankGroup_id = 0; bankGroup_id < memSpec.numberOfBankGroups; bankGroup_id++) {
//		auto targetBankId = (bankGroup_id * memSpec.banksPerGroup) + relative_bank_id;
//		Bank & bank = rank.banks[targetBankId];
//		handlePre(rank, bank, timestamp);
//	};
//};
//
// void DRAM::handleRefreshOnBank(Rank & rank, Bank & bank, timestamp_t timestamp, uint64_t timing, uint64_t & counter)
//{
//	++counter;
//
//	if (!rank.isActive(timestamp)) {
//		rank.cycles.act.start_interval(timestamp);
//	};
//
//	bank.bankState = Bank::BankState::BANK_ACTIVE;
//
//	auto timestamp_end = timestamp + timing;
//	bank.refreshEndTime = timestamp_end;
//
//	if (!bank.cycles.act.is_open())
//		bank.cycles.act.start_interval(timestamp);
//
//	// Execute implicit pre-charge at refresh end
//	addImplicitCommand(timestamp_end, [this, &bank, &rank, timestamp_end]() {
//		bank.bankState = Bank::BankState::BANK_PRECHARGED;
//		bank.cycles.act.close_interval(timestamp_end);
//
//		if (!rank.isActive(timestamp_end)) {
//			rank.cycles.act.close_interval(timestamp_end);
//		};
//	});
//};
//
//
// void DRAM::handleRefAll(Rank & rank, timestamp_t timestamp)
//{
//	for (auto& bank : rank.banks) {
//		handleRefreshOnBank(rank, bank, timestamp, memSpec.memTimingSpec.tRFC, bank.counter.refAllBank);
//	}
//
//	// Required for precharge power-down // ToDo: noch nötig?
//	rank.endRefreshTime = timestamp + memSpec.memTimingSpec.tRFC;
//};
//
// void DRAM::handleRefPerBank(Rank & rank, Bank & bank, timestamp_t timestamp)
//{
//	handleRefreshOnBank(rank, bank, timestamp, memSpec.memTimingSpec.tRFCPB, bank.counter.refPerBank);
//};
//
// void DRAM::handleRefSameBank(Rank & rank, std::size_t relative_bank_id, timestamp_t timestamp) // Only in DDR5!
//{
//	for (std::size_t bankGroup_id = 0; bankGroup_id < memSpec.numberOfBankGroups; bankGroup_id++) {
//        auto targetBankId = (bankGroup_id * memSpec.banksPerGroup) + relative_bank_id;
//		Bank & bank = rank.banks[targetBankId];
//		handleRefreshOnBank(rank, bank, timestamp, memSpec.memTimingSpec.tRFCsb_slr, bank.counter.refSameBank);
//    };
//}
//
// void DRAM::handleRefPerTwoBanks(Rank & rank, std::size_t bank_id, timestamp_t timestamp) // Only in DDR5!
//{
//		Bank & bank_1 = rank.banks[bank_id];
//		Bank & bank_2 = rank.banks[bank_id + memSpec.perTwoBankOffset];
//
//		handleRefreshOnBank(rank, bank_1, timestamp, memSpec.memTimingSpec.tRFCPB, bank_1.counter.refPerTwoBanks);
//		handleRefreshOnBank(rank, bank_2, timestamp, memSpec.memTimingSpec.tRFCPB, bank_2.counter.refPerTwoBanks);
//}
//
// void DRAM::handleSelfRefreshEntry(Rank & rank, timestamp_t timestamp)
//{
//	// Issue implicit refresh
//	handleRefAll(rank, timestamp);
//
//	// Handle self-refresh entry after tRFC
//	auto timestampSelfRefreshStart = timestamp + memSpec.memTimingSpec.tRFC;
//
//	addImplicitCommand(timestampSelfRefreshStart, [this, &rank, timestampSelfRefreshStart]() {
//		spdlog::debug("Implicit Command: Timestamp: {} Type: SRFEN", timestampSelfRefreshStart);
//		rank.counter.selfRefresh++;
//		rank.cycles.sref.start_interval(timestampSelfRefreshStart);
//		rank.memState = MemState::SREF;
//	});
//};
//
// void DRAM::handleSelfRefreshExit(Rank & rank, timestamp_t timestamp)
//{
//	assert(rank.memState == MemState::SREF);
//
//	rank.cycles.sref.close_interval(timestamp);  // Duration start between entry and exit
//    rank.memState = MemState::NOT_IN_PD;
//};
//
// void DRAM::handleDSMEntry(Rank & rank, timestamp_t timestamp) // DSM only in LPDDR5 !!
//{
//	assert(rank.memState == MemState::SREF);
//
//	rank.cycles.deepSleepMode.start_interval(timestamp);
//	rank.counter.deepSleepMode++; // TODO: Make sure to not count twice
//    rank.memState = MemState::DSM;
//};
//
// void DRAM::handleDSMExit(Rank & rank, timestamp_t timestamp)
//{
//	assert(rank.memState == MemState::DSM);
//
//	rank.cycles.deepSleepMode.close_interval(timestamp);
//	rank.memState = MemState::SREF;
//};
//
// void DRAM::handlePowerDownActEntry(Rank & rank, timestamp_t timestamp)
//{
//	auto earliestPossibleEntry = this->earliestPossiblePowerDownEntryTime(rank, timestamp);
//	auto entryTime = std::max(timestamp, earliestPossibleEntry);
//
//	addImplicitCommand(entryTime, [this, &rank, entryTime]() {
//		spdlog::debug("Implicit Command: Timestamp: {} Type: PDEA", entryTime);
//		rank.cycles.powerDownAct.start_interval(entryTime);
//		rank.memState = MemState::PDN_ACT;
//
//		if (rank.cycles.act.is_open()) {
//			rank.cycles.act.close_interval(entryTime);
//		}
//
//		for (auto & bank : rank.banks) {
//			if (bank.cycles.act.is_open()) {
//				bank.cycles.act.close_interval(entryTime);
//			}
//		};
//
//	});
//}
//
// void DRAM::handlePowerDownActExit(Rank & rank, timestamp_t timestamp)
//{
//	auto earliestPossibleExit = this->earliestPossiblePowerDownEntryTime(rank, timestamp);
//	auto exitTime = std::max(timestamp, earliestPossibleExit);
//
//	addImplicitCommand(exitTime, [this, &rank, exitTime]() {
//		spdlog::debug("Implicit Command: Timestamp: {} Type: PDXA", exitTime);
//		rank.memState = MemState::NOT_IN_PD;
//		rank.cycles.powerDownAct.close_interval(exitTime);
//
//		if (rank.cycles.act.get_end() == rank.cycles.powerDownAct.get_start()) {
//			rank.cycles.powerDownAct.close_interval(exitTime);
//			rank.cycles.act.start_interval(exitTime);
//		}
//
//		for (auto & bank : rank.banks) {
//			if (bank.cycles.act.get_end() == rank.cycles.powerDownAct.get_start()) {
//				bank.cycles.act.start_interval(exitTime);
//			}
//		};
//	});
//};
//
//
// void DRAM::handlePowerDownPreEntry(Rank & rank, timestamp_t timestamp)
//{
//	auto earliestPossibleEntry = this->earliestPossiblePowerDownEntryTime(rank, timestamp);
//	auto entryTime = std::max(timestamp, earliestPossibleEntry);
//
//	addImplicitCommand(entryTime, [this, &rank, entryTime]() {
//		spdlog::debug("Implicit Command: Timestamp: {} Type: PDEP", entryTime);
//		//rank.powerDownCyclesPre.start = entryTime;
//		rank.cycles.powerDownPre.start_interval(entryTime);
//		rank.memState = MemState::PDN_PRE;
//	});
//}
//
// void DRAM::handlePowerDownPreExit(Rank & rank, timestamp_t timestamp)
//{
//	auto earliestPossibleExit = this->earliestPossiblePowerDownEntryTime(rank, timestamp);
//	auto exitTime = std::max(timestamp, earliestPossibleExit);
//
//	addImplicitCommand(exitTime, [this, &rank, exitTime]() {
//		spdlog::debug("Implicit Command: Timestamp: {} Type: PDXP", exitTime);
//		rank.memState = MemState::NOT_IN_PD;
//		//rank.powerDownCyclesPre.count += exitTime - rank.powerDownCyclesPre.start;
//		rank.cycles.powerDownPre.close_interval(exitTime);
//	});
//};
//
// void DRAM::handlePowerUpPre_DDR3(timestamp_t timestamp)
//{
//    if (1 == 2)
//    // TODO: if(DLL_on) 	// will we allow change of DLL during runtime?
//    {
//        //cycles.powerDown.pre += timestamp - cycles.powerDown.start;
//        //preCycles.end = timestamp;
//    } else {
//        //cycles.powerDown.pre += timestamp - cycles.powerDown.start;
//        //cycles.powerUp.pre += memSpec.memTimingSpec.tXPDLL - memSpec.memTimingSpec.tRCD;
//    }
//
//    //this->memState = MemState::NOT_IN_PD;
//};
//
// void DRAM::handleRead(Rank & rank, Bank & bank, timestamp_t timestamp)
//{
//	++bank.counter.reads;
//};
//
// void DRAM::handleWrite(Rank & rank, Bank & bank, timestamp_t timestamp)
//{
//	++bank.counter.writes;
//};
//
// void DRAM::handleReadAuto(Rank & rank, Bank & bank, timestamp_t timestamp)
//{
//	++bank.counter.readAuto;
//
//	// VARIATION POINT: (gilt nur für ddr3/4)
//	// VARIATION POINT: kann auch anderes Timing als tRAS verwendet werden
//	auto minBankActiveTime = bank.cycles.act.get_start() + this->memSpec.memTimingSpec.tRAS;
//	auto minReadActiveTime = timestamp + this->memSpec.memTimingSpec.tRTP;
//
//	auto delayed_timestamp = std::max(minBankActiveTime, minReadActiveTime);
//
//	// Execute PRE after minimum active time
//	addImplicitCommand(delayed_timestamp, [this, &rank, &bank, delayed_timestamp]() {
//		spdlog::debug("Implicit Command: Timestamp: {} Type: RDA", delayed_timestamp);
//		this->handlePre(rank, bank, delayed_timestamp);
//	});
//};
//
// void DRAM::handleWriteAuto(Rank & rank, Bank & bank, timestamp_t timestamp)
//{
//	++bank.counter.writeAuto;
//
//	// VARIATION POINT: (gilt nur für ddr3/4)
//	auto minBankActiveTime = bank.cycles.act.get_start() + this->memSpec.memTimingSpec.tRAS;
//	auto minWriteActiveTime = timestamp
//		+ this->memSpec.memTimingSpec.tWR
//		+ this->memSpec.memTimingSpec.tWL
//		+ (int)(this->memSpec.burstLength / (this->memSpec.dataRate * this->memSpec.memTimingSpec.tCK)); // ToDO: <- tBurst
//
//	auto delayed_timestamp = std::max(minBankActiveTime, minWriteActiveTime);
//
//	// Execute PRE after minimum active time
//	addImplicitCommand(delayed_timestamp, [this, &rank, &bank, delayed_timestamp]() {
//		spdlog::debug("Implicit Command: Timestamp: {} Type: WRA", delayed_timestamp);
//		this->handlePre(rank, bank, delayed_timestamp);
//	});
//};
//
// void DRAM::endOfSimulation(timestamp_t timestamp)
//{
//	if (this->implicitCommandCount() > 0)
//		spdlog::warn("End of simulation but still implicit commands left!");
//};
//
//
// energy_t DRAM::calcEnergy(timestamp_t timestamp)
//{
//	Calculation calculation;
//
//	return calculation.calcEnergy(timestamp, *this);
//};
//
// interface_energy_info_t calcInterfaceEnergy(timestamp_t timestamp)
//{
//	return interface_energy_info_t{};
//};
//
//
// SimulationStats DRAM::getWindowStats(timestamp_t timestamp)
//{
//	// If there are still implicit commands queued up, process them first
//	this->processImplicitCommandQueue(timestamp);
//
//	SimulationStats stats;
//	stats.bank.resize(this->memSpec.numberOfBanks);
//
//	auto & rank = this->ranks[0];
//	auto simulation_duration = timestamp;
//
//	for (std::size_t i = 0; i < this->memSpec.numberOfBanks; ++i) {
//		stats.bank[i].counter = rank.banks[i].counter;
//
//		auto & cycles = stats.bank[i].cycles;
//		cycles.act = rank.banks[i].cycles.act.get_count_at(timestamp);
//		cycles.ref = rank.banks[i].cycles.ref.get_count_at(timestamp);
//		cycles.powerDownAct = rank.banks[i].cycles.powerDownAct.get_count_at(timestamp);
//		cycles.powerDownPre = rank.banks[i].cycles.powerDownPre.get_count_at(timestamp);
//		cycles.pre = simulation_duration - (cycles.act + rank.cycles.powerDownAct.get_count_at(timestamp) + rank.cycles.powerDownPre.get_count_at(timestamp));
//	}
//
//	stats.total.cycles.pre = simulation_duration - (rank.cycles.act.get_count_at(timestamp) + rank.cycles.powerDownAct.get_count_at(timestamp) + rank.cycles.powerDownPre.get_count_at(timestamp));
//	stats.total.cycles.act = rank.cycles.act.get_count_at(timestamp);
//	stats.total.cycles.ref = rank.cycles.act.get_count_at(timestamp);
//	stats.total.cycles.powerDownAct = rank.cycles.powerDownAct.get_count_at(timestamp);
//	stats.total.cycles.powerDownPre = rank.cycles.powerDownPre.get_count_at(timestamp);
//	stats.total.cycles.deepSleepMode = rank.cycles.deepSleepMode.get_count_at(timestamp);
//	stats.total.cycles.selfRefresh = rank.cycles.sref.get_count_at(timestamp) - stats.total.cycles.deepSleepMode;
//
//	stats.commandBus = this->commandBus.get_stats(timestamp);
//	//stats.dataBus= this->dataBus.get_stats(timestamp);
//
//	return stats;
//};
//
// SimulationStats DRAM::getStats()
//{
//	return getWindowStats(getLastCommandTime());
//};
//
//}