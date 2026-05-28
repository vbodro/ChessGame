#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include "../Board.h"

struct DragState {
    bool             isDragging = false;
    int              fromRow    = -1;
    int              fromCol    = -1;
    Piece            piece;
    sf::Vector2f     mousePos;
    std::vector<sf::Vector2i> validMoves; // Vector2i(col, row)
};

struct PromotionState {
    bool isPending = false;
    int  row       = -1;
    int  col       = -1;
};

void handleMousePressed   (const sf::Event::MouseButtonEvent& evt, Board& board, DragState& drag, const CastlingRights& castling, const PromotionState& promotion);
// Returns true when a complete white move has been committed to the board.
// Returns false on an invalid drop, or when a pawn promotion is still pending.
bool handleMouseReleased  (const sf::Event::MouseButtonEvent& evt, Board& board, DragState& drag, CastlingRights& castling, PromotionState& promotion);
void handleMouseMoved     (const sf::Event::MouseMoveEvent&   evt, DragState& drag);
// Returns true when the promotion choice has been made and the board is final.
bool handlePromotionClick (const sf::Event::MouseButtonEvent& evt, Board& board, PromotionState& promotion);
