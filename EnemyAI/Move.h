#pragma once
#include <cstdint>

struct Move {
    uint16_t data;  // bits  0-7 : from coord (y<<4 | x)
                    // bits 8-15 : to   coord (y<<4 | x)

    static constexpr uint16_t make(uint8_t fromCoord, uint8_t toCoord) {
        return (uint16_t(toCoord) << 8) | fromCoord;
    }
    static constexpr uint8_t getFrom(uint16_t m) { return uint8_t(m); }
    static constexpr uint8_t getTo  (uint16_t m) { return uint8_t(m >> 8); }

    // Pack / unpack a single coordinate byte
    static constexpr uint8_t pack(uint8_t x, uint8_t y) { return (y << 4) | x; }
    static constexpr uint8_t getX(uint8_t coord)         { return coord & 0x0F; }
    static constexpr uint8_t getY(uint8_t coord)         { return coord >> 4; }

    // Convert 12x12 matrix index to packed coordinate byte — call only once on final move output
    static constexpr uint8_t fromMatSq(int matSq) {
        return pack((uint8_t)(matSq % 12 - 2), (uint8_t)(matSq / 12 - 2));
    }
};

// 218 = theoretical maximum number of legal chess moves in any position —
// no bounds check ever needed.
constexpr int MAX_MOVES = 218;

struct MoveList {
    Move    moves[MAX_MOVES];
    uint8_t count = 0;
};
