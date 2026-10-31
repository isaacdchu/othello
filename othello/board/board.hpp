#ifndef BOARD_HPP
#define BOARD_HPP

#include <bit>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>

#include "../util/util.hpp"

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

static constexpr uint64_t FILE_A = 0x8080808080808080ULL;
static constexpr uint64_t FILE_H = 0x0101010101010101ULL;

namespace othello::board {

class Board {
private:
    uint64_t black_ = 0;
    uint64_t white_ = 0;
    othello::Color turn_ = othello::Color::BLACK;
    bool game_over_ = false;
    bool passed_ = false;

    // Shift a bitboard one step in a direction, dropping bits that would wrap
    // around a board edge. Directions: 0=E, 1=W, 2=S, 3=N, 4=SE, 5=SW, 6=NE, 7=NW.
    // East (col+1) is a right shift by 1; south (row+1) is a right shift by 8.
    static constexpr uint64_t shift(uint64_t b, int dir) {
        switch (dir) {
            case 0: return (b >> 1) & ~FILE_A; // E
            case 1: return (b << 1) & ~FILE_H; // W
            case 2: return b >> 8;             // S
            case 3: return b << 8;             // N
            case 4: return (b >> 9) & ~FILE_A; // SE
            case 5: return (b >> 7) & ~FILE_H; // SW
            case 6: return (b << 7) & ~FILE_A; // NE
            case 7: return (b << 9) & ~FILE_H; // NW
            default: return 0;
        }
    }

    // All squares where `own` could legally play against `opp`.
    // For each direction, grow runs of opponent pieces outward from our pieces;
    // an empty square just past such a run is a legal move.
    static constexpr uint64_t moves_for(uint64_t own, uint64_t opp) {
        const uint64_t empty = ~(own | opp);
        uint64_t moves = 0;
        for (int dir = 0; dir < 8; dir++) {
            uint64_t run = shift(own, dir) & opp;
            // a run of opponent pieces is at most 6 long
            for (int i = 0; i < 5; i++) {
                run |= shift(run, dir) & opp;
            }
            moves |= shift(run, dir) & empty;
        }
        return moves;
    }
public:
    Board() :
    black_(START_BLACK),
    white_(START_WHITE),
    turn_(othello::Color::BLACK),
    game_over_(false)
    {
        //
    }

    void make_move(uint64_t move) {
        // compute flips before placing, since captures() reads the current board
        const uint64_t flipped = captures(move);
        if (turn_ == othello::Color::BLACK) {
            black_ |= move | flipped;
            white_ &= ~flipped;
        } else {
            white_ |= move | flipped;
            black_ &= ~flipped;
        }
        turn_ = opponent(turn_);
        passed_ = false;

        if (legal_moves() == 0) {
            // next player has no moves: they pass, and the turn comes back
            turn_ = opponent(turn_);
            if (legal_moves() == 0) {
                // neither player can move
                game_over_ = true;
            } else {
                passed_ = true;
            }
        }
    }

    // Bitboard of every legal move for the player to move.
    uint64_t legal_moves() const {
        return (turn_ == othello::Color::BLACK) ?
            moves_for(black_, white_) : moves_for(white_, black_);
    }

    // True if the opponent had to pass after the last move,
    // so the same player is to move again.
    bool last_move_caused_pass() const {
        return passed_;
    }

    int black_count() const {
        return std::popcount(black_);
    }

    int white_count() const {
        return std::popcount(white_);
    }

    static constexpr othello::Color opponent(othello::Color c) {
        return (c == othello::Color::BLACK) ? othello::Color::WHITE : othello::Color::BLACK;
    }

    bool move_valid(uint64_t move) const {
        // must be exactly one square
        if (!std::has_single_bit(move)) {
            return false;
        }
        if (move & (black_ | white_)) {
            // already occupied
            return false;
        }
        // must capture at least one opponent piece
        return captures(move) != 0;
    }

    // Returns the bitboard of opponent pieces that would be flipped
    // if the current player placed a piece at `move`.
    uint64_t captures(uint64_t move) const {
        const uint64_t own = (turn_ == othello::Color::BLACK) ? black_ : white_;
        const uint64_t opp = (turn_ == othello::Color::BLACK) ? white_ : black_;
        uint64_t flipped = 0;
        for (int dir = 0; dir < 8; dir++) {
            uint64_t line = 0;
            uint64_t cur = shift(move, dir);
            // walk over contiguous opponent pieces
            while (cur & opp) {
                line |= cur;
                cur = shift(cur, dir);
            }
            // the run counts only if it is capped by one of our own pieces
            if (cur & own) {
                flipped |= line;
            }
        }
        return flipped;
    }

    othello::Color turn() const {
        return turn_;
    }

    bool game_over() const {
        return game_over_;
    }

    std::pair<int, int> score() const {
        return {black_count(), white_count()};
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
