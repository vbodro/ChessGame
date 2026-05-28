#include "MoveGen.h"

static bool isSquareAttacked(const Board& board, int row, int col, PieceColor attackerColor) {
    auto inBounds = [](int r, int c) { return r >= 0 && r < 8 && c >= 0 && c < 8; };

    // Rook / Queen (straight)
    constexpr int straight[4][2] = {{-1,0},{1,0},{0,-1},{0,1}};
    for (auto& d : straight) {
        int r = row + d[0], c = col + d[1];
        while (inBounds(r, c)) {
            if (board[r][c].type != PieceType::None) {
                if (board[r][c].color == attackerColor &&
                    (board[r][c].type == PieceType::Rook || board[r][c].type == PieceType::Queen))
                    return true;
                break;
            }
            r += d[0]; c += d[1];
        }
    }

    // Bishop / Queen (diagonal)
    constexpr int diag[4][2] = {{-1,-1},{-1,1},{1,-1},{1,1}};
    for (auto& d : diag) {
        int r = row + d[0], c = col + d[1];
        while (inBounds(r, c)) {
            if (board[r][c].type != PieceType::None) {
                if (board[r][c].color == attackerColor &&
                    (board[r][c].type == PieceType::Bishop || board[r][c].type == PieceType::Queen))
                    return true;
                break;
            }
            r += d[0]; c += d[1];
        }
    }

    // Knight
    constexpr int knightOff[8][2] = {{-2,-1},{-2,1},{-1,-2},{-1,2},{1,-2},{1,2},{2,-1},{2,1}};
    for (auto& o : knightOff) {
        int r = row + o[0], c = col + o[1];
        if (inBounds(r, c) && board[r][c].color == attackerColor && board[r][c].type == PieceType::Knight)
            return true;
    }

    // Pawn (attacker's pawns strike diagonally forward — from attacker's perspective)
    int pawnDir = (attackerColor == PieceColor::White) ? 1 : -1;
    for (int dc : {-1, 1}) {
        int r = row + pawnDir, c = col + dc;
        if (inBounds(r, c) && board[r][c].color == attackerColor && board[r][c].type == PieceType::Pawn)
            return true;
    }

    // King
    for (int dr = -1; dr <= 1; ++dr)
        for (int dc = -1; dc <= 1; ++dc) {
            if (!dr && !dc) continue;
            int r = row + dr, c = col + dc;
            if (inBounds(r, c) && board[r][c].color == attackerColor && board[r][c].type == PieceType::King)
                return true;
        }

    return false;
}

bool isKingInCheck(const Board& board, PieceColor kingColor) {
    PieceColor enemy = (kingColor == PieceColor::White) ? PieceColor::Black : PieceColor::White;
    for (int r = 0; r < 8; ++r)
        for (int c = 0; c < 8; ++c)
            if (board[r][c].type == PieceType::King && board[r][c].color == kingColor)
                return isSquareAttacked(board, r, c, enemy);
    return false;
}

bool hasAnyLegalMove(const Board& board, PieceColor color, const CastlingRights& castling) {
    for (int r = 0; r < 8; ++r) {
        for (int c = 0; c < 8; ++c) {
            if (board[r][c].color != color) continue;
            if (!getValidMoves(board, r, c, castling).empty())
                return true;
        }
    }
    return false;
}

