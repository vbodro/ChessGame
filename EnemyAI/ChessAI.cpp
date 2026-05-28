#include "ChessAI.h"
#include "MoveGenAI.h"
#include "AIState.h"
#include "Directions.h"
#include <climits>
#include <chrono>
#include <iostream>

// -----------------------------------------------------------------------
// Internal helpers
// -----------------------------------------------------------------------

static inline uint8_t findKing(const uint8_t* pieces, uint8_t count, const BoardMatrix& board) {
    for (int i = 0; i < count; ++i)
        if (typeOf(board[pieces[i]]) == TYPE_KING)
            return pieces[i];
    return 0;
}

// Computes the squares attacked (= defended) by black pieces, WITHOUT
// generating the move list. Dedicated function so the original
// generateBlackMoves stays untouched and we don't pay for building a move
// list that's not needed here. Attack-marking logic mirrors MoveGenAI.cpp
// — only the addMove calls are skipped.
static uint64_t computeBlackAttackMask(const AIState& s) {
    uint64_t attacks = 0;

    auto markSliding = [&](int sq, int dir) {
        int next = sq + dir;
        while (isEmpty(s.board[next])) {
            attacks |= (1ULL << MAT_TO_BIT[next]);
            next += dir;
        }
        if (!isSentinel(s.board[next]))
            attacks |= (1ULL << MAT_TO_BIT[next]);
    };

    for (int i = 0; i < s.blackCount; ++i) {
        const int sq = s.blackPieces[i];
        switch (typeOf(s.board[sq])) {
        case TYPE_PAWN:
            // Black pawns attack diagonally downward
            for (int d : { Dir::DOWN_LEFT, Dir::DOWN_RIGHT }) {
                int t = sq + d;
                if (!isSentinel(s.board[t])) attacks |= (1ULL << MAT_TO_BIT[t]);
            }
            break;

        case TYPE_KNIGHT:
            for (int off : { KnightDir::UP2_LEFT1,   KnightDir::UP2_RIGHT1,
                             KnightDir::UP1_LEFT2,   KnightDir::UP1_RIGHT2,
                             KnightDir::DOWN1_LEFT2, KnightDir::DOWN1_RIGHT2,
                             KnightDir::DOWN2_LEFT1, KnightDir::DOWN2_RIGHT1 }) {
                int t = sq + off;
                if (!isSentinel(s.board[t])) attacks |= (1ULL << MAT_TO_BIT[t]);
            }
            break;

        case TYPE_BISHOP:
            for (int d : { Dir::UP_LEFT, Dir::UP_RIGHT, Dir::DOWN_LEFT, Dir::DOWN_RIGHT })
                markSliding(sq, d);
            break;

        case TYPE_ROOK:
            for (int d : { Dir::UP, Dir::DOWN, Dir::LEFT, Dir::RIGHT })
                markSliding(sq, d);
            break;

        case TYPE_QUEEN:
            for (int d : { Dir::UP_LEFT, Dir::UP_RIGHT, Dir::DOWN_LEFT, Dir::DOWN_RIGHT,
                           Dir::UP,      Dir::DOWN,     Dir::LEFT,      Dir::RIGHT })
                markSliding(sq, d);
            break;

        case TYPE_KING:
            for (int off : { Dir::UP_LEFT,   Dir::UP,   Dir::UP_RIGHT,
                             Dir::LEFT,                 Dir::RIGHT,
                             Dir::DOWN_LEFT, Dir::DOWN, Dir::DOWN_RIGHT }) {
                int t = sq + off;
                if (!isSentinel(s.board[t])) attacks |= (1ULL << MAT_TO_BIT[t]);
            }
            break;
        }
    }
    return attacks;
}

// Number of black pieces (excluding the king) whose square is in the black
// attack mask (= defended by another black piece).
static int countProtectedBlackPieces(const AIState& s) {
    const uint64_t attacks = computeBlackAttackMask(s);
    int count = 0;
    for (int i = 0; i < s.blackCount; ++i) {
        uint8_t sq = s.blackPieces[i];
        if (typeOf(s.board[sq]) == TYPE_KING) continue;  // king is not counted
        if ((attacks >> MAT_TO_BIT[sq]) & 1) ++count;
    }
    return count;
}

