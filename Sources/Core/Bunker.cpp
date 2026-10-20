#include "invaders/Bunker.hpp"

#include <algorithm>
#include <cmath>

namespace invaders {
namespace {

constexpr std::array<float, 4> kBunkerX = {12.0F, 64.0F, 116.0F, 168.0F};

/// The classic shield silhouette: a rounded top with a widening notch cut up
/// from the bottom centre. Everything is authored in cells, so the shape is
/// crisp at every logical pixel size.
[[nodiscard]] constexpr auto isShapeCell(int column, int row) noexcept -> bool {
    if (column < 0 || column >= Bunker::kColumns || row < 0 || row >= Bunker::kRows) {
        return false;
    }

    if (row < 4) {
        const int inset = 4 - row;
        if (column < inset || column >= Bunker::kColumns - inset) {
            return false;
        }
    }

    if (row >= 11) {
        const float centre = (static_cast<float>(Bunker::kColumns) - 1.0F) * 0.5F;
        const float halfWidth = 0.8F + static_cast<float>(row - 11) * 0.62F;
        if (std::abs(static_cast<float>(column) - centre) <= halfWidth) {
            return false;
        }
    }
    return true;
}

} // namespace

Bunker::Bunker(float x, float y) : origin_{x, y} {
    for (int row = 0; row < kRows; ++row) {
        for (int column = 0; column < kColumns; ++column) {
            if (isShapeCell(column, row)) {
                cells_[static_cast<std::size_t>(row * kColumns + column)] = 1U;
                ++remaining_;
            }
        }
    }
}

auto Bunker::isSolid(int column, int row) const noexcept -> bool {
    if (column < 0 || column >= kColumns || row < 0 || row >= kRows) {
        return false;
    }
    return cells_[static_cast<std::size_t>(row * kColumns + column)] != 0U;
}

auto Bunker::cellRect(int column, int row) const noexcept -> Rect {
    return {origin_.x + static_cast<float>(column) * kCellSize,
            origin_.y + static_cast<float>(row) * kCellSize, kCellSize, kCellSize};
}

auto Bunker::hitTest(Rect projectile) const noexcept -> bool {
    const int firstColumn =
        std::max(0, static_cast<int>(std::floor((projectile.x - origin_.x) / kCellSize)));
    const int lastColumn = std::min(
        kColumns - 1, static_cast<int>(std::floor((projectile.right() - origin_.x) / kCellSize)));
    const int firstRow =
        std::max(0, static_cast<int>(std::floor((projectile.y - origin_.y) / kCellSize)));
    const int lastRow = std::min(
        kRows - 1, static_cast<int>(std::floor((projectile.bottom() - origin_.y) / kCellSize)));

    for (int row = firstRow; row <= lastRow; ++row) {
        for (int column = firstColumn; column <= lastColumn; ++column) {
            if (isSolid(column, row) && cellRect(column, row).intersects(projectile)) {
                return true;
            }
        }
    }
    return false;
}

auto Bunker::hitPoint(Rect projectile) const noexcept -> std::optional<Vec2> {
    const int firstColumn =
        std::max(0, static_cast<int>(std::floor((projectile.x - origin_.x) / kCellSize)));
    const int lastColumn = std::min(
        kColumns - 1, static_cast<int>(std::floor((projectile.right() - origin_.x) / kCellSize)));
    const int firstRow =
        std::max(0, static_cast<int>(std::floor((projectile.y - origin_.y) / kCellSize)));
    const int lastRow = std::min(
        kRows - 1, static_cast<int>(std::floor((projectile.bottom() - origin_.y) / kCellSize)));

    for (int row = firstRow; row <= lastRow; ++row) {
        for (int column = firstColumn; column <= lastColumn; ++column) {
            if (isSolid(column, row) && cellRect(column, row).intersects(projectile)) {
                return cellRect(column, row).center();
            }
        }
    }
    return std::nullopt;
}

auto Bunker::carve(Vec2 center, float radius) noexcept -> bool {
    if (radius <= 0.0F) {
        return false;
    }
    bool changed = false;
    const int firstColumn =
        std::max(0, static_cast<int>(std::floor((center.x - radius - origin_.x) / kCellSize)));
    const int lastColumn = std::min(
        kColumns - 1, static_cast<int>(std::floor((center.x + radius - origin_.x) / kCellSize)));
    const int firstRow =
        std::max(0, static_cast<int>(std::floor((center.y - radius - origin_.y) / kCellSize)));
    const int lastRow = std::min(
        kRows - 1, static_cast<int>(std::floor((center.y + radius - origin_.y) / kCellSize)));

    for (int row = firstRow; row <= lastRow; ++row) {
        for (int column = firstColumn; column <= lastColumn; ++column) {
            if (!isSolid(column, row)) {
                continue;
            }
            const Vec2 cellCentre = cellRect(column, row).center();
            const float dx = cellCentre.x - center.x;
            const float dy = cellCentre.y - center.y;
            if (dx * dx + dy * dy <= radius * radius) {
                cells_[static_cast<std::size_t>(row * kColumns + column)] = 0U;
                --remaining_;
                changed = true;
            }
        }
    }
    return changed;
}

auto makeBunkers() -> std::array<Bunker, 4> {
    return {Bunker{kBunkerX[0], kBunkerY}, Bunker{kBunkerX[1], kBunkerY},
            Bunker{kBunkerX[2], kBunkerY}, Bunker{kBunkerX[3], kBunkerY}};
}

} // namespace invaders
