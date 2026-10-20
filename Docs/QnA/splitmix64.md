# Why is a SplitMix64 implementation needed?

**Short answer:** the simulation has to make random decisions that are
*reproducible*: the same seed must produce the same run on every machine and
every standard library. No standard facility guarantees that, so the project
carries a ~30-line header-only generator whose algorithm is spelled out in the
repository itself.

## What actually needs randomness

[Game.cpp](../../Sources/Core/Game.cpp) — `Game::updateEnemyFire` and
`Game::spawnEnemyShot` pick the fire cadence and the column that shoots;
`Game::updateUfo` picks the mystery ship's timer, direction, and points. All of
it comes from a single `rng_` member, seeded once from `GameOptions::seed` in
the `Game` constructor. Nothing reads the wall clock or a global device.

## Why not `std::rand`, `arc4random`, or `std::random_device`

Each of those fails at least one requirement:

- `std::rand` has process-global state, an unspecified sequence, and usually a
  small period. Two `Game` instances in one process would interfere.
- `arc4random` is a platform API with no seed, so a run cannot be repeated.
- `std::random_device` is deliberately non-deterministic and may block on
  entropy; the simulation must be headless and identical everywhere.

## Why not `std::mt19937_64` with a distribution

The engine's own output is standardised, but the *distributions* are not: the
standard leaves the algorithm for `std::uniform_int_distribution` and
`std::uniform_real_distribution` unspecified. Two standard libraries can map the
same engine bits to different columns or different float values, which would
quietly change which alien fires on a given frame. MT19937 also carries a
2.5 KB state for decisions this game resolves in a handful of draws per second.

[`Rng.hpp`](../../Sources/Core/include/invaders/Rng.hpp) — `SplitMix64` is
therefore fully defined in the repository: a 64-bit counter plus two xor-shift
multiply rounds. Same code on Apple Clang, GCC, or any future toolchain, so the
bit stream is fixed by the source, not by the library.

## What the wrappers are for

- `next` returns the raw 64-bit value and advances the counter.
- `unit` takes the top 24 bits and scales by 1/2²⁴, giving `[0, 1)` and
  deliberately never rounding up to `1.0`, which matters when the result feeds
  timers.
- `range` returns an inclusive integer span, used for "which surviving column"
  and "which of the four ship scores".

Both are `constexpr`, allocation-free, and hold only eight bytes of state, so a
`Game` stays trivially replayable and cheap to copy in checks.

## Statistical quality

SplitMix64 has a 2⁶⁴ period, passes the usual empirical test batteries, and
mixes well from a counter, which is exactly the profile needed for a few dozen
draws per wave. It is not cryptographic and does not need to be: nothing here
protects a secret, and predictability is a feature because it enables replay.

## The renderer has its own copy

[Scene.cpp](../../Sources/Render/Scene.cpp) — `Scene::Scene` seeds an
`effectsRng_` for the star field, particle bursts, and score popups. Keeping
presentation randomness separate means a visual flourish can never consume a
simulation draw and change the outcome of a wave.

## The payoff

[`Checks/main.cpp`](../../Checks/main.cpp) — `testDeterminismAndSoak` builds two
`Game` instances with the same seed, drives them with identical inputs for 30
seconds, and asserts the positions, scores, and alien counts match frame by
frame. That test is only meaningful if the generator is specified, seeded, and
identical across platforms, which is what `SplitMix64` buys.
