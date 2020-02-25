cmake_minimum_required(VERSION 3.10)

# set the project name
project(DRAMPower)

include_directories(src/)

add_library(DRAMPower
	src/libdrampower/LibDRAMPower.cc
	src/CommandAnalysis.cc
	src/MemArchitectureSpec.cc
	src/MemCommand.cc
	src/MemPowerSpec.cc
	src/MemTimingSpec.cc
	src/MemoryPowerModel.cc
	src/MemorySpecification.cc
	src/Parameter.cc
	src/Parametrisable.cc
	src/libdrampower/LibDRAMPower.cc
	src/CAHelpers.cc
	src/CmdHandlers.cc
	src/MemBankWiseParams.cc
)
