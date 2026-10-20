# A Metal Crash Course, Using This Renderer

This is an introduction to Metal for someone who has never written a GPU API.
It assumes you know C++ and roughly how a window gets pixels, but not what a
command buffer or a shader is. Every concept is tied to the actual code in this
repository.

If you only remember one thing: **the CPU builds a list of triangles; the GPU
turns them into pixels.** Everything else is plumbing around that sentence.

## 1. What the GPU is actually doing

A draw call gives the GPU three things:

1. **Vertices** — points in space, with per-vertex data (here, a position and a
   colour).
2. **A pipeline** — the vertex program that converts points into clip space,
   and the fragment program that computes the colour of each covered pixel.
3. **A render target** — a texture to draw into, usually the window's drawable.

The CPU does not tell the GPU "draw an alien." It says "here are triangles."
Aliens, bullets, letters, and the scanlines are all triangles by the time they
reach Metal.

In this game a frame is roughly 30,000 vertices at the default window size.
That sounds like a lot, but each alien is around 60 rectangles, so it adds up
quickly and still fits comfortably in one vertex buffer and one draw call.

## 2. The window path: MTKView, delegate, drawable

`MTKView` is an AppKit view backed by a `CAMetalLayer`. It owns the drawable
pool: one or more textures that the compositor can display. The view calls a
delegate method on a display-linked loop. In this project the view is
`SIGameView`, and [GameView.mm](../Sources/Platform/GameView.mm) — `drawInMTKView`
is the entry point for every frame.

The first thing the draw method does is decide how much simulated time has
elapsed. It does **not** advance the game by a variable amount:

```text
now          = CACurrentMediaTime()
accumulator += clamp(now - lastFrameTime, 0, 0.1)
while accumulator >= 1/60 and steps < 8:
    game.update(1/60)
    drain events -> audio + particles
    accumulator -= 1/60
```

Rendering may run at 60, 120, or anything else the display supports, but
gameplay sees only fixed 1/60 steps. That is why the checks can replay a game
exactly.

After updating the simulation, the view calls the renderer. The renderer asks
the view for:

- `currentDrawable` — the texture to present;
- `currentRenderPassDescriptor` — a description of the render target,
  including its multisample texture and resolve texture.

If either is nil (window off-screen, minimized, or between resizes), the frame
is skipped rather than forced.

## 3. Command queue, command buffer, encoder

Metal work is recorded, not executed immediately:

```text
MTLCommandQueue
  └── MTLCommandBuffer       (one frame's work)
        ├── MTLRenderCommandEncoder
        │     set pipeline, vertex buffer, uniforms
        │     drawPrimitives(...)
        └── present(drawable)
      commit()
```

The queue serialises command buffers. The encoder is the object that records
state and draw calls. Once the encoder ends and the command buffer is committed,
the GPU executes asynchronously and presents the drawable when finished.

In [MetalRenderer.mm](../Sources/Render/MetalRenderer.mm) — `SIRenderer` the queue
is created once during initialization. Each frame creates a transient command
buffer and encoder; Metal recycles the underlying allocations.

## 4. The vertex format

A vertex in this renderer is six floats:

```text
x, y, r, g, b, a
```

Positions are in **logical arcade pixels**: a 224 × 256 portrait playfield. They
are not in window pixels and not yet in clip space. Colours are straight-alpha
floats in [0, 1].

[Canvas.cpp](../Sources/Render/Canvas.cpp) — `Canvas::addRect` and its primitives
write those floats into a `std::vector<float>`. The same buffer layout is
declared to the shader as packed types:

```metal
struct VertexIn {
    packed_float2 position;
    packed_float4 color;
};
```

`packed_float2` matters. A normal `float2` in a Metal struct has 8-byte
alignment and a `float4` has 16-byte alignment, which can insert padding and
make the GPU read the wrong bytes. Using `packed` on both fields makes the
shader's stride exactly 24 bytes, matching the C++ float sequence.

