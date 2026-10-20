#pragma once

#include <cstdint>
#include <limits>

namespace invaders {

/// SplitMix64: tiny, deterministic, and statistically adequate for enemy fire
/// selection and mystery-ship scoring. Simulation randomness is deliberately
/// seedable so a run can be replayed exactly in checks.
class SplitMix64 {
public:
    explicit constexpr SplitMix64(std::uint64_t seed = 0x1978'5ACE'CAFE'BEEFULL) noexcept
        : state_(seed) {}

    [[nodiscard]] constexpr auto next() noexcept -> std::uint64_t {
        state_ += 0x9E37'79B9'7F4A'7C15ULL;
        std::uint64_t z = state_;
        z = (z ^ (z >> 30U)) * 0xBF58'476D'1CE4'E5B9ULL;
        z = (z ^ (z >> 27U)) * 0x94D0'49BB'1331'11EBULL;
        return z ^ (z >> 31U);
    }

    /// Uniform in [0, 1). The result deliberately never rounds up to 1.0.
    [[nodiscard]] constexpr auto unit() noexcept -> float {
        constexpr auto scale = 1.0F / 16'777'216.0F;
        return static_cast<float>(next() >> 40U) * scale;
    }

    [[nodiscard]] constexpr auto range(int minInclusive, int maxInclusive) noexcept -> int {
        if (maxInclusive <= minInclusive) {
            return minInclusive;
        }
        const auto span = static_cast<std::uint64_t>(maxInclusive - minInclusive + 1);
        return minInclusive + static_cast<int>(next() % span);
    }

private:
    std::uint64_t state_;
};

} // namespace invaders
