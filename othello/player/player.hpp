#ifndef PLAYER_HPP
#define PLAYER_HPP

#include <cstddef>
#include <cstdint>

#include "../board/board.hpp"

namespace othello::player {

class Player {
protected:
    othello::Color color_;
public:
    Player(othello::Color color) : color_(color) {}
    virtual ~Player() = default;

    virtual uint64_t get_move(const othello::board::Board& board);

    othello::Color color() const {
        return color_;
    }
};

} // namespace othello::player

#endif // PLAYER_HPP
