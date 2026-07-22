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
    DRAMPower/standards/lpddr5/LPDDR5Core.cpp
    DRAMPower/standards/lpddr5/LPDDR5Interface.cpp
    DRAMPower/standards/lpddr5/interface_calculation_LPDDR5.cpp
    DRAMPower/util/extensions.cpp
)
add_library(DRAMPower::DRAMPower ALIAS DRAMPower)

target_include_directories(DRAMPower PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/DRAMPower>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_link_libraries(DRAMPower
PUBLIC
    DRAMUtils::DRAMUtils
)

target_compile_features(DRAMPower PUBLIC cxx_std_17)
set_target_properties(DRAMPower PROPERTIES CXX_EXTENSIONS OFF)
set_target_properties(DRAMPower PROPERTIES CXX_STANDARD_REQUIRED ON)

target_sources(DRAMPower
PUBLIC
    FILE_SET HEADERS
FILES
    DRAMPower/Exceptions.h
    DRAMPower/Types.h
    DRAMPower/command/CmdType.h
    DRAMPower/command/Command.h
    DRAMPower/command/Pattern.h
    DRAMPower/data/energy.h
    DRAMPower/data/stats.h
    DRAMPower/dram/Bank.h
    DRAMPower/dram/Interface.h
    DRAMPower/dram/Rank.h
    DRAMPower/dram/dram_base.h
    DRAMPower/memspec/MemSpec.h
    DRAMPower/memspec/MemSpecDDR4.h
    DRAMPower/memspec/MemSpecDDR5.h
    DRAMPower/memspec/MemSpecLPDDR4.h
    DRAMPower/memspec/MemSpecLPDDR5.h
    DRAMPower/simconfig/simconfig.h
    DRAMPower/standards/ddr4/DDR4.h
    DRAMPower/standards/ddr4/DDR4Core.h
    DRAMPower/standards/ddr4/DDR4Interface.h
    DRAMPower/standards/ddr4/core_calculation_DDR4.h
    DRAMPower/standards/ddr4/interface_calculation_DDR4.h
    DRAMPower/standards/ddr4/types.h
    DRAMPower/standards/ddr5/DDR5.h
    DRAMPower/standards/ddr5/DDR5Core.h
    DRAMPower/standards/ddr5/DDR5Interface.h
    DRAMPower/standards/ddr5/core_calculation_DDR5.h
    DRAMPower/standards/ddr5/interface_calculation_DDR5.h
    DRAMPower/standards/ddr5/types.h
    DRAMPower/standards/lpddr4/LPDDR4.h
    DRAMPower/standards/lpddr4/LPDDR4Core.h
    DRAMPower/standards/lpddr4/LPDDR4Interface.h
    DRAMPower/standards/lpddr4/core_calculation_LPDDR4.h
    DRAMPower/standards/lpddr4/interface_calculation_LPDDR4.h
    DRAMPower/standards/lpddr4/types.h
    DRAMPower/standards/lpddr5/LPDDR5.h
    DRAMPower/standards/lpddr5/LPDDR5Core.h
    DRAMPower/standards/lpddr5/LPDDR5Interface.h
    DRAMPower/standards/lpddr5/core_calculation_LPDDR5.h
    DRAMPower/standards/lpddr5/interface_calculation_LPDDR5.h
    DRAMPower/standards/lpddr5/types.h
    DRAMPower/standards/test_accessor.h
    DRAMPower/util/Deserialize.h
    DRAMPower/util/ImplicitCommandHandler.h
    DRAMPower/util/PatternHandler.h
    DRAMPower/util/RegisterHelper.h
    DRAMPower/util/Router.h
    DRAMPower/util/Serialize.h
    DRAMPower/util/binary_ops.h
    DRAMPower/util/burst_storage.h
    DRAMPower/util/bus.h
    DRAMPower/util/bus_types.h
    DRAMPower/util/cli_architecture_config.h
    DRAMPower/util/clock.h
    DRAMPower/util/cycle_stats.h
    DRAMPower/util/databus.h
    DRAMPower/util/databus_presets.h
    DRAMPower/util/databus_types.h
    DRAMPower/util/dbi.h
    DRAMPower/util/dbialgos.h
    DRAMPower/util/dbihelpers.h
    DRAMPower/util/dbitypes.h
    DRAMPower/util/dynamic_bitset.h
    DRAMPower/util/extension_base.h
    DRAMPower/util/extension_manager.h
    DRAMPower/util/extension_manager_static.h
    DRAMPower/util/extensions.h
    DRAMPower/util/pending_stats.h
    DRAMPower/util/pin.h
    DRAMPower/util/pin_types.h
    DRAMPower/util/sub_bitset.h
)

# Add test functions
if(DRAMPOWER_BUILD_TESTS)
    target_compile_definitions(DRAMPower PUBLIC DRAMPOWER_TESTING)
endif()

if(DRAMPOWER_INSTALL)
    include(GNUInstallDirs)
    include(CMakePackageConfigHelpers)

    install(
        TARGETS DRAMPower
        EXPORT  DRAMPowerTargets
        FILE_SET HEADERS
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )

    install(
        EXPORT DRAMPowerTargets
        FILE DRAMPowerTargets.cmake
        NAMESPACE DRAMPower::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/DRAMPower
    )

    configure_package_config_file(
        ${PROJECT_SOURCE_DIR}/cmake/DRAMPowerConfig.cmake.in
        ${CMAKE_CURRENT_BINARY_DIR}/DRAMPowerConfig.cmake
        INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/DRAMPower
    )

    write_basic_package_version_file(
        ${CMAKE_CURRENT_BINARY_DIR}/DRAMPowerConfigVersion.cmake
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY SameMajorVersion
    )

    install(
        FILES
            ${CMAKE_CURRENT_BINARY_DIR}/DRAMPowerConfig.cmake
            ${CMAKE_CURRENT_BINARY_DIR}/DRAMPowerConfigVersion.cmake
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/DRAMPower
    )
endif()
