#include <gtest/gtest.h>

#include <array>
#include <bit>
#include <cstdint>
#include <random>
#include <sstream>
#include <string>
#include <utility>

#include "board/board.hpp"
#include "test_helpers.hpp"
#include "util/util.hpp"

using othello::Color;
using othello::board::Board;
using othello::test::play;
using othello::test::squares;
using othello::test::WIPEOUT_GAME;
using othello::util::parse_move;

namespace {

// A naive row/column model of the board, built by parsing Board::to_string().
// Used as an independent reference for the bitboard move generation.
struct Grid {
    std::array<std::array<char, 8>, 8> cells{};

    static Grid from(const Board& board) {
        Grid grid;
        std::istringstream in(board.to_string());
        std::string line;
        std::getline(in, line); // column header
        for (int row = 0; row < 8; ++row) {
            std::getline(in, line);
            for (int col = 0; col < 8; ++col) {
                grid.cells[row][col] = line[2 + 2 * col];
            }
        }
        return grid;
    }

    static uint64_t bit(int row, int col) {
        return 1ULL << (63 - (row * 8 + col));
    }

    static bool on_board(int row, int col) {
        return row >= 0 && row < 8 && col >= 0 && col < 8;
    }

    // Discs flipped if `color` played at (row, col).
    uint64_t captures(int row, int col, Color color) const {
        if (cells[row][col] != '.') {
            return 0;
        }
        const char own = color == Color::BLACK ? 'B' : 'W';
        const char opp = color == Color::BLACK ? 'W' : 'B';
        uint64_t flipped = 0;
        for (int dr = -1; dr <= 1; ++dr) {
            for (int dc = -1; dc <= 1; ++dc) {
                if (dr == 0 && dc == 0) {
                    continue;
                }
                uint64_t line = 0;
                int r = row + dr;
                int c = col + dc;
                while (on_board(r, c) && cells[r][c] == opp) {
                    line |= bit(r, c);
                    r += dr;
                    c += dc;
                }
                if (on_board(r, c) && cells[r][c] == own) {
                    flipped |= line;
                }
            }
        }
        return flipped;
    }

    uint64_t legal_moves(Color color) const {
        uint64_t moves = 0;
        for (int row = 0; row < 8; ++row) {
            for (int col = 0; col < 8; ++col) {
                if (captures(row, col, color) != 0) {
                    moves |= bit(row, col);
                }
            }
        }
        return moves;
    }

    uint64_t discs(char c) const {
        uint64_t result = 0;
        for (int row = 0; row < 8; ++row) {
            for (int col = 0; col < 8; ++col) {
                if (cells[row][col] == c) {
                    result |= bit(row, col);
                }
            }
        }
        return result;
    }
};

// Picks a uniformly random set bit.
uint64_t random_bit(uint64_t bits, std::mt19937& rng) {
    std::uniform_int_distribution<int> dist(0, std::popcount(bits) - 1);
    for (int k = dist(rng); k > 0; --k) {
        bits &= bits - 1;
    }
    return bits & -bits;
}

} // namespace

TEST(Board, StartingPosition) {
    Board board;
    EXPECT_EQ(board.turn(), Color::BLACK);
    EXPECT_FALSE(board.game_over());
    EXPECT_FALSE(board.last_move_caused_pass());
    EXPECT_EQ(board.black_count(), 2);
    EXPECT_EQ(board.white_count(), 2);
    EXPECT_EQ(board.score(), std::make_pair(2, 2));

    const Grid grid = Grid::from(board);
    EXPECT_EQ(grid.discs('B'), squares({"d5", "e4"}));
    EXPECT_EQ(grid.discs('W'), squares({"d4", "e5"}));
}

TEST(Board, StartingToString) {
    Board board;
    EXPECT_EQ(
        board.to_string(),
        "  a b c d e f g h\n"
        "1 . . . . . . . .\n"
        "2 . . . . . . . .\n"
        "3 . . . . . . . .\n"
        "4 . . . W B . . .\n"
        "5 . . . B W . . .\n"
        "6 . . . . . . . .\n"
        "7 . . . . . . . .\n"
        "8 . . . . . . . .\n"
    );
}

TEST(Board, StartingLegalMoves) {
    Board board;
    EXPECT_EQ(board.legal_moves(), squares({"d3", "c4", "f5", "e6"}));
}

TEST(Board, Opponent) {
    EXPECT_EQ(Board::opponent(Color::BLACK), Color::WHITE);
    EXPECT_EQ(Board::opponent(Color::WHITE), Color::BLACK);
}

TEST(Board, MoveValidAcceptsLegalMoves) {
    Board board;
    for (auto name : {"d3", "c4", "f5", "e6"}) {
        EXPECT_TRUE(board.move_valid(parse_move(name))) << name;
    }
}

TEST(Board, MoveValidRejectsOccupiedSquare) {
    Board board;
    for (auto name : {"d4", "d5", "e4", "e5"}) {
        EXPECT_FALSE(board.move_valid(parse_move(name))) << name;
    }
}

