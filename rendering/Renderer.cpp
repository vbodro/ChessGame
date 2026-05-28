#include "Renderer.h"

void renderBoard(sf::RenderWindow& window, const Assets& assets) {
    sf::Sprite boardSprite(assets.board);
    window.draw(boardSprite);
}

void renderHighlights(sf::RenderWindow& window, const DragState& drag, const Board& board) {
    if (!drag.isDragging) return;

    // Selected square — yellow overlay
    sf::RectangleShape sel({ (float)SQUARE_SIZE, (float)SQUARE_SIZE });
    sel.setFillColor({ 255, 255, 0, 110 });
    sel.setPosition((float)(drag.fromCol * SQUARE_SIZE), (float)(drag.fromRow * SQUARE_SIZE));
    window.draw(sel);

    for (const auto& move : drag.validMoves) {
        int col = move.x, row = move.y;
        bool isCapture = board[row][col].type != PieceType::None;

        if (isCapture) {
            // Green ring around the square for captures
            sf::RectangleShape overlay({ (float)SQUARE_SIZE, (float)SQUARE_SIZE });
            overlay.setFillColor({ 0, 180, 0, 100 });
            overlay.setPosition((float)(col * SQUARE_SIZE), (float)(row * SQUARE_SIZE));
            window.draw(overlay);
        } else {
            // Small dark dot for empty valid squares
            float radius = SQUARE_SIZE * 0.18f;
            sf::CircleShape dot(radius);
            dot.setFillColor({ 0, 0, 0, 75 });
            dot.setPosition(
                col * SQUARE_SIZE + SQUARE_SIZE * 0.5f - radius,
                row * SQUARE_SIZE + SQUARE_SIZE * 0.5f - radius
            );
            window.draw(dot);
        }
    }
}

void renderPieces(sf::RenderWindow& window, const Assets& assets, const Board& board, const DragState& drag) {
    for (int row = 0; row < 8; ++row) {
        for (int col = 0; col < 8; ++col) {
            const Piece& p = board[row][col];
            if (p.type == PieceType::None) continue;

            sf::Sprite sprite(assets.getPiece(p.color, p.type));
            auto texSize = sprite.getTexture()->getSize();
            sprite.setScale(
                (float)SQUARE_SIZE / texSize.x,
                (float)SQUARE_SIZE / texSize.y
            );
            sprite.setPosition((float)(col * SQUARE_SIZE), (float)(row * SQUARE_SIZE));
            window.draw(sprite);
        }
    }
}

void renderPromotion(sf::RenderWindow& window, const Assets& assets, const PromotionState& promotion) {
    if (!promotion.isPending) return;

    // Dark overlay
    sf::RectangleShape overlay({ (float)BOARD_PX, (float)BOARD_PX });
    overlay.setFillColor({ 0, 0, 0, 140 });
    window.draw(overlay);

    bool isWhite   = (promotion.row == 0);
    int  panelStart = isWhite ? 0 : 4;
    int  panelX    = promotion.col * SQUARE_SIZE;
    int  panelY    = panelStart * SQUARE_SIZE;

    // Panel background
    sf::RectangleShape panel({ (float)SQUARE_SIZE, 4.f * SQUARE_SIZE });
    panel.setFillColor({ 235, 235, 210, 245 });
    panel.setOutlineColor({ 60, 60, 60, 255 });
    panel.setOutlineThickness(2.f);
    panel.setPosition((float)panelX, (float)panelY);
    window.draw(panel);

    constexpr PieceType options[4] = {
        PieceType::Queen, PieceType::Rook, PieceType::Bishop, PieceType::Knight
    };
    PieceColor color = isWhite ? PieceColor::White : PieceColor::Black;

    for (int i = 0; i < 4; ++i) {
        sf::Sprite sprite(assets.getPiece(color, options[i]));
        auto texSize = sprite.getTexture()->getSize();
        sprite.setScale(
            (float)SQUARE_SIZE / texSize.x,
            (float)SQUARE_SIZE / texSize.y
        );
        sprite.setPosition((float)panelX, (float)(panelY + i * SQUARE_SIZE));
        window.draw(sprite);
    }
}

void renderAIMove(sf::RenderWindow& window, const Assets& assets, const AIAnimState& anim) {
    if (!anim.active) return;

    const float t    = anim.progress;
    const float fromX = static_cast<float>(anim.from.x * SQUARE_SIZE);
    const float fromY = static_cast<float>(anim.from.y * SQUARE_SIZE);
    const float toX   = static_cast<float>(anim.to.x   * SQUARE_SIZE);
    const float toY   = static_cast<float>(anim.to.y   * SQUARE_SIZE);

    sf::Sprite sprite(assets.getPiece(anim.piece.color, anim.piece.type));
    auto texSize = sprite.getTexture()->getSize();
    sprite.setScale(
        static_cast<float>(SQUARE_SIZE) / texSize.x,
        static_cast<float>(SQUARE_SIZE) / texSize.y
    );
    sprite.setPosition(fromX + (toX - fromX) * t, fromY + (toY - fromY) * t);
    window.draw(sprite);
}

