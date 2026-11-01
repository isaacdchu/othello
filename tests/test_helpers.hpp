#ifndef OTHELLO_TEST_HELPERS_HPP
#define OTHELLO_TEST_HELPERS_HPP

#include <cstdint>
#include <initializer_list>
#include <string_view>

#include <othello/board/board.hpp>
#include <othello/util/util.hpp>

namespace othello::test {

// Bitboard with the given squares set, e.g. squares({"d4", "e5"}).
inline uint64_t squares(std::initializer_list<std::string_view> names) {
    uint64_t result = 0;
    for (auto name : names) {
        result |= othello::util::parse_move(name);
    }
    return result;
}

// Plays each move in order, asserting it is valid first.
// Returns false (without playing further) on the first invalid move.
inline bool play(othello::board::Board& board, std::initializer_list<std::string_view> moves) {
    for (auto name : moves) {
        uint64_t move = othello::util::parse_move(name);
        if (!board.move_valid(move)) {
            return false;
        }
        board.make_move(move);
    }
    return true;
}

// A 9-move game in which black captures every white disc (13-0).
inline constexpr std::initializer_list<std::string_view> WIPEOUT_GAME = {
    "e6", "f4", "e3", "f6", "g5", "d6", "e7", "f5", "c5"
};

} // namespace othello::test

#endif // OTHELLO_TEST_HELPERS_HPP
