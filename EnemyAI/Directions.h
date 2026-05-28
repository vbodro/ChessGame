#pragma once

// Movement offsets in the 12x12 matrix (row-major, 12 cols per row)

namespace Dir {
    constexpr int UP         = -12;
    constexpr int DOWN       = +12;
    constexpr int LEFT       =  -1;
    constexpr int RIGHT      =  +1;
    constexpr int UP_LEFT    = -13;
    constexpr int UP_RIGHT   = -11;
    constexpr int DOWN_LEFT  = +11;
    constexpr int DOWN_RIGHT = +13;
}

namespace KnightDir {
    constexpr int UP2_LEFT1    = -25;
    constexpr int UP2_RIGHT1   = -23;
    constexpr int UP1_LEFT2    = -14;
    constexpr int UP1_RIGHT2   = -10;
    constexpr int DOWN1_LEFT2  = +10;
    constexpr int DOWN1_RIGHT2 = +14;
    constexpr int DOWN2_LEFT1  = +23;
    constexpr int DOWN2_RIGHT1 = +25;
}
