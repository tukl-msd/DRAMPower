########################################
###             DRAMPower            ###
########################################

find_package(DRAMUtils REQUIRED)

add_library(DRAMPower
    DRAMPower/command/Command.cpp
    DRAMPower/command/Pattern.cpp
    DRAMPower/data/energy.cpp
    DRAMPower/dram/Interface.cpp
    DRAMPower/dram/Rank.cpp
    DRAMPower/memspec/MemSpecDDR4.cpp
    DRAMPower/memspec/MemSpecDDR5.cpp
    DRAMPower/memspec/MemSpecLPDDR4.cpp
    DRAMPower/memspec/MemSpecLPDDR5.cpp
    DRAMPower/standards/ddr4/core_calculation_DDR4.cpp
    DRAMPower/standards/ddr4/DDR4.cpp
    DRAMPower/standards/ddr4/DDR4Core.cpp
    DRAMPower/standards/ddr4/DDR4Interface.cpp
    DRAMPower/standards/ddr4/interface_calculation_DDR4.cpp
    DRAMPower/standards/ddr5/core_calculation_DDR5.cpp
    DRAMPower/standards/ddr5/DDR5.cpp
    DRAMPower/standards/ddr5/DDR5Core.cpp
    DRAMPower/standards/ddr5/DDR5Interface.cpp
    DRAMPower/standards/ddr5/interface_calculation_DDR5.cpp
    DRAMPower/standards/lpddr4/core_calculation_LPDDR4.cpp
    DRAMPower/standards/lpddr4/LPDDR4.cpp
    DRAMPower/standards/lpddr4/LPDDR4Core.cpp
    DRAMPower/standards/lpddr4/LPDDR4Interface.cpp
    DRAMPower/standards/lpddr4/interface_calculation_LPDDR4.cpp
    DRAMPower/standards/lpddr5/core_calculation_LPDDR5.cpp
    DRAMPower/standards/lpddr5/LPDDR5.cpp
    DRAMPower/standards/lpddr5/interface_calculation_LPDDR5.cpp
    DRAMPower/util/binary_ops.cpp
    DRAMPower/util/extensions.cpp
    DRAMPower/util/pin.cpp
    DRAMPower/util/dbi.cpp
    DRAMPower/util/ImplicitCommandHandler.cpp
)

target_include_directories(DRAMPower PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_compile_features(DRAMPower PUBLIC cxx_std_17)
set_target_properties(DRAMPower PROPERTIES CXX_EXTENSIONS OFF)
set_target_properties(DRAMPower PROPERTIES CXX_STANDARD_REQUIRED ON)

target_link_libraries(DRAMPower
PUBLIC 
    DRAMUtils::DRAMUtils
)

# Add test functions
if(DRAMPOWER_BUILD_TESTS)
    target_compile_definitions(DRAMPower PUBLIC DRAMPOWER_TESTING)
endif()

add_library(DRAMPower::DRAMPower ALIAS DRAMPower)
