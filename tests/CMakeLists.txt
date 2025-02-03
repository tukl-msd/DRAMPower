include(GoogleTest)

option(DRAMPOWER_USE_FETCH_CONTENT_GOOGLE_TEST "Enable FetchContent to provide Google Test" ${DRAMPOWER_USE_FETCH_CONTENT})

if (DRAMPOWER_USE_FETCH_CONTENT)
    if (DRAMPOWER_USE_FETCH_CONTENT_GOOGLE_TEST)
        FetchContent_Declare(
            GTest
            URL https://github.com/google/googletest/releases/download/v1.15.2/googletest-1.15.2.tar.gz
            OVERRIDE_FIND_PACKAGE
        )
        # For Windows: Prevent overriding the parent project's compiler/linker settings
        set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

        FetchContent_MakeAvailable(GTest)
        # TODO: Necessary target properties?
        set_target_properties(gmock PROPERTIES FOLDER lib)
        set_target_properties(gmock_main PROPERTIES FOLDER lib)
        set_target_properties(gtest PROPERTIES FOLDER lib)
        set_target_properties(gtest_main PROPERTIES FOLDER lib)
    endif()
endif()

find_package(GTest REQUIRED)

add_subdirectory(tests_drampower)
add_subdirectory(tests_misc)