// Returns true if applying the black move on a copy of the root state would
// leave white in stalemate (white to move, king not in check, no legal
// moves). Used in poll() to drop stalemate-causing moves when a better
// alternative exists.
static bool causesWhiteStalemate(AIState s, const ChessAI::Move& m) {
    // Apply the black move on a local copy
    const uint8_t fromMat = (uint8_t)((m.from.y + 2) * 12 + (m.from.x + 2));
    const uint8_t toMat   = (uint8_t)((m.to.y   + 2) * 12 + (m.to.x   + 2));
    const uint8_t captured    = s.board[toMat];
    const uint8_t movingPiece = s.board[fromMat];

    s.board[toMat]   = movingPiece;
    s.board[fromMat] = EMPTY;
    {
        int idx = findPiece(s.blackPieces, s.blackCount, fromMat);
        s.blackPieces[idx] = toMat;
    }
    if (captured != EMPTY) {
        int idx = findPiece(s.whitePieces, s.whiteCount, toMat);
        removePiece(s.whitePieces, s.whiteCount, idx);
    }
    // Black pawn promotion to queen
    if (movingPiece == B_PAWN && toMat >= 110 && toMat <= 117)
        s.board[toMat] = B_QUEEN;

    // Is the white king already in check? If yes, this is not stalemate
    // (could be checkmate, but not the case we're filtering).
    {
        GenResult black = generateBlackMoves(s);
        uint8_t   wK    = findKing(s.whitePieces, s.whiteCount, s.board);
        if ((black.attacks >> MAT_TO_BIT[wK]) & 1) return false;
    }

    // Try every pseudo-legal white move; if any keeps the king out of check
    // then white has a legal move → not stalemate.
    GenResult white = generateWhiteMoves(s);
    for (int i = 0; i < white.moves.count; ++i) {
        const uint8_t f = ::Move::getFrom(white.moves.moves[i].data);
        const uint8_t t = ::Move::getTo  (white.moves.moves[i].data);

        const uint8_t cap = s.board[t];
        const uint8_t mvp = s.board[f];

        // Make the white move
        s.board[t] = mvp;
        s.board[f] = EMPTY;
        {
            int idx = findPiece(s.whitePieces, s.whiteCount, f);
            s.whitePieces[idx] = t;
        }
        if (cap != EMPTY) {
            int idx = findPiece(s.blackPieces, s.blackCount, t);
            removePiece(s.blackPieces, s.blackCount, idx);
        }

        // Is the white king in check after the move?
        GenResult black2 = generateBlackMoves(s);
        uint8_t   wK     = findKing(s.whitePieces, s.whiteCount, s.board);
        const bool stillInCheck = (black2.attacks >> MAT_TO_BIT[wK]) & 1;

        // Unmake
        s.board[f] = mvp;
        s.board[t] = cap;
        {
            int idx = findPiece(s.whitePieces, s.whiteCount, t);
            s.whitePieces[idx] = f;
        }
        if (cap != EMPTY)
            s.blackPieces[s.blackCount++] = t;

        if (!stillInCheck) return false;  // white has a legal move
    }

    return true;  // king not in check, but no legal moves → stalemate
}

// -----------------------------------------------------------------------
// quiescence — continuation of the search past getMaxPoints' leaf, but
// ONLY for capture moves. Solves the horizon effect: the AI no longer
// hangs pieces because the search continues as long as exchanges are
// available.
//
//   Stand pat = 0 (no capture taken → no further material change)
//   Otherwise: max over capture moves of (capturedValue − quiescence(child))
// -----------------------------------------------------------------------

