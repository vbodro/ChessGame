# Chess

> Built collaboratively with [Claude Code](https://claude.com/claude-code),
> Anthropic's AI coding agent. Most non-trivial design decisions — the
> captured-minus-search score scheme, the quiescence integration, the stalemate
> filter, the GUI panels — went through several iterations of prompting,
> review, and refactoring inside the editor before landing in the form they're
> in here.

A C++ chess implementation with an SFML-rendered GUI and a custom AI opponent.

## Engine highlights

- **Custom alpha-beta-style pruning** with explicit captured-piece scoring at each
  level instead of the usual negamax sign flip. The recursion is `score = captured
  − search(child)`, which keeps the material flow visible directly in the score.
- **3 moves of full-width search + 2 moves of quiescence**. At the leaf the
  search transitions into a capture-only continuation that resolves pending
  exchanges and removes the horizon effect.
- **Multithreaded root**. Every candidate move is dispatched to a thread pool
  sized to the host's logical core count, so the search scales with available
  hardware. The shared state is per-job (move state is captured by value into
  each task), so there's no locking on the hot path.
- **Cache-friendly board layout**: a 12×12 mailbox with sentinel borders, a
  one-byte piece encoding that packs color, type, and capture score into the
  same bits, and a 144-entry `constexpr` lookup table that replaces the
  divide-by-12 needed when projecting matrix indices onto a 64-bit attack mask.
- **Single-pass move generation**. Legal targets and attacked squares are
  produced together; the legality check at the next level just tests one bit
  of the attacker's mask against the king square.
- **Opening book with a positional heuristic fallback**. When the search has no
  preference among quiet moves, an in-engine heuristic scores them by vertical
  advance toward the opponent and the change in defended pieces.
- **Stalemate avoidance**. The engine drops moves that would put the opponent
  in stalemate unless every alternative is a losing line.
- **Depth-aware mate scoring**. Shorter forced mates are scored as more
  extreme than equivalent longer ones, so the AI converts an advantage instead
  of shuffling its king.

## Game features

- Drag-and-drop input with legal-move filtering that respects pins and castling.
- Pawn promotion to queen for both sides, with an animated transition on the AI move.
- Undo button (or `U` key) that walks back any number of full move pairs.
- In-window game-over banner with system-font fallback.

## Building

### Prerequisites
- Visual Studio 2022 (or later) with the **Desktop development with C++**
  workload installed.
- [SFML 2.6.2](https://www.sfml-dev.org/download/sfml/2.6.2/) — the
  *Visual C++ 17, 64-bit* prebuilt package.

### SFML setup
The Visual Studio project expects SFML at the absolute path
`C:\MyProjects\SFML\SFML-2.6.2\`, with the standard `include/`, `lib/`, and
`bin/` subfolders. The simplest path is to mirror that layout — clone this
repo next to an `SFML\SFML-2.6.2\` folder, e.g.:

```
C:\MyProjects\
    Chess\          ← this repo
    SFML\
        SFML-2.6.2\
            bin\
            include\
            lib\
```

If you place SFML elsewhere, open `Chess.vcxproj` and adjust the two entries
under the `x64` configurations:
- `AdditionalIncludeDirectories` → your `<SFML>/include`
- `AdditionalLibraryDirectories` → your `<SFML>/lib`

### Build
1. Open `Chess.slnx` in Visual Studio.
2. Pick the **`Release | x64`** configuration (`Debug | x64` works too; the
   `Win32` configurations don't link SFML and won't build the renderer).
3. Build the solution (`Ctrl+Shift+B`).

The project compiles as C++20 (`/std:c++20`) under the v145 platform toolset.

### Runtime
The executable needs the SFML runtime DLLs alongside `Chess.exe`. For
`Release | x64`, copy these from SFML's `bin/` directory into the output
folder (`x64\Release\` next to the exe):

- `sfml-graphics-2.dll`
- `sfml-window-2.dll`
- `sfml-system-2.dll`

For `Debug | x64`, use the `-d` variants
(`sfml-graphics-d-2.dll`, etc.). Alternatively, add SFML's `bin/` directory
to your `PATH` and skip the copy step.

The game also needs the `assets/` folder to be in the working directory.
Visual Studio runs the exe from the project root by default, so this works
out of the box; if you copy the exe elsewhere, copy `assets/` with it.
