# othello

Header-only Othello board game API for `othello_bot`.

## Requirements

- C++23 (uses `<print>`)
- CMake 3.21+
- On MinGW, `stdc++exp` is linked automatically for `std::print`

## Using it in another project

With CMake `FetchContent`:

```cmake
include(FetchContent)
FetchContent_Declare(othello
    GIT_REPOSITORY https://github.com/isaacdchu/othello.git
    GIT_TAG v0.2.0
)
FetchContent_MakeAvailable(othello)

target_link_libraries(your_target PRIVATE othello::othello)
```

Or as a git submodule:

```cmake
add_subdirectory(external/othello)
target_link_libraries(your_target PRIVATE othello::othello)
```

When included this way, the demo executable and tests are not built.

Then include everything at once:

```cpp
#include <othello/othello.hpp>
```

or just the parts you need, e.g. `<othello/board/board.hpp>`, `<othello/game/game.hpp>`,
`<othello/player/player.hpp>`.

## Building this repo

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build
```

Options (default `ON` when this is the top-level project, `OFF` otherwise):

- `OTHELLO_BUILD_EXAMPLES` - build the `main` demo
- `OTHELLO_BUILD_TESTS` - build the googletest suite
