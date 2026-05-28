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
- Built with C++17 and SFML 2.x; tested on Windows.