TEST(Board, MoveValidRejectsNonCapturingSquare) {
    Board board;
    // far from any disc
    EXPECT_FALSE(board.move_valid(parse_move("a1")));
    EXPECT_FALSE(board.move_valid(parse_move("h8")));
    // touching discs, but not bracketing any white disc
    EXPECT_FALSE(board.move_valid(parse_move("c3")));
    EXPECT_FALSE(board.move_valid(parse_move("f3")));
}

TEST(Board, MoveValidRejectsMalformedBitboards) {
    Board board;
    EXPECT_FALSE(board.move_valid(0));
    // two individually legal squares at once
    EXPECT_FALSE(board.move_valid(squares({"d3", "c4"})));
    EXPECT_FALSE(board.move_valid(~0ULL));
}

TEST(Board, CapturesOnOpeningMoves) {
    Board board;
    EXPECT_EQ(board.captures(parse_move("d3")), parse_move("d4"));
    EXPECT_EQ(board.captures(parse_move("c4")), parse_move("d4"));
    EXPECT_EQ(board.captures(parse_move("f5")), parse_move("e5"));
    EXPECT_EQ(board.captures(parse_move("e6")), parse_move("e5"));
    EXPECT_EQ(board.captures(parse_move("a1")), 0u);
}

TEST(Board, MakeMoveFlipsAndSwitchesTurn) {
    Board board;
    board.make_move(parse_move("f5"));

    EXPECT_EQ(board.turn(), Color::WHITE);
    EXPECT_FALSE(board.last_move_caused_pass());
    EXPECT_EQ(board.score(), std::make_pair(4, 1));

    const Grid grid = Grid::from(board);
    EXPECT_EQ(grid.discs('B'), squares({"d5", "e4", "e5", "f5"}));
    EXPECT_EQ(grid.discs('W'), squares({"d4"}));

    // white's replies: perpendicular, diagonal and parallel openings
    EXPECT_EQ(board.legal_moves(), squares({"d6", "f4", "f6"}));
}

TEST(Board, WipeoutEndsGame) {
    Board board;
    ASSERT_TRUE(play(board, WIPEOUT_GAME));
    EXPECT_TRUE(board.game_over());
    EXPECT_FALSE(board.last_move_caused_pass());
    EXPECT_EQ(board.score(), std::make_pair(13, 0));
    EXPECT_EQ(board.legal_moves(), 0u);
}

TEST(Board, GameIsNotOverBeforeFinalMove) {
    Board board;
    ASSERT_TRUE(play(board, {"e6", "f4", "e3", "f6", "g5", "d6", "e7", "f5"}));
    EXPECT_FALSE(board.game_over());
    EXPECT_EQ(board.turn(), Color::BLACK);
    EXPECT_TRUE(board.move_valid(parse_move("c5")));
}

// Plays many random games and checks every position against the naive
// reference model: legal moves, captures, disc counts, turn order, passes.
// The reference walks rows/columns directly, so it catches bitboard shifts
// that wrap around an edge of the board.
TEST(Board, RandomGamesMatchReferenceModel) {
    std::mt19937 rng(12345);
    int passes_seen = 0;

    for (int game = 0; game < 200; ++game) {
        Board board;
        int moves_played = 0;
        while (!board.game_over()) {
            const Grid grid = Grid::from(board);
            const Color mover = board.turn();

            const uint64_t legal = board.legal_moves();
            ASSERT_EQ(legal, grid.legal_moves(mover)) << board.to_string();
            ASSERT_NE(legal, 0u) << board.to_string();

            for (uint64_t rest = legal; rest; rest &= rest - 1) {
                ASSERT_TRUE(board.move_valid(rest & -rest));
            }
            for (uint64_t rest = ~legal; rest; rest &= rest - 1) {
                ASSERT_FALSE(board.move_valid(rest & -rest));
            }

            const uint64_t move = random_bit(legal, rng);
            const int idx = 63 - std::countr_zero(move);
            const uint64_t flips = board.captures(move);
            ASSERT_EQ(flips, grid.captures(idx / 8, idx % 8, mover)) << board.to_string();

            const auto [black_before, white_before] = board.score();
            board.make_move(move);
            ++moves_played;

            const int gained = std::popcount(flips);
            if (mover == Color::BLACK) {
                EXPECT_EQ(board.black_count(), black_before + gained + 1);
                EXPECT_EQ(board.white_count(), white_before - gained);
            } else {
                EXPECT_EQ(board.white_count(), white_before + gained + 1);
                EXPECT_EQ(board.black_count(), black_before - gained);
            }

            const Grid next = Grid::from(board);
            if (board.game_over()) {
                EXPECT_FALSE(board.last_move_caused_pass());
                EXPECT_EQ(next.legal_moves(Color::BLACK), 0u);
                EXPECT_EQ(next.legal_moves(Color::WHITE), 0u);
            } else if (board.last_move_caused_pass()) {
                ++passes_seen;
                EXPECT_EQ(board.turn(), mover);
                EXPECT_EQ(next.legal_moves(Board::opponent(mover)), 0u);
            } else {
                EXPECT_EQ(board.turn(), Board::opponent(mover));
            }
        }
        EXPECT_LE(board.black_count() + board.white_count(), 64);
        EXPECT_LE(moves_played, 60);
    }
    // make sure the pass branch was actually exercised
    EXPECT_GT(passes_seen, 0);
}
