#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <regex>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <othello/board/board.hpp>
#include <othello/game/game.hpp>
#include <othello/player/player.hpp>
#include <othello/player/random_player.hpp>
#include "test_helpers.hpp"
#include <othello/util/util.hpp>

using othello::Color;
using othello::game::Game;
using othello::player::Player;
using othello::player::RandomPlayer;

namespace {

// Plays a fixed list of moves, in order. Records how many times it was asked.
class ScriptedPlayer : public Player {
public:
    explicit ScriptedPlayer(std::vector<std::string> moves) : moves_(std::move(moves)) {}

    uint64_t get_move(const othello::board::Board& board) override {
        EXPECT_EQ(board.turn(), color()) << "asked to move out of turn";
        if (next_ >= moves_.size()) {
            ADD_FAILURE() << "script exhausted";
            // fall back to any legal move so the game can still finish
            const uint64_t legal = board.legal_moves();
            return legal & -legal;
        }
        return othello::util::parse_move(moves_[next_++]);
    }

    std::size_t calls() const { return next_; }

private:
    std::vector<std::string> moves_;
    std::size_t next_ = 0;
};

// Always plays the lowest-bit legal move (deterministic, never invalid).
class FirstMovePlayer : public Player {
public:
    uint64_t get_move(const othello::board::Board& board) override {
        const uint64_t legal = board.legal_moves();
        return legal & -legal;
    }
};

std::string run_and_capture(Game& game) {
    testing::internal::CaptureStdout();
    game.run();
    return testing::internal::GetCapturedStdout();
}

bool contains(const std::string& haystack, const std::string& needle) {
    return haystack.find(needle) != std::string::npos;
}

} // namespace

TEST(Game, ConstructorAssignsColors) {
    auto black = std::make_unique<FirstMovePlayer>();
    auto white = std::make_unique<FirstMovePlayer>();
    Player* black_ptr = black.get();
    Player* white_ptr = white.get();

    Game game(std::move(black), std::move(white));

    EXPECT_EQ(black_ptr->color(), Color::BLACK);
    EXPECT_EQ(white_ptr->color(), Color::WHITE);
}

TEST(Game, ScriptedWipeoutBlackWins) {
    auto black = std::make_unique<ScriptedPlayer>(
        std::vector<std::string>{"e6", "e3", "g5", "e7", "c5"});
    auto white = std::make_unique<ScriptedPlayer>(
        std::vector<std::string>{"f4", "f6", "d6", "f5"});
    ScriptedPlayer* black_ptr = black.get();
    ScriptedPlayer* white_ptr = white.get();

    Game game(std::move(black), std::move(white));
    const std::string out = run_and_capture(game);

    EXPECT_EQ(black_ptr->calls(), 5u);
    EXPECT_EQ(white_ptr->calls(), 4u);
    EXPECT_TRUE(contains(out, "Game over!")) << out;
    EXPECT_TRUE(contains(out, "Black wins!")) << out;
    EXPECT_TRUE(contains(out, "Final score: Black 13 - White 0")) << out;
    EXPECT_FALSE(contains(out, "Invalid move")) << out;
}

TEST(Game, InvalidMovesAreRejectedAndRetried) {
    // black tries an occupied square and a non-capturing square first
    auto black = std::make_unique<ScriptedPlayer>(
        std::vector<std::string>{"d4", "a1", "e6", "e3", "g5", "e7", "c5"});
    // white tries a square that's legal for black but not for white
    auto white = std::make_unique<ScriptedPlayer>(
        std::vector<std::string>{"c4", "f4", "f6", "d6", "f5"});
    ScriptedPlayer* black_ptr = black.get();
    ScriptedPlayer* white_ptr = white.get();

    Game game(std::move(black), std::move(white));
    const std::string out = run_and_capture(game);

    EXPECT_EQ(black_ptr->calls(), 7u);
    EXPECT_EQ(white_ptr->calls(), 5u);

    std::size_t invalid_count = 0;
    for (std::size_t pos = out.find("Invalid move"); pos != std::string::npos;
         pos = out.find("Invalid move", pos + 1)) {
        ++invalid_count;
    }
    EXPECT_EQ(invalid_count, 3u) << out;
    EXPECT_TRUE(contains(out, "Final score: Black 13 - White 0")) << out;
}

TEST(Game, PrintsTurnAndBoardEachMove) {
    auto black = std::make_unique<ScriptedPlayer>(
        std::vector<std::string>{"e6", "e3", "g5", "e7", "c5"});
    auto white = std::make_unique<ScriptedPlayer>(
        std::vector<std::string>{"f4", "f6", "d6", "f5"});

    Game game(std::move(black), std::move(white));
    const std::string out = run_and_capture(game);

    // the starting board is shown before black's first move
    EXPECT_TRUE(contains(out, "Current turn: Black\n" + othello::board::Board().to_string()))
        << out;
    EXPECT_TRUE(contains(out, "Current turn: White")) << out;
}

