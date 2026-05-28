#include "Board.h"

Board makeStartingBoard() {
    Board board{};

    auto place = [&](int row, int col, PieceType type, PieceColor color) {
        board[row][col] = { type, color };
    };

    place(0, 0, PieceType::Rook,   PieceColor::Black);
    place(0, 1, PieceType::Knight, PieceColor::Black);
    place(0, 2, PieceType::Bishop, PieceColor::Black);
    place(0, 3, PieceType::Queen,  PieceColor::Black);
    place(0, 4, PieceType::King,   PieceColor::Black);
    place(0, 5, PieceType::Bishop, PieceColor::Black);
    place(0, 6, PieceType::Knight, PieceColor::Black);
    place(0, 7, PieceType::Rook,   PieceColor::Black);

    for (int col = 0; col < 8; ++col)
        place(1, col, PieceType::Pawn, PieceColor::Black);

    for (int col = 0; col < 8; ++col)
        place(6, col, PieceType::Pawn, PieceColor::White);

    place(7, 0, PieceType::Rook,   PieceColor::White);
    place(7, 1, PieceType::Knight, PieceColor::White);
    place(7, 2, PieceType::Bishop, PieceColor::White);
    place(7, 3, PieceType::Queen,  PieceColor::White);
    place(7, 4, PieceType::King,   PieceColor::White);
    place(7, 5, PieceType::Bishop, PieceColor::White);
    place(7, 6, PieceType::Knight, PieceColor::White);
    place(7, 7, PieceType::Rook,   PieceColor::White);

    return board;
}

Board makeTestBoard() {
    Board board{};

    auto place = [&](int row, int col, PieceType type, PieceColor color) {
        board[row][col] = { type, color };
    };

    // Black king on e8, black pawns on the 7th rank
    place(0, 4, PieceType::King, PieceColor::Black);
    for (int col = 0; col < 8; ++col)
        place(1, col, PieceType::Pawn, PieceColor::Black);

    // White king on e1, white pawns on the 2nd rank
    place(7, 4, PieceType::King, PieceColor::White);
    for (int col = 0; col < 8; ++col)
        place(6, col, PieceType::Pawn, PieceColor::White);

    return board;
}
