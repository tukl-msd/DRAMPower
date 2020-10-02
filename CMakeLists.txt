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
            src/dramtypes/DDR3.cc
            src/memspec/MemSpec.cc
            src/memspec/MemSpecDDR3.cc
            src/MemCommand.cc
            src/Counters.cc
            src/TraceParser.cc
)

add_executable(DRAMPowerCli
    src/cli/drampower.cc
    src/cli/CliHandler.cc
)

target_link_libraries(DRAMPowerCli
    DRAMPower
)
