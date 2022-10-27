###############################################
###          enable_cppcheck                ###
###############################################
###
### Enables a cppcheck target, if
### cppcheck is installed, to run static
### analysis on all sources
###

function( enable_cppcheck )
    file(GLOB_RECURSE SOURCE_FILES src/*.c src/*.cpp src/*.cxx src/*.cc )
    file(GLOB_RECURSE HEADER_FILES src/*.h src/*.hpp src/*.hxx src/*.hh )
	
	find_program(UTIL_CPPCHECK_PATH cppcheck)
	if(UTIL_CPPCHECK_PATH)
		message(STATUS "Using cppcheck static-analysis: yes ")
		add_custom_target(
				cppcheck
				COMMAND ${UTIL_CPPCHECK_PATH}
				--enable=warning,performance,portability,information,missingInclude
				--language=c++
				--std=c++11
				--template=gcc
				--verbose
				--quiet
				${SOURCE_FILES} ${HEADER_FILES}
		)
	else()
		message(STATUS "Using cppcheck static-analysis: no ")
	endif()
endfunction()
