# How do the fixed-step loop and keyboard input work?

**Short answer:** AppKit delivers key events to `SIGameView`, which translates
them into explicit simulation commands. The display-linked draw callback
accumulates wall-clock time and feeds the simulation exact 1/60 second updates.

## Input

[GameView.mm](../../Sources/Platform/GameView.mm) — `keyDown` maps:

- left arrow / A to `setMoveLeft(true)`;
- right arrow / D to `setMoveRight(true)`;
- Space to "start" on attract/game-over screens or `setFiring(true)` in play;
- Return to "start" on attract/game-over;
- P to `togglePause`;
- Escape to `pause`; and
- M to the persisted mute toggle.

`keyUp` releases held movement and firing. `Make First Responder` is set when
the view moves into the window. The simulation never sees an `NSEvent`; it only
sees booleans and commands.

## The loop

`drawInMTKView` computes a frame delta, clamps it to 0.1 seconds, and adds it to
an accumulator. While the accumulator holds at least one fixed step—up to
eight—the view:

1. calls `Game::update(1/60)`;
2. drains events to audio and the renderer's effect queue; and
3. subtracts one step.

After eight steps the accumulator is reset, preventing a long stall (debugger,
window drag) from producing an unbounded catch-up spiral. Rendering happens once
per callback regardless of how many simulation steps ran.

## Why this matters

A 120 Hz display may render twice as often, but it runs the same number of
simulation steps per wall-clock second. A slow display that misses frames may
run a couple of catch-up steps. Either way, alien march intervals, projectile
speeds, and RNG sequences are expressed in simulation steps and remain stable.

The checks executable uses the same `1/60` step directly. It does not need a
window, a Metal device, or AppKit.

## Pause and focus

The app auto-pauses on `NSApplicationDidResignActiveNotification` unless
`SPACEINVADERS_AUTOPAUSE=0` is set for development. Pausing clears held input so
a key released while another app was focused cannot remain stuck on resume.