static int quiescence(AIState& state, int qDepth, int parentValue, int parentMaxScore, bool isWhite)
{
    GenResult gen = isWhite ? generateWhiteMoves(state) : generateBlackMoves(state);

    uint8_t* ownPieces   = isWhite ? state.whitePieces : state.blackPieces;
    uint8_t& ownCount    = isWhite ? state.whiteCount  : state.blackCount;
    uint8_t* enemyPieces = isWhite ? state.blackPieces : state.whitePieces;
    uint8_t& enemyCount  = isWhite ? state.blackCount  : state.whiteCount;

    // No moves — checkmate or stalemate. Stalemate = 0 (draw, neutral) so
    // it doesn't look like an exceptional opponent move.
    if (gen.moves.count == 0) {
        GenResult opGen  = isWhite ? generateBlackMoves(state) : generateWhiteMoves(state);
        uint8_t   kingSq = findKing(ownPieces, ownCount, state.board);
        return ((opGen.attacks >> MAT_TO_BIT[kingSq]) & 1) ? -1000 : 0;
    }

    // Parent left their king in check — illegal move
    {
        uint8_t kingSq = findKing(enemyPieces, enemyCount, state.board);
        if ((gen.attacks >> MAT_TO_BIT[kingSq]) & 1)
            return 1000;
    }

    if (qDepth == 0) return 0;

    sortMoves(gen.moves, state.board);

    // Stand pat: the "don't capture anything" option → no further material change
    int newMaxScore = 0;

    for (int i = 0; i < gen.moves.count; ++i) {
        const uint8_t from = ::Move::getFrom(gen.moves.moves[i].data);
        const uint8_t to   = ::Move::getTo  (gen.moves.moves[i].data);

        const uint8_t captured = state.board[to];
        // Quiescence only considers captures. Moves are sorted captures-first,
        // so the first non-capture means the rest are also non-captures → bail out.
        if (captured == EMPTY) break;

        int capturedValue = (int)scoreOf(captured);

        // Make
        const uint8_t movingPiece = state.board[from];
        state.board[to]   = movingPiece;
        state.board[from] = EMPTY;
        {
            int idx = findPiece(ownPieces, ownCount, from);
            ownPieces[idx] = to;
        }
        {
            int idx = findPiece(enemyPieces, enemyCount, to);
            removePiece(enemyPieces, enemyCount, idx);
        }

        // Promotion on capture — pawn taking a piece on the last rank.
        // White: matrix indices 26..33; black: 110..117 (see getMaxPoints).
        if (isWhite && movingPiece == W_PAWN && to >= 26 && to <= 33) {
            state.board[to] = W_QUEEN;
            capturedValue += (int)scoreOf(W_QUEEN) - (int)scoreOf(W_PAWN);
        }
        else if (!isWhite && movingPiece == B_PAWN && to >= 110 && to <= 117) {
            state.board[to] = B_QUEEN;
            capturedValue += (int)scoreOf(B_QUEEN) - (int)scoreOf(B_PAWN);
        }

        const int score = capturedValue - quiescence(state, qDepth - 1,
                                                     capturedValue, newMaxScore, !isWhite);
        const int parentScore = parentValue - score;

        // Unmake — use the saved original piece (on promotion state.board[to]
        // would be the queen).
        state.board[from] = movingPiece;
        state.board[to]   = captured;
        {
            int idx = findPiece(ownPieces, ownCount, to);
            ownPieces[idx] = from;
        }
        enemyPieces[enemyCount++] = to;

        newMaxScore = std::max(newMaxScore, score);
        if (parentScore <= parentMaxScore) break;
    }

    return newMaxScore;
}

// -----------------------------------------------------------------------
// getMaxPoints — alpha-beta search returning a score from the current
// player's perspective.
//
//   score(move) = capturedValue − getMaxPoints(child)
//
// Leaf (depth == 0): transition to quiescence (capture-only search).
// Stalemate (no moves, king not in check):                       0
// Checkmate (no moves, king in check):              −1000 * (depth+1)
// Illegal parent move (my attacks reach opponent king):  (depth+1) * 1000
// -----------------------------------------------------------------------

