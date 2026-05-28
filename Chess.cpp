#include <SFML/Graphics.hpp>
#include <vector>
#include <iostream>
#include "Board.h"
#include "rendering/Renderer.h"
#include "rendering/Assets.h"
#include "input/InputHandler.h"
#include "moves/MoveGen.h"
#include "EnemyAI/ChessAI.h"

// Checks whether `sideToMove` is in checkmate or stalemate. Returns the
// message (used both for the GUI banner and the log); an empty string
// means the game is still ongoing.
static std::string checkGameOver(const Board& board, PieceColor sideToMove,
                                 const CastlingRights& castling) {
    if (hasAnyLegalMove(board, sideToMove, castling)) return "";
    if (isKingInCheck(board, sideToMove)) {
        const char* winner = (sideToMove == PieceColor::White) ? "Black" : "White";
        return std::string("Checkmate! ") + winner + " wins.";
    }
    return "Stalemate! Draw.";
}

// Snapshot of the game state — pushed onto the stack after every full move
// pair (white + AI response), so Undo (U key) can revert one pair at a time.
struct HistoryEntry {
    Board          board;
    CastlingRights castling;
};

int main() {
    sf::RenderWindow window(
        sf::VideoMode(WINDOW_W, WINDOW_H),
        "Chess",
        sf::Style::Titlebar | sf::Style::Close
    );
    window.setFramerateLimit(60);

    Assets assets;
    if (!assets.load()) return 1;

    Board          board    = makeStartingBoard();  // TODO: switch back to makeStartingBoard() after testing
    CastlingRights castling;
    DragState      drag;
    PromotionState promotion;

    ChessAI      ai;
    AIAnimState  aiAnim;
    bool         aiThinking = false;
    bool         gameOver   = false;  // true after checkmate or stalemate — blocks input
    std::string  gameOverMessage;     // text for the GUI banner (empty while the game is live)

    // History stack for Undo. Push the initial state right away —
    // history.back() always represents "current state, white to move".
    std::vector<HistoryEntry> history;
    history.push_back({board, castling});

    sf::Clock clock;
    constexpr float ANIM_DURATION = 0.3f;  // seconds for AI piece to slide to destination

    while (window.isOpen()) {
        const float dt = clock.restart().asSeconds();

        // ----------------------------------------------------------------
        // Check if AI has finished computing
        // ----------------------------------------------------------------
        if (aiThinking) {
            ChessAI::Move aiMove;
            if (ai.poll(aiMove)) {
                aiThinking = false;
                if (aiMove.isValid()) {
                    // Remove the moving piece from the board and start animation.
                    // The destination square is also cleared so the captured piece
                    // disappears immediately as the AI piece slides in.
                    aiAnim.piece    = board[aiMove.from.y][aiMove.from.x];
                    aiAnim.from     = aiMove.from;
                    aiAnim.to       = aiMove.to;
                    aiAnim.progress = 0.f;
                    aiAnim.active   = true;
                    aiAnim.promo    = static_cast<PieceType>(aiMove.promo);

                    board[aiMove.from.y][aiMove.from.x] = {};
                    board[aiMove.to.y  ][aiMove.to.x  ] = {};
                } else {
                    // AI has no moves (mate or stalemate) — game over.
                    // Push current state so Undo still works from this position.
                    history.push_back({board, castling});
                }
            }
        }

        // ----------------------------------------------------------------
        // Advance animation
        // ----------------------------------------------------------------
        if (aiAnim.active) {
            aiAnim.progress += dt / ANIM_DURATION;
            if (aiAnim.progress >= 1.f) {
                // Animation complete — place piece at destination
                board[aiAnim.to.y][aiAnim.to.x] = aiAnim.piece;

                // Promotion: swap the piece type (pawn becomes queen on last rank)
                if (aiAnim.promo != PieceType::None) {
                    board[aiAnim.to.y][aiAnim.to.x].type = aiAnim.promo;
                }

                aiAnim = {};

                // Full move pair (white + AI) finished — record the new state
                history.push_back({board, castling});

                // Black just moved — check whether white is mated or stalemated.
                std::string msg = checkGameOver(board, PieceColor::White, castling);
                if (!msg.empty()) {
                    std::cout << msg << std::endl;
                    gameOver        = true;
                    gameOverMessage = msg;
                }
            }
        }

        // ----------------------------------------------------------------
        // Input — blocked while AI is computing or animating
        // ----------------------------------------------------------------
        const bool aiActive = aiThinking || aiAnim.active;

        sf::Event event;
        while (window.pollEvent(event)) {
            switch (event.type) {
            case sf::Event::Closed:
                window.close();
                break;

            case sf::Event::MouseButtonPressed:
                if (event.mouseButton.button == sf::Mouse::Left) {
                    // Undo button — works even when the game is over (only way
                    // out of game-over). Disabled while the AI is busy.
                    if (!aiActive && isUndoButtonHit(event.mouseButton.x, event.mouseButton.y)) {
                        if (history.size() > 1) {
                            history.pop_back();
                            const auto& prev = history.back();
                            board    = prev.board;
                            castling = prev.castling;
                            drag     = {};
                            promotion = {};
                            gameOver = false;  // game continues
                            gameOverMessage.clear();
                        }
                    }
                    else if (!aiActive && !gameOver) {
                        if (promotion.isPending) {
                            if (handlePromotionClick(event.mouseButton, board, promotion)) {
                                std::string msg = checkGameOver(board, PieceColor::Black, castling);
                                if (!msg.empty()) {
                                    std::cout << msg << std::endl;
                                    gameOver        = true;
                                    gameOverMessage = msg;
                                    history.push_back({board, castling});
                                } else {
                                    ai.startThinking(board, castling);
                                    aiThinking = true;
                                }
                            }
                        } else {
                            handleMousePressed(event.mouseButton, board, drag, castling, promotion);
                        }
                    }
                }
                break;

            case sf::Event::MouseButtonReleased:
                if (!aiActive && !gameOver && event.mouseButton.button == sf::Mouse::Left) {
                    if (handleMouseReleased(event.mouseButton, board, drag, castling, promotion)) {
                        // White just moved — check whether black is mated or stalemated.
                        std::string msg = checkGameOver(board, PieceColor::Black, castling);
                        if (!msg.empty()) {
                            std::cout << msg << std::endl;
                            gameOver        = true;
                            gameOverMessage = msg;
                            history.push_back({board, castling});  // enable undo
                        } else {
                            ai.startThinking(board, castling);
                            aiThinking = true;
                        }
                    }
                }
                break;

            case sf::Event::MouseMoved:
                handleMouseMoved(event.mouseMove, drag);
                break;

            case sf::Event::KeyPressed:
                // Undo (U key): revert to the previous "white to move" moment.
                // Works even when the game is over (only way to continue).
                // Disabled while the AI is busy.
                if (event.key.code == sf::Keyboard::U && !aiActive && history.size() > 1) {
                    history.pop_back();                  // drop current state
                    const auto& prev = history.back();   // new top = previous state
                    board    = prev.board;
                    castling = prev.castling;
                    drag     = {};                       // cancel any drag
                    promotion = {};                      // cancel pending promotion
                    gameOver = false;                    // game continues
                    gameOverMessage.clear();
                }
                break;

            default: break;
            }
        }

        // ----------------------------------------------------------------
        // Render
        // ----------------------------------------------------------------
        window.clear();
        renderBoard(window, assets);
        renderHighlights(window, drag, board);
        renderPieces(window, assets, board, drag);
        renderAIMove(window, assets, aiAnim);
        renderDraggedPiece(window, assets, drag);
        renderPromotion(window, assets, promotion);
        // Side panel with Undo button — drawn after the board so it stays on top
        renderSidePanel(window, !aiActive && history.size() > 1);
        // Game-over banner over the board (topmost layer above the panel)
        if (gameOver) renderGameOver(window, assets, gameOverMessage);
        window.display();
    }

    return 0;
}
