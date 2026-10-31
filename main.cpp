#include <iostream>
#include <print>

#include <othello>

int main() {
    othello::game::Game game(
        std::make_unique<othello::player::RandomPlayer>(othello::Color::BLACK),
        std::make_unique<othello::player::RandomPlayer>(othello::Color::WHITE)
    );
    game.run();
    return 0;
}
