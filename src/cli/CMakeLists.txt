########################################
###                CLI               ###
########################################

project(cli)

#### Dependencies ###

### spdlog ###
# FetchContent_Declare(
#         spdlog
#         GIT_REPOSITORY https://github.com/gabime/spdlog.git
#         GIT_TAG        v1.9.2)

# FetchContent_GetProperties(spdlog)
# if (NOT spdlog_POPULATED)
#     FetchContent_Populate(spdlog)
#     add_subdirectory(${spdlog_SOURCE_DIR} ${spdlog_BINARY_DIR} EXCLUDE_FROM_ALL)
#     set_target_properties(spdlog PROPERTIES FOLDER lib)
# endif()

#### Project configuration ####

file(GLOB_RECURSE SOURCE_FILES CONFIGURE_DEPENDS *.cpp)
file(GLOB_RECURSE HEADER_FILES CONFIGURE_DEPENDS *.h)

add_executable(drampower_cli ${SOURCE_FILES})
target_link_libraries(drampower_cli 
	PRIVATE 
		DRAMSys::DRAMPower
		# spdlog::spdlog
)

build_source_group()