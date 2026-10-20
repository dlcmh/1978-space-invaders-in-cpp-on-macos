#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string_view>
#include <vector>

namespace invaders {

/// The arcade cabinet's portrait playfield, expressed in logical pixels.
/// Presentation scales this coordinate space to the drawable; simulation never
/// depends on the window size or display refresh rate.
inline constexpr int kScreenWidth = 224;
inline constexpr int kScreenHeight = 256;

inline constexpr int kAlienColumns = 11;
inline constexpr int kAlienRows = 5;
inline constexpr int kAlienCount = kAlienColumns * kAlienRows;

inline constexpr float kHudHeight = 28.0F;
inline constexpr float kPlayerY = 228.0F;
inline constexpr float kBunkerY = 186.0F;
inline constexpr float kInvasionY = 218.0F;

/// A point or vector in logical screen space. Y increases downward, matching
/// both the arcade's raster order and Metal's viewport convention.
struct Vec2 {
    float x = 0.0F;
    float y = 0.0F;

    [[nodiscard]] constexpr auto operator+(Vec2 other) const noexcept -> Vec2 {
        return {x + other.x, y + other.y};
    }
    [[nodiscard]] constexpr auto operator-(Vec2 other) const noexcept -> Vec2 {
        return {x - other.x, y - other.y};
    }
    [[nodiscard]] constexpr auto operator==(const Vec2& other) const noexcept -> bool = default;
};

struct Rect {
    float x = 0.0F;
    float y = 0.0F;
    float width = 0.0F;
    float height = 0.0F;

    [[nodiscard]] constexpr auto right() const noexcept -> float { return x + width; }
    [[nodiscard]] constexpr auto bottom() const noexcept -> float { return y + height; }
    [[nodiscard]] constexpr auto center() const noexcept -> Vec2 {
        return {x + width * 0.5F, y + height * 0.5F};
    }
    [[nodiscard]] constexpr auto intersects(Rect other) const noexcept -> bool {
        return x < other.right() && right() > other.x && y < other.bottom() && bottom() > other.y;
    }
    [[nodiscard]] constexpr auto contains(Vec2 point) const noexcept -> bool {
        return point.x >= x && point.x <= right() && point.y >= y && point.y <= bottom();
    }
};

enum class Phase {
    Attract,
    Ready,
    Playing,
    Dying,
    WaveCleared,
    GameOver,
};

enum class AlienKind : std::uint8_t {
    Squid,
    Crab,
    Octopus,
};

enum class EventType : std::uint8_t {
    GameStarted,
    PlayerFired,
    AlienMarch,
    AlienHit,
    PlayerHit,
    UfoAppeared,
    UfoHit,
    BunkerHit,
    WaveCleared,
    GameOver,
};

/// A small, renderer-facing description of something audible or visible that
/// happened during an update. Events carry positions so presentation can react
/// without re-deriving simulation state.
struct Event {
    EventType type = EventType::GameStarted;
    Vec2 position{};
    int points = 0;
};

struct Alien {
    AlienKind kind = AlienKind::Octopus;
    int column = 0;
    int row = 0;
    bool alive = true;
};

struct Projectile {
    Vec2 position{};
    Vec2 size{1.0F, 6.0F};
};

struct Ufo {
    bool active = false;
    Vec2 position{};
    int points = 100;
    float direction = 1.0F;
};

struct Player {
    Vec2 position{};
    bool alive = true;
};

/// Runtime tuning knobs that belong to a game rather than to the renderer.
/// Defaults reproduce the shipped arcade settings; tests may vary the seed
/// without reaching into simulation internals.
struct GameOptions {
    std::uint64_t seed = 0x1978'5ACE'CAFE'BEEFULL;
    int startingLives = 3;
    int highScore = 0;
};

} // namespace invaders
