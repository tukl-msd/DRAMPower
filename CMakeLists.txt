# Copyright (c) 2022, Fraunhofer IESE
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
#
# 1. Redistributions of source code must retain the above copyright notice,
#    this list of conditions and the following disclaimer.
#
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#
# 3. Neither the name of the copyright holder nor the names of its
#    contributors may be used to endorse or promote products derived from
#    this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
# TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
# PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER
# OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
# EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
# PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
# LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
# NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
# Author:
#    Thomas Psota
#    Marco Mörz

###############################################
###              DRAMPower 5.0              ###
###############################################
cmake_minimum_required(VERSION 3.22.0)

# TODO change for release
set(DRAMPOWER_VERSION_MAJOR 5)
set(DRAMPOWER_VERSION_MINOR 1)
set(DRAMPOWER_VERSION_STRING "${DRAMPOWER_VERSION_MAJOR}.${DRAMPOWER_VERSION_MINOR}")
add_compile_definitions(DRAMPOWER_VERSION_STRING="${DRAMPOWER_VERSION_STRING}")

set(PROJECT_NAME "DRAMPower ${DRAMPOWER_VERSION_STRING}")
set(PROJECT_SHORTNAME "DRAMPower")

project(${PROJECT_NAME} VERSION ${DRAMPOWER_VERSION_MAJOR}.${DRAMPOWER_VERSION_MINOR} LANGUAGES CXX)

if(POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)
endif()

### CMake settings ###
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")
include(build_source_group)
include(diagnostics_print)
include(enable_clang_format)
include(enable_clang_tidy)
include(enable_cppcheck)

if (PROJECT_IS_TOP_LEVEL)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
endif()

# set_property(GLOBAL PROPERTY USE_FOLDERS ON)
# set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

### Project settings ###
message(STATUS "CMAKE_SOURCE_DIR: ${CMAKE_SOURCE_DIR}")
message(STATUS "CMAKE_BINARY_DIR: ${CMAKE_BINARY_DIR}")
message(STATUS "" )

### Build options ###
option(DRAMPOWER_BUILD_CLI "Build DRAMPower Command Line Tool" ${PROJECT_IS_TOP_LEVEL})
option(DRAMPOWER_BUILD_BENCHMARKS "Build DRAMPower Command Line Tool" OFF)
option(DRAMPOWER_BUILD_TESTS "Build DRAMPower unit tests" OFF)

### Compiler optimization settings ###
if(PROJECT_IS_TOP_LEVEL)
    option(OPTIMIZE_FOR_NATIVE "Build with -march=native (overrides CPU_TYPE if enabled)" ON)
    set(CPU_TYPE "" CACHE STRING "CPU type for -march=CPU_TYPE and -mtune=CPU_TYPE compile options")
    # Set CPU_TYPE to native if OPTIMIZE_FOR_NATIVE is enabled
    if(OPTIMIZE_FOR_NATIVE)
        if(CPU_TYPE)
            message(NOTICE "OPTIMIZE_FOR_NATIVE is enabled. Overriding CPU_TYPE from \"${CPU_TYPE}\" to \"native\".")
        endif()
        set(CPU_TYPE "native")
    endif()

    # add CPU_TYPE to cmake cxx flags
    if(CPU_TYPE)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=${CPU_TYPE} -mtune=${CPU_TYPE}")
    endif()
endif()

# Use sane defaults for FetchContent:
# In case we are the top-level project, get everything by default
# In case we are included in another project, the user might want to provide their own system dependencies
option(DRAMPOWER_USE_FETCH_CONTENT "Enable the FetchContent module" ${PROJECT_IS_TOP_LEVEL})
option(DRAMPOWER_USE_FETCH_CONTENT_INTERNAL "Enable FetchContent to provide internal dependencies" ${DRAMPOWER_USE_FETCH_CONTENT})
option(DRAMPOWER_USE_FETCH_CONTENT_CLI11 "Enable FetchContent to provide CLI11" ${DRAMPOWER_USE_FETCH_CONTENT})
option(DRAMPOWER_USE_FETCH_CONTENT_SPDLOG "Enable FetchContent to provide spdlog" ${DRAMPOWER_USE_FETCH_CONTENT})
option(DRAMPOWER_USE_FETCH_CONTENT_NLOHMANN_JSON "Enable FetchContent to provide nlohmann json" ${DRAMPOWER_USE_FETCH_CONTENT})

### DRAMPower directories ###
set(DRAMPOWER_SOURCE_DIR      "${CMAKE_CURRENT_SOURCE_DIR}/src")
set(DRAMPOWER_LIBRARY_DIR     "${CMAKE_CURRENT_SOURCE_DIR}/lib")
set(DRAMPOWER_TESTS_DIR       "${CMAKE_CURRENT_SOURCE_DIR}/tests")

###############################################
###           Library Settings              ###
###############################################

### Detect OS threading library ###
find_package(Threads)

if (DRAMPOWER_USE_FETCH_CONTENT)
    include(FetchContent)

    # nlohmann_json for DRAMUtils
    if (DRAMPOWER_USE_FETCH_CONTENT_NLOHMANN_JSON)
            FetchContent_Declare(nlohmann_json
            URL "https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz"
            OVERRIDE_FIND_PACKAGE
        )

        FetchContent_MakeAvailable(nlohmann_json)
    endif()

    # DRAMUtils
    if (DRAMPOWER_USE_FETCH_CONTENT_INTERNAL)
        FetchContent_Declare(
            DRAMUtils
            GIT_REPOSITORY "https://github.com/tukl-msd/DRAMUtils.git"
            GIT_TAG "feat/lpddr6"
            # URL "https://github.com/tukl-msd/DRAMUtils/archive/refs/tags/v1.6.0.tar.gz"
            OVERRIDE_FIND_PACKAGE
        )

        FetchContent_MakeAvailable(DRAMUtils)
    endif()

    # cli11
    if (DRAMPOWER_USE_FETCH_CONTENT_CLI11 AND DRAMPOWER_BUILD_CLI)
        add_subdirectory(${DRAMPOWER_LIBRARY_DIR}/cli11)
    endif()

    # spdlog
    if (DRAMPOWER_USE_FETCH_CONTENT_SPDLOG AND (DRAMPOWER_BUILD_CLI OR DRAMPOWER_BUILD_BENCHMARKS))
        add_subdirectory(${DRAMPOWER_LIBRARY_DIR}/spdlog)
    endif()
endif()

###############################################
###           Source Directory              ###
###############################################

add_subdirectory(src/DRAMPower)

if(DRAMPOWER_BUILD_CLI OR DRAMPOWER_BUILD_BENCHMARKS)
    add_subdirectory(src/cli)
endif()

###############################################
###           Test Directory                ###
###############################################

if(DRAMPOWER_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

###############################################
###         Benchmark Directory             ###
###############################################

if(DRAMPOWER_BUILD_BENCHMARKS)
    add_subdirectory(benches)
endif()

###############################################
###           Utility Projects              ###
###############################################

if(${DRAMPOWER_UTILITY_PROJECTS})
    enable_clang_format()
    enable_clang_tidy()
    enable_cppcheck()
endif()
