#include "MoveGenAI.h"
#include "Directions.h"
#include "Position.h"

// -----------------------------------------------------------------------
// Internal helpers
// -----------------------------------------------------------------------

static inline void addMove(MoveList& moves, int from, int to) {
    moves.moves[moves.count++].data = Move::make((uint8_t)from, (uint8_t)to);
}

static inline void markAttack(uint64_t& attacks, int matSq) {
    attacks |= (1ULL << MAT_TO_BIT[matSq]);
}

// Single pass: generates moves AND marks attacked squares for sliding pieces
static inline void addSliding(const BoardMatrix& board, MoveList& moves, uint64_t& attacks,
                               int sq, int dir, uint8_t enemyColor) {
    int next = sq + dir;
    while (isEmpty(board[next])) {
        markAttack(attacks, next);
        addMove(moves, sq, next);
        next += dir;
    }
    if (!isSentinel(board[next])) {
        markAttack(attacks, next);
        if (board[next] & enemyColor)
            addMove(moves, sq, next);
    }
}

// -----------------------------------------------------------------------
// White move generation
// -----------------------------------------------------------------------

static void addWhitePawnMoves(const BoardMatrix& board, MoveList& moves, uint64_t& attacks, int sq) {
    // Forward moves (pawns do not attack forward)
    int fwd = sq + Dir::UP;
    if (isEmpty(board[fwd])) {
        addMove(moves, sq, fwd);
        int dbl = fwd + Dir::UP;
        if (sq / 12 == 8 && isEmpty(board[dbl]))
            addMove(moves, sq, dbl);
    }

    // Diagonal attacks (always marked, capture only if enemy is there)
    for (int capDir : { Dir::UP_LEFT, Dir::UP_RIGHT }) {
        int capSq = sq + capDir;
        if (!isSentinel(board[capSq])) {
            markAttack(attacks, capSq);
            if (isBlack(board[capSq]))
                addMove(moves, sq, capSq);
        }
    }
}

static void addWhiteKnightMoves(const BoardMatrix& board, MoveList& moves, uint64_t& attacks, int sq) {
    for (int off : { KnightDir::UP2_LEFT1,   KnightDir::UP2_RIGHT1,
                     KnightDir::UP1_LEFT2,   KnightDir::UP1_RIGHT2,
                     KnightDir::DOWN1_LEFT2, KnightDir::DOWN1_RIGHT2,
                     KnightDir::DOWN2_LEFT1, KnightDir::DOWN2_RIGHT1 }) {
        int target = sq + off;
        if (!isSentinel(board[target])) {
            markAttack(attacks, target);
            if (!isWhite(board[target]))
                addMove(moves, sq, target);
        }
    }
}

static void addWhiteBishopMoves(const BoardMatrix& board, MoveList& moves, uint64_t& attacks, int sq) {
    for (int d : { Dir::UP_LEFT, Dir::UP_RIGHT, Dir::DOWN_LEFT, Dir::DOWN_RIGHT })
        addSliding(board, moves, attacks, sq, d, BLACK);
}

static void addWhiteRookMoves(const BoardMatrix& board, MoveList& moves, uint64_t& attacks, int sq) {
    for (int d : { Dir::UP, Dir::DOWN, Dir::LEFT, Dir::RIGHT })
        addSliding(board, moves, attacks, sq, d, BLACK);
}

static void addWhiteQueenMoves(const BoardMatrix& board, MoveList& moves, uint64_t& attacks, int sq) {
    addWhiteBishopMoves(board, moves, attacks, sq);
    addWhiteRookMoves  (board, moves, attacks, sq);
}

static void addWhiteKingMoves(const BoardMatrix& board, MoveList& moves, uint64_t& attacks, int sq) {
    for (int off : { Dir::UP_LEFT,   Dir::UP,   Dir::UP_RIGHT,
                     Dir::LEFT,                 Dir::RIGHT,
                     Dir::DOWN_LEFT, Dir::DOWN, Dir::DOWN_RIGHT }) {
        int target = sq + off;
        if (!isSentinel(board[target])) {
            markAttack(attacks, target);
            if (!isWhite(board[target]))
                addMove(moves, sq, target);
        }
    }
}

// -----------------------------------------------------------------------
// Black move generation
// -----------------------------------------------------------------------

