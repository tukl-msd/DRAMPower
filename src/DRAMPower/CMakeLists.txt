########################################
###             DRAMPower            ###
########################################

project(DRAMPower)

file(GLOB_RECURSE SOURCE_FILES CONFIGURE_DEPENDS *.cpp)
file(GLOB_RECURSE HEADER_FILES CONFIGURE_DEPENDS *.h;*.hpp)

add_library(DRAMPower ${SOURCE_FILES} ${HEADER_FILES})
target_include_directories(DRAMPower PUBLIC ${PROJECT_SOURCE_DIR})
target_link_libraries(DRAMPower PUBLIC 
	DRAMUtils
)

add_library(DRAMSys::DRAMPower ALIAS DRAMPower)

build_source_group()