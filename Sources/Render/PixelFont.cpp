#include "PixelFont.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <unordered_map>

namespace invaders::render {
namespace {

using Glyph = std::array<std::uint8_t, PixelFont::kGlyphHeight>;

const std::unordered_map<char, Glyph>& glyphs() {
    static const std::unordered_map<char, Glyph> table = {
        {'A', {0b01110, 0b10001, 0b10001, 0b11111, 0b10001, 0b10001, 0b10001}},
        {'B', {0b11110, 0b10001, 0b10001, 0b11110, 0b10001, 0b10001, 0b11110}},
        {'C', {0b01111, 0b10000, 0b10000, 0b10000, 0b10000, 0b10000, 0b01111}},
        {'D', {0b11110, 0b10001, 0b10001, 0b10001, 0b10001, 0b10001, 0b11110}},
        {'E', {0b11111, 0b10000, 0b10000, 0b11110, 0b10000, 0b10000, 0b11111}},
        {'F', {0b11111, 0b10000, 0b10000, 0b11110, 0b10000, 0b10000, 0b10000}},
        {'G', {0b01111, 0b10000, 0b10000, 0b10111, 0b10001, 0b10001, 0b01111}},
        {'H', {0b10001, 0b10001, 0b10001, 0b11111, 0b10001, 0b10001, 0b10001}},
        {'I', {0b11111, 0b00100, 0b00100, 0b00100, 0b00100, 0b00100, 0b11111}},
        {'J', {0b00111, 0b00010, 0b00010, 0b00010, 0b00010, 0b10010, 0b01100}},
        {'K', {0b10001, 0b10010, 0b10100, 0b11000, 0b10100, 0b10010, 0b10001}},
        {'L', {0b10000, 0b10000, 0b10000, 0b10000, 0b10000, 0b10000, 0b11111}},
        {'M', {0b10001, 0b11011, 0b10101, 0b10101, 0b10001, 0b10001, 0b10001}},
        {'N', {0b10001, 0b10001, 0b11001, 0b10101, 0b10011, 0b10001, 0b10001}},
        {'O', {0b01110, 0b10001, 0b10001, 0b10001, 0b10001, 0b10001, 0b01110}},
        {'P', {0b11110, 0b10001, 0b10001, 0b11110, 0b10000, 0b10000, 0b10000}},
        {'Q', {0b01110, 0b10001, 0b10001, 0b10001, 0b10101, 0b10010, 0b01101}},
        {'R', {0b11110, 0b10001, 0b10001, 0b11110, 0b10100, 0b10010, 0b10001}},
        {'S', {0b01111, 0b10000, 0b10000, 0b01110, 0b00001, 0b00001, 0b11110}},
        {'T', {0b11111, 0b00100, 0b00100, 0b00100, 0b00100, 0b00100, 0b00100}},
        {'U', {0b10001, 0b10001, 0b10001, 0b10001, 0b10001, 0b10001, 0b01110}},
        {'V', {0b10001, 0b10001, 0b10001, 0b10001, 0b10001, 0b01010, 0b00100}},
        {'W', {0b10001, 0b10001, 0b10001, 0b10101, 0b10101, 0b11011, 0b10001}},
        {'X', {0b10001, 0b10001, 0b01010, 0b00100, 0b01010, 0b10001, 0b10001}},
        {'Y', {0b10001, 0b10001, 0b01010, 0b00100, 0b00100, 0b00100, 0b00100}},
        {'Z', {0b11111, 0b00001, 0b00010, 0b00100, 0b01000, 0b10000, 0b11111}},
        {'0', {0b01110, 0b10001, 0b10011, 0b10101, 0b11001, 0b10001, 0b01110}},
        {'1', {0b00100, 0b01100, 0b00100, 0b00100, 0b00100, 0b00100, 0b11111}},
        {'2', {0b01110, 0b10001, 0b00001, 0b00010, 0b00100, 0b01000, 0b11111}},
        {'3', {0b11111, 0b00010, 0b00100, 0b00010, 0b00001, 0b10001, 0b01110}},
        {'4', {0b00010, 0b00110, 0b01010, 0b10010, 0b11111, 0b00010, 0b00010}},
        {'5', {0b11111, 0b10000, 0b11110, 0b00001, 0b00001, 0b10001, 0b01110}},
        {'6', {0b00110, 0b01000, 0b10000, 0b11110, 0b10001, 0b10001, 0b01110}},
        {'7', {0b11111, 0b00001, 0b00010, 0b00100, 0b01000, 0b01000, 0b01000}},
        {'8', {0b01110, 0b10001, 0b10001, 0b01110, 0b10001, 0b10001, 0b01110}},
        {'9', {0b01110, 0b10001, 0b10001, 0b01111, 0b00001, 0b00010, 0b01100}},
        {'!', {0b00100, 0b00100, 0b00100, 0b00100, 0b00100, 0b00000, 0b00100}},
        {'?', {0b01110, 0b10001, 0b00001, 0b00010, 0b00100, 0b00000, 0b00100}},
        {'.', {0b00000, 0b00000, 0b00000, 0b00000, 0b00000, 0b00000, 0b00100}},
        {',', {0b00000, 0b00000, 0b00000, 0b00000, 0b00000, 0b00100, 0b01000}},
        {':', {0b00000, 0b00100, 0b00100, 0b00000, 0b00100, 0b00100, 0b00000}},
        {'-', {0b00000, 0b00000, 0b00000, 0b11111, 0b00000, 0b00000, 0b00000}},
        {'_', {0b00000, 0b00000, 0b00000, 0b00000, 0b00000, 0b00000, 0b11111}},
        {'/', {0b00001, 0b00010, 0b00010, 0b00100, 0b01000, 0b01000, 0b10000}},
        {'\\', {0b10000, 0b01000, 0b01000, 0b00100, 0b00010, 0b00010, 0b00001}},
        {'\'', {0b00100, 0b00100, 0b00000, 0b00000, 0b00000, 0b00000, 0b00000}},
        {'"', {0b01010, 0b01010, 0b00000, 0b00000, 0b00000, 0b00000, 0b00000}},
        {'+', {0b00000, 0b00100, 0b00100, 0b11111, 0b00100, 0b00100, 0b00000}},
        {'=', {0b00000, 0b00000, 0b11111, 0b00000, 0b11111, 0b00000, 0b00000}},
        {'*', {0b00000, 0b10101, 0b01110, 0b11111, 0b01110, 0b10101, 0b00000}},
        {'(', {0b00010, 0b00100, 0b01000, 0b01000, 0b01000, 0b00100, 0b00010}},
        {')', {0b01000, 0b00100, 0b00010, 0b00010, 0b00010, 0b00100, 0b01000}},
    };
    return table;
}

const Glyph kBlank{0, 0, 0, 0, 0, 0, 0};

} // namespace

auto PixelFont::glyphFor(char character) noexcept -> const Glyph& {
    const char upper = static_cast<char>(std::toupper(static_cast<unsigned char>(character)));
    const auto& table = glyphs();
    const auto found = table.find(upper);
    if (found == table.end()) {
        return kBlank;
    }
    return found->second;
}

auto PixelFont::measure(std::string_view text, float pixelSize, float letterSpacing) const noexcept
    -> float {
    if (text.empty()) {
        return 0.0F;
    }
    const float spacing = letterSpacing < 0.0F ? pixelSize : letterSpacing;
    return static_cast<float>(text.size()) * static_cast<float>(kGlyphWidth) * pixelSize +
           static_cast<float>(text.size() - 1) * spacing;
}

void PixelFont::draw(Canvas& canvas, std::string_view text, float x, float y, float pixelSize,
                     Color color, TextAlign alignment, float letterSpacing) const {
    if (text.empty() || pixelSize <= 0.0F) {
        return;
    }
    const float spacing = letterSpacing < 0.0F ? pixelSize : letterSpacing;
    const float width = measure(text, pixelSize, spacing);
    float cursorX = x;
    if (alignment == TextAlign::Center) {
        cursorX -= width * 0.5F;
    } else if (alignment == TextAlign::Right) {
        cursorX -= width;
    }

    for (const char character : text) {
        const Glyph& glyph = glyphFor(character);
        for (int row = 0; row < kGlyphHeight; ++row) {
            for (int column = 0; column < kGlyphWidth; ++column) {
                const auto mask = static_cast<std::uint8_t>(
                    1U << static_cast<unsigned>(kGlyphWidth - 1 - column));
                if ((glyph[static_cast<std::size_t>(row)] & mask) == 0U) {
                    continue;
                }
                canvas.addRect(cursorX + static_cast<float>(column) * pixelSize,
                               y + static_cast<float>(row) * pixelSize, pixelSize + 0.02F,
                               pixelSize + 0.02F, color);
            }
        }
        cursorX += static_cast<float>(kGlyphWidth) * pixelSize + spacing;
    }
}

void PixelFont::drawGlow(Canvas& canvas, std::string_view text, float x, float y, float pixelSize,
                         Color color, Color glow, TextAlign alignment) const {
    const float blur = pixelSize * 0.7F;
    const Color soft = glow.withAlpha(glow.a * 0.38F);
    draw(canvas, text, x - blur, y, pixelSize, soft, alignment);
    draw(canvas, text, x + blur, y, pixelSize, soft, alignment);
    draw(canvas, text, x, y - blur, pixelSize, soft, alignment);
    draw(canvas, text, x, y + blur, pixelSize, soft, alignment);
    draw(canvas, text, x, y, pixelSize, color, alignment);
}

} // namespace invaders::render
