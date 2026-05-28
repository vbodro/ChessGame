#pragma once
#include <SFML/System/Vector2.hpp>
#include "../Board.h"
#include "ThreadPool.h"
#include "AIState.h"
#include <future>
#include <vector>

class ChessAI {
public:
    struct Move {
        sf::Vector2i from  = {-1, -1};
        sf::Vector2i to    = {-1, -1};
        uint8_t      promo = 0;
        bool isValid() const { return from.x != -1; }
    };

    ChessAI() = default;

    // Kicks off a parallel evaluation of every black candidate move.
    // depth = number of plies of regular alpha-beta search (getMaxPoints).
    // After that, quiescence search (capture-only) continues for 4 more plies.
    void startThinking(const Board& board, const CastlingRights& castling, int depth = 6);

    // Non-blocking. Returns true (and fills outMove) once all workers are done.
    bool poll(Move& outMove);

    bool isThinking() const { return thinking; }

private:
    ThreadPool pool;

    struct PendingMove {
        Move             move;
        std::future<int> score;
    };
    std::vector<PendingMove> pending;
    bool thinking = false;

    // Snapshot of the position taken at startThinking — poll uses it for
    // the heuristic that picks between score == 0 moves.
    AIState rootState;
};
