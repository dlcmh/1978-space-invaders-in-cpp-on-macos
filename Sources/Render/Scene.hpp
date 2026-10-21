#pragma once

#include "Canvas.hpp"
#include "PixelFont.hpp"

#include "invaders/Game.hpp"
#include "invaders/Rng.hpp"

#include <array>
#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace invaders::render {

/// Presentation-local effects. None of this feeds back into the simulation:
/// particles, score popups, and flashes are driven by one-way events.
struct Particle {
    Vec2 position{};
    Vec2 velocity{};
    float life = 0.0F;
    float lifetime = 0.0F;
    float size = 1.0F;
    Color color{};
};

struct ScorePopup {
    Vec2 position{};
    std::string text;
    float life = 0.0F;
    float lifetime = 0.0F;
    Color color{};
};

/// Composes a complete arcade frame onto the immediate-mode canvas. The scene
/// reads the simulation as immutable input; all art is generated procedurally.
class Scene {
public:
    Scene();

    void update(float deltaSeconds);
    void handleEvent(const Event& event);
    void setMuted(bool muted) noexcept { muted_ = muted; }
    [[nodiscard]] auto muted() const noexcept -> bool { return muted_; }

    void build(const Game& game);
    [[nodiscard]] auto canvas() const noexcept -> const Canvas& { return canvas_; }

private:
    struct Star {
        float x = 0.0F;
        float y = 0.0F;
        float size = 1.0F;
        float phase = 0.0F;
        float speed = 1.0F;
        Color color{};
    };

    void drawBackdrop(const Game& game);
    void drawHud(const Game& game);
    void drawBunkers(const Game& game);
    void drawAliens(const Game& game);
    void drawUfo(const Game& game);
    void drawPlayer(const Game& game);
    void drawProjectiles(const Game& game);
    void drawParticles();
    void drawPopups();
    void drawAttract(const Game& game);
    void drawReady(const Game& game);
    void drawGameOver(const Game& game);
    void drawWaveCleared(const Game& game);
    void drawPaused();
    void drawFooter(const Game& game);
    void drawScanlines();
    void drawAlienSprite(AlienKind kind, Vec2 center, int frame, float scale = 1.0F);
    void drawUfoSprite(Vec2 center, float scale = 1.0F);
    void drawCannonSprite(Vec2 center, float scale, Color color);
    void drawSprite(const std::array<std::string_view, 8>& rows, Vec2 topLeft, float scale,
                    Color color);
    void addScorePopup(Vec2 position, int points, Color color);
    void addBurst(Vec2 position, Color color, int count, float speed, float lifetime);

    Canvas canvas_;
    PixelFont font_;
    SplitMix64 effectsRng_;
    std::vector<Particle> particles_;
    std::vector<ScorePopup> popups_;
    std::array<Star, 110> stars_{};
    float flash_ = 0.0F;
    float muzzleFlash_ = 0.0F;
    Color flashColor_{};
    bool muted_ = false;
};

} // namespace invaders::render
