#include "InputHandler.h"
#include "../moves/MoveGen.h"
#include "../rendering/Renderer.h"
#include <cstdlib>

static int squareFromPixel(int pixel) {
    return pixel / SQUARE_SIZE;
}

void handleMousePressed(const sf::Event::MouseButtonEvent& evt, Board& board, DragState& drag, const CastlingRights& castling, const PromotionState& promotion) {
    if (promotion.isPending) return;

    int col = squareFromPixel(evt.x);
    int row = squareFromPixel(evt.y);

    if (col < 0 || col >= 8 || row < 0 || row >= 8) return;

    Piece& p = board[row][col];
    if (p.type == PieceType::None || p.color != PieceColor::White) return;

    drag.isDragging = true;
    drag.fromRow    = row;
    drag.fromCol    = col;
    drag.piece      = p;
    drag.mousePos   = { static_cast<float>(evt.x), static_cast<float>(evt.y) };
    drag.validMoves = getValidMoves(board, row, col, castling);

    board[row][col] = {};
}

bool handleMouseReleased(const sf::Event::MouseButtonEvent& evt, Board& board, DragState& drag, CastlingRights& castling, PromotionState& promotion) {
    if (!drag.isDragging) return false;

    int col = squareFromPixel(evt.x);
    int row = squareFromPixel(evt.y);

    bool validDrop = false;
    if (col >= 0 && col < 8 && row >= 0 && row < 8) {
        for (const auto& m : drag.validMoves)
            if (m.x == col && m.y == row) { validDrop = true; break; }
    }

    if (validDrop) {
        // Check for pawn promotion
        bool isPromotion = (drag.piece.type == PieceType::Pawn) &&
                           ((drag.piece.color == PieceColor::White && row == 0) ||
                            (drag.piece.color == PieceColor::Black && row == 7));

        board[row][col] = drag.piece;

        if (isPromotion) {
            promotion.isPending = true;
            promotion.row       = row;
            promotion.col       = col;
            drag = {};
            return false;  // move not yet final — waiting for promotion choice
        }

        // Castling — move the rook
        if (drag.piece.type == PieceType::King) {
            int colDiff = col - drag.fromCol;
            if (std::abs(colDiff) == 2) {
                if (colDiff == 2) {
                    board[row][5] = { PieceType::Rook, drag.piece.color };
                    board[row][7] = {};
                } else {
                    board[row][3] = { PieceType::Rook, drag.piece.color };
                    board[row][0] = {};
                }
            }
            if (drag.piece.color == PieceColor::White) {
                castling.whiteKingSide  = false;
                castling.whiteQueenSide = false;
            } else {
                castling.blackKingSide  = false;
                castling.blackQueenSide = false;
            }
        }

        if (drag.piece.type == PieceType::Rook) {
            if (drag.fromRow == 7 && drag.fromCol == 7) castling.whiteKingSide  = false;
            if (drag.fromRow == 7 && drag.fromCol == 0) castling.whiteQueenSide = false;
            if (drag.fromRow == 0 && drag.fromCol == 7) castling.blackKingSide  = false;
            if (drag.fromRow == 0 && drag.fromCol == 0) castling.blackQueenSide = false;
        }

        // Rook captured on its starting square
        if (drag.fromRow == 7 && col == 7 && row == 7) castling.whiteKingSide  = false;
        if (drag.fromRow == 7 && col == 0 && row == 7) castling.whiteQueenSide = false;
        if (drag.fromRow == 0 && col == 7 && row == 0) castling.blackKingSide  = false;
        if (drag.fromRow == 0 && col == 0 && row == 0) castling.blackQueenSide = false;

        drag = {};
        return true;  // move complete
    } else {
        board[drag.fromRow][drag.fromCol] = drag.piece;
        drag = {};
        return false;
    }
}

void handleMouseMoved(const sf::Event::MouseMoveEvent& evt, DragState& drag) {
    drag.mousePos = { static_cast<float>(evt.x), static_cast<float>(evt.y) };
}

bool handlePromotionClick(const sf::Event::MouseButtonEvent& evt, Board& board, PromotionState& promotion) {
    if (!promotion.isPending) return false;

    int clickCol = squareFromPixel(evt.x);
    int clickRow = squareFromPixel(evt.y);

    // The panel is 4 squares tall, starting at row 0 (white promotion).
    // For black promotion (row 7) the panel goes upward: rows 4-7.
    bool isWhite   = (promotion.row == 0);
    int  panelStart = isWhite ? 0 : 4;

    if (clickCol != promotion.col) return false;
    int idx = clickRow - panelStart;
    if (idx < 0 || idx >= 4) return false;

    constexpr PieceType options[4] = {
        PieceType::Queen, PieceType::Rook, PieceType::Bishop, PieceType::Knight
    };

    PieceColor color = isWhite ? PieceColor::White : PieceColor::Black;
    board[promotion.row][promotion.col] = { options[idx], color };
    promotion = {};
    return true;  // promotion resolved, move complete
}