static int getMaxPoints(AIState& state, int depth, int parentValue, int parentMaxScore, bool isWhite)
{
    GenResult gen = isWhite ? generateWhiteMoves(state) : generateBlackMoves(state);

    uint8_t* ownPieces   = isWhite ? state.whitePieces : state.blackPieces;
    uint8_t& ownCount    = isWhite ? state.whiteCount  : state.blackCount;
    uint8_t* enemyPieces = isWhite ? state.blackPieces : state.whitePieces;
    uint8_t& enemyCount  = isWhite ? state.blackCount  : state.whiteCount;

    // No pseudo-legal moves — checkmate or stalemate. Stalemate = 0
    // (neutral draw) so the AI doesn't perceive forcing a stalemate as an
    // exceptional opponent result.
    if (gen.moves.count == 0) {
        GenResult opGen  = isWhite ? generateBlackMoves(state) : generateWhiteMoves(state);
        uint8_t   kingSq = findKing(ownPieces, ownCount, state.board);
        return ((opGen.attacks >> MAT_TO_BIT[kingSq]) & 1) ? -1000 * (depth + 1) : 0;
    }

    // Parent left their own king in check — illegal move
    {
        uint8_t kingSq = findKing(enemyPieces, enemyCount, state.board);
        if ((gen.attacks >> MAT_TO_BIT[kingSq]) & 1)
        {
            return (depth + 1) * 1000;
        }
    }

    if (depth == 0) {
        return quiescence(state, 4, parentValue, parentMaxScore, isWhite);
    }

    // Leaf: switch to quiescence — keep searching capture-only moves for 4
    // more plies. Lets us see late exchanges that would otherwise fall
    // outside the search horizon.

    sortMoves(gen.moves, state.board);
    int newMaxScore = -10000;
    for (int i = 0; i < gen.moves.count; ++i) {
        const uint8_t from = ::Move::getFrom(gen.moves.moves[i].data);
        const uint8_t to   = ::Move::getTo  (gen.moves.moves[i].data);

        const uint8_t captured = state.board[to];
        int capturedValue = (captured == EMPTY) ? 0 : (int)scoreOf(captured) + depth / 2;

        // Make
        const uint8_t movingPiece = state.board[from];
        state.board[to]   = movingPiece;
        state.board[from] = EMPTY;
        {
            int idx = findPiece(ownPieces, ownCount, from);
            ownPieces[idx] = to;
        }
        if (captured != EMPTY) {
            int idx = findPiece(enemyPieces, enemyCount, to);
            removePiece(enemyPieces, enemyCount, idx);
        }

        // Pawn promotion to queen. White promotes on matrix row 2 (real
        // squares 26..33), black on matrix row 9 (real squares 110..117).
        // Direct bounds comparison is faster than `to / 12`. Bonus = the
        // material delta (queen − pawn).
        if (isWhite && movingPiece == W_PAWN && to >= 26 && to <= 33) {
            state.board[to] = W_QUEEN;
            capturedValue += (int)scoreOf(W_QUEEN) - (int)scoreOf(W_PAWN);
        }
        else if (!isWhite && movingPiece == B_PAWN && to >= 110 && to <= 117) {
            state.board[to] = B_QUEEN;
            capturedValue += (int)scoreOf(B_QUEEN) - (int)scoreOf(B_PAWN);
        }

        const int tempScore = getMaxPoints(state, depth - 1,
            capturedValue,
            newMaxScore,
            !isWhite);
        const int score = capturedValue - tempScore;

        const int parentScore = parentValue - score;

        // Unmake — restore the original moving piece (movingPiece) on `from`.
        // After promotion state.board[to] is the queen, so we MUST NOT read
        // from it — use the saved value instead.
        state.board[from] = movingPiece;
        state.board[to]   = captured;
        {
            int idx = findPiece(ownPieces, ownCount, to);
            ownPieces[idx] = from;
        }
        if (captured != EMPTY)
            enemyPieces[enemyCount++] = to;

        newMaxScore = std::max(newMaxScore, score);
        if (parentScore <= parentMaxScore)
        {
            break;
        }
    }

    // If newMaxScore was never updated (still at the initial -10000), no
    // move beat that threshold — typically meaning every pseudo-legal move
    // was illegal (each leaves our own king in check). That's a real
    // checkmate or stalemate. Distinguish via king-attacked check.
    if (newMaxScore == -10000) {
        GenResult opGen  = isWhite ? generateBlackMoves(state) : generateWhiteMoves(state);
        uint8_t   kingSq = findKing(ownPieces, ownCount, state.board);
        return ((opGen.attacks >> MAT_TO_BIT[kingSq]) & 1) ? -1000 * (depth + 1) : 0;
    }

    return newMaxScore;
}

