#include <gtest/gtest.h>

#include <bit>
#include <stdexcept>
#include <string>

#include <othello/util/util.hpp>

using othello::util::move_to_string;
using othello::util::parse_move;

TEST(ParseMove, CornersMapToExpectedBits) {
    EXPECT_EQ(parse_move("a1"), 1ULL << 63);
    EXPECT_EQ(parse_move("h1"), 1ULL << 56);
    EXPECT_EQ(parse_move("a8"), 1ULL << 7);
    EXPECT_EQ(parse_move("h8"), 1ULL << 0);
}

TEST(ParseMove, AdjacentSquares) {
    // b1 is the second most significant bit
    EXPECT_EQ(parse_move("b1"), 1ULL << 62);
    // a2 is directly below a1, one row (8 bits) further down
    EXPECT_EQ(parse_move("a2"), 1ULL << 55);
}

TEST(ParseMove, EverySquareIsASingleDistinctBit) {
    uint64_t seen = 0;
    for (char col = 'a'; col <= 'h'; ++col) {
        for (char row = '1'; row <= '8'; ++row) {
            const std::string name{col, row};
            const uint64_t bit = parse_move(name);
            EXPECT_TRUE(std::has_single_bit(bit)) << name;
            EXPECT_EQ(seen & bit, 0u) << name << " collides with another square";
            seen |= bit;
        }
    }
    EXPECT_EQ(seen, ~0ULL);
}

TEST(ParseMove, RejectsWrongLength) {
    EXPECT_THROW(parse_move(""), std::invalid_argument);
    EXPECT_THROW(parse_move("a"), std::invalid_argument);
    EXPECT_THROW(parse_move("a10"), std::invalid_argument);
    EXPECT_THROW(parse_move("pass"), std::invalid_argument);
}

TEST(ParseMove, RejectsBadColumn) {
    EXPECT_THROW(parse_move("i1"), std::invalid_argument);
    EXPECT_THROW(parse_move("A1"), std::invalid_argument);
    EXPECT_THROW(parse_move("11"), std::invalid_argument);
}

TEST(ParseMove, RejectsBadRow) {
    EXPECT_THROW(parse_move("a0"), std::invalid_argument);
    EXPECT_THROW(parse_move("a9"), std::invalid_argument);
    EXPECT_THROW(parse_move("ab"), std::invalid_argument);
}

TEST(MoveToString, Corners) {
    EXPECT_EQ(move_to_string(1ULL << 63), "a1");
    EXPECT_EQ(move_to_string(1ULL << 56), "h1");
    EXPECT_EQ(move_to_string(1ULL << 7), "a8");
    EXPECT_EQ(move_to_string(1ULL << 0), "h8");
}

TEST(MoveToString, RoundTripsWithParseMove) {
    for (char col = 'a'; col <= 'h'; ++col) {
        for (char row = '1'; row <= '8'; ++row) {
            const std::string name{col, row};
            EXPECT_EQ(move_to_string(parse_move(name)), name);
        }
    }
}

TEST(Color, BlackAndWhiteAreDistinct) {
    EXPECT_NE(othello::Color::BLACK, othello::Color::WHITE);
}
