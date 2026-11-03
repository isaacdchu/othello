#ifndef OTHELLO_BOARD_HPP
#define OTHELLO_BOARD_HPP

#include <bit>
#include <cstddef>
#include <cstdint>
#include <functional>
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

namespace othello::board {

// d5, e4
inline constexpr uint64_t START_BLACK = 0x0000000810000000;
// d4, e5
inline constexpr uint64_t START_WHITE = 0x0000001008000000;

inline constexpr uint64_t FILE_A = 0x8080808080808080ULL;
inline constexpr uint64_t FILE_H = 0x0101010101010101ULL;

class Board {
public:
    enum class Symmetry {
        FLIP_HORIZONTAL,
        FLIP_VERTICAL,
        FLIP_DIAGONAL_A1,
        FLIP_DIAGONAL_H1,
        ROTATE_90,
        ROTATE_180,
        ROTATE_270
    };
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
        // Initialize the board with the starting position and black to move.
    }

    Board(uint64_t black, uint64_t white, othello::Color turn) :
    black_(black),
    white_(white),
    turn_(turn),
    game_over_(false),
    passed_(false)
    {
        // Initialize the board with the given position and turn.
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

    // Two boards are equal when they hold the same position: the same discs
    // and the same side to move. game_over_ and passed_ are not compared;
    // game_over_ follows from the position, and passed_ is move history.
    bool operator==(const Board& other) const {
        return (
            (black_ == other.black_) &&
            (white_ == other.white_) &&
            (turn_ == other.turn_)
        );
    }

    bool operator!=(const Board& other) const {
        return !(*this == other);
    }

    uint64_t black() const {
        return black_;
    }

    uint64_t white() const {
        return white_;
    }

    // Hash of the position, consistent with operator==.
    std::size_t hash() const noexcept {
        uint64_t h = othello::util::mix(black_);
        h = othello::util::mix(h ^ white_);
        if (turn_ == othello::Color::WHITE) {
            h = othello::util::mix(h ^ 0x9E3779B97F4A7C15ULL);
        }
        return static_cast<std::size_t>(h);
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
        return (
            (turn_ == othello::Color::BLACK) ?
            moves_for(black_, white_) : moves_for(white_, black_)
        );
    }

    uint64_t legal_moves(othello::Color for_player) const {
        return (
            (for_player == othello::Color::BLACK) ?
            moves_for(black_, white_) : moves_for(white_, black_)
        );
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

    int margin() const {
        return (black_count() - white_count());
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

    // Apply a symmetry of the square to a single bitboard.
    // With a1 at the MSB, each byte is one row (row 1 in the top byte)
    // and within a byte column a is the high bit.
    static constexpr uint64_t transform(uint64_t b, Symmetry symmetry) {
        switch (symmetry) {
            case Symmetry::FLIP_HORIZONTAL: return mirror_columns(b);
            case Symmetry::FLIP_VERTICAL: return mirror_rows(b);
            case Symmetry::FLIP_DIAGONAL_A1: return transpose_a1h8(b);
            case Symmetry::FLIP_DIAGONAL_H1: return transpose_h1a8(b);
            // clockwise: transpose, then mirror columns
            case Symmetry::ROTATE_90: return mirror_columns(transpose_a1h8(b));
            case Symmetry::ROTATE_180: return mirror_rows(mirror_columns(b));
            // counter-clockwise: transpose, then mirror rows
            case Symmetry::ROTATE_270: return mirror_rows(transpose_a1h8(b));
            default: return b;
        }
    }

    // A board isomorphic to `board` under `symmetry`. The side to move and
    // pass/game-over state carry over unchanged, since symmetries preserve
    // legal moves.
    //   FLIP_HORIZONTAL:  mirror left-right (a <-> h)
    //   FLIP_VERTICAL:    mirror top-bottom (1 <-> 8)
    //   FLIP_DIAGONAL_A1: reflect across the a1-h8 diagonal
    //   FLIP_DIAGONAL_H1: reflect across the h1-a8 diagonal
    //   ROTATE_*:         rotate clockwise by the given angle
    static Board isomorphism(const Board& board, Symmetry symmetry) {
        Board result = board;
        result.black_ = transform(board.black_, symmetry);
        result.white_ = transform(board.white_, symmetry);
        return result;
    }

private:
    // col -> 7 - col: reverse the bits within each byte
    static constexpr uint64_t mirror_columns(uint64_t b) {
        constexpr uint64_t k1 = 0x5555555555555555ULL;
        constexpr uint64_t k2 = 0x3333333333333333ULL;
        constexpr uint64_t k4 = 0x0F0F0F0F0F0F0F0FULL;
        b = ((b >> 1) & k1) | ((b & k1) << 1);
        b = ((b >> 2) & k2) | ((b & k2) << 2);
        b = ((b >> 4) & k4) | ((b & k4) << 4);
        return b;
    }

    // row -> 7 - row: reverse the byte order
    static constexpr uint64_t mirror_rows(uint64_t b) {
        return std::byteswap(b);
    }

    // (row, col) -> (col, row), via delta swaps
    static constexpr uint64_t transpose_a1h8(uint64_t b) {
        constexpr uint64_t k1 = 0x5500550055005500ULL;
        constexpr uint64_t k2 = 0x3333000033330000ULL;
        constexpr uint64_t k4 = 0x0F0F0F0F00000000ULL;
        uint64_t t = k4 & (b ^ (b << 28));
        b ^= t ^ (t >> 28);
        t = k2 & (b ^ (b << 14));
        b ^= t ^ (t >> 14);
        t = k1 & (b ^ (b << 7));
        b ^= t ^ (t >> 7);
        return b;
    }

    // (row, col) -> (7 - col, 7 - row), via delta swaps
    static constexpr uint64_t transpose_h1a8(uint64_t b) {
        constexpr uint64_t k1 = 0xAA00AA00AA00AA00ULL;
        constexpr uint64_t k2 = 0xCCCC0000CCCC0000ULL;
        constexpr uint64_t k4 = 0xF0F0F0F00F0F0F0FULL;
        uint64_t t = b ^ (b << 36);
        b ^= k4 & (t ^ (b >> 36));
        t = k2 & (b ^ (b << 18));
        b ^= t ^ (t >> 18);
        t = k1 & (b ^ (b << 9));
        b ^= t ^ (t >> 9);
        return b;
    }
};

} // namespace othello::board

template <>
struct std::hash<othello::board::Board> {
    std::size_t operator()(const othello::board::Board& board) const noexcept {
        return board.hash();
    }
};

#endif // OTHELLO_BOARD_HPP
