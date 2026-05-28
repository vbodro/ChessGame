#pragma once
#include <SFML/System/Vector2.hpp>
#include <vector>
#include "../Board.h"

// Returns valid target squares as Vector2i(col, row)
std::vector<sf::Vector2i> getValidMoves(const Board& board, int row, int col, const CastlingRights& castling);

// True if the king of the given color is attacked by the opponent.
bool isKingInCheck(const Board& board, PieceColor kingColor);

// True if the side has at least one legal move (= one that doesn't leave the king in check).
bool hasAnyLegalMove(const Board& board, PieceColor color, const CastlingRights& castling);
