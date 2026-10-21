# What is "the arcade's raster order"?

**Short answer:** a CRT picture is drawn line by line from the **top** to the
**bottom**, so in the arcade's coordinate system `y = 0` is the top edge and `y`
grows **downward**. [Types.hpp](../../Sources/Core/include/invaders/Types.hpp) —
`Vec2` uses the same convention, which the comment calls "the arcade's raster
order". It matches how the original hardware laid out memory and how Metal's
viewport happens to work, so the simulation and the GPU agree without a flip.

The rest of this page unpacks "raster order": what a raster is, why the top of a
coordinate system is usually the bottom, and what it would cost if this project
chose the other way.

## What a raster is

A CRT draws one **scan line** at a time. An electron beam sweeps left to right
across the screen, then moves down one line and sweeps again, repeating until
the bottom of the frame is reached. The beam then returns to the top and starts
the next frame. That pattern is called **raster scanning**, and the set of lines
it traces is the **raster**.

"Raster order" is therefore the order in which the display (and the memory that
feeds it) visits pixels: top row first, left to right, then the next row down.
A frame buffer laid out that way is a flat array where the first pixel is the
top-left corner.

## Why `y` points down

Mathematics draws the origin at the bottom-left with `y` increasing upward. A
raster has no such freedom: the first line the beam produces is at the physical
top of the tube. So bitmap-era systems — arcade PCBs, the Apple II, the Amiga,
and most GPU APIs — store their pictures top row first and treat `y = 0` as the
top.

SPACE INVADERS follows that convention. In this project:

- [`Types.hpp`](../../Sources/Core/include/invaders/Types.hpp) — `Vec2` documents
  `y` as increasing downward, so `kPlayerY = 228` is **near the bottom** of the
  256-unit-tall portrait playfield and `kHudHeight = 28` is a band **at the
  top**.
- `Game` places the cannon at `kPlayerY` and fires shots in the negative `y`
  direction, because "up the screen" is "smaller `y`".
- `Scene` draws the HUD at `y = 0` and the drop shadow under the playfield at
  `y = kScreenHeight`. Both are the natural expression of a top-down field.
- Aliens descend by **increasing** `y`. "Descend" and "increase `y`" are the
  same movement, with no sign flip in the rules.

None of that is a rendering choice; it is how the simulation itself describes
space. The rules never need to know which way is visually up.

## Why it also matches Metal

Metal, like OpenGL and Vulkan before it, defines its viewport with the origin at
the **top-left** of the render target and `y` increasing downward. Normalised
device coordinates put `y = +1` at the top of the viewport.

So a coordinate system that grows downward is already "GPU-native". The
[`MetalShaders.hpp`](../../Sources/Render/MetalShaders.hpp) — `arcade_vertex`
shader still has to convert from logical units to device coordinates, but the
conversion is a scale and a sign:

```metal
float2 ndc = float2((screen.x / uniforms.viewportSize.x) * 2.0 - 1.0,
                    1.0 - (screen.y / uniforms.viewportSize.y) * 2.0);
```

The `1.0 - ...` mirrors `y` because NDC counts upward while the arcade counts
downward. That is one subtraction, computed on the GPU, once per vertex. If the
simulation used "up is positive" instead, the same mirror would be needed — just
at a different layer.

The renderer also maps the fixed 224 × 256 playfield onto the drawable with a
letterbox offset in
[`MetalRenderer.mm`](../../Sources/Render/MetalRenderer.mm) — `renderGame`.
The uniform `offset` uses the same downward `y`, so the bars above and below
the playfield are symmetric without a special case.

## What the alternative would cost

A "maths" coordinate system — origin bottom-left, `y` up — is perfectly usable,
but choosing it here would mean:

- every rule that says "descend" would have to add to a `y` that points up, or
  store `kPlayerY` as a distance from the bottom instead of the top;
- the shader would flip `y` anyway, or the renderer would flip the whole scene,
  moving the conversion away from the one place it is already documented;
- reading the constants against a screenshot would be harder:
  `kHudHeight = 28` would no longer be "the strip at the top".

The project's rule is the usual one for raster systems: **`y` grows in the same
direction the picture is drawn.** The band at `y = 0` is the top band, the HUD
is the HUD, and the cannon at `y = 228` is at the bottom.

## Pitfalls worth knowing

- **Do not mix conventions in one module.** A flipped `y` in `Scene` but not in
  `Game` produces a vertically mirrored world that still looks plausible until
  a collision is wrong. The comment on `Vec2` is the single source of truth.
- **Texture coordinates are not the raster.** On most GPU APIs the first
  texture row is the top row, matching this convention, but some image loaders
  and libraries hand you bottom-up data. This project has no image assets, so
  the question does not arise — procedural art is authored in the same space.
- **Angles and physics signs follow the axes.** A positive angular velocity
  rotates in the direction implied by the coordinate system. If you ever add
  rotation, pick a convention and apply it consistently; do not assume "positive
  is clockwise" from the visual result.
- **`std::clamp` and bounds stay simple.** `kScreenHeight` is the bottom edge in
  both the simulation and the shader, so a projectile that leaves the screen is
  `y > kScreenHeight` everywhere; there is no `-y` in a bounds check.

## Glossary

| Term | Meaning |
| --- | --- |
| Raster | The grid of scan lines a display draws, visited top to bottom. |
| Raster order | Top-left first, row by row — the order hardware reads and writes pixels. |
| Scan line | One horizontal line of the picture drawn by the beam in one sweep. |
| Frame buffer | Memory holding the pixels of one frame, usually stored in raster order. |
| NDC | Normalised device coordinates: GPU space with `y = +1` at the top. |
| Letterbox | Uniform scaling that leaves bars so the aspect ratio is preserved. |

Return to the [Q&A index](README.md) or the
[project README](../../README.md).
