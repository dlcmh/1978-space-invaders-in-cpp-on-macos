#include "Canvas.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace invaders::render {

Canvas::Canvas(std::size_t reservedVertices) {
    vertices_.reserve(reservedVertices * kFloatsPerVertex);
}

void Canvas::reset() noexcept {
    vertices_.clear();
}

void Canvas::addVertex(float x, float y, Color color) {
    vertices_.push_back(x);
    vertices_.push_back(y);
    vertices_.push_back(color.r);
    vertices_.push_back(color.g);
    vertices_.push_back(color.b);
    vertices_.push_back(color.a);
}

void Canvas::addTriangle(float ax, float ay, float bx, float by, float cx, float cy, Color color) {
    addVertex(ax, ay, color);
    addVertex(bx, by, color);
    addVertex(cx, cy, color);
}

void Canvas::addQuad(float ax, float ay, float bx, float by, float cx, float cy, float dx, float dy,
                     Color color) {
    addTriangle(ax, ay, bx, by, cx, cy, color);
    addTriangle(ax, ay, cx, cy, dx, dy, color);
}

void Canvas::addRect(float x, float y, float width, float height, Color color) {
    if (width <= 0.0F || height <= 0.0F) {
        return;
    }
    addQuad(x, y, x + width, y, x + width, y + height, x, y + height, color);
}

void Canvas::addGradientRect(float x, float y, float width, float height, Color top, Color bottom) {
    if (width <= 0.0F || height <= 0.0F) {
        return;
    }
    addVertex(x, y, top);
    addVertex(x + width, y, top);
    addVertex(x + width, y + height, bottom);
    addVertex(x, y, top);
    addVertex(x + width, y + height, bottom);
    addVertex(x, y + height, bottom);
}

void Canvas::addCircle(float cx, float cy, float radius, Color color, int segments) {
    addEllipse(cx, cy, radius, radius, color, segments);
}

void Canvas::addEllipse(float cx, float cy, float radiusX, float radiusY, Color color,
                        int segments) {
    if (radiusX <= 0.0F || radiusY <= 0.0F) {
        return;
    }
    const int count = std::max(segments, 8);
    float previousX = cx + radiusX;
    float previousY = cy;
    for (int index = 1; index <= count; ++index) {
        const float angle = static_cast<float>(index) / static_cast<float>(count) *
                            std::numbers::pi_v<float> * 2.0F;
        const float nextX = cx + std::cos(angle) * radiusX;
        const float nextY = cy + std::sin(angle) * radiusY;
        addTriangle(cx, cy, previousX, previousY, nextX, nextY, color);
        previousX = nextX;
        previousY = nextY;
    }
}

void Canvas::addRing(float cx, float cy, float radius, float thickness, Color color, int segments) {
    if (radius <= 0.0F || thickness <= 0.0F) {
        return;
    }
    const int count = std::max(segments, 12);
    const float inner = std::max(0.0F, radius - thickness);
    float previousOuterX = cx + radius;
    float previousOuterY = cy;
    float previousInnerX = cx + inner;
    float previousInnerY = cy;
    for (int index = 1; index <= count; ++index) {
        const float angle = static_cast<float>(index) / static_cast<float>(count) *
                            std::numbers::pi_v<float> * 2.0F;
        const float c = std::cos(angle);
        const float s = std::sin(angle);
        const float outerX = cx + c * radius;
        const float outerY = cy + s * radius;
        const float innerX = cx + c * inner;
        const float innerY = cy + s * inner;
        addQuad(previousOuterX, previousOuterY, outerX, outerY, innerX, innerY, previousInnerX,
                previousInnerY, color);
        previousOuterX = outerX;
        previousOuterY = outerY;
        previousInnerX = innerX;
        previousInnerY = innerY;
    }
}

void Canvas::addLine(float x0, float y0, float x1, float y1, float thickness, Color color,
                     bool rounded) {
    const float dx = x1 - x0;
    const float dy = y1 - y0;
    const float length = std::sqrt(dx * dx + dy * dy);
    if (length < 0.0001F || thickness <= 0.0F) {
        return;
    }
    const float nx = -dy / length * thickness * 0.5F;
    const float ny = dx / length * thickness * 0.5F;
    addQuad(x0 + nx, y0 + ny, x1 + nx, y1 + ny, x1 - nx, y1 - ny, x0 - nx, y0 - ny, color);
    if (rounded) {
        addCircle(x0, y0, thickness * 0.5F, color, 10);
        addCircle(x1, y1, thickness * 0.5F, color, 10);
    }
}

void Canvas::addPolygon(const std::vector<std::array<float, 2>>& points, Color color) {
    if (points.size() < 3) {
        return;
    }
    float centerX = 0.0F;
    float centerY = 0.0F;
    for (const auto& point : points) {
        centerX += point[0];
        centerY += point[1];
    }
    centerX /= static_cast<float>(points.size());
    centerY /= static_cast<float>(points.size());
    for (std::size_t index = 0; index < points.size(); ++index) {
        const auto& current = points[index];
        const auto& next = points[(index + 1) % points.size()];
        addTriangle(centerX, centerY, current[0], current[1], next[0], next[1], color);
    }
}

void Canvas::addGlow(float cx, float cy, float radius, Color color, int layers) {
    for (int layer = 0; layer < layers; ++layer) {
        const float factor = static_cast<float>(layer + 1) / static_cast<float>(layers);
        addCircle(cx, cy, radius * factor, color.withAlpha(color.a * (1.0F - factor * 0.6F)), 24);
    }
}

} // namespace invaders::render
