# Keep the 55-entity world as value containers, not an ECS

- **Date:** 2026-10-20
- **Status:** accepted
- **Scope:** simulation storage and update architecture

## Proposal

Replace `std::vector<Alien>`, `std::vector<Projectile>`, and the small bunker
array with an entity-component-system: entity IDs, sparse sets or archetypes,
and a scheduler that runs movement, collision, and render-extraction systems.

## Why it was considered

An ECS is a common default for games and promises compositional flexibility:
new entity types can be assembled from data, and systems can be scheduled and
parallelised independently. It would also avoid the branch inside the alien
draw loop by grouping sprites by kind.

## Options

1. **Adopt a third-party ECS.** Rejected: adds a runtime dependency and an API
   to learn for a 55-entity fixed shooter.
2. **Write a minimal ECS.** Rejected: the project would own a framework and its
   tests while still solving only a handful of entity types.
3. **Keep typed value containers.** Chosen.
4. **Hybrid: typed storage plus a generic collision list.** Rejected: it adds
   indirection without removing any of the real complexity, which is formation
   logic and projectile ordering, not entity storage.

## Evidence

- The simulation has one alien type with an `AlienKind` discriminator. The
  [Alien](../../Sources/Core/include/invaders/Types.hpp) — `Alien` is 12 bytes or so and
  all 55 entries fit in one cache-friendly vector.
- [Game.cpp](../../Sources/Core/Game.cpp) — `Game::alienRect` derives a collision
  rectangle from the formation and a column/row pair. There is no per-alien
  position to synchronise, which removes an entire class of ECS-system ordering
  bugs.
- The full regression suite, including an autopilot that clears a wave and hits
  a mystery ship, runs in under a second. Iteration and check speed are not
  constrained by the current design.
- There is no second gameplay mode, no scripting requirement, and no content
  pipeline. The extension pressure that usually justifies an ECS is absent.

## Decision

Keep the typed value containers. Reconsider only if the entity count grows by
orders of magnitude, if multiple independent gameplay systems need scheduling,
or if serialisation/networking creates a real need for stable entity IDs. Any
such change must be benchmarked against the current check suite and proposed as
its own refactor.

## Consequences

- Gameplay systems remain explicit, ordered C++ calls.
- Adding a new alien kind is a value in an enum plus a sprite table, not an
  entity archetype.
- Collision code is typed and readable, at the cost of some repetition between
  player shots, enemy shots, and the UFO. That repetition is small and has not
  caused drift; each path has distinct ordering and scoring rules.
