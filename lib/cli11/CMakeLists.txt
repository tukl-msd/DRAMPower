########################################
###              CLI11               ###
########################################


FetchContent_Declare(cli11
    URL "https://github.com/CLIUtils/CLI11/archive/refs/tags/v2.4.2.tar.gz"
    OVERRIDE_FIND_PACKAGE
)

FetchContent_MakeAvailable(cli11)