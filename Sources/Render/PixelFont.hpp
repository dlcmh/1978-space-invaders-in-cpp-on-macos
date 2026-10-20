#pragma once

#include "Canvas.hpp"

#include <array>
#include <cstdint>
#include <string_view>

namespace invaders::render {

enum class TextAlign {
    Left,
    Center,
    Right,
};

/// A 5x7 pixel font built as filled rectangles on the canvas. It needs no font
/// file, texture atlas, or platform text stack, so it looks identical at every
/// window size and keeps the renderer asset-free.
class PixelFont {
public:
    static constexpr int kGlyphWidth = 5;
    static constexpr int kGlyphHeight = 7;

    PixelFont() = default;

    [[nodiscard]] auto measure(std::string_view text, float pixelSize,
                               float letterSpacing = -1.0F) const noexcept -> float;
    void draw(Canvas& canvas, std::string_view text, float x, float y, float pixelSize, Color color,
              TextAlign alignment = TextAlign::Left, float letterSpacing = -1.0F) const;
    void drawGlow(Canvas& canvas, std::string_view text, float x, float y, float pixelSize,
                  Color color, Color glow, TextAlign alignment = TextAlign::Left) const;

private:
    using Glyph = std::array<std::uint8_t, kGlyphHeight>;
    [[nodiscard]] static auto glyphFor(char character) noexcept -> const Glyph&;
};

} // namespace invaders::render
