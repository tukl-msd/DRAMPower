#include <gtest/gtest.h>

#include "DRAMPower/command/Command.h"

#include <DRAMPower/standards/hbm2/HBM2.h>
#include <DRAMPower/memspec/MemSpec.h>

#include <DRAMUtils/memspec/standards/MemSpecHBM2.h>

#include <stdint.h>


using namespace DRAMPower;

class DramPowerTest_HBM2_0 : public ::testing::Test {
protected:
    // Test pattern
    std::vector<Command> testPattern = {
            { 0, CmdType::ACT,  { 0, 0, 0 }},
            { 15, CmdType::PRE,  { 0, 0, 0 }},
            { 16, CmdType::END_OF_SIMULATION },
    };


    // Test variables
    std::unique_ptr<DRAMPower::HBM2> ddr;
    std::unique_ptr<DRAMPower::MemSpecHBM2> memSpec;

    virtual void SetUp()
    {
        auto data = DRAMUtils::parse_memspec_from_file(std::filesystem::path(TEST_RESOURCE_DIR) / "hbm2.json");
        memSpec = std::make_unique<DRAMPower::MemSpecHBM2>(DRAMPower::MemSpecHBM2::from_memspec(*data));
        ddr = std::make_unique<HBM2>(*memSpec);
    }

    virtual void TearDown()
    {
    }
};

TEST_F(DramPowerTest_HBM2_0, Counters_and_Cycles){
    for (const auto& command : testPattern) {
        ddr->doCoreCommand(command);
        ddr->doInterfaceCommand(command);
    }

    auto stats = ddr->getStats();

    // Check bank command count: ACT
    const auto n_banks = memSpec->numberOfPseudoChannels * memSpec->numberOfDevices * memSpec->numberOfBanks * memSpec->numberOfStacks;
    ASSERT_EQ(stats.bank.size(), n_banks);
    ASSERT_EQ(stats.bank.at(0).counter.act, 1);
    for(uint64_t b = 1; b < memSpec->numberOfBanks * memSpec->numberOfStacks; b++)  ASSERT_EQ(stats.bank.at(b).counter.act, 0);


    // Check cycles count
    ASSERT_EQ(stats.rank_total.at(0).cycles.act, 15);
    ASSERT_EQ(stats.rank_total.at(0).cycles.pre, 1);

    // Check bank specific ACT cycle count
    ASSERT_EQ(stats.bank.at(0).cycles.act, 15);
    for(uint64_t b = 1; b < memSpec->numberOfBanks; b++)
        ASSERT_EQ(stats.bank.at(b).cycles.act, 0);

    // Check bank specific PRE cycle count
    ASSERT_EQ(stats.bank.at(0).cycles.pre, 1);
    for(uint64_t b = 1; b < memSpec->numberOfBanks * memSpec->numberOfStacks; b++)
        ASSERT_EQ(stats.bank.at(b).cycles.pre, 16);
}

TEST_F(DramPowerTest_HBM2_0, Energy) {
    for (const auto& command : testPattern) {
        ddr->doCoreCommand(command);
    }

    auto energy = ddr->calcCoreEnergy(testPattern.back().timestamp);
    auto total_energy = energy.aggregated_bank_energy();


    ASSERT_EQ(std::round(total_energy.E_act*1e12), 332);
    ASSERT_EQ(std::round(total_energy.E_pre*1e12), 21);
    ASSERT_EQ(std::round(total_energy.E_bg_act*1e12), 277);
    ASSERT_EQ(std::round(energy.E_bg_act_shared*1e12), 457);
    ASSERT_EQ(std::round(total_energy.E_bg_pre*1e12), 518);
    ASSERT_EQ(std::round(total_energy.total()*1e12), 1148);
}