// Whatever the outcome, the announced winner must agree with the final score.
TEST(Game, WinnerMatchesFinalScore) {
    std::vector<std::pair<std::unique_ptr<Player>, std::unique_ptr<Player>>> matchups;
    matchups.emplace_back(std::make_unique<RandomPlayer>(), std::make_unique<RandomPlayer>());
    matchups.emplace_back(std::make_unique<FirstMovePlayer>(), std::make_unique<RandomPlayer>());
    matchups.emplace_back(std::make_unique<RandomPlayer>(), std::make_unique<FirstMovePlayer>());
    matchups.emplace_back(std::make_unique<FirstMovePlayer>(), std::make_unique<FirstMovePlayer>());

    const std::regex score_re(R"(Final score: Black (\d+) - White (\d+))");
    for (auto& [black, white] : matchups) {
        Game game(std::move(black), std::move(white));
        const std::string out = run_and_capture(game);

        EXPECT_TRUE(contains(out, "Game over!")) << out;
        EXPECT_FALSE(contains(out, "Invalid move")) << out;

        std::smatch m;
        ASSERT_TRUE(std::regex_search(out, m, score_re)) << out;
        const int b = std::stoi(m[1]);
        const int w = std::stoi(m[2]);
        EXPECT_LE(b + w, 64);
        if (b > w) {
            EXPECT_TRUE(contains(out, "Black wins!")) << out;
        } else if (w > b) {
            EXPECT_TRUE(contains(out, "White wins!")) << out;
        } else {
            EXPECT_TRUE(contains(out, "It's a draw!")) << out;
        }
    }
}

namespace {

std::unique_ptr<ScriptedPlayer> scripted(std::vector<std::string> moves) {
    return std::make_unique<ScriptedPlayer>(std::move(moves));
}

// Runs run_silent and also returns whatever it printed.
std::pair<int, std::string> run_silent_and_capture(Game& game) {
    testing::internal::CaptureStdout();
    int result = 0;
    try {
        result = game.run_silent();
    } catch (...) {
        testing::internal::GetCapturedStdout();
        throw;
    }
    return {result, testing::internal::GetCapturedStdout()};
}

} // namespace

TEST(GameRunSilent, BlackWinReturnsOneAndPrintsNothing) {
    auto black = scripted({"e6", "e3", "g5", "e7", "c5"});
    auto white = scripted({"f4", "f6", "d6", "f5"});
    ScriptedPlayer* black_ptr = black.get();
    ScriptedPlayer* white_ptr = white.get();

    Game game(std::move(black), std::move(white));
    const auto [result, out] = run_silent_and_capture(game);

    EXPECT_EQ(result, 1);
    EXPECT_EQ(out, "");
    EXPECT_EQ(black_ptr->calls(), 5u);
    EXPECT_EQ(white_ptr->calls(), 4u);
}

TEST(GameRunSilent, WhiteWinReturnsMinusOne) {
    // black has no move here, so white is to move and wins with c1
    othello::board::Board board(
        othello::test::squares({"b1"}), othello::test::squares({"a1"}), Color::BLACK);
    auto black = scripted({});
    auto white = scripted({"c1"});
    ScriptedPlayer* black_ptr = black.get();
    ScriptedPlayer* white_ptr = white.get();

    Game game(std::move(black), std::move(white), board);
    EXPECT_EQ(game.run_silent(), -1);
    EXPECT_EQ(black_ptr->calls(), 0u);
    EXPECT_EQ(white_ptr->calls(), 1u);
}

TEST(GameRunSilent, DrawReturnsZero) {
    // already over at 1-1: no one is asked to move
    othello::board::Board board(
        othello::test::squares({"a1"}), othello::test::squares({"h8"}), Color::BLACK);
    auto black = scripted({});
    auto white = scripted({});
    ScriptedPlayer* black_ptr = black.get();
    ScriptedPlayer* white_ptr = white.get();

    Game game(std::move(black), std::move(white), board);
    EXPECT_EQ(game.run_silent(), 0);
    EXPECT_EQ(black_ptr->calls(), 0u);
    EXPECT_EQ(white_ptr->calls(), 0u);
}

TEST(GameRunSilent, ContinuesFromGivenBoard) {
    othello::board::Board board;
    ASSERT_TRUE(othello::test::play(board, {"e6", "f4", "e3", "f6", "g5", "d6", "e7", "f5"}));
    auto black = scripted({"c5"});
    auto white = scripted({});
    ScriptedPlayer* black_ptr = black.get();

    Game game(std::move(black), std::move(white), board);
    EXPECT_EQ(game.run_silent(), 1);
    EXPECT_EQ(black_ptr->calls(), 1u);
}

TEST(GameRunSilent, InvalidBlackMoveThrows) {
    // d4 is occupied
    Game game(scripted({"d4"}), scripted({}));
    EXPECT_THROW(game.run_silent(), std::invalid_argument);
}

TEST(GameRunSilent, InvalidWhiteMoveThrows) {
    // c4 is legal for black, but not for white after f5
    Game game(scripted({"f5"}), scripted({"c4"}));
    EXPECT_THROW(game.run_silent(), std::invalid_argument);
}

// run_silent reports the same outcome that run prints.
TEST(GameRunSilent, AgreesWithRun) {
    Game loud(std::make_unique<FirstMovePlayer>(), std::make_unique<FirstMovePlayer>());
    const std::string out = run_and_capture(loud);

    Game quiet(std::make_unique<FirstMovePlayer>(), std::make_unique<FirstMovePlayer>());
    const int result = quiet.run_silent();

    if (contains(out, "Black wins!")) {
        EXPECT_EQ(result, 1) << out;
    } else if (contains(out, "White wins!")) {
        EXPECT_EQ(result, -1) << out;
    } else {
        ASSERT_TRUE(contains(out, "It's a draw!")) << out;
        EXPECT_EQ(result, 0) << out;
    }
}

TEST(GameRunSilent, RandomGamesFinishWithValidResult) {
    for (int i = 0; i < 50; ++i) {
        Game game(std::make_unique<RandomPlayer>(), std::make_unique<RandomPlayer>());
        const auto [result, out] = run_silent_and_capture(game);
        EXPECT_TRUE(result == 1 || result == -1 || result == 0) << result;
        EXPECT_EQ(out, "");
    }
}
