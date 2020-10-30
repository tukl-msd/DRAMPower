cmake_minimum_required(VERSION 3.10)

# set the project name
project(DRAMPower)

include_directories(src/
                    src/dramtypes
                    src/memspec
                    src/cli)

add_library(DRAMPower src/cli/drampower.cc
            src/cli/CliHandler.cc
            src/dramtypes/DRAMPowerIF.cc
            src/dramtypes/DRAMPowerDDR3.cc
            src/dramtypes/DRAMPowerDDR4.cc
            src/dramtypes/DRAMPowerWideIO.cc
            src/memspec/MemSpec.cc
            src/memspec/MemSpecDDR3.cc
            src/memspec/MemSpecDDR4.cc
            src/memspec/MemSpecWideIO.cc
            src/MemCommand.cc
            src/counters/Counters.cc
            src/counters/CountersDDR3.cc
            src/counters/CountersDDR4.cc
            src/counters/CountersWideIO.cc
            src/TraceParser.cc
            src/common/DebugManager.cc
)

add_executable(DRAMPowerCli
    src/cli/drampower.cc
    src/cli/CliHandler.cc
)

target_link_libraries(DRAMPowerCli
    DRAMPower
)
