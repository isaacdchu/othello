#ifndef OTHELLO_UTIL_HPP
#define OTHELLO_UTIL_HPP

#include <bit>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <string_view>

namespace othello {

enum class Color {
    BLACK,
    WHITE
};

} // namespace othello

namespace othello::util {

inline uint64_t parse_move(std::string_view input) {
    // convert input like "c3" to a bitboard move
    if (input.size() != 2) {
        throw std::invalid_argument("[othello::util::parse_move] Invalid move format");
    }
    if (input[0] < 'a' || input[0] > 'h') {
        throw std::invalid_argument("[othello::util::parse_move] Invalid column");
    }
    if (input[1] < '1' || input[1] > '8') {
        throw std::invalid_argument("[othello::util::parse_move] Invalid row");
    }
    int col = input[0] - 'a';
    int row = input[1] - '1';
    return 1ULL << (63 - (row * 8 + col));
}

inline std::string move_to_string(uint64_t move) {
    int index = std::countr_zero(move);
    int row = (63 - index) / 8;
    int col = (63 - index) % 8;
    return std::string(
        1,
        static_cast<char>('a' + col)
    ) + std::string(1, static_cast<char>('1' + row));
}

// splitmix64 finalizer: spreads every input bit across the whole output.
// Useful as a building block for hashing bitboards.
constexpr uint64_t mix(uint64_t x) {
    x ^= x >> 30;
    x *= 0xBF58476D1CE4E5B9ULL;
    x ^= x >> 27;
    x *= 0x94D049BB133111EBULL;
    x ^= x >> 31;
    return x;
}

} // namespace othello::util

#endif // OTHELLO_UTIL_HPP
