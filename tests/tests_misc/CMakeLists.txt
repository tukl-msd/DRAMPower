###############################################
###              tests_misc                 ###
###############################################

add_executable(tests_misc
	test_bus_extended.cpp
	test_bus.cpp
	test_clock.cpp
	test_dynamic_bitset.cpp
	test_interval.cpp
	test_misc.cpp
	test_pattern.cpp
)

target_link_libraries(tests_misc
	DRAMPower::DRAMPower
	DRAMUtils::DRAMUtils
	gtest
	gtest_main
)

gtest_discover_tests(tests_misc
	WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
