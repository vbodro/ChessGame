#pragma once
#include "AIState.h"
#include "Move.h"

struct GenResult {
    MoveList moves;
    uint64_t attacks = 0;
};

GenResult generateWhiteMoves(const AIState& state);
GenResult generateBlackMoves(const AIState& state);

// Sorts moves in descending order of capture score (scoreOf(board[to])).
// Quiet moves (empty destination, score=0) are skipped immediately —
// they cannot outrank anything already processed, so no inner loop runs.
// Move is 2 bytes, so copies in the inner loop are cheap.
inline void sortMoves(MoveList& moves, const BoardMatrix& board) {
    for (int i = 1; i < moves.count; ++i) {
        Move     key   = moves.moves[i];
        uint16_t score = scoreOf(board[Move::getTo(key.data)]);

        if (score == 0) continue;  // quiet move — already in correct relative position

        int j = i - 1;
        while (j >= 0 && scoreOf(board[Move::getTo(moves.moves[j].data)]) < score) {
            moves.moves[j + 1] = moves.moves[j];
            --j;
        }
        moves.moves[j + 1] = key;
    }
}
