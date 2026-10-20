# How do the C++ core and Metal presentation fit together?

**Short answer:** the simulation is a headless value-owning C++ library. The
AppKit layer turns keyboard input into explicit commands, advances the
simulation at a fixed 60 Hz, and asks a C++ scene to compose a frame. A thin
Objective-C++ class submits that frame to Metal. The core never includes a
window or GPU header.

## The data flow

1. [GameView.mm](../../Sources/Platform/GameView.mm) — `SIGameView` is the `MTKView`
   delegate. Its `drawInMTKView` method measures elapsed time and runs zero or
   more fixed `1/60` second updates.
2. Each update mutates only [Game.hpp](../../Sources/Core/include/invaders/Game.hpp) —
   `Game`. Input commands such as `setFiring` and `setMoveLeft` are queued
   through the same fixed loop.
3. The core appends small [Event](../../Sources/Core/include/invaders/Types.hpp)
   — `Event` — values (`AlienHit`, `PlayerHit`, `UfoHit`, and so on). The view drains them
   once per step, forwards them to procedural audio, and passes them to the
   scene for particles and score popups.
4. [Scene.cpp](../../Sources/Render/Scene.cpp) — `Scene::build` reads the current game
   state and writes a triangle list. It does not mutate the game.
5. [MetalRenderer.mm](../../Sources/Render/MetalRenderer.mm) — `SIRenderer` uploads
   the vertex list, encodes one draw call, and presents the drawable.

## Why the event stream exists

The renderer could infer that an alien disappeared by comparing frames, but it
would then have to duplicate collision knowledge to know where the explosion
belongs. Events carry the minimum presentation facts—type, position, points—so
the renderer stays a consumer of rules rather than a second implementation of
them.

The same events drive audio. The simulation does not know whether the machine
has a speaker; it only reports that a shot was fired or a ship was destroyed.

## Why Objective-C++ lives at one edge

Metal resources are Objective-C protocols. Rather than exposing raw Metal
handles through a C++ facade, the project allows exactly one Objective-C++ class
to own GPU state. The scene itself is portable C++, which keeps the drawing code
testable and the boundary small. The [C++ design record](../CPP_DESIGN.md)
explains this trade-off in more detail.

## What this buys

- The checks executable runs without a window server, Metal device, or audio
  route.
- Rendering bugs cannot change the score, alien positions, or RNG.
- The scene can be rebuilt or replaced without touching gameplay rules.
