########################################
###              CLI11               ###
########################################


FetchContent_Declare(CLI11
    URL "https://github.com/CLIUtils/CLI11/archive/refs/tags/v2.4.2.tar.gz"
    OVERRIDE_FIND_PACKAGE
)

FetchContent_MakeAvailable(CLI11)