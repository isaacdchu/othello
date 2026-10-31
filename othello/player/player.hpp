#ifndef PLAYER_HPP
#define PLAYER_HPP

#include <cstddef>
#include <cstdint>

#include "../board/board.hpp"

namespace othello::player {

class Player {
public:
    virtual ~Player() = default;

    virtual uint64_t get_move(const othello::board::Board& board) = 0;
};

} // namespace othello::player

#endif // PLAYER_HPP
