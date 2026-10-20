# Why does the alien march speed up as they are destroyed?

**Short answer:** the march interval is derived from the fraction of the
formation still alive. A full formation ticks about once every 0.755 seconds;
one remaining alien ticks roughly ten times faster. Each wave also shortens the
interval.

## The rule

[Game.cpp](../../Sources/Core/Game.cpp) — `Game::marchInterval` computes:

```text
density  = aliensRemaining / 55
base     = 0.075 + 0.68 * density^1.35
interval = base / (1 + 0.16 * (wave - 1))
```

At 55 aliens on wave 1 that is about 0.755 s. At one alien it is just under
0.08 s (clamped to a minimum of 0.055 s). The exponent makes the early kills
feel incremental and the final kills dramatic, while the wave divisor makes
later waves start under more pressure.

The original 1978 hardware moved one alien per frame and appeared to accelerate
because fewer sprites meant less CPU work per loop. This project models the
observable consequence directly instead of trying to emulate 1978 processor
timing.

## What happens on each tick

`Game::stepFormation` does four things:

1. toggles the two-frame alien animation;
2. moves the formation two logical pixels horizontally;
3. reverses direction and descends eight pixels if the next step would cross a
   wall; and
4. checks whether any surviving alien has reached the invasion line at
   `kInvasionY`.

On a descent, any bunker cells touched by the formation are carved away. The
next march timer is then recomputed from the new `aliensRemaining` count, so the
tempo changes immediately after each kill.

## Why a timer rather than variable speed

Every alien row shares one formation position. Moving individual aliens at
different speeds would break the distinctive rigid formation and complicate
collision. The game uses a discrete step clock and adjusts the interval, which is
the arcade behaviour players recognise. Because the simulation is fixed at
60 Hz, the interval in seconds is deterministic; a slower display cannot make
the invaders move faster or slower.
