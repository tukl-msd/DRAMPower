/*
 * Copyright (c) 2023, RPTU Kaiserslautern-Landau
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Authors:
 *    Derek Christ
 *    Marco Mörz
 */

#include <DRAMPower/memspec/MemSpecLPDDR4.h>
#include <DRAMUtils/memspec/MemSpec.h>
#include <DRAMUtils/memspec/standards/MemSpecLPDDR4.h>
#include <DRAMPower/standards/lpddr4/LPDDR4.h>
#include <DRAMPower/dram/dram_base.h>
#include <DRAMPower/command/CmdType.h>
#include <DRAMPower/command/Command.h>

#include <DRAMPower/cli/run.hpp>

#include <benchmark/benchmark.h>
#include <filesystem>
#include <iostream>
#include <exception>
#include <optional>
#include <string>
#include <memory>

class LPDDR4_Bench : public benchmark::Fixture
{
public:

    using Command = DRAMPower::Command;
    using CmdType = DRAMPower::CmdType;
    using CommandList_t = std::vector<std::pair<Command, std::unique_ptr<uint8_t[]>>>;
    using BaseDDR_t = DRAMPower::dram_base<CmdType>;

    void SetUp(::benchmark::State&) {
        std::string memspecFile{DRAMPOWER_BENCHMARK_CONFIGS_DIR"/lpddr4.json"};
        std::string commandFile{DRAMPOWER_BENCHMARK_CONFIGS_DIR"/lpddr4.csv"};
        
        // Get dram
        // auto ddr = DRAMPower::CLI::getMemory(memspecFile, std::nullopt);
        auto memspeccontainer = DRAMUtils::parse_memspec_from_file(memspecFile);
        if(!memspeccontainer)
        {
            throw std::runtime_error("Failed to parse memspec from file");
        }
        memspec = std::make_unique<DRAMPower::MemSpecLPDDR4>(DRAMPower::MemSpecLPDDR4::from_memspec(*memspeccontainer));

        // Parse command list
        if (!DRAMPower::DRAMPowerCLI::parse_command_list(commandFile, commandlist))
        {
            throw std::runtime_error("Failed to parse command list");
        }
    }

    void TearDown(::benchmark::State&) {
        commandlist.clear();
    }
    std::unique_ptr<DRAMPower::MemSpecLPDDR4> memspec;
    CommandList_t commandlist;
};

BENCHMARK_DEFINE_F(LPDDR4_Bench, lpddr4PowerSimulation)(benchmark::State& state)
{
    auto rdbuf = std::cout.rdbuf(nullptr);
    for (auto _ : state)
    {
        std::unique_ptr<BaseDDR_t> ddr = std::make_unique<DRAMPower::LPDDR4>(*memspec);
        DRAMPower::DRAMPowerCLI::runCommands(ddr, commandlist);
    }
    std::cout.rdbuf(rdbuf);
}

BENCHMARK_REGISTER_F(LPDDR4_Bench, lpddr4PowerSimulation)->Unit(benchmark::kMicrosecond)->Iterations(10);

BENCHMARK_DEFINE_F(LPDDR4_Bench, lpddr4PowerSimulationToggling)(benchmark::State& state)
{
    auto rdbuf = std::cout.rdbuf(nullptr);
    for (auto _ : state)
    {
        std::unique_ptr<BaseDDR_t> ddr = std::make_unique<DRAMPower::LPDDR4>(*memspec);
        ddr->setToggleRate(0, DRAMUtils::Config::ToggleRateDefinition{
            0.5, // togglingRateRead
            0.5, // togglingRateWrite
            0.5, // dutyCycleRead
            0.5, // dutyCycleWrite
            DRAMUtils::Config::TogglingRateIdlePattern::L, // idlePatternRead
            DRAMUtils::Config::TogglingRateIdlePattern::L  // idlePatternWrite
        });
        DRAMPower::DRAMPowerCLI::runCommands(ddr, commandlist);
    }
    std::cout.rdbuf(rdbuf);
}

BENCHMARK_REGISTER_F(LPDDR4_Bench, lpddr4PowerSimulationToggling)->Unit(benchmark::kMicrosecond)->Iterations(10);