########################################
###             DRAMPower            ###
########################################

project(DRAMPower)

file(GLOB_RECURSE SOURCE_FILES CONFIGURE_DEPENDS *.cpp)
file(GLOB_RECURSE HEADER_FILES CONFIGURE_DEPENDS *.h;*.hpp)

add_library(${PROJECT_NAME} ${SOURCE_FILES} ${HEADER_FILES})
target_include_directories(${PROJECT_NAME} PUBLIC ${PROJECT_SOURCE_DIR})
target_link_libraries(${PROJECT_NAME} PUBLIC 
    DRAMUtils
)
message(DEBUG "${PROJECT_NAME} using CMAKE_CXX_FLAGS: ${CMAKE_CXX_FLAGS}")

add_library(DRAMSys::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

build_source_group()