## 5. The vertex shader and the letterbox transform

The shader lives in [MetalShaders.hpp](../Sources/Render/MetalShaders.hpp) —
`arcade_vertex`. Its job is to map logical arcade coordinates to Normalized
Device Coordinates (NDC), where x and y are in [-1, 1] and y points up.

The app computes a uniform scale and offset on the CPU:

```text
scale  = min(drawableWidth / 224, drawableHeight / 256)
offset = centred remainder of the drawable
screen = logicalPosition * scale + offset
ndc.x  = (screen.x / drawableWidth)  * 2 - 1
ndc.y  = 1 - (screen.y / drawableHeight) * 2
```

The `1 - ...` flips the y-axis because the game's coordinates increase downward,
while NDC increases upward. The `min` is what letterboxes the game: the scene
keeps its aspect ratio and leaves black bars when the window is a different
shape. Resize the window and the transform changes; the simulation and all
sprite coordinates do not.

Uniforms are passed as 24 bytes of vertex data in buffer index 1. The C++
`Uniforms` struct and the MSL `Uniforms` struct have the same field order and
size. There is no reflection or automatic layout here; this is a hand-maintained
boundary and changing one side means changing the other.

## 6. The fragment shader and blending

The fragment shader is almost trivial:

```metal
fragment float4 arcade_fragment(VertexOut in [[stage_in]]) {
    return in.color;
}
```

Every triangle has a constant colour, so the fragment shader just passes it
through. The interesting visual work happens in the composition: multiple
overlapping translucent rectangles create glow, scans, and vignettes.

Blending is configured in the pipeline descriptor:

```text
srcRGB * srcAlpha + dstRGB * (1 - srcAlpha)
```

This is ordinary source-alpha blending for a non-premultiplied colour. A glow
rectangle with alpha 0.05 adds a faint tint without replacing the pixels behind
it. Opaque background triangles are drawn first; the scene is painter's order.

## 7. Drawing a whole game as triangles

[Scene.cpp](../Sources/Render/Scene.cpp) — `Scene::build` clears the canvas and
draws in layers:

1. gradient background and stars;
2. labels and HUD;
3. bunker cells;
4. aliens and mystery ship;
5. player cannon;
6. projectiles and particles;
7. overlays and scanlines.

The art is not texture-based. Aliens are arrays of `#` and `.` rows; each `#`
becomes a rectangle. The pixel font uses a 5 × 7 glyph table and emits a
rectangle per lit pixel. Bunkers are 22 × 16 bitmaps of 2-logical-pixel cells.
The star field is a deterministic array of points with individual twinkle
phases. Even the scanlines are translucent rectangles.

This is why there is no asset pipeline: the "sprites" are data and a loop.

## 8. One draw call

The renderer submits the entire frame at once:

```objc
[encoder setRenderPipelineState:_pipelineState];
[encoder setVertexBuffer:buffer offset:0 atIndex:0];
[encoder setVertexBytes:&uniforms length:sizeof(Uniforms) atIndex:1];
[encoder drawPrimitives:MTLPrimitiveTypeTriangle
            vertexStart:0
            vertexCount:canvas.vertexCount()];
```

No textures, material changes, or state switches are needed between sprites, so
one draw call is both simpler and faster than batching per sprite type. If the
game later needed textures or per-sprite effects, it would need additional
pipelines or bindings; that is a deliberate scope boundary.

## 9. MSAA and the resolve you must not accidentally remove

Aliasing is especially visible on diagonal pixel edges and circles. The view
uses 4× MSAA when the GPU supports it. The render pass draws into a
multisampled texture, then Metal resolves it into the drawable's single-sample
texture.

`MTKView` already configures that resolve. The renderer changes only the clear
colour. An earlier version also assigned
`storeAction = MTLStoreActionStore`, which replaced the view's
"store and resolve" action, so the multisampled image was never resolved into
the drawable and the window stayed black even though commands were submitting.
The fix is to leave the render pass descriptor's store/resolve configuration
alone. This is a useful debugging lesson: a successful command buffer can still
produce a black screen if presentation wiring is wrong.

