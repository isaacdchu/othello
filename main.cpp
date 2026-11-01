#include <iostream>
#include <print>

#include <othello/othello.hpp>

int main() {
    othello::game::Game game(
        std::make_unique<othello::player::RandomPlayer>(),
        std::make_unique<othello::player::RandomPlayer>()
    );
    game.run();
    return 0;
}
