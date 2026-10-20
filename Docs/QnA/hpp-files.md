# What is a `.hpp` file?

**Short answer:** `.hpp` is a C++ **header**: a text file that the preprocessor
pastes into every `.cpp` or `.mm` that includes it, so those files can compile
against a type without seeing how it works. The extension is a convention, not a
language rule — `.hpp` marks the file as C++ and pairs with the `.cpp` that
supplies the definitions.

## The preprocessor copies, it does not link

`Bunker.cpp` opens with `#include "invaders/Bunker.hpp"`, and
[Game.hpp](../../Sources/Core/include/invaders/Game.hpp) — `Game::bunkers`
returns `const std::array<Bunker, 4>&`, so the complete class definition has to
be visible there too. Each `.cpp` plus everything it includes is one
**translation unit**, compiled on its own. The compiler never sees "a header"; it
sees one long source file.

That is also why `Bunker.cpp` includes its own header first: it makes the
compiler check the declaration against the definition, so a signature that
drifts in one place fails to compile in the other.

## What is actually in `Bunker.hpp`

- `#pragma once` — an include guard. Including the file twice in one translation
  unit would otherwise redefine the class. It is not ISO C++ but is supported by
  every compiler this project targets; the classic `#ifndef` guard is the
  portable alternative.
- `class Bunker` — the **declaration**: the 22 × 16 cell grid (`kColumns`,
  `kRows`), 2-unit cells (`kCellSize`, so 44 × 32 logical units), the private
  data (`origin_`, `cells_`, `remaining_`), and the signature of every operation.
- Inline getters such as `origin`, `remainingCells`, and `cells` — defined inside
  the class, so they are implicitly `inline` and may appear in every translation
  unit that includes the header.
- **Declarations only** for the expensive operations — `carve`, `hitTest`, and
  `hitPoint` are named here, but their bodies live in `Bunker.cpp`.
- `makeBunkers`, also declared here and defined in `Bunker.cpp`.

Nothing about the carving algorithm is visible to a caller, so the header is the
contract: `Game` can own four bunkers and the renderer can call `cellRect`
without knowing how `isShapeCell` draws the silhouette.

## The one-definition rule

A class definition may appear in many translation units as long as every copy is
identical. A non-inline function body may appear exactly **once** in the whole
program. That rule explains the layout:

- `Bunker.cpp` — `carve` and friends are defined once, so they are not marked
  `inline` and live in exactly one object file.
- [`Rng.hpp`](../../Sources/Core/include/invaders/Rng.hpp) — `SplitMix64` defines
  `next`, `unit`, and `range` inside the class, so they are implicitly inline and
  the header is self-sufficient. Header-only is legal; it just means every
  includer re-parses the code.
- If `carve`'s body were pasted into the header, every `.cpp` that includes it
  would emit the same symbol and the linker would report a duplicate definition.

## Why `.hpp` and where the files live

The extension is style: C++ does not care whether it is `.hpp`, `.h`, or `.inc`.
`.hpp` makes C++ headers obvious next to C headers and gives editors the right
language mode. This repository uses `.hpp` throughout:

- public simulation API in `Sources/Core/include/invaders/` — `Bunker`, `Game`,
  `Rng`, `Types`;
- renderer internals beside their sources — `Scene`, `Canvas`, `PixelFont`,
  `MetalShaders`;
- `AudioEngine` under `Sources/Platform/`.

The split is enforced by the build. [`CMakeLists.txt`](../../Sources/Core/CMakeLists.txt)
— `target_include_directories` exposes `Sources/Core/include` publicly, so a
consumer writes `#include "invaders/Bunker.hpp"` and never a relative path. The
render target instead exposes its own directory, which is why
`Sources/Render/Scene.cpp` can use `#include "Scene.hpp"`. Both forms resolve
through the same mechanism: the compiler is told which directories to search.

Each portable header is also compiled as its own translation unit by
[`HeaderChecks.cmake`](../../cmake/HeaderChecks.cmake) —
`space_invaders_add_header_checks`, so a missing direct include cannot hide
behind a transitive one.

Headers hold more than declarations. [`MetalShaders.hpp`](../../Sources/Render/MetalShaders.hpp)
— `kMetalShaderSource` is the entire Metal Shading Language program stored as a
string, because it is compiled at runtime and must ship inside the binary.

## What a header costs

Every includer re-reads it, and editing it recompiles all of them. That is the
price of header-only code, and the reason the small, stable `SplitMix64` lives in
a header while the larger `Game`, `Bunker`, and `Scene` keep their algorithms in
`.cpp` files. A header is also not a module: after preprocessing it leaves no
trace, so there is no runtime object, no linkage, and no cost beyond compile time.
