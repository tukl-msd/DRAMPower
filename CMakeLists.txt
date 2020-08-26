cmake_minimum_required(VERSION 3.10)

# set the project name
project(DRAMPower)

include_directories(src/
                    src/libdrampower
                    src/cli)

add_library(DRAMPower src/libdrampower/LibDRAMPower.cc
            src/cli/CliHandler.cc
            src/cli/drampower.cc
            src/CommandAnalysis.cc
            src/MemArchitectureSpec.cc
            src/MemBankWiseParams.cc
            src/MemSpecParser.cc
            src/MemCommand.cc
            src/MemPowerSpec.cc
            src/MemTimingSpec.cc
            src/MemoryPowerModel.cc
            src/MemorySpecification.cc
            src/TraceParser.cc
            src/CAHelpers.cc
            src/CmdHandlers.cc
            src/MemBankWiseParams.cc
            )
