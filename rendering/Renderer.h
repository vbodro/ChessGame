#pragma once
#include <SFML/Graphics.hpp>
#include <string>
#include "../Board.h"
#include "../input/InputHandler.h"
#include "Assets.h"

constexpr int SQUARE_SIZE  = 80;
constexpr int BOARD_PX     = SQUARE_SIZE * 8;
constexpr int PANEL_WIDTH  = 120;             // side panel for buttons
constexpr int WINDOW_W     = BOARD_PX + PANEL_WIDTH;
constexpr int WINDOW_H     = BOARD_PX;

// Position and size of the Undo button (in window coordinates)
constexpr int UNDO_BTN_X = BOARD_PX + 10;
constexpr int UNDO_BTN_Y = 10;
constexpr int UNDO_BTN_W = 100;
constexpr int UNDO_BTN_H = 50;

// -----------------------------------------------------------------------
// AI move animation state
// When active, the moving piece has been removed from the board and is
// rendered separately at the interpolated position between from and to.
// -----------------------------------------------------------------------
struct AIAnimState {
    bool         active   = false;
    sf::Vector2i from     = {-1, -1};  // board col/row of source square
    sf::Vector2i to       = {-1, -1};  // board col/row of destination square
    Piece        piece    = {};
    float        progress = 0.f;       // 0 = at source, 1 = at destination
    PieceType    promo    = PieceType::None;  // != None → swap piece.type at the end of the animation
};

void renderBoard       (sf::RenderWindow& window, const Assets& assets);
void renderHighlights  (sf::RenderWindow& window, const DragState& drag, const Board& board);
void renderPieces      (sf::RenderWindow& window, const Assets& assets, const Board& board, const DragState& drag);
void renderDraggedPiece(sf::RenderWindow& window, const Assets& assets, const DragState& drag);
void renderPromotion   (sf::RenderWindow& window, const Assets& assets, const PromotionState& promotion);
void renderAIMove      (sf::RenderWindow& window, const Assets& assets, const AIAnimState& anim);
void renderSidePanel   (sf::RenderWindow& window, bool undoEnabled);
void renderGameOver    (sf::RenderWindow& window, const Assets& assets, const std::string& message);

// Returns true if (x, y) lies within the Undo button.
bool isUndoButtonHit(int x, int y);
