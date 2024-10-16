########################################
###                CLI               ###
########################################

project(drampower_cli)

#### Dependencies ###

### spdlog ###
FetchContent_Declare(
        spdlog
        GIT_REPOSITORY https://github.com/gabime/spdlog.git
        GIT_TAG        v1.9.2)
FetchContent_MakeAvailable(spdlog)
set_target_properties(spdlog PROPERTIES FOLDER lib)
# FetchContent_GetProperties(spdlog)
# if (NOT spdlog_POPULATED)
#     FetchContent_Populate(spdlog)
#     add_subdirectory(${spdlog_SOURCE_DIR} ${spdlog_BINARY_DIR} EXCLUDE_FROM_ALL)
#     set_target_properties(spdlog PROPERTIES FOLDER lib)
# endif()


#### Project configuration ####

file(GLOB_RECURSE SOURCE_FILES CONFIGURE_DEPENDS *.cpp)
file(GLOB_RECURSE HEADER_FILES CONFIGURE_DEPENDS *.h)

add_compile_definitions(DRAMPOWER_VERSION_STRING="${DRAMPOWER_VERSION_STRING}")

add_executable(${PROJECT_NAME} ${SOURCE_FILES})
target_link_libraries(${PROJECT_NAME} 
    PRIVATE 
        DRAMSys::DRAMPower
        spdlog::spdlog
        DRAMUtils
        cli11
)
if (CPU_TYPE)
    message(STATUS "${PROJECT_NAME} using CPU_TYPE ${CPU_TYPE}")
    target_compile_options(${PROJECT_NAME} PRIVATE -march=${CPU_TYPE})
endif()


build_source_group()