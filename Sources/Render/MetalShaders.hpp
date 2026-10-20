#pragma once

#include <string_view>

namespace invaders::render {

/// Metal Shading Language is compiled at runtime through
/// `newLibraryWithSource:options:error:`. Keeping the source beside the C++
/// renderer makes the GPU pipeline inspectable without a separate build tool,
/// and it keeps the repository free of precompiled `.metallib` binaries.
inline constexpr std::string_view kMetalShaderSource = R"MSL(
#include <metal_stdlib>
using namespace metal;

struct VertexIn {
    packed_float2 position;
    packed_float4 color;
};

struct VertexOut {
    float4 position [[position]];
    float4 color;
};

struct Uniforms {
    float2 viewportSize;
    float2 scale;
    float2 offset;
};

vertex VertexOut arcade_vertex(const device VertexIn *vertices [[buffer(0)]],
                               constant Uniforms &uniforms [[buffer(1)]],
                               uint vertexID [[vertex_id]]) {
    VertexIn input = vertices[vertexID];
    float2 screen = float2(input.position) * uniforms.scale + uniforms.offset;
    float2 ndc = float2((screen.x / uniforms.viewportSize.x) * 2.0 - 1.0,
                        1.0 - (screen.y / uniforms.viewportSize.y) * 2.0);
    VertexOut out;
    out.position = float4(ndc, 0.0, 1.0);
    out.color = float4(input.color);
    return out;
}

fragment float4 arcade_fragment(VertexOut in [[stage_in]]) {
    return in.color;
}
)MSL";

} // namespace invaders::render