TEST_F(DramPowerTest_HBM2_0, RefreshCompletion) {
    for (uint64_t tRFC : {1, 512}) {
        SCOPED_TRACE(tRFC);
        memSpec->memTimingSpec.tRFC = tRFC;
        memSpec->memTimingSpec.tRFCSB = tRFC == 1 ? 1 : 256;
        for (auto command : {CmdType::REFA, CmdType::REFB, CmdType::SREFEN}) {
            SCOPED_TRACE(CmdTypeUtil::to_string(command));
            HBM2Core core(*memSpec);
            core.doCommand({0, command});
            const auto refreshCycles = command == CmdType::REFB ? memSpec->memTimingSpec.tRFCSB : tRFC;
            const bool selfRefresh = command == CmdType::SREFEN;
            const timestamp_t exitTime = tRFC == 1 ? 10 : 4096;
            const timestamp_t queryTime = selfRefresh ? exitTime + (tRFC == 1 ? 2 : 1024) : (tRFC == 1 ? 10 : 2000);
            const auto selfRefreshCycles = selfRefresh ? exitTime - tRFC : 0;
            if (selfRefresh) core.doCommand({exitTime, CmdType::SREFEX});

            SimulationStats stats;
            core.getWindowStats(queryTime, stats);
            const auto banksPerChannel = memSpec->numberOfBanks * memSpec->numberOfStacks;
            for (size_t pc = 0; pc < memSpec->numberOfPseudoChannels; ++pc) {
                const bool refreshedChannel = pc == 0 || selfRefresh;
                const auto activeCycles = refreshedChannel ? refreshCycles : 0;
                ASSERT_EQ(stats.rank_total.at(pc).cycles.act, activeCycles);
                ASSERT_EQ(stats.rank_total.at(pc).cycles.selfRefresh, selfRefreshCycles);
                ASSERT_EQ(stats.rank_total.at(pc).cycles.pre, queryTime - activeCycles - selfRefreshCycles);
                for (size_t b = 0; b < banksPerChannel; ++b) {
                    const auto& bank = stats.bank.at(pc * banksPerChannel + b);
                    const bool refreshedBank = refreshedChannel && (command != CmdType::REFB || b == 0);
                    const auto bankActiveCycles = refreshedBank ? refreshCycles : 0;
                    ASSERT_EQ(bank.cycles.act, bankActiveCycles);
                    ASSERT_EQ(bank.cycles.selfRefresh, selfRefreshCycles);
                    ASSERT_EQ(bank.cycles.pre, queryTime - bankActiveCycles - selfRefreshCycles);
                    ASSERT_EQ(bank.counter.act, 0);
                    ASSERT_EQ(bank.counter.pre, 0);
                    ASSERT_EQ(bank.counter.refAllBank, refreshedBank && command != CmdType::REFB ? 1 : 0);
                    ASSERT_EQ(bank.counter.refPerBank, refreshedBank && command == CmdType::REFB ? 1 : 0);
                }
            }
        }
    }

    // The pseudo-channel stays active until its last refreshing or active bank closes.
    memSpec->memTimingSpec.tRFCSB = 10;
    HBM2Core core(*memSpec);
    core.doCommand({0, CmdType::REFB, {0, 0, 0, 0, 0}});
    core.doCommand({5, CmdType::REFB, {1, 0, 0, 0, 0}});
    SimulationStats stats;
    core.getWindowStats(10, stats);
    ASSERT_EQ(stats.bank.at(0).cycles.act, 10);
    ASSERT_EQ(stats.bank.at(1).cycles.act, 5);
    ASSERT_EQ(stats.rank_total.at(0).cycles.act, 10);
    core.getWindowStats(20, stats);
    ASSERT_EQ(stats.bank.at(0).cycles.act, 10);
    ASSERT_EQ(stats.bank.at(1).cycles.act, 10);
    ASSERT_EQ(stats.rank_total.at(0).cycles.act, 15);
    ASSERT_EQ(stats.rank_total.at(0).cycles.pre, 5);

    core.doCommand({20, CmdType::ACT, {0, 0, 0, 0, 0}});
    core.doCommand({25, CmdType::REFB, {1, 0, 0, 0, 0}});
    core.getWindowStats(40, stats);
    ASSERT_EQ(stats.bank.at(0).cycles.act, 30);
    ASSERT_EQ(stats.bank.at(1).cycles.act, 20);
    ASSERT_EQ(stats.rank_total.at(0).cycles.act, 35);
    core.doCommand({40, CmdType::PRE, {0, 0, 0, 0, 0}});
    core.getWindowStats(50, stats);
    ASSERT_EQ(stats.bank.at(0).cycles.act, 30);
    ASSERT_EQ(stats.bank.at(0).counter.act, 1);
    ASSERT_EQ(stats.bank.at(0).counter.pre, 1);
    ASSERT_EQ(stats.bank.at(1).counter.refPerBank, 2);
    ASSERT_EQ(stats.bank.at(1).counter.pre, 0);
    ASSERT_EQ(stats.rank_total.at(0).cycles.act, 35);
    ASSERT_EQ(stats.rank_total.at(0).cycles.pre, 15);
}
