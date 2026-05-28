#include "Assets.h"
#include <string>

bool Assets::load() {
    if (!board.loadFromFile("assets/board.png"))
        return false;

    const char* names[6]  = { "K", "Q", "R", "B", "N", "P" };
    const char* colors[2] = { "w", "b" };

    for (int c = 0; c < 2; ++c) {
        for (int t = 0; t < 6; ++t) {
            std::string path = std::string("assets/") + colors[c] + names[t] + ".png";
            if (!pieces[c][t].loadFromFile(path))
                return false;
            pieces[c][t].setSmooth(true);
        }
    }

    // Font: try the project-local assets/arial.ttf first, then the Windows
    // system font. Failure is not fatal — GUI text is simply skipped (the
    // gameOver message still goes to stdout).
    fontLoaded = font.loadFromFile("assets/arial.ttf")
              || font.loadFromFile("C:/Windows/Fonts/arial.ttf");

    return true;
}

const sf::Texture& Assets::getPiece(PieceColor color, PieceType type) const {
    int c = (color == PieceColor::White) ? 0 : 1;
    int t = static_cast<int>(type) - 1; // PieceType starts at 1 (King=1)
    return pieces[c][t];
}
