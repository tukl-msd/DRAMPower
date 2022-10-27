###############################################
###          enable_clang_tidy              ###
###############################################
###
### Enables a clang-tidy target, if
### clang-tidy is installed, to run static
### code analysis on all sources
###

function( enable_clang_tidy )
    file(GLOB_RECURSE SOURCE_FILES src/*.c src/*.cpp src/*.cxx src/*.cc )
    file(GLOB_RECURSE HEADER_FILES src/*.h src/*.hpp src/*.hxx src/*.hh )
		
	find_program(UTIL_TIDY_PATH clang-tidy)
	if(UTIL_TIDY_PATH)
		message(STATUS "Using clang-tidy static-analysis: yes")
		add_custom_target(clang-tidy
			COMMAND ${UTIL_TIDY_PATH} ${SOURCE_FILES} ${HEADER_FILES} -p=./ )
	else()
		message(STATUS "Using clang-tidy static-analysis: no")
	endif()
endfunction()
