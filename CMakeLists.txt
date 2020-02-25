cmake_minimum_required(VERSION 3.10)

# set the project name
project(DRAMPower)

include_directories(src/)

add_library(DRAMPower src/libdrampower/LibDRAMPower.cc)
