#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <regex>
#include <string>
#include <utility>
#include <vector>

#include "board/board.hpp"
#include "game/game.hpp"
#include "player/player.hpp"
#include "player/random_player.hpp"
#include "test_helpers.hpp"
#include "util/util.hpp"

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
