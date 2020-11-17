cmake_minimum_required(VERSION 3.10)

# set the project name
project(DRAMPower)

# Configuration:
set(CMAKE_CXX_STANDARD 11 CACHE STRING "C++ Version")

include_directories(src/
                    src/cli)

# Add DRAMPowerLibrary:
add_subdirectory(src/library)

add_executable(DRAMPower
    src/cli/drampower_cli.cc
    src/cli/CliHandler.cc
    src/cli/TraceParser.cc
    src/cli/common/jsonparser/JSONParser.cc
)

target_link_libraries(DRAMPower
    DRAMPowerLibrary
)
