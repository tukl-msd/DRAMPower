########################################
###              cli11               ###
########################################

project(cli11 VERSION 2.4.2)

if(NOT TARGET cli11)

add_library(${PROJECT_NAME} INTERFACE)
target_include_directories(${PROJECT_NAME} INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/include")

add_library(${PROJECT_NAME}::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

endif()