###############################################
###              tests_drampower            ###
###############################################

cmake_minimum_required(VERSION 3.1.0)

project(tests_drampower)

file(GLOB_RECURSE SOURCE_FILES core/DDR5/*.cpp
							   core/DDR4/*.cpp
							   core/LPDDR5/*.cpp
		                       core/LPDDR4/*.cpp
		 					   base/*.cpp
							   interface/*.cpp)

add_executable(${PROJECT_NAME} ${SOURCE_FILES} ${HEADER_FILES})

target_compile_definitions(${PROJECT_NAME} PUBLIC TEST_RESOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/resources/")
target_include_directories(${PROJECT_NAME} PUBLIC ${PROJECT_SOURCE_DIR})
target_link_libraries(${PROJECT_NAME}
	DRAMSys::DRAMPower
	gtest_main
)

gtest_discover_tests(${PROJECT_NAME})

build_source_group()