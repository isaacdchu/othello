include(FetchContent)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.17.0.zip
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
# keep gtest's runtime consistent with ours on Windows
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

add_executable(othello_tests
    board_test.cpp
    game_test.cpp
    util_test.cpp
)
target_link_libraries(othello_tests PRIVATE othello GTest::gtest_main)
target_compile_options(othello_tests PRIVATE -Wall -Wextra -Werror)

include(GoogleTest)
gtest_discover_tests(othello_tests)