// -----------------------------------------------------------------------
// Opening book
// Used as a fallback when no black move immediately captures a white piece.
// Entries are tried in order; the first one whose move exists in the
// generated set (the piece is still on the expected square, the path is
// free) gets played.
// -----------------------------------------------------------------------

struct OpeningEntry {
    int8_t  fromCol, fromRow;
    int8_t  toCol,   toRow;
    uint8_t pieceType;  // TYPE_* constant from Position.h
};

// Board coordinates: row 0 = black back rank, col 0 = left side of screen
//   1.1  e-pawn   (next to king,   col 4) advances 2 squares          e7→e5
//   1.2  d-pawn   (next to queen,  col 3) advances 1 square           d7→d6
//   1.3  left knight  (col 1) jumps 2 down, 1 right                   b8→c6
//   1.4  left bishop  (col 2) slides 4 squares diagonally right-down  c8→g4
//   1.5  right knight (col 6) jumps 2 down, 1 left                    g8→f6
//   1.6  left knight  (col 1) jumps 2 down, 1 left                    b8→a6
//   1.7  right knight (col 6) jumps 2 down, 1 right                   g8→h6
//   1.8  b-pawn   (col 1) advances 1 square                           b7→b6
//   1.9  a-pawn   (col 0) advances 2 squares                          a7→a5
//   1.10 h-pawn   (col 7) advances 1 square                           h7→h6
//   1.11 g-pawn   (col 6) advances 2 squares                          g7→g5
//   1.12 c-pawn   (col 2) advances 1 square                           c7→c6
//   1.13 e-pawn   (col 4) advances 1 square                           e7→e6
//   1.14 queen    (col 3) slides 4 squares diagonally right-down      d8→h4
//   1.15 queen    (col 3) slides 5 squares straight down              d8→d3
static const OpeningEntry OPENING_BOOK[] = {
    { 4, 1,  4, 3,  TYPE_PAWN   },
    { 3, 1,  3, 2,  TYPE_PAWN   },
    { 1, 0,  2, 2,  TYPE_KNIGHT },
    { 2, 0,  6, 4,  TYPE_BISHOP },
    { 6, 0,  5, 2,  TYPE_KNIGHT },
    { 1, 0,  0, 2,  TYPE_KNIGHT },
    { 6, 0,  7, 2,  TYPE_KNIGHT },
    { 1, 1,  1, 2,  TYPE_PAWN   },
    { 0, 1,  0, 3,  TYPE_PAWN   },
    { 7, 1,  7, 2,  TYPE_PAWN   },
    { 6, 1,  6, 3,  TYPE_PAWN   },
    { 2, 1,  2, 2,  TYPE_PAWN   },
    { 4, 1,  4, 2,  TYPE_PAWN   },
    { 3, 0,  7, 4,  TYPE_QUEEN  },
    { 3, 0,  3, 5,  TYPE_QUEEN  },
};


// -----------------------------------------------------------------------
// startThinking — dispatch one thread-pool job per black candidate move
// -----------------------------------------------------------------------