void renderSidePanel(sf::RenderWindow& window, bool undoEnabled) {
    // Side panel background
    sf::RectangleShape panel({ (float)PANEL_WIDTH, (float)WINDOW_H });
    panel.setPosition((float)BOARD_PX, 0.f);
    panel.setFillColor({ 200, 200, 200, 255 });
    window.draw(panel);

    // Undo button — outline
    sf::RectangleShape btn({ (float)UNDO_BTN_W, (float)UNDO_BTN_H });
    btn.setPosition((float)UNDO_BTN_X, (float)UNDO_BTN_Y);
    btn.setFillColor(undoEnabled ? sf::Color(235, 235, 235) : sf::Color(180, 180, 180));
    btn.setOutlineColor({ 60, 60, 60, 255 });
    btn.setOutlineThickness(2.f);
    window.draw(btn);

    // Back arrow icon "←"
    const sf::Color iconColor = undoEnabled ? sf::Color(40, 40, 40) : sf::Color(120, 120, 120);
    const float cy = UNDO_BTN_Y + UNDO_BTN_H * 0.5f;   // button center y
    const float cx = UNDO_BTN_X + UNDO_BTN_W * 0.5f;   // button center x

    // Arrow shaft (horizontal rectangle)
    sf::RectangleShape shaft({ 36.f, 6.f });
    shaft.setOrigin(0.f, 3.f);
    shaft.setPosition(cx - 10.f, cy);
    shaft.setFillColor(iconColor);
    window.draw(shaft);

    // Arrow head (triangle)
    sf::ConvexShape head;
    head.setPointCount(3);
    head.setPoint(0, { cx - 26.f, cy });          // tip (left)
    head.setPoint(1, { cx - 10.f, cy - 14.f });   // upper right
    head.setPoint(2, { cx - 10.f, cy + 14.f });   // lower right
    head.setFillColor(iconColor);
    window.draw(head);
}

bool isUndoButtonHit(int x, int y) {
    return x >= UNDO_BTN_X && x < UNDO_BTN_X + UNDO_BTN_W &&
           y >= UNDO_BTN_Y && y < UNDO_BTN_Y + UNDO_BTN_H;
}

void renderGameOver(sf::RenderWindow& window, const Assets& assets, const std::string& message) {
    if (message.empty()) return;

    // Dark semi-transparent overlay across the board
    sf::RectangleShape overlay({ (float)BOARD_PX, (float)BOARD_PX });
    overlay.setFillColor({ 0, 0, 0, 170 });
    window.draw(overlay);

    if (!assets.fontLoaded) return;  // fallback: overlay only, no text

    // Centered banner with the message
    const float bannerH = 90.f;
    sf::RectangleShape banner({ (float)BOARD_PX, bannerH });
    banner.setPosition(0.f, (BOARD_PX - bannerH) * 0.5f);
    banner.setFillColor({ 235, 235, 210, 250 });
    banner.setOutlineColor({ 60, 60, 60, 255 });
    banner.setOutlineThickness(3.f);
    window.draw(banner);

    sf::Text text(message, assets.font, 32);
    text.setFillColor({ 30, 30, 30 });
    text.setStyle(sf::Text::Bold);

    // Center the text within the banner
    auto bounds = text.getLocalBounds();
    text.setOrigin(bounds.left + bounds.width  * 0.5f,
                   bounds.top  + bounds.height * 0.5f);
    text.setPosition(BOARD_PX * 0.5f, BOARD_PX * 0.5f);
    window.draw(text);
}

void renderDraggedPiece(sf::RenderWindow& window, const Assets& assets, const DragState& drag) {
    if (!drag.isDragging) return;

    sf::Sprite sprite(assets.getPiece(drag.piece.color, drag.piece.type));
    auto texSize = sprite.getTexture()->getSize();
    sprite.setScale(
        (float)SQUARE_SIZE / texSize.x,
        (float)SQUARE_SIZE / texSize.y
    );
    // Center piece on cursor
    sprite.setPosition(
        drag.mousePos.x - SQUARE_SIZE * 0.5f,
        drag.mousePos.y - SQUARE_SIZE * 0.5f
    );
    window.draw(sprite);
}
