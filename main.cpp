#include <print>
#include <iostream>

#include <othello>

int main() {
    othello::board::Board board{};
    std::print("{}", board.to_string());
    return 0;
}
