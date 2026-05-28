#pragma once
#include <array>

enum class PieceType  { None, King, Queen, Rook, Bishop, Knight, Pawn };
enum class PieceColor { None, White, Black };

struct Piece {
    PieceType  type  = PieceType::None;
    PieceColor color = PieceColor::None;
};

using Board = std::array<std::array<Piece, 8>, 8>;

struct CastlingRights {
    bool whiteKingSide  = true;
    bool whiteQueenSide = true;
    bool blackKingSide  = true;
    bool blackQueenSide = true;
};

Board makeStartingBoard();

// Temporary test position: only kings and pawns on their starting squares.
// Useful for testing promotion and king-and-pawn endgame strategies.
Board makeTestBoard();
