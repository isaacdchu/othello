#include <gtest/gtest.h>

#include <array>
#include <bit>
#include <cstdint>
#include <random>
#include <sstream>
#include <string>
#include <unordered_set>
#include <utility>

#include <othello/board/board.hpp>
#include "test_helpers.hpp"
#include <othello/util/util.hpp>

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

TEST(Board, EqualBoardsHashEqually) {
    Board a;
    Board b;
    EXPECT_EQ(a, b);
    EXPECT_EQ(a.hash(), b.hash());
    EXPECT_EQ(std::hash<Board>{}(a), a.hash());

    // the same position reached through two different move orders
    ASSERT_TRUE(play(a, {"e6", "f6", "f5", "d6"}));
    ASSERT_TRUE(play(b, {"f5", "f6", "e6", "d6"}));
    ASSERT_EQ(a, b);
    EXPECT_EQ(a.hash(), b.hash());
}

TEST(Board, DifferentPositionsHashDifferently) {
    std::unordered_set<Board> seen;
    for (auto opening : {"d3", "c4", "f5", "e6"}) {
        Board board;
        board.make_move(parse_move(opening));
        seen.insert(board);
    }
    seen.insert(Board{});
    EXPECT_EQ(seen.size(), 5u);
    EXPECT_TRUE(seen.contains(Board{}));
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

TEST(BoardFromState, StartingBitboardsMatchDefault) {
    Board board(othello::board::START_BLACK, othello::board::START_WHITE, Color::BLACK);
    EXPECT_EQ(board, Board());
    EXPECT_EQ(board.hash(), Board().hash());
    EXPECT_FALSE(board.game_over());
    EXPECT_FALSE(board.last_move_caused_pass());
    EXPECT_EQ(board.legal_moves(), squares({"d3", "c4", "f5", "e6"}));
}

TEST(BoardFromState, KeepsGivenTurn) {
    Board board(othello::board::START_BLACK, othello::board::START_WHITE, Color::WHITE);
    EXPECT_EQ(board.turn(), Color::WHITE);
    EXPECT_NE(board, Board());
    EXPECT_FALSE(board.game_over());
    EXPECT_FALSE(board.last_move_caused_pass());
    EXPECT_EQ(board.legal_moves(), squares({"e3", "f4", "c5", "d6"}));
}

TEST(BoardFromState, ExposesGivenDiscs) {
    const uint64_t black = squares({"a1", "c3", "h8"});
    const uint64_t white = squares({"b2", "g7"});
    Board board(black, white, Color::BLACK);
    EXPECT_EQ(board.black(), black);
    EXPECT_EQ(board.white(), white);
    EXPECT_EQ(board.score(), std::make_pair(3, 2));

    const Grid grid = Grid::from(board);
    EXPECT_EQ(grid.discs('B'), black);
    EXPECT_EQ(grid.discs('W'), white);
}

// Rebuilding a mid-game board from its own state gives the same position.
TEST(BoardFromState, RoundTripsMidGamePosition) {
    Board played;
    ASSERT_TRUE(play(played, {"e6", "f4", "e3", "f6"}));

    Board rebuilt(played.black(), played.white(), played.turn());
    EXPECT_EQ(rebuilt, played);
    EXPECT_EQ(rebuilt.hash(), played.hash());
    EXPECT_EQ(rebuilt.legal_moves(), played.legal_moves());
    EXPECT_EQ(rebuilt.game_over(), played.game_over());
    EXPECT_EQ(rebuilt.to_string(), played.to_string());
}

// Black is to move but cannot, so the turn passes to white.
TEST(BoardFromState, SideWithNoMovesPasses) {
    // black b1 cannot bracket white's corner disc; white can play c1
    Board board(squares({"b1"}), squares({"a1"}), Color::BLACK);
    EXPECT_EQ(board.turn(), Color::WHITE);
    EXPECT_TRUE(board.last_move_caused_pass());
    EXPECT_FALSE(board.game_over());
    EXPECT_EQ(board.legal_moves(), squares({"c1"}));

    // play continues normally from there, and ends the game
    board.make_move(parse_move("c1"));
    EXPECT_TRUE(board.game_over());
    EXPECT_EQ(board.score(), std::make_pair(0, 3));
}

TEST(BoardFromState, NoMovesForEitherSideIsGameOver) {
    for (Color turn : {Color::BLACK, Color::WHITE}) {
        Board only_black(squares({"d4", "e5"}), 0, turn);
        EXPECT_TRUE(only_black.game_over());
        EXPECT_FALSE(only_black.last_move_caused_pass());
        EXPECT_EQ(only_black.legal_moves(), 0u);

        Board empty(0, 0, turn);
        EXPECT_TRUE(empty.game_over());

        // discs that can't reach each other
        Board apart(squares({"a1"}), squares({"h8"}), turn);
        EXPECT_TRUE(apart.game_over());
        EXPECT_EQ(apart.score(), std::make_pair(1, 1));
    }
}

TEST(BoardFromState, FullBoardIsGameOver) {
    const uint64_t black = 0xFFFFFFFF00000000ULL;
    Board board(black, ~black, Color::BLACK);
    EXPECT_TRUE(board.game_over());
    EXPECT_EQ(board.score(), std::make_pair(32, 32));
}

namespace {

using Symmetry = Board::Symmetry;

constexpr std::array<Symmetry, 7> ALL_SYMMETRIES = {
    Symmetry::FLIP_HORIZONTAL, Symmetry::FLIP_VERTICAL,
    Symmetry::FLIP_DIAGONAL_A1, Symmetry::FLIP_DIAGONAL_H1,
    Symmetry::ROTATE_90, Symmetry::ROTATE_180, Symmetry::ROTATE_270,
};

// Where (row, col) lands under each symmetry, square by square.
std::pair<int, int> map_square(int row, int col, Symmetry symmetry) {
    switch (symmetry) {
        case Symmetry::FLIP_HORIZONTAL: return {row, 7 - col};
        case Symmetry::FLIP_VERTICAL: return {7 - row, col};
        case Symmetry::FLIP_DIAGONAL_A1: return {col, row};
        case Symmetry::FLIP_DIAGONAL_H1: return {7 - col, 7 - row};
        case Symmetry::ROTATE_90: return {col, 7 - row};
        case Symmetry::ROTATE_180: return {7 - row, 7 - col};
        case Symmetry::ROTATE_270: return {7 - col, row};
    }
    return {row, col};
}

uint64_t naive_transform(uint64_t b, Symmetry symmetry) {
    uint64_t result = 0;
    for (int row = 0; row < 8; ++row) {
        for (int col = 0; col < 8; ++col) {
            if (b & Grid::bit(row, col)) {
                auto [r, c] = map_square(row, col, symmetry);
                result |= Grid::bit(r, c);
            }
        }
    }
    return result;
}

} // namespace

TEST(BoardIsomorphism, MatchesSquareBySquareReference) {
    std::mt19937_64 rng(12345);
    for (int i = 0; i < 1000; ++i) {
        const uint64_t black = rng();
        const uint64_t white = rng() & ~black;
        Board board(black, white, Color::BLACK);
        for (Symmetry s : ALL_SYMMETRIES) {
            Board iso = Board::isomorphism(board, s);
            EXPECT_EQ(iso.black(), naive_transform(board.black(), s));
            EXPECT_EQ(iso.white(), naive_transform(board.white(), s));
        }
    }
}

TEST(BoardIsomorphism, MapsKnownSquares) {
    Board board(squares({"b1"}), squares({"a3"}), Color::WHITE);
    Board h = Board::isomorphism(board, Symmetry::FLIP_HORIZONTAL);
    EXPECT_EQ(h.black(), squares({"g1"}));
    EXPECT_EQ(h.white(), squares({"h3"}));
    Board r90 = Board::isomorphism(board, Symmetry::ROTATE_90);
    EXPECT_EQ(r90.black(), squares({"h2"}));
    EXPECT_EQ(r90.white(), squares({"f1"}));
}

TEST(BoardIsomorphism, StartingPositionSymmetries) {
    const Board start;
    // the opening is fixed by both diagonal reflections and the half turn
    EXPECT_EQ(Board::isomorphism(start, Symmetry::FLIP_DIAGONAL_A1), start);
    EXPECT_EQ(Board::isomorphism(start, Symmetry::FLIP_DIAGONAL_H1), start);
    EXPECT_EQ(Board::isomorphism(start, Symmetry::ROTATE_180), start);
    // the others swap the colours
    for (Symmetry s : {Symmetry::FLIP_HORIZONTAL, Symmetry::FLIP_VERTICAL,
                       Symmetry::ROTATE_90, Symmetry::ROTATE_270}) {
        Board iso = Board::isomorphism(start, s);
        EXPECT_EQ(iso.black(), start.white());
        EXPECT_EQ(iso.white(), start.black());
    }
}

TEST(BoardIsomorphism, PreservesTurnAndCommutesWithMoves) {
    std::mt19937_64 rng(777);
    for (int game = 0; game < 50; ++game) {
        Board board;
        while (!board.game_over()) {
            for (Symmetry s : ALL_SYMMETRIES) {
                Board iso = Board::isomorphism(board, s);
                EXPECT_EQ(iso.turn(), board.turn());
                EXPECT_EQ(iso.game_over(), board.game_over());
                EXPECT_EQ(iso.legal_moves(), naive_transform(board.legal_moves(), s));
            }
            // pick a random legal move
            uint64_t moves = board.legal_moves();
            int skip = static_cast<int>(rng() % std::popcount(moves));
            for (int k = 0; k < skip; ++k) {
                moves &= moves - 1;
            }
            const uint64_t move = moves & -moves;
            for (Symmetry s : ALL_SYMMETRIES) {
                Board iso = Board::isomorphism(board, s);
                iso.make_move(naive_transform(move, s));
                Board next = board;
                next.make_move(move);
                EXPECT_EQ(iso, Board::isomorphism(next, s));
            }
            board.make_move(move);
        }
    }
}