static void addBlackPawnMoves(const BoardMatrix& board, MoveList& moves, uint64_t& attacks, int sq) {
    // Forward moves (pawns do not attack forward)
    int fwd = sq + Dir::DOWN;
    if (isEmpty(board[fwd])) {
        addMove(moves, sq, fwd);
        int dbl = fwd + Dir::DOWN;
        if (sq / 12 == 3 && isEmpty(board[dbl]))
            addMove(moves, sq, dbl);
    }

    // Diagonal attacks (always marked, capture only if enemy is there)
    for (int capDir : { Dir::DOWN_LEFT, Dir::DOWN_RIGHT }) {
        int capSq = sq + capDir;
        if (!isSentinel(board[capSq])) {
            markAttack(attacks, capSq);
            if (isWhite(board[capSq]))
                addMove(moves, sq, capSq);
        }
    }
}

static void addBlackKnightMoves(const BoardMatrix& board, MoveList& moves, uint64_t& attacks, int sq) {
    for (int off : { KnightDir::UP2_LEFT1,   KnightDir::UP2_RIGHT1,
                     KnightDir::UP1_LEFT2,   KnightDir::UP1_RIGHT2,
                     KnightDir::DOWN1_LEFT2, KnightDir::DOWN1_RIGHT2,
                     KnightDir::DOWN2_LEFT1, KnightDir::DOWN2_RIGHT1 }) {
        int target = sq + off;
        if (!isSentinel(board[target])) {
            markAttack(attacks, target);
            if (!isBlack(board[target]))
                addMove(moves, sq, target);
        }
    }
}

static void addBlackBishopMoves(const BoardMatrix& board, MoveList& moves, uint64_t& attacks, int sq) {
    for (int d : { Dir::UP_LEFT, Dir::UP_RIGHT, Dir::DOWN_LEFT, Dir::DOWN_RIGHT })
        addSliding(board, moves, attacks, sq, d, WHITE);
}

static void addBlackRookMoves(const BoardMatrix& board, MoveList& moves, uint64_t& attacks, int sq) {
    for (int d : { Dir::UP, Dir::DOWN, Dir::LEFT, Dir::RIGHT })
        addSliding(board, moves, attacks, sq, d, WHITE);
}

static void addBlackQueenMoves(const BoardMatrix& board, MoveList& moves, uint64_t& attacks, int sq) {
    addBlackBishopMoves(board, moves, attacks, sq);
    addBlackRookMoves  (board, moves, attacks, sq);
}

static void addBlackKingMoves(const BoardMatrix& board, MoveList& moves, uint64_t& attacks, int sq) {
    for (int off : { Dir::UP_LEFT,   Dir::UP,   Dir::UP_RIGHT,
                     Dir::LEFT,                 Dir::RIGHT,
                     Dir::DOWN_LEFT, Dir::DOWN, Dir::DOWN_RIGHT }) {
        int target = sq + off;
        if (!isSentinel(board[target])) {
            markAttack(attacks, target);
            if (!isBlack(board[target]))
                addMove(moves, sq, target);
        }
    }
}

// -----------------------------------------------------------------------
// Public functions
// -----------------------------------------------------------------------

GenResult generateWhiteMoves(const AIState& state) {
    GenResult result;
    for (int i = 0; i < state.whiteCount; ++i) {
        int sq = state.whitePieces[i];
        switch (typeOf(state.board[sq])) {
        case TYPE_PAWN:   addWhitePawnMoves  (state.board, result.moves, result.attacks, sq); break;
        case TYPE_KNIGHT: addWhiteKnightMoves(state.board, result.moves, result.attacks, sq); break;
        case TYPE_BISHOP: addWhiteBishopMoves(state.board, result.moves, result.attacks, sq); break;
        case TYPE_ROOK:   addWhiteRookMoves  (state.board, result.moves, result.attacks, sq); break;
        case TYPE_QUEEN:  addWhiteQueenMoves (state.board, result.moves, result.attacks, sq); break;
        case TYPE_KING:   addWhiteKingMoves  (state.board, result.moves, result.attacks, sq); break;
        }
    }
    return result;
}

GenResult generateBlackMoves(const AIState& state) {
    GenResult result;
    for (int i = 0; i < state.blackCount; ++i) {
        int sq = state.blackPieces[i];
        switch (typeOf(state.board[sq])) {
        case TYPE_PAWN:   addBlackPawnMoves  (state.board, result.moves, result.attacks, sq); break;
        case TYPE_KNIGHT: addBlackKnightMoves(state.board, result.moves, result.attacks, sq); break;
        case TYPE_BISHOP: addBlackBishopMoves(state.board, result.moves, result.attacks, sq); break;
        case TYPE_ROOK:   addBlackRookMoves  (state.board, result.moves, result.attacks, sq); break;
        case TYPE_QUEEN:  addBlackQueenMoves (state.board, result.moves, result.attacks, sq); break;
        case TYPE_KING:   addBlackKingMoves  (state.board, result.moves, result.attacks, sq); break;
        }
    }
    return result;
}
