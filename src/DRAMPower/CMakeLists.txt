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
if (CPU_TYPE)
    message(STATUS "${PROJECT_NAME} using CPU_TYPE ${CPU_TYPE}")
    target_compile_options(${PROJECT_NAME} PRIVATE -march=${CPU_TYPE})
endif()

add_library(DRAMSys::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

build_source_group()