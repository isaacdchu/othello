#ifndef UTIL_HPP
#define UTIL_HPP

#include <cstdint>
#include <stdexcept>
#include <string>

namespace othello::util {

uint64_t parse_move(const std::string& input) {
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

} // namespace othello::util

#endif // UTIL_HPP
