#ifndef BOARD_HPP
#define BOARD_HPP

#include <cstddef>
#include <cstdint>
#include <string>
#include <iostream>
#include <stdexcept>

/*
Bit board representation:
MSB is a1 (top left), LSB is h8 (bottom right).
Bits go from left to right, then top to bottom.
Therefore, the second MSB is b1
  a b c ...
1
2
3
...
*/

// d5, e4
static constexpr uint64_t START_BLACK = 0x0000000810000000;
// d4, e5
static constexpr uint64_t START_WHITE = 0x0000001008000000;

namespace othello::board {

class Board {
public:
    enum class Color {
        BLACK,
        WHITE
    };
private:
    uint64_t black_ = 0;
    uint64_t white_ = 0;
    Color turn_ = Color::BLACK;
    bool game_over_ = false;
public:
    Board() :
    black_(START_BLACK),
    white_(START_WHITE),
    turn_(Color::BLACK),
    game_over_(false)
    {
        //
    }

    void make_move(uint64_t move) {
        if (turn_ == Color::BLACK) {
            black_ |= move;
        } else {
            white_ |= move;
        }
        turn_ = (turn_ == Color::BLACK) ? Color::WHITE : Color::BLACK;
    }

    bool move_valid(uint64_t move) const {
        // todo
        return true;
    }

    Color turn() const {
        return turn_;
    }

    bool game_over() const {
        return game_over_;
    }

    std::string to_string() const {
        std::string result = "  a b c d e f g h\n";
        for (int row = 0; row < 8; ++row) {
            result += std::to_string(row + 1);
            result += ' ';
            for (int col = 0; col < 8; ++col) {
                // MSB is a1, so square index i lives at bit (63 - i)
                uint64_t bit = 1ULL << (63 - (row * 8 + col));
                if (col > 0) {
                    result += ' ';
                }
                if (black_ & bit) {
                    result += 'B';
                } else if (white_ & bit) {
                    result += 'W';
                } else {
                    result += '.';
                }
            }
            result += '\n';
        }
        return result;
    }
};

} // namespace othello::board

#endif // BOARD_HPP
