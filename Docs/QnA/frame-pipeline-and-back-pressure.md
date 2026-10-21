# What does "a centralized two-stage pipeline with a 3-slot back-pressure ring, not a task graph" mean?

**Short answer:** the frame is built in a fixed order on one thread, with no work
split up or reordered. The only thing keeping the GPU from falling arbitrarily
far behind is a counting semaphore sized to three buffers. A pipeline is a
chain of dependent steps; a task graph is a web of parallel jobs joined at
explicit sync points. This renderer is the chain.

## Two stages, one boundary

The boundary between the CPU stage and the GPU stage is a single `memcpy`.

1. **CPU stage.** [Scene.cpp](../../Sources/Render/Scene.cpp) — `Scene::update`
   advances presentation state (particles, popups, flashes) from the frame
   delta. Then `Scene::build` reads the simulation and re-emits every triangle
   for the frame into `canvas_`.
2. **GPU stage.** [MetalRenderer.mm](../../Sources/Render/MetalRenderer.mm) —
   `SIRenderer` copies that float list into a shared-storage `MTLBuffer` and
   issues one draw call.

There is no retained scene graph, no dirty tracking, and no per-object GPU
buffer. [Canvas.hpp](../../Sources/Render/Canvas.hpp) — `Canvas` is an
immediate-mode list of triangles, and every pixel in the game is one.

"Centralized" means nothing decides *which* job runs *when* or on *what core*.
[GameView.mm](../../Sources/Platform/GameView.mm) — `drawInMTKView` is the
scheduler, and it is a linear sequence:

```text
Scene::update()            → advance particles and effects
Scene::build()             → emit every triangle for this frame
wait(semaphore)            → block if three frames are still in flight
memcpy(buffer.contents)    → copy vertices into the current ring slot
encode + commit            → one triangle draw, then presentDrawable
completion handler         → GPU finished; signal the semaphore
```

Note where the wait sits: the `wait` precedes the `memcpy`, and the signal is
registered as the command buffer's completion handler. A slot is released when
the GPU has finished *reading* it, not when the CPU has finished writing it.

## Why it is a ring, and where the back-pressure is

- **Three buffers, round-robin.** `kBufferCount` is three, and `_frameIndex`
  advances mod three, so the CPU writes into a slot the GPU is not reading.
- **Counting semaphore initialized to three.** `_inFlightSemaphore` starts with
  three permits, so the CPU can never be more than three frames ahead.
- **Release on GPU completion.** The `addCompletedHandler` block signals the
  semaphore once the GPU is done with the frame.

One buffer would mean overwriting memory the GPU is still reading. Three let
the CPU build frame N+1 while N and N−1 are still rendering. Because a slot is
only returned on completion, buffer reuse is safe by construction.

## Why "not a task graph"

A task graph exists so independent work runs in parallel and dependencies are
joined explicitly: fork over physics cells, join to merge contacts, fork again
for narrowphase. This renderer has neither half of that.

- **No fork.** The CPU is one thread; the semaphore is the only concurrency
  primitive outside `Sources/Core/`. Nothing is parallelised or recombined.
- **No join.** Each step needs the one before it exactly: `Scene::build` needs
  the simulation, the copy needs the canvas, the draw needs the copy.
- **No scheduling.** Nothing is reordered, priority-classed, or pinned to a
  core.

## What "latency bounded at three frames" does and does not mean

It does **not** mean a frame takes three times the render time. It means at most
three frames of GPU work are outstanding at once. If the GPU stalls, the wait
blocks the CPU instead of letting frames pile up in the queue. The bound is a
safety property on memory reuse and queue depth, not a statement about frame
time.

The fixed-step driver reinforces this at a different level:
[GameView.mm](../../Sources/Platform/GameView.mm) — `drawInMTKView` runs
simulation ticks in a bounded catch-up loop (at most eight 60 Hz steps per
frame). Presentation is rebuilt from whatever state the simulation reached, and
the simulation never depends on how long the GPU took.

## Why not parallelise it

The scene is a few thousand triangles at 224 × 256 logical resolution, rebuilt
in microseconds. Splitting that work would cost more in scheduling overhead than
it saves, which is exactly the dispatch-overhead trap that single-threaded
renderers avoid. The deterministic, ordered pipeline is also what makes the
checks and the replay run meaningful: every frame is a pure function of the
fixed-step simulation state.

Closely related: [How does the whole game render in one Metal draw
call?](one-draw-call.md) covers the vertex list and the single draw; this entry
covers the ordering and the buffering around it.
