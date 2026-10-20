#pragma once

#include "invaders/Types.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>

namespace invaders {

/// One of the four destructible shields. The shield is a small bitmap of solid
/// cells rather than a hit-point pool, so a projectile blast erases the same
/// pixels the player can see. This is the central reason bunker damage looks
/// organic while remaining completely deterministic.
class Bunker {
public:
    static constexpr int kColumns = 22;
    static constexpr int kRows = 16;
    static constexpr float kCellSize = 2.0F;
    static constexpr float kWidth = static_cast<float>(kColumns) * kCellSize;
    static constexpr float kHeight = static_cast<float>(kRows) * kCellSize;

    Bunker() = default;
    Bunker(float x, float y);

    [[nodiscard]] auto origin() const noexcept -> Vec2 { return origin_; }
    [[nodiscard]] auto cells() const noexcept -> const std::array<std::uint8_t, kColumns * kRows>& {
        return cells_;
    }
    [[nodiscard]] auto isSolid(int column, int row) const noexcept -> bool;
    [[nodiscard]] auto remainingCells() const noexcept -> int { return remaining_; }
    [[nodiscard]] auto cellRect(int column, int row) const noexcept -> Rect;
    [[nodiscard]] auto hitTest(Rect projectile) const noexcept -> bool;
    [[nodiscard]] auto hitPoint(Rect projectile) const noexcept -> std::optional<Vec2>;

    /// Erase every cell whose centre lies within `radius` of `center`. Returns
    /// true when at least one cell changed.
    auto carve(Vec2 center, float radius) noexcept -> bool;

private:
    Vec2 origin_{};
    std::array<std::uint8_t, kColumns * kRows> cells_{};
    int remaining_ = 0;
};

/// A wave always begins with four pristine shields at the canonical positions.
[[nodiscard]] auto makeBunkers() -> std::array<Bunker, 4>;

} // namespace invaders
