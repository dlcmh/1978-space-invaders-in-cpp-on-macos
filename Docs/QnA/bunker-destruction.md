# How are the bunkers destroyed pixel by pixel?

**Short answer:** each bunker is a 22 × 16 grid of solid cells. Collision tests
and rendering read the same grid, and a hit erases every cell whose centre lies
inside a circular blast radius. There is no separate health bar.

## The data

[Bunker.hpp](../../Sources/Core/include/invaders/Bunker.hpp) — `Bunker` stores 352
`std::uint8_t` cells, an origin, and a remaining-cell count. Each cell is two
logical pixels square. Four bunkers are created by `makeBunkers` at the start of
each wave.

The initial silhouette is shaped by `isShapeCell`: a rounded top and a widening
notch cut up from the bottom. The renderer does not know that shape; it simply
walks every solid cell and draws it. That is the central design decision: the
collision map is the art.

## Hit testing

[Bunker.cpp](../../Sources/Core/Bunker.cpp) — `Bunker::hitTest` converts a projectile
rectangle into a range of cell columns and rows, then checks only those cells
for solidity. `Bunker::hitPoint` returns the centre of the first solid cell hit,
which is a stable point for the subsequent carve.

The projectile loops in [Game.cpp](../../Sources/Core/Game.cpp) — `Game::updatePlayerShot`
and `Game::updateEnemyShots` query every bunker in order. A player shot carves a
4.4-pixel radius; enemy fire carves 3.2 pixels. When no solid cell remains, the
projectile continues through the empty space, so repeated shots tunnel through
cover exactly as expected.

## Carving

`Bunker::carve` visits only cells within the blast's bounding box and removes
cells whose centres are within the radius. It returns whether anything changed,
which prevents a redundant hit event. Because the cell grid is small and
deterministic, a shot from the same position always produces the same hole.

When the alien formation descends, `Game::stepFormation` intersects the
formation's bounds with each bunker and carves a wider 6.8-pixel radius. This
approximates the original's aliens grinding through the shields as they
advance.

## What the renderer sees

[Scene.cpp](../../Sources/Render/Scene.cpp) — `Scene::drawBunkers` reads
`Bunker::isSolid` for every cell. It draws the base green cell, a highlight where
the cell above is empty, and a shadow where the cell below is empty. Neighbour
checks create the bevelled edge automatically after any amount of damage.

No sprite mask, texture, or second hit-point model exists. What you see is the
collision surface.