std::vector<sf::Vector2i> getValidMoves(const Board& board, int row, int col, const CastlingRights& castling) {
    const Piece& piece = board[row][col];
    if (piece.type == PieceType::None) return {};

    std::vector<sf::Vector2i> moves;

    auto inBounds  = [](int r, int c) { return r >= 0 && r < 8 && c >= 0 && c < 8; };
    auto isEmpty   = [&](int r, int c) { return inBounds(r, c) && board[r][c].type == PieceType::None; };
    auto isEnemy   = [&](int r, int c) { return inBounds(r, c) && board[r][c].color != PieceColor::None && board[r][c].color != piece.color && board[r][c].type != PieceType::King; };
    auto canMoveTo = [&](int r, int c) { return inBounds(r, c) && board[r][c].color != piece.color && board[r][c].type != PieceType::King; };
    auto add       = [&](int r, int c) { moves.push_back({ c, r }); };

    auto addSliding = [&](int dr, int dc) {
        int r = row + dr, c = col + dc;
        while (inBounds(r, c)) {
            if (isEmpty(r, c)) { add(r, c); r += dr; c += dc; }
            else { if (isEnemy(r, c)) add(r, c); break; }
        }
    };

    switch (piece.type) {

    case PieceType::Pawn: {
        int dir      = (piece.color == PieceColor::White) ? -1 : 1;
        int startRow = (piece.color == PieceColor::White) ?  6 : 1;
        if (isEmpty(row + dir, col)) {
            add(row + dir, col);
            if (row == startRow && isEmpty(row + 2 * dir, col))
                add(row + 2 * dir, col);
        }
        if (isEnemy(row + dir, col - 1)) add(row + dir, col - 1);
        if (isEnemy(row + dir, col + 1)) add(row + dir, col + 1);
        break;
    }

    case PieceType::Rook:
        addSliding(-1, 0); addSliding(1, 0);
        addSliding(0, -1); addSliding(0, 1);
        break;

    case PieceType::Bishop:
        addSliding(-1, -1); addSliding(-1, 1);
        addSliding(1, -1);  addSliding(1,  1);
        break;

    case PieceType::Queen:
        addSliding(-1, 0); addSliding(1, 0);
        addSliding(0, -1); addSliding(0, 1);
        addSliding(-1, -1); addSliding(-1, 1);
        addSliding(1, -1);  addSliding(1,  1);
        break;

    case PieceType::Knight: {
        constexpr int offsets[8][2] = {{-2,-1},{-2,1},{-1,-2},{-1,2},{1,-2},{1,2},{2,-1},{2,1}};
        for (auto& o : offsets)
            if (canMoveTo(row + o[0], col + o[1]))
                add(row + o[0], col + o[1]);
        break;
    }

    case PieceType::King: {
        PieceColor enemy = (piece.color == PieceColor::White) ? PieceColor::Black : PieceColor::White;

        // Remove king temporarily so he doesn't block sliding attackers behind him
        Board boardCopy = board;
        boardCopy[row][col] = {};

        // Normal moves — filter attacked squares
        for (int dr = -1; dr <= 1; ++dr)
            for (int dc = -1; dc <= 1; ++dc) {
                if (!dr && !dc) continue;
                int r = row + dr, c = col + dc;
                if (canMoveTo(r, c) && !isSquareAttacked(boardCopy, r, c, enemy))
                    add(r, c);
            }

        // Castling — king must not be in check on current square
        if (!isSquareAttacked(boardCopy, row, col, enemy)) {
            bool isWhite = (piece.color == PieceColor::White);
            int backRank = isWhite ? 7 : 0;

            if (row == backRank && col == 4) {
                bool kingSide  = isWhite ? castling.whiteKingSide  : castling.blackKingSide;
                bool queenSide = isWhite ? castling.whiteQueenSide : castling.blackQueenSide;

                // Kingside: f and g empty, f and g not attacked
                if (kingSide &&
                    isEmpty(backRank, 5) && isEmpty(backRank, 6) &&
                    !isSquareAttacked(boardCopy, backRank, 5, enemy) &&
                    !isSquareAttacked(boardCopy, backRank, 6, enemy))
                    add(backRank, 6);

                // Queenside: b, c, d empty; c and d not attacked
                if (queenSide &&
                    isEmpty(backRank, 3) && isEmpty(backRank, 2) && isEmpty(backRank, 1) &&
                    !isSquareAttacked(boardCopy, backRank, 3, enemy) &&
                    !isSquareAttacked(boardCopy, backRank, 2, enemy))
                    add(backRank, 2);
            }
        }
        break;
    }

    default: break;
    }

    // -------------------------------------------------------------------
    // Drop moves that would leave our own king in check.
    // For each pseudo-legal move we simulate it on a copy and check
    // whether our king would be attacked afterwards.
    // -------------------------------------------------------------------

    PieceColor enemyColor = (piece.color == PieceColor::White) ? PieceColor::Black : PieceColor::White;

    // Find our king on the original board (for non-king moves the position
    // stays the same; for king moves we use the move's destination).
    int kingRowOrig = -1, kingColOrig = -1;
    for (int r = 0; r < 8 && kingRowOrig == -1; ++r)
        for (int c = 0; c < 8; ++c)
            if (board[r][c].type == PieceType::King && board[r][c].color == piece.color) {
                kingRowOrig = r; kingColOrig = c;
                break;
            }

    if (kingRowOrig == -1) return moves;  // defensive: no king, don't filter

    std::vector<sf::Vector2i> legal;
    legal.reserve(moves.size());

    for (const auto& m : moves) {
        int toRow = m.y, toCol = m.x;

        // Simulate the move: move the piece, clear the source square.
        // For captures the enemy piece on `to` is replaced by our piece.
        Board sim = board;
        sim[toRow][toCol] = sim[row][col];
        sim[row][col]     = {};

        // King move: king now sits on the destination.
        // Other moves: king stays on its original square.
        int kr, kc;
        if (piece.type == PieceType::King) { kr = toRow; kc = toCol; }
        else                               { kr = kingRowOrig; kc = kingColOrig; }

        if (!isSquareAttacked(sim, kr, kc, enemyColor))
            legal.push_back(m);
    }

    return legal;
}
