#ifndef OTHELLO_GAME_HPP
#define OTHELLO_GAME_HPP

#include <cstdint>
#include <iostream>
#include <memory>
#include <print>
#include <stdexcept>
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
        std::unique_ptr<othello::player::Player> player_white,
        othello::board::Board board = othello::board::Board()
    ) :
    board_(board),
    player_black_(std::move(player_black)),
    player_white_(std::move(player_white))
    {
        player_black_->set_color(othello::Color::BLACK);
        player_white_->set_color(othello::Color::WHITE);
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

    int run_silent() {
        // returns 1 if Black wins, -1 if White wins, 0 if it's a draw.
        while (!board_.game_over()) {
            if (board_.turn() == othello::Color::BLACK) {
                uint64_t move = player_black_->get_move(board_);
                if (!board_.move_valid(move)) {
                    throw std::invalid_argument("Invalid move by black player.");
                }
                board_.make_move(move);
            } else {
                uint64_t move = player_white_->get_move(board_);
                if (!board_.move_valid(move)) {
                    throw std::invalid_argument("Invalid move by white player.");
                }
                board_.make_move(move);
            }
        }
        if (board_.score().first > board_.score().second) {
            return 1; // Black wins
        } else if (board_.score().first < board_.score().second) {
            return -1; // White wins
        } else {
            return 0; // Draw
        }
    }
};

} // namespace othello::game

#endif // OTHELLO_GAME_HPP
