
if (DRAMPOWER_BUILD_CLI)
    add_subdirectory(main)
endif()
if (DRAMPOWER_BUILD_CLI OR DRAMPOWER_BUILD_BENCHMARKS)
    add_subdirectory(lib)
endif()
