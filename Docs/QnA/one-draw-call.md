# How does the whole game render in one Metal draw call?

**Short answer:** the CPU expands every visible shape into triangles and writes
them into one contiguous float buffer. All shapes share a single pipeline and a
single uniform block, so the renderer can bind one buffer and issue one
`drawPrimitives` call.

## The vertex list

[Canvas.cpp](../../Sources/Render/Canvas.cpp) — `Canvas::addQuad` appends six vertices
per rectangle. A circle is a fan of triangles; a line is a rotated quad;
bunkers, aliens, text, stars, and scanlines use those primitives. The vertex
format is six floats: x, y, red, green, blue, alpha.

[Scene.cpp](../../Sources/Render/Scene.cpp) — `Scene::build` composes the frame in
painter's order. Because blending is enabled, each translucent glow or scanline
is just another triangle pair drawn later in the list.

## The GPU side

[MetalRenderer.mm](../../Sources/Render/MetalRenderer.mm) — `SIRenderer` copies the
vertex list into one shared-memory buffer, then:

```objc
[encoder setRenderPipelineState:_pipelineState];
[encoder setVertexBuffer:buffer offset:0 atIndex:0];
[encoder setVertexBytes:&uniforms length:sizeof(Uniforms) atIndex:1];
[encoder drawPrimitives:MTLPrimitiveTypeTriangle
            vertexStart:0 vertexCount:canvas.vertexCount()];
```

The pipeline uses a 4× MSAA render pass and standard source-alpha blending.
The vertex shader in [MetalShaders.hpp](../../Sources/Render/MetalShaders.hpp) —
`arcade_vertex` applies the letterbox scale and offset and converts y-down
logical coordinates into Metal's y-up NDC.

## Why this is fast enough

The scene is a few thousand triangles at the 224 × 256 logical resolution; even
with stars, particles, and scanlines it is comfortably below the capacity of a
single buffer. Eliminating per-sprite bindings also removes a whole class of
render-state bugs.

The trade-off is that immediate mode re-expands static shapes each frame. For a
small arcade game that cost is negligible. A much larger game would need
retained geometry or instancing; that would be a measured refactor, not a
default complexity tax.

## Triple buffering

Three buffers rotate behind a counting semaphore so the CPU can compose frame
N+1 while the GPU consumes frame N. A command-buffer completion handler signals
the semaphore. This is the same idea as a swap chain at the vertex-data level.
