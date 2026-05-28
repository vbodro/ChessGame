#pragma once
#include <cstdint>
#include <array>

// -----------------------------------------------------------------------
// Bit encoding  (1 byte per square)
//
//   bit 0 (0x01) — sentinel: square is off the board (border padding)
//   bit 1 (0x02) — white piece
//   bit 2 (0x04) — black piece
//   bits 3-6     — piece type, encoded as its capture score:
//                    pawn=1, bishop=4, knight=5, rook=6, king=9, queen=10
//                  reading: score = piece >> 3
//                  king score (9) is a placeholder; the king cannot be captured
//   bit 7        — unused
//
// Examples:
//   white queen  = WHITE | (10 << 3) = 0b01010010 = 82
//   black pawn   = BLACK | ( 1 << 3) = 0b00001100 = 12
// -----------------------------------------------------------------------

constexpr uint8_t EMPTY    = 0x00;
constexpr uint8_t SENTINEL = 0x01;
constexpr uint8_t WHITE    = 0x02;
constexpr uint8_t BLACK    = 0x04;

// Bits 3-6 = piece type = capture score (piece >> 3 gives score directly)
constexpr uint8_t TYPE_PAWN   =  1 << 3;  //  8
constexpr uint8_t TYPE_BISHOP =  4 << 3;  // 32
constexpr uint8_t TYPE_KNIGHT =  5 << 3;  // 40
constexpr uint8_t TYPE_ROOK   =  6 << 3;  // 48
constexpr uint8_t TYPE_KING   =  9 << 3;  // 72  (placeholder — king cannot be captured)
constexpr uint8_t TYPE_QUEEN  = 10 << 3;  // 80

constexpr uint8_t W_PAWN   = WHITE | TYPE_PAWN;
constexpr uint8_t W_KNIGHT = WHITE | TYPE_KNIGHT;
constexpr uint8_t W_BISHOP = WHITE | TYPE_BISHOP;
constexpr uint8_t W_ROOK   = WHITE | TYPE_ROOK;
constexpr uint8_t W_QUEEN  = WHITE | TYPE_QUEEN;
constexpr uint8_t W_KING   = WHITE | TYPE_KING;

constexpr uint8_t B_PAWN   = BLACK | TYPE_PAWN;
constexpr uint8_t B_KNIGHT = BLACK | TYPE_KNIGHT;
constexpr uint8_t B_BISHOP = BLACK | TYPE_BISHOP;
constexpr uint8_t B_ROOK   = BLACK | TYPE_ROOK;
constexpr uint8_t B_QUEEN  = BLACK | TYPE_QUEEN;
constexpr uint8_t B_KING   = BLACK | TYPE_KING;

// Helper queries
constexpr bool    isSentinel(uint8_t p) { return p & SENTINEL; }
constexpr bool    isEmpty   (uint8_t p) { return p == EMPTY; }
constexpr bool    isWhite   (uint8_t p) { return p & WHITE; }
constexpr bool    isBlack   (uint8_t p) { return p & BLACK; }
constexpr bool    isPiece   (uint8_t p) { return p & (WHITE | BLACK); }
constexpr uint8_t typeOf    (uint8_t p) { return p & 0x78; }  // bits 3-6 only (bit 7 unused)
constexpr uint16_t scoreOf   (uint8_t p) { return (p >> 3) * 100; }    // capture score, same bits as type

// -----------------------------------------------------------------------
// 12x12 board
// Real squares: rows 2-9, cols 2-9
// Square index: (boardRow + 2) * 12 + (boardCol + 2)
// boardRow 0 = black back rank, boardRow 7 = white back rank
// -----------------------------------------------------------------------

using BoardMatrix = std::array<uint8_t, 144>;

// Lookup table: matrix index → bit position in 64-bit attack mask
// Eliminates division by 12 from the hot path; 144 B, stays in L1 cache permanently
inline constexpr auto buildMatToBitTable() {
    std::array<uint8_t, 144> t{};
    for (int i = 0; i < 144; ++i)
        t[i] = (uint8_t)((i / 12 - 2) * 8 + (i % 12 - 2));
    return t;
}
inline constexpr auto MAT_TO_BIT = buildMatToBitTable();

#define S SENTINEL
#define __ EMPTY

constexpr BoardMatrix INITIAL_BOARD = {{
    S, S, S,      S,      S,      S,      S,      S,      S,      S,  S, S,
    S, S, S,      S,      S,      S,      S,      S,      S,      S,  S, S,
    S, S, B_ROOK, B_KNIGHT,B_BISHOP,B_QUEEN,B_KING,B_BISHOP,B_KNIGHT,B_ROOK, S, S,
    S, S, B_PAWN, B_PAWN, B_PAWN, B_PAWN, B_PAWN, B_PAWN, B_PAWN, B_PAWN, S, S,
    S, S, __,     __,     __,     __,     __,     __,     __,     __,  S, S,
    S, S, __,     __,     __,     __,     __,     __,     __,     __,  S, S,
    S, S, __,     __,     __,     __,     __,     __,     __,     __,  S, S,
    S, S, __,     __,     __,     __,     __,     __,     __,     __,  S, S,
    S, S, W_PAWN, W_PAWN, W_PAWN, W_PAWN, W_PAWN, W_PAWN, W_PAWN, W_PAWN, S, S,
    S, S, W_ROOK, W_KNIGHT,W_BISHOP,W_QUEEN,W_KING,W_BISHOP,W_KNIGHT,W_ROOK, S, S,
    S, S, S,      S,      S,      S,      S,      S,      S,      S,  S, S,
    S, S, S,      S,      S,      S,      S,      S,      S,      S,  S, S,
}};

#undef S
#undef __
