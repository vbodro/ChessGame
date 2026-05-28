#pragma once
#include <SFML/Graphics.hpp>
#include "../Board.h"

struct Assets {
    sf::Texture board;
    sf::Texture pieces[2][6]; // [0=white, 1=black][King=0 .. Pawn=5]
    sf::Font    font;
    bool        fontLoaded = false;  // if false, GUI text rendering is skipped

    bool load();
    const sf::Texture& getPiece(PieceColor color, PieceType type) const;
};
