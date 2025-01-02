###############################
###         cli_lib         ###
###############################

find_package(spdlog REQUIRED)

add_library(cli_lib 
    DRAMPower/cli/run.cpp
    DRAMPower/cli/util.cpp
)

target_include_directories(cli_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_compile_features(cli_lib PUBLIC cxx_std_17)
set_target_properties(cli_lib PROPERTIES CXX_EXTENSIONS OFF)
set_target_properties(cli_lib PROPERTIES CXX_STANDARD_REQUIRED ON)

target_link_libraries(cli_lib 
PUBLIC
    DRAMPower::DRAMPower
    DRAMUtils::DRAMUtils
    spdlog::spdlog
)

target_compile_definitions(cli_lib PRIVATE DRAMPOWER_VERSION_STRING="${DRAMPOWER_VERSION_STRING}")
add_library(DRAMPower::cli_lib ALIAS cli_lib)