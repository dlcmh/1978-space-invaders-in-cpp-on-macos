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

## Where the name comes from

The name is a three-part description of the algorithm, published by Guy L.
Steele Jr., Doug Lea, and Christine H. Flood in
[Fast Splittable Pseudorandom Number Generators](https://doi.org/10.1145/2660193.2660195)
(OOPSLA 2014):

- **Split** — the generator is *splittable*. An existing instance can mint a new
  independent instance (a new seed and a new increment) without touching global
  state. That was the point of the paper: fork/join tasks need their own streams,
  and Java 8 shipped the design as `java.util.SplittableRandom` for parallel
  streams.
- **Mix** — each output is a counter value pushed through a bijective bit-mixing
  function, rather than the raw state. SplitMix's state transition is only a
  Weyl sequence (repeated addition of an odd constant), so the mixing step is
  what makes the output look random.
- **64** — the concrete instantiation uses one 64-bit word of state and returns
  64 bits.

The paper derives SplitMix from DOTMIX (Leiserson, Schardl, and Sukha's
deterministic parallel RNG), and the name reads as the successor: DOTMIX's dot
products and coefficient tables are gone, replaced by splitting and mixing.

The pieces inside `next` are older than the name:

- the added constant `0x9E37'79B9'7F4A'7C15` is the golden-ratio increment,
  `floor(2⁶⁴ / φ)` — a Weyl sequence chosen so that successive states are
  equidistributed; the paper calls it gamma and defaults to it;
- the two multiply/xor rounds are David Stafford's *Mix13*, the thirteenth of the
  candidate finalizers in
  [Better Bit Mixing](http://zimbry.blogspot.com/2011/09/better-bit-mixing-improving-on.html)
  (2011), which the paper cites as `mix64variant13`. The project's `SplitMix64`
  uses Mix13 directly as the output mixer, the form that spread through C, Rust,
  and Haskell libraries and into the xoshiro family as a seeding function.

A footnote worth knowing: the `mixGamma` code printed in the paper contained an
inverted test, which M. E. O'Neill documented in
[Bugs in SplitMix(es)](https://www.pcg-random.org/posts/bugs-in-splitmix.html)
(2017); Java's implementation was written earlier and was correct. This project
never hits the issue, because `SplitMix64` implements no splitting and no gamma
repair: it always runs the one golden increment.

The default seed here, `0x1978'5ACE'CAFE'BEEF`, is this project's own joke —
1978, SPACE, and a well-known filler word — not part of the published algorithm.

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