void ChessAI::startThinking(const Board& board, const CastlingRights& /*castling*/, int depth) {
    rootState = AIState::fromBoard(board);
    GenResult blackGen = generateBlackMoves(rootState);
    sortMoves(blackGen.moves, rootState.board);

    pending.clear();
    pending.reserve(blackGen.moves.count);

    for (int i = 0; i < blackGen.moves.count; ++i) {
        const uint8_t from = ::Move::getFrom(blackGen.moves.moves[i].data);
        const uint8_t to   = ::Move::getTo  (blackGen.moves.moves[i].data);

        // Output move in board coordinates (col=x, row=y; row 0 = black back rank)
        ChessAI::Move aiMove;
        aiMove.from = sf::Vector2i(from % 12 - 2, from / 12 - 2);
        aiMove.to   = sf::Vector2i(to   % 12 - 2, to   / 12 - 2);

        // Copy root state and apply this black move to the copy
        AIState copy              = rootState;
        const uint8_t movingPiece = copy.board[from];

        // Flag the renderer side that this is a promotion (black pawn →
        // matrix row 9). The renderer swaps the piece type when the
        // animation completes.
        if (movingPiece == B_PAWN && to >= 110 && to <= 117) {
            aiMove.promo = static_cast<uint8_t>(PieceType::Queen);
        }
        const uint8_t captured    = copy.board[to];
        int capturedValue         = (captured == EMPTY) ? 0 : (int)scoreOf(captured) + depth / 2;

        copy.board[to]   = movingPiece;
        copy.board[from] = EMPTY;
        {
            int idx = findPiece(copy.blackPieces, copy.blackCount, from);
            copy.blackPieces[idx] = to;
        }
        if (captured != EMPTY) {
            int idx = findPiece(copy.whitePieces, copy.whiteCount, to);
            removePiece(copy.whitePieces, copy.whiteCount, idx);
        }

        // Promotion — at the root the AI plays black. Matrix row 9 = real
        // squares 110..117 (see getMaxPoints for details).
        if (movingPiece == B_PAWN && to >= 110 && to <= 117) {
            copy.board[to] = B_QUEEN;
            capturedValue += (int)scoreOf(B_QUEEN) - (int)scoreOf(B_PAWN);
        }

        // Evaluate this root move:
        //   score = capturedValue − getMaxPoints(child)
        auto future = pool.submit([c = std::move(copy), capturedValue, depth]() mutable {
            return capturedValue - getMaxPoints(c, depth - 1, capturedValue, -10000, true);
        });

        pending.push_back({ aiMove, std::move(future) });
    }

    thinking = !pending.empty();
}

// -----------------------------------------------------------------------
// poll — check whether every worker thread finished, return the best move
// -----------------------------------------------------------------------

