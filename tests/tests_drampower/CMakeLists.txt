###############################################
###              tests_drampower            ###
###############################################

add_executable(tests_drampower
	base/test_ddr_base.cpp
	base/test_ddr_data.cpp
	base/test_pattern_pre_cycles.cpp

	core/DDR4/ddr4_multidevice_tests.cpp
	core/DDR4/ddr4_multirank_tests.cpp
	core/DDR4/ddr4_test_pattern_0.cpp
	core/DDR4/ddr4_test_pattern_1.cpp
	core/DDR4/ddr4_test_pattern_2.cpp
	core/DDR4/ddr4_test_pattern_3.cpp
	core/DDR4/ddr4_test_pattern_4.cpp
	core/DDR4/ddr4_test_pattern_5.cpp
	core/DDR4/ddr4_test_pattern_6.cpp
	core/DDR4/ddr4_test_pattern_7.cpp
	core/DDR4/ddr4_test_pattern_8.cpp
	core/DDR4/ddr4_test_pattern_9.cpp
	core/DDR4/ddr4_test_pattern_10.cpp
	core/DDR4/ddr4_test_pattern_11.cpp
	core/DDR4/ddr4_test_pattern_12.cpp
	core/DDR4/ddr4_test_pattern_13.cpp
	core/DDR4/ddr4_test_pattern_14.cpp
	core/DDR4/ddr4_test_pattern_15.cpp

	core/DDR5/ddr5_multidevice_tests.cpp
	core/DDR5/ddr5_multirank_tests.cpp
	core/DDR5/ddr5_test_pattern_0.cpp
	core/DDR5/ddr5_test_pattern_1.cpp
	core/DDR5/ddr5_test_pattern_2.cpp
	core/DDR5/ddr5_test_pattern_3.cpp
	core/DDR5/ddr5_test_pattern_4.cpp
	core/DDR5/ddr5_test_pattern_5.cpp
	core/DDR5/ddr5_test_pattern_6.cpp
	core/DDR5/ddr5_test_pattern_7.cpp
	core/DDR5/ddr5_test_pattern_8.cpp
	core/DDR5/ddr5_test_pattern_9.cpp
	core/DDR5/ddr5_test_pattern_10.cpp
	core/DDR5/ddr5_test_pattern_11.cpp
	core/DDR5/ddr5_test_pattern_12.cpp
	core/DDR5/ddr5_test_pattern_13.cpp
	core/DDR5/ddr5_test_pattern_14.cpp
	core/DDR5/ddr5_test_pattern_15.cpp
	core/DDR5/ddr5_test_pattern_16.cpp
	core/DDR5/ddr5_test_pattern_17.cpp
	core/DDR5/ddr5_test_pattern_18.cpp

	core/LPDDR4/lpddr4_multidevice_tests.cpp
	core/LPDDR4/lpddr4_multirank_tests.cpp
	core/LPDDR4/lpddr4_test_pattern_0.cpp
	core/LPDDR4/lpddr4_test_pattern_1.cpp
	core/LPDDR4/lpddr4_test_pattern_2.cpp
	core/LPDDR4/lpddr4_test_pattern_3.cpp
	core/LPDDR4/lpddr4_test_pattern_4.cpp
	core/LPDDR4/lpddr4_test_pattern_5.cpp
	core/LPDDR4/lpddr4_test_pattern_6.cpp
	core/LPDDR4/lpddr4_test_pattern_7.cpp
	core/LPDDR4/lpddr4_test_pattern_8.cpp
	core/LPDDR4/lpddr4_test_pattern_9.cpp
	core/LPDDR4/lpddr4_test_pattern_10.cpp
	core/LPDDR4/lpddr4_test_pattern_11.cpp
	core/LPDDR4/lpddr4_test_pattern_12.cpp
	core/LPDDR4/lpddr4_test_pattern_13.cpp
	core/LPDDR4/lpddr4_test_pattern_14.cpp
	core/LPDDR4/lpddr4_test_pattern_15.cpp
	core/LPDDR4/lpddr4_test_pattern_16.cpp
	core/LPDDR4/lpddr4_test_pattern_17.cpp
	core/LPDDR4/lpddr4_test_pattern_18.cpp

	core/LPDDR5/lpddr5_multidevice_tests.cpp
	core/LPDDR5/lpddr5_multirank_tests.cpp
	core/LPDDR5/lpddr5_test_pattern_0.cpp
	core/LPDDR5/lpddr5_test_pattern_1.cpp
	core/LPDDR5/lpddr5_test_pattern_2.cpp
	core/LPDDR5/lpddr5_test_pattern_3.cpp
	core/LPDDR5/lpddr5_test_pattern_4.cpp
	core/LPDDR5/lpddr5_test_pattern_5.cpp
	core/LPDDR5/lpddr5_test_pattern_6.cpp
	core/LPDDR5/lpddr5_test_pattern_7.cpp
	core/LPDDR5/lpddr5_test_pattern_8.cpp
	core/LPDDR5/lpddr5_test_pattern_9.cpp
	core/LPDDR5/lpddr5_test_pattern_10.cpp
	core/LPDDR5/lpddr5_test_pattern_11.cpp
	core/LPDDR5/lpddr5_test_pattern_12.cpp
	core/LPDDR5/lpddr5_test_pattern_13.cpp
	core/LPDDR5/lpddr5_test_pattern_14.cpp
	core/LPDDR5/lpddr5_test_pattern_15.cpp
	core/LPDDR5/lpddr5_test_pattern_16.cpp
	core/LPDDR5/lpddr5_test_pattern_17.cpp
	core/LPDDR5/lpddr5_test_pattern_18.cpp
	core/LPDDR5/lpddr5_test_pattern_19.cpp
	core/LPDDR5/lpddr5_test_pattern_20.cpp
	core/LPDDR5/lpddr5_test_pattern_21.cpp

	interface/test_interface_ddr4.cpp
	interface/test_interface_ddr5.cpp
	interface/test_interface_lpddr4.cpp
	interface/test_interface_lpddr5.cpp

	interface/test_togglingrate_ddr4.cpp
	interface/test_togglingrate_ddr5.cpp
	interface/test_togglingrate_lpddr4.cpp
	interface/test_togglingrate_lpddr5.cpp

	interface/test_dbi_ddr4.cpp
	interface/test_dbi_lpddr4.cpp
	interface/test_dbi_lpddr5.cpp
)

set_target_properties(tests_drampower PROPERTIES FOLDER tests/drampower)

target_compile_definitions(tests_drampower PUBLIC TEST_RESOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/resources/")
target_include_directories(tests_drampower PUBLIC ${PROJECT_SOURCE_DIR})

target_link_libraries(tests_drampower
	DRAMPower::DRAMPower
	gtest
	gtest_main
)

gtest_discover_tests(tests_drampower
	WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
