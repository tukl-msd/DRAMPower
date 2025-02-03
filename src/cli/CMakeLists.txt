#########################################
###           drampower_cli           ###
#########################################

find_package(spdlog REQUIRED)
find_package(cli11 REQUIRED)

add_executable(drampower_cli 
    main.cpp
    util.cpp
)
target_link_libraries(drampower_cli 
    PRIVATE
       DRAMPower::DRAMPower
       DRAMUtils::DRAMUtils
       spdlog::spdlog
       CLI11::CLI11
)
target_compile_definitions(drampower_cli PRIVATE DRAMPOWER_VERSION_STRING="${DRAMPOWER_VERSION_STRING}")
