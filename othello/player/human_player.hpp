#ifndef HUMAN_PLAYER_HPP
#define HUMAN_PLAYER_HPP

#include <iostream>

#include "player.hpp"
#include "util/util.hpp"

namespace othello::player {

class HumanPlayer : public Player {
public:
    HumanPlayer() :
    Player()
    {
        //
    }
    ~HumanPlayer() override = default;

    uint64_t get_move(const othello::board::Board& board) override {
        std::cout << "Enter your move: ";
        std::string input;
        std::cin >> input;
        return othello::util::parse_move(input);
    }
};

} // namespace othello::player

#endif // HUMAN_PLAYER_HPP