When adding a custom post-processing pass, do the resolve explicitly and treat
the render pass descriptor as owned by the view unless you fully re-specify it.

## 10. Triple buffering and the in-flight semaphore

The CPU must not overwrite a vertex buffer while the GPU is still reading it.
The renderer keeps three shared-memory buffers and a counting semaphore with a
count of three. Before filling a buffer it waits for a slot; a command buffer
completion handler signals the semaphore. Each frame advances to the next
buffer index. That gives the CPU up to three frames in flight without races.

The buffer grows geometrically if a frame exceeds the initial capacity. Vertex
data is copied with `memcpy`; the buffer storage mode is shared, which is valid
for Apple silicon's unified memory and appropriate for this small workload.

## 11. Why the shader is compiled at runtime

Metal libraries can be compiled ahead of time into `.metallib` files, or at
runtime from source. This project uses
`newLibraryWithSource:options:error:` so the repository needs no extra build
tool, no binary shader artifact, and no packaging step for a Metal library.

The trade-off is that shader errors are runtime errors. `make check` cannot
validate the Metal compiler output by itself. Always launch the app after
changing [MetalShaders.hpp](../Sources/Render/MetalShaders.hpp) — `arcade_fragment`
or the pipeline code; a failed compilation should produce a clear alert from the
app initializer.

## 12. A debugging sequence for a black frame

1. **Is the window visible?** Bring the app frontmost, then capture that window
   rather than the full screen.
2. **Is `currentDrawable` non-nil?** A minimized or off-screen window may not
   provide one.
3. **Did the command buffer complete?** Add a completion handler and log its
   error. Metal validation can be enabled in Xcode's scheme or with environment
   variables during development.
4. **Is the pass resolving?** For MSAA, ensure the resolve texture and store
   action are intact.
5. **Is the transform in range?** Log the first few vertex positions and the
   uniforms. A wrong stride or scale can put every triangle off-screen.
6. **Is the vertex buffer the one being read?** Triple buffering means an
   indexing mistake can present a stale frame or garbage.
7. **Is the shader returning alpha zero?** Opaque geometry should still show a
   dark background; a fully black screen often points at the resolve or
   presentation, not the fragment colour.

The project keeps launch verification in the definition of done for exactly this
reason.

## 13. Glossary

| Term | Meaning here |
| --- | --- |
| **Drawable** | A texture the window compositor can display; provided by `MTKView`. |
| **Command buffer** | A recorded batch of GPU work. |
| **Encoder** | The object that records render state and draw commands. |
| **Pipeline** | The compiled vertex + fragment programs and fixed-function state. |
| **Vertex** | A position plus colour; triangles are made of three. |
| **Fragment** | A candidate pixel produced by rasterising a triangle. |
| **MSAA** | Multisample anti-aliasing; extra samples per pixel, resolved once. |
| **Resolve** | Converting multisampled pixels to the single-sample drawable. |
| **NDC** | Normalized device coordinates, x/y in [-1, 1]. |
| **Uniform** | Constant data shared by all vertices/fragments in a draw. |
| **Triple buffering** | Rotating three buffers so CPU and GPU can overlap safely. |

## 14. Where to look next

- [MetalRenderer.mm](../Sources/Render/MetalRenderer.mm) — `renderGame` for the
  complete frame path.
- [MetalShaders.hpp](../Sources/Render/MetalShaders.hpp) — `arcade_vertex` and
  `arcade_fragment`.
- [Canvas.cpp](../Sources/Render/Canvas.cpp) — `Canvas::addTriangle` for the lowest
  level of geometry.
- [Scene.cpp](../Sources/Render/Scene.cpp) — `Scene::drawBackdrop` for the visual
  composition.
- [GameView.mm](../Sources/Platform/GameView.mm) — `drawInMTKView` for timing and
  presentation.
