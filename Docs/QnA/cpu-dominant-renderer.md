# Why is this renderer "CPU-dominant" rather than GPU-driven?

**Short answer:** the GPU does not compute anything. The CPU makes every
decision — simulation, collision, scoring, animation, particle motion — and the
GPU receives finished triangles and rasterises them with one pipeline and one
draw call. Modern engines have moved particles, skinning, and culling onto the
GPU; this game is deliberately at the opposite end of that spectrum.

## The GPU only transforms and rasterises

[MetalRenderer.mm](../../Sources/Render/MetalRenderer.mm) — `SIRenderer`
issues exactly one non-instanced draw:

```objc
[encoder drawPrimitives:MTLPrimitiveTypeTriangle
            vertexStart:0 vertexCount:canvas.vertexCount()];
```

One vertex buffer at index 0 and one small uniform block at index 1 (drawable
size, letterbox scale, and offsets) carry the whole frame. There are no compute
shaders and no indirect draws, and there is not a single texture in the game. [Canvas.hpp](../../Sources/Render/Canvas.hpp) — `Canvas`
states that there are no sprite sheets or asset files, because every pixel is
an expanded triangle. [MetalShaders.hpp](../../Sources/Render/MetalShaders.hpp) —
`arcade_vertex` letterboxes and flips y into Metal's coordinate space, and
`arcade_fragment` returns the interpolated vertex colour untouched. Together
they are the entire GPU program: no texture fetch, no lighting, no compute.

## Where each engine workload actually lives

| Workload in a modern engine | Here |
| --- | --- |
| GPU particle systems | CPU: [Scene.cpp](../../Sources/Render/Scene.cpp) — `Scene::update` advances every particle in a plain loop |
| GPU skinning | None: sprites are procedural triangle expansions, there are no skeletons |
| GPU culling | None: `Scene::build` emits all geometry every frame unconditionally |
| Physics solver | None: collision is discrete cell and rectangle tests inside `Game` |
| Animation | Procedural: frames and marching come from deterministic step tables |
| AI / behaviour trees | None: the formation follows a fixed pattern |
| Networking | None: the game is offline by design |
| Asset streaming | None: art, audio, and the font are generated in code |

The reason is structural: the game has no asset pipeline. A deep engine loads
millions of vertices per frame across many meshes; this one builds a few
thousand triangles from numbers in memory.

## Why CPU dominance is a requirement, not a limitation

[AGENTS.md](../../AGENTS.md#non-negotiable-architecture) — the non-negotiable
architecture keeps `Sources/Core/` free of Metal and windowing so the
simulation builds and runs headlessly. The gameplay invariants depend on that:
`invaders::Game` is authoritative, the loop is fixed-step, and randomness comes
from a seeded generator.

Computing gameplay on the GPU would break those properties. Bunker destruction
is the clear case: the 22 × 16 grid at [Bunker.hpp](../../Sources/Core/include/invaders/Bunker.hpp)
— `Bunker` is simulation state, and its holes must be identical on every replay.
A shader computing those holes would make them depend on GPU execution order
and floating-point behaviour, and the deterministic checks would stop being
meaningful.

## Relationship to modern engine design

In a renderer that has offloaded culling and skinning to the GPU, the CPU's
hard problem becomes orchestrating a heterogeneous pipeline around fences and
async compute. This project faces the opposite problem and solves it by not
having one: the only CPU–GPU handoff is the letterbox uniform block, and the
only synchronisation is the three-slot semaphore described in [What does a
centralized two-stage pipeline with a 3-slot back-pressure ring
mean?](frame-pipeline-and-back-pressure.md).

If the game ever needed thousands of simulated particles or much more geometry,
the honest path would be measured: a GPU particle system in `Scene` (which is
already presentation-only and outside the simulation), or retained geometry and
instancing. Neither is warranted by the current workload, and neither would be
allowed to move gameplay rules off the CPU.
