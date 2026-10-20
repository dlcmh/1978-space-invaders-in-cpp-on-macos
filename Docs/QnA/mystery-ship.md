# How is the mystery ship scored?

**Short answer:** a seeded timer spawns it from one side of the screen; it
crosses at a constant speed and awards one of four canonical values: 50, 100,
150, or 300 points. A player shot that intersects it awards the value and emits
a `UfoHit` event with the position and points.

## Spawning

[Game.cpp](../../Sources/Core/Game.cpp) — `Game::updateUfo` runs only while playing.
The initial timer is 11–21 seconds, chosen from the deterministic RNG. When it
expires, the ship starts just outside the left or right edge at `y = 36` and
picks a direction. It travels 38 logical pixels per second and disappears on
the opposite side.

After the ship leaves or is destroyed, the timer is reset to a 15–25 second
window. On a new wave the timer is reset with the rest of the field.

## Score selection

`Game::nextUfoScore` chooses uniformly from:

```text
50, 100, 150, 300
```

The value is stored on the instance so the hit event and collision resolution
agree. [MetalRenderer.mm](../../Sources/Render/MetalRenderer.mm) — `SIRenderer` is not
involved in the rule; it receives the resulting event and the camera-facing
scene turns it into a magenta burst and a floating score label.

## Hitting it

[Game.cpp](../../Sources/Core/Game.cpp) — `Game::updatePlayerShot` tests the 20 × 8
logical-pixel ship rectangle before checking aliens. On a hit it marks the ship
inactive, adds the stored points, resets the next timer, and emits `UfoHit`. The
renderer draws the ship with a small blinking light row, so it remains readable
against the star field.

Because all randomness comes from `SplitMix64` seeded by `GameOptions`, a
specific game seed produces the same spawn times, directions, and point values
during replay. The checks exploit this to intercept a ship deterministically.
