#ifndef RANDOM_PLAYER_HPP
#define RANDOM_PLAYER_HPP

#include <bit>
#include <random>

#include "player.hpp"

namespace othello::player {

class RandomPlayer : public Player {
private:
    std::mt19937 rng;
public:
    RandomPlayer(othello::Color color) :
    Player(color),
    rng(0)
    {
        //
    }
    ~RandomPlayer() override = default;

    uint64_t get_move(const othello::board::Board& board) override {
        // moves are single-bit bitboards, not square indices
        uint64_t moves = board.legal_moves();
        std::uniform_int_distribution<int> dist(0, std::popcount(moves) - 1);
        for (int k = dist(rng); k > 0; --k) {
            moves &= moves - 1; // clear lowest set bit
        }
        return moves & -moves; // isolate lowest remaining bit
    }
};

} // namespace othello::player

#endif // RANDOM_PLAYER_HPP
