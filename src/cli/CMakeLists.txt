#########################################
###           drampower_cli           ###
#########################################

find_package(spdlog REQUIRED)
find_package(cli11 REQUIRED)

add_executable(drampower_cli 
    main.cpp
    util.cpp
    validators.cpp
)

target_compile_features(drampower_cli PUBLIC cxx_std_17)
set_target_properties(drampower_cli PROPERTIES CXX_EXTENSIONS OFF)
set_target_properties(drampower_cli PROPERTIES CXX_STANDARD_REQUIRED ON)

target_link_libraries(drampower_cli 
    PRIVATE
       DRAMPower::DRAMPower
       DRAMUtils::DRAMUtils
       spdlog::spdlog
       CLI11::CLI11
)
target_compile_definitions(drampower_cli PRIVATE DRAMPOWER_VERSION_STRING="${DRAMPOWER_VERSION_STRING}")
