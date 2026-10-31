#ifndef GAME_HPP
#define GAME_HPP

#include <cstdint>
#include <iostream>
#include <memory>
#include <print>
#include <string>
#include <utility>

#include "../board/board.hpp"
#include "../player/player.hpp"
#include "../util/util.hpp"

namespace othello::game {

class Game {
private:
    othello::board::Board board_;
    std::unique_ptr<othello::player::Player> player_black_;
    std::unique_ptr<othello::player::Player> player_white_;
public:
    Game(
        std::unique_ptr<othello::player::Player> player_black,
        std::unique_ptr<othello::player::Player> player_white
    ) :
    board_(),
    player_black_(std::move(player_black)),
    player_white_(std::move(player_white))
    {
        //
    }

    void run() {
        while (!board_.game_over()) {
            std::println(
                "Current turn: {}",
                board_.turn() == othello::Color::BLACK ? "Black" : "White"
            );
            std::println("{}", board_.to_string());
            if (board_.turn() == othello::Color::BLACK) {
                uint64_t move = player_black_->get_move(board_);
                while (!board_.move_valid(move)) {
                    std::println("Invalid move. Please try again.");
                    move = player_black_->get_move(board_);
                }
                board_.make_move(move);
            } else {
                uint64_t move = player_white_->get_move(board_);
                while (!board_.move_valid(move)) {
                    std::println("Invalid move. Please try again.");
                    move = player_white_->get_move(board_);
                }
                board_.make_move(move);
            }
        }
        std::println("Game over!");
        std::println("{}", board_.to_string());
        if (board_.score().first > board_.score().second) {
            std::println("Black wins!");
        } else if (board_.score().first < board_.score().second) {
            std::println("White wins!");
        } else {
            std::println("It's a draw!");
        }
        std::println(
            "Final score: Black {} - White {}",
            board_.score().first,
            board_.score().second
        );
    }
};

} // namespace othello::game

#endif // GAME_HPP
