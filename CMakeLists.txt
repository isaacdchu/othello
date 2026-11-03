cmake_minimum_required(VERSION 3.21)
project(othello VERSION 0.3.0 LANGUAGES CXX)

add_library(othello INTERFACE)
add_library(othello::othello ALIAS othello)
target_include_directories(othello INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_compile_features(othello INTERFACE cxx_std_23)

# on mingw, std::print's windows terminal support lives in libstdc++exp
if(MINGW)
    target_link_libraries(othello INTERFACE stdc++exp)
endif()

# only build the demo and tests when this is the top-level project,
# not when pulled in via add_subdirectory/FetchContent
option(OTHELLO_BUILD_EXAMPLES "Build the othello demo executable" ${PROJECT_IS_TOP_LEVEL})
option(OTHELLO_BUILD_TESTS "Build the othello tests" ${PROJECT_IS_TOP_LEVEL})

if(OTHELLO_BUILD_EXAMPLES)
    add_executable(main main.cpp)
    target_link_libraries(main PRIVATE othello::othello)
    target_compile_options(main PRIVATE -Wall -Wextra -Wno-unused-parameter -Wno-unused-variable -Wno-unused-function -Werror -O2)
endif()

if(OTHELLO_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
