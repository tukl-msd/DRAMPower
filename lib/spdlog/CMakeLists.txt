########################################
###              spdlog              ###
########################################


FetchContent_Declare(spdlog
    URL "https://github.com/gabime/spdlog/archive/refs/tags/v1.17.0.tar.gz"
    OVERRIDE_FIND_PACKAGE
)

FetchContent_MakeAvailable(spdlog)