bool ChessAI::poll(Move& outMove) {
    if (!thinking) return false;

    // Non-blocking check: are all futures ready?
    for (auto& p : pending)
        if (p.score.wait_for(std::chrono::seconds(0)) != std::future_status::ready)
            return false;

    // All done — collect results
    struct Result { Move move; int score; };
    std::vector<Result> results;
    results.reserve(pending.size());
    for (auto& p : pending)
        results.push_back({ p.move, p.score.get() });

    thinking = false;
    pending.clear();

    // Drop moves that would put white in stalemate — but only if at least
    // one alternative scores > -900 (= isn't a forced loss). Mate values
    // are ≤ -1000*(depth+1), so -900 is a safe threshold.
    {
        std::vector<size_t> stalemateIdx;
        for (size_t i = 0; i < results.size(); ++i)
            if (causesWhiteStalemate(rootState, results[i].move))
                stalemateIdx.push_back(i);

        if (!stalemateIdx.empty()) {
            // Is there a non-stalemate alternative that isn't a losing line?
            bool haveSafeAlternative = false;
            for (size_t i = 0; i < results.size(); ++i) {
                bool isStalemateMove = false;
                for (size_t k : stalemateIdx) if (k == i) { isStalemateMove = true; break; }
                if (isStalemateMove) continue;
                if (results[i].score > -900) { haveSafeAlternative = true; break; }
            }

            if (haveSafeAlternative) {
                std::vector<Result> filtered;
                filtered.reserve(results.size() - stalemateIdx.size());
                for (size_t i = 0; i < results.size(); ++i) {
                    bool drop = false;
                    for (size_t k : stalemateIdx) if (k == i) { drop = true; break; }
                    if (!drop) filtered.push_back(results[i]);
                }
                results = std::move(filtered);
            }
        }
    }

    int bestScore  = INT_MIN + 1;
    int worstScore = INT_MAX;
    int i = 0;
    for (auto& r : results) {
        std::cout << "Move " << i << " score: " <<r.score << std::endl;
        if (r.score > bestScore)  bestScore  = r.score;
        if (r.score < worstScore) worstScore = r.score;
    }

    // 1. Some move wins material outright — play the best one immediately.
    if (bestScore > 0) {
        for (auto& r : results)
            if (r.score == bestScore) {
                std::cout << "Best Move " << i << " score: " << r.score << std::endl;
                outMove = r.move;
                return true;
            }
    }

    // 2. Opening book — only in quiet positions where no move is losing.
    //    If some move scores noticeably worse than others (a tactical
    //    threat), skip the book and let the alpha-beta best move handle it.
    if (bestScore == 0) {
        for (const auto& e : OPENING_BOOK) {
            sf::Vector2i bookFrom(e.fromCol, e.fromRow);
            sf::Vector2i bookTo  (e.toCol,   e.toRow);
            for (auto& r : results) {
                if (r.move.from == bookFrom && r.move.to == bookTo && r.score == bestScore) {
                    std::cout << "Best Opening Move " << i << " score: " << r.score << std::endl;
                    outMove = r.move;
                    return true;
                }
            }
        }

        // 2b. No book entry matched — alpha-beta has no preference between
        //     score==0 moves. Use a heuristic:
        //       bonus = (vertical advance toward white)
        //             + (defended black pieces after − before)
        //     and pick the move with the largest bonus.
        const int protectedBefore = countProtectedBlackPieces(rootState);
        int  bestBonus     = 0;
        Move bestBonusMove;

        for (auto& r : results) {
            if (r.score != 0) continue;

            // Apply r.move to a copy of the root state (same logic as
            // startThinking). Board coord → matrix index: (y+2)*12 + (x+2).
            AIState copy = rootState;
            const uint8_t fromMat = (uint8_t)((r.move.from.y + 2) * 12 + (r.move.from.x + 2));
            const uint8_t toMat   = (uint8_t)((r.move.to.y   + 2) * 12 + (r.move.to.x   + 2));

            const uint8_t captured = copy.board[toMat];
            copy.board[toMat]   = copy.board[fromMat];
            copy.board[fromMat] = EMPTY;
            {
                int idx = findPiece(copy.blackPieces, copy.blackCount, fromMat);
                copy.blackPieces[idx] = toMat;
            }
            if (captured != EMPTY) {
                int idx = findPiece(copy.whitePieces, copy.whiteCount, toMat);
                removePiece(copy.whitePieces, copy.whiteCount, idx);
            }

            // Black advances toward white as y grows (row 0 = black side,
            // row 7 = white side). Retreating moves get a negative bonus.
            // Pawns get 2× weight — encourages central/wing pawn development
            // in the opening.
            const bool isPawn = typeOf(rootState.board[fromMat]) == TYPE_PAWN;
            const int  raw    = r.move.to.y - r.move.from.y;
            const int  advance        = raw * (isPawn ? 2 : 1);
            const int  protectedAfter = countProtectedBlackPieces(copy);
            const int  bonus          = advance + 2*(protectedAfter - protectedBefore);

            if (bonus > bestBonus) {
                bestBonus     = bonus;
                bestBonusMove = r.move;
            }
        }

        if (bestBonus > 0) {
            std::cout << "Best Heuristic move bonus: " << bestBonus << std::endl;
            outMove = bestBonusMove;
            return true;
        }
    }

    // 3. Fallback: best alpha-beta move.
    for (auto& r : results) {
        if (r.score == bestScore) {
            std::cout << "Best Fallback Move " << i << " score: " << r.score << std::endl;
            outMove = r.move;
            return true;
        }
    }

    return true;
}
