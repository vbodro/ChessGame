#pragma once
#include "Position.h"
#include "../Board.h"

// Converts rendering piece to AI encoding
constexpr uint8_t toAIPiece(const Piece& p) {
    if (p.type == PieceType::None) return EMPTY;

    uint8_t color = (p.color == PieceColor::White) ? WHITE : BLACK;
    uint8_t type  = 0;

    switch (p.type) {
    case PieceType::Pawn:   type = TYPE_PAWN;   break;
    case PieceType::Knight: type = TYPE_KNIGHT; break;
    case PieceType::Bishop: type = TYPE_BISHOP; break;
    case PieceType::Rook:   type = TYPE_ROOK;   break;
    case PieceType::Queen:  type = TYPE_QUEEN;  break;
    case PieceType::King:   type = TYPE_KING;   break;
    default: break;
    }

    return color | type;
}

// -----------------------------------------------------------------------
// Piece list helpers  (swap-with-last, O(1) add/remove)
// -----------------------------------------------------------------------

// Returns the index of sq in the list, or -1 if not found
inline int findPiece(const uint8_t* pieces, uint8_t count, uint8_t sq) {
    for (int i = 0; i < count; ++i)
        if (pieces[i] == sq) return i;
    return -1;
}

// Removes element at index by swapping with last and decrementing count.
// Caller must save pieces[index] before calling if undo is needed.
// Undo: pieces[index] = savedValue; count++;
inline void removePiece(uint8_t* pieces, uint8_t& count, int index) {
    pieces[index] = pieces[--count];
}

// Appends sq to the list.
// Undo: count--;
inline void addPiece(uint8_t* pieces, uint8_t& count, uint8_t sq) {
    pieces[count++] = sq;
}

// -----------------------------------------------------------------------
// AIState
// -----------------------------------------------------------------------

struct alignas(64) AIState {
    BoardMatrix board           = INITIAL_BOARD;
    uint8_t     whitePieces[16] = {};
    uint8_t     blackPieces[16] = {};
    uint8_t     whiteCount      = 0;
    uint8_t     blackCount      = 0;

    static AIState fromBoard(const Board& renderBoard) {
        AIState state;
        state.whiteCount = 0;
        state.blackCount = 0;

        for (int row = 0; row < 8; ++row) {
            for (int col = 0; col < 8; ++col) {
                uint8_t p  = toAIPiece(renderBoard[row][col]);
                uint8_t sq = (uint8_t)((row + 2) * 12 + (col + 2));
                state.board[sq] = p;

                if      (isWhite(p)) addPiece(state.whitePieces, state.whiteCount, sq);
                else if (isBlack(p)) addPiece(state.blackPieces, state.blackCount, sq);
            }
        }
        return state;
    }
};
