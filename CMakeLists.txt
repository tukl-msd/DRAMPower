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
# Author: Thomas Psota

###############################################
###              DRAMPower 5.0              ###
###############################################
cmake_minimum_required(VERSION 3.13.0)

set(PROJECT_NAME "DRAMPower 5.0")
set(PROJECT_SHORTNAME "DRAMPower")

project(${PROJECT_NAME} VERSION "5.0")

### CMake settings ###
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")
include(build_source_group)
include(diagnostics_print)
include(enable_clang_format)
include(enable_clang_tidy)
include(enable_cppcheck)
include(FetchContent)

# Check if standalone build or being included as submodule
get_directory_property(DRAMPOWER_IS_SUBMODULE PARENT_DIRECTORY)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

### Project settings ###
message(STATUS "CMAKE_SOURCE_DIR: ${CMAKE_SOURCE_DIR}")
message(STATUS "CMAKE_BINARY_DIR: ${CMAKE_BINARY_DIR}")
message(STATUS "" )

if(NOT DRAMPOWER_IS_SUBMODULE)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
endif()

### DRAMPower directories ###
set(DRAMPOWER_SOURCE_DIR  "${CMAKE_CURRENT_SOURCE_DIR}/src")
set(DRAMPOWER_LIBRARY_DIR "${CMAKE_CURRENT_SOURCE_DIR}/lib")
set(DRAMPOWER_TEST_DIR    "${CMAKE_CURRENT_SOURCE_DIR}/tests")

### Build options ###
option(DRAMPOWER_BUILD_TESTS "Build DRAMPower unit tests" OFF)
option(DRAMPOWER_VERBOSE_CMAKE_OUTPUT "Show detailed CMake output" OFF)
option(DRAMPOWER_BUILD_CLI "Build DRAMPower Command Line Tool" OFF)

set(DRAMPOWER_LOCAL_DRAMUTILS_FOLDER "" CACHE PATH "Local DRAMUtils installation path (optional)")

### Compiler settings ###
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

###############################################
###           Library Settings              ###
###############################################

### Detect OS threading library ###
find_package(Threads)

### GoogleTest ###
if(DRAMPOWER_BUILD_TESTS)
	FetchContent_Declare(
		googletest
		GIT_REPOSITORY https://github.com/google/googletest
		GIT_TAG release-1.12.1
	)

	# For Windows: Prevent overriding the parent project's compiler/linker settings
	set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
	FetchContent_MakeAvailable(googletest)
	set_target_properties(gmock PROPERTIES FOLDER lib)
	set_target_properties(gmock_main PROPERTIES FOLDER lib)
	set_target_properties(gtest PROPERTIES FOLDER lib)
	set_target_properties(gtest_main PROPERTIES FOLDER lib)
endif()

### DRAMUtils ###
if (DRAMPOWER_LOCAL_DRAMUTILS_FOLDER) # Fetch DRAMUtils from supplied local path
	FetchContent_Declare(
		DRAMUtils
		SOURCE_DIR ${DRAMPOWER_LOCAL_DRAMUTILS_FOLDER}
	)
else() # Fetch DRAMUtils from github
	FetchContent_Declare(
		DRAMUtils
		GIT_REPOSITORY https://github.com/tukl-msd/DRAMUtils
		GIT_TAG v1.0
	)
endif()

FetchContent_MakeAvailable(DRAMUtils)
set_target_properties(DRAMUtils PROPERTIES FOLDER lib)


###############################################
###           Source Directory              ###
###############################################

add_subdirectory(src/DRAMPower)

if(DRAMPOWER_BUILD_CLI)
	add_subdirectory(src/cli)
endif()

###############################################
###           Test Directory                ###
###############################################

if(DRAMPOWER_BUILD_TESTS)
	include( GoogleTest )
	include( CTest )
	add_subdirectory(tests)
endif()

###############################################
###           Utility Projects              ###
###############################################

if(${DRAMPOWER_UTILITY_PROJECTS})
	enable_clang_format()
	enable_clang_tidy()
	enable_cppcheck()
endif()
