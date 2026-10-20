#pragma once

#include <array>
#include <cstddef>
#include <vector>

namespace invaders::render {

struct Color {
    float r = 0.0F;
    float g = 0.0F;
    float b = 0.0F;
    float a = 1.0F;

    [[nodiscard]] constexpr auto withAlpha(float alpha) const noexcept -> Color {
        return {r, g, b, alpha};
    }
    [[nodiscard]] constexpr auto scaled(float amount) const noexcept -> Color {
        return {r * amount, g * amount, b * amount, a};
    }
};

struct Vertex {
    float x = 0.0F;
    float y = 0.0F;
    float r = 0.0F;
    float g = 0.0F;
    float b = 0.0F;
    float a = 1.0F;
};

/// Immediate-mode triangle canvas. The scene is rebuilt every frame and
/// submitted to Metal in one draw call. There are no sprite sheets, textures,
/// or asset files: every pixel in this game is an expanded triangle.
class Canvas {
public:
    static constexpr int kFloatsPerVertex = 6;

    explicit Canvas(std::size_t reservedVertices = 160'000);

    void reset() noexcept;
    void addVertex(float x, float y, Color color);
    void addTriangle(float ax, float ay, float bx, float by, float cx, float cy, Color color);
    void addQuad(float ax, float ay, float bx, float by, float cx, float cy, float dx, float dy,
                 Color color);
    void addRect(float x, float y, float width, float height, Color color);
    void addGradientRect(float x, float y, float width, float height, Color top, Color bottom);
    void addCircle(float cx, float cy, float radius, Color color, int segments = 28);
    void addEllipse(float cx, float cy, float radiusX, float radiusY, Color color,
                    int segments = 28);
    void addRing(float cx, float cy, float radius, float thickness, Color color, int segments = 40);
    void addLine(float x0, float y0, float x1, float y1, float thickness, Color color,
                 bool rounded = false);
    void addPolygon(const std::vector<std::array<float, 2>>& points, Color color);
    void addGlow(float cx, float cy, float radius, Color color, int layers = 3);

    [[nodiscard]] auto data() const noexcept -> const std::vector<float>& { return vertices_; }
    [[nodiscard]] auto vertexCount() const noexcept -> std::size_t { return vertices_.size() / 6; }
    [[nodiscard]] auto empty() const noexcept -> bool { return vertices_.empty(); }

private:
    std::vector<float> vertices_;
};

} // namespace invaders::render
