cmake_minimum_required(VERSION 3.10)

# set the project name
project(DRAMPowerLibrary)

# Configuration:
set(CMAKE_CXX_STANDARD 11 CACHE STRING "C++ Version")

include_directories(counters/
                    dramtypes/
                    memspec/)

add_library(DRAMPowerLibrary
            dramtypes/DRAMPowerDDR3.cc
            dramtypes/DRAMPowerDDR4.cc
            dramtypes/DRAMPowerWideIO.cc
            memspec/MemSpec.cc
            memspec/MemSpecDDR3.cc
            memspec/MemSpecDDR4.cc
            memspec/MemSpecWideIO.cc
            MemCommand.cc
            counters/Counters.cc
            counters/CountersDDR3.cc
            counters/CountersDDR4.cc
            counters/CountersWideIO.cc
            DebugManager.cc
)

