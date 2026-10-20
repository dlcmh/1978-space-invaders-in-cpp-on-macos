#include "Scene.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <string>
#include <string_view>

namespace invaders::render {
namespace {

constexpr Color kSpaceTop{0.004F, 0.011F, 0.023F, 1.0F};
constexpr Color kSpaceBottom{0.001F, 0.003F, 0.008F, 1.0F};
constexpr Color kBlack{0.0F, 0.0F, 0.0F, 1.0F};
constexpr Color kWhite{0.90F, 1.0F, 0.94F, 1.0F};
constexpr Color kMint{0.57F, 1.0F, 0.74F, 1.0F};
constexpr Color kGreen{0.34F, 0.96F, 0.54F, 1.0F};
constexpr Color kDimGreen{0.27F, 0.65F, 0.43F, 1.0F};
constexpr Color kFaintGreen{0.17F, 0.43F, 0.30F, 1.0F};
constexpr Color kCyan{0.35F, 0.95F, 0.94F, 1.0F};
constexpr Color kLime{0.88F, 1.0F, 0.53F, 1.0F};
constexpr Color kMagenta{1.0F, 0.40F, 0.63F, 1.0F};
constexpr Color kAmber{1.0F, 0.68F, 0.34F, 1.0F};
constexpr Color kDanger{1.0F, 0.32F, 0.30F, 1.0F};
constexpr Color kBunker{0.23F, 0.78F, 0.46F, 1.0F};
constexpr Color kBunkerHighlight{0.61F, 1.0F, 0.72F, 0.85F};
constexpr Color kBunkerShadow{0.08F, 0.34F, 0.20F, 0.9F};
constexpr Color kGlowGreen{0.25F, 1.0F, 0.56F, 1.0F};
constexpr Color kGlowCyan{0.30F, 0.90F, 1.0F, 1.0F};
constexpr Color kGlowMagenta{1.0F, 0.25F, 0.55F, 1.0F};

struct AlienSprite {
    int width = 0;
    std::array<std::string_view, 8> rows{};
};

constexpr std::array<std::array<AlienSprite, 2>, 3> kAlienSprites = {{
    // Squid: 30 points, the small top-row alien.
    {{
        {8,
         {"...##...", "..####..", ".######.", "##.##.##", "########", "..#..#..", ".#.##.#.",
          "#.#..#.#"}},
        {8,
         {"...##...", "..####..", ".######.", "##.##.##", "########", ".#.##.#.", "#......#",
          ".#....#."}},
    }},
    // Crab: 20 points, the two middle rows.
    {{
        {11,
         {"..#.....#..", "...#...#...", "..#######..", ".##.###.##.", "###########", "#.#######.#",
          "#.#.....#.#", "...##.##..."}},
        {11,
         {"..#.....#..", "#..#...#..#", "#.#######.#", "###.###.###", "###########", ".#########.",
          "..#.....#..", ".#.......#."}},
    }},
    // Octopus: 10 points, the two bottom rows.
    {{
        {12,
         {"....####....", ".##########.", "############", "###..##..###", "############",
          "..###..###..", ".##..##..##.", "##..####..##"}},
        {12,
         {"....####....", ".##########.", "############", "###..##..###", "############",
          "...######...", "..##.##.##..", ".#..#..#..#."}},
    }},
}};

constexpr std::array<std::string_view, 7> kUfoRows = {
    "....########....", "..############..", ".##############.", "###.##.##.##.###",
    "################", "..###..##..###..", "...#....#....#.."};

constexpr std::array<std::string_view, 8> kCannonRows = {
    "......#......", ".....###.....", ".....###.....", "..#########..",
    ".###########.", "#############", "#############", "##.........##"};

[[nodiscard]] auto padded(int value, int digits) -> std::string {
    std::string text = std::to_string(std::max(0, value));
    if (static_cast<int>(text.size()) < digits) {
        text.insert(text.begin(), static_cast<std::size_t>(digits) - text.size(), '0');
    }
    if (static_cast<int>(text.size()) > digits) {
        text.erase(0, text.size() - static_cast<std::size_t>(digits));
    }
    return text;
}

[[nodiscard]] auto alienColor(AlienKind kind) -> Color {
    switch (kind) {
    case AlienKind::Squid:
        return kLime;
    case AlienKind::Crab:
        return kGreen;
    case AlienKind::Octopus:
        return kCyan;
    }
    return kGreen;
}

[[nodiscard]] auto alienGlow(AlienKind kind) -> Color {
    switch (kind) {
    case AlienKind::Squid:
        return {0.85F, 1.0F, 0.35F, 1.0F};
    case AlienKind::Crab:
        return kGlowGreen;
    case AlienKind::Octopus:
        return kGlowCyan;
    }
    return kGlowGreen;
}

[[nodiscard]] auto kindIndex(AlienKind kind) -> std::size_t {
    switch (kind) {
    case AlienKind::Squid:
        return 0;
    case AlienKind::Crab:
        return 1;
    case AlienKind::Octopus:
        return 2;
    }
    return 2;
}

[[nodiscard]] auto blink(const Game& game, float period = 1.1F) -> bool {
    return std::fmod(game.elapsed(), period) < period * 0.68F;
}

} // namespace

Scene::Scene() : effectsRng_(0x1978'51A7'CAFE'BEEFULL) {
    for (Star& star : stars_) {
        star.x = effectsRng_.unit() * static_cast<float>(kScreenWidth);
        star.y = kHudHeight + 2.0F +
                 effectsRng_.unit() * (static_cast<float>(kScreenHeight) - kHudHeight - 8.0F);
        star.size = 0.35F + effectsRng_.unit() * 1.0F;
        star.phase = effectsRng_.unit() * 6.2831853F;
        star.speed = 0.35F + effectsRng_.unit() * 1.4F;
        const float shade = 0.45F + effectsRng_.unit() * 0.55F;
        star.color = Color{0.55F * shade, 0.90F * shade, 0.72F * shade, 1.0F};
    }
}

void Scene::update(float deltaSeconds) {
    const float delta = std::clamp(deltaSeconds, 0.0F, 0.1F);
    flash_ = std::max(0.0F, flash_ - delta * 2.4F);
    muzzleFlash_ = std::max(0.0F, muzzleFlash_ - delta);

    for (Particle& particle : particles_) {
        particle.position.x += particle.velocity.x * delta;
        particle.position.y += particle.velocity.y * delta;
        particle.velocity.y += 18.0F * delta;
        particle.life -= delta;
    }
    particles_.erase(std::remove_if(particles_.begin(), particles_.end(),
                                    [](const Particle& particle) { return particle.life <= 0.0F; }),
                     particles_.end());

    for (ScorePopup& popup : popups_) {
        popup.life -= delta;
    }
    popups_.erase(std::remove_if(popups_.begin(), popups_.end(),
                                 [](const ScorePopup& popup) { return popup.life <= 0.0F; }),
                  popups_.end());
}

void Scene::handleEvent(const Event& event) {
    switch (event.type) {
    case EventType::PlayerFired:
        muzzleFlash_ = 0.10F;
        break;
    case EventType::AlienHit:
        addBurst(event.position, kGreen, 13, 42.0F, 0.65F);
        addBurst(event.position, kMint, 5, 25.0F, 0.45F);
        addScorePopup(event.position, event.points, kMint);
        flash_ = std::max(flash_, 0.16F);
        flashColor_ = kGlowGreen;
        break;
    case EventType::BunkerHit:
        addBurst(event.position, kBunker, 4, 20.0F, 0.35F);
        break;
    case EventType::PlayerHit:
        addBurst(event.position, kWhite, 16, 66.0F, 0.95F);
        addBurst(event.position, kCyan, 18, 45.0F, 0.85F);
        addBurst(event.position, kDanger, 8, 30.0F, 0.7F);
        flash_ = 0.8F;
        flashColor_ = kDanger;
        break;
    case EventType::UfoAppeared:
        break;
    case EventType::UfoHit:
        addBurst(event.position, kMagenta, 20, 60.0F, 0.9F);
        addBurst(event.position, kAmber, 12, 34.0F, 0.7F);
        addScorePopup(event.position, event.points, kMagenta);
        flash_ = 0.5F;
        flashColor_ = kGlowMagenta;
        break;
    case EventType::WaveCleared:
        flash_ = 0.4F;
        flashColor_ = kGlowGreen;
        break;
    case EventType::GameOver:
        flash_ = 0.55F;
        flashColor_ = kDanger;
        break;
    case EventType::GameStarted:
    case EventType::AlienMarch:
        break;
    }
}

void Scene::build(const Game& game) {
    canvas_.reset();
    drawBackdrop(game);

    if (game.phase() == Phase::Attract) {
        drawAttract(game);
        drawScanlines();
        return;
    }

    drawBunkers(game);
    drawAliens(game);
    drawUfo(game);
    drawPlayer(game);
    drawProjectiles(game);
    drawParticles();
    drawPopups();
    drawHud(game);
    drawFooter(game);

    if (game.phase() == Phase::Ready) {
        drawReady(game);
    } else if (game.phase() == Phase::WaveCleared) {
        drawWaveCleared(game);
    } else if (game.phase() == Phase::GameOver) {
        drawGameOver(game);
    }
    if (game.isPaused() && game.phase() == Phase::Playing) {
        drawPaused();
    }

    if (flash_ > 0.0F) {
        canvas_.addRect(0.0F, 0.0F, static_cast<float>(kScreenWidth),
                        static_cast<float>(kScreenHeight), flashColor_.withAlpha(flash_ * 0.10F));
    }
    drawScanlines();
}

void Scene::drawBackdrop(const Game& game) {
    canvas_.addGradientRect(0.0F, 0.0F, static_cast<float>(kScreenWidth),
                            static_cast<float>(kScreenHeight), kSpaceTop, kSpaceBottom);

    const float t = game.elapsed();
    canvas_.addGlow(44.0F, 84.0F, 88.0F, Color{0.06F, 0.32F, 0.26F, 0.035F}, 4);
    canvas_.addGlow(190.0F, 190.0F, 96.0F, Color{0.28F, 0.06F, 0.26F, 0.025F}, 4);
    canvas_.addGlow(112.0F, 238.0F, 96.0F, Color{0.02F, 0.22F, 0.16F, 0.04F}, 3);

    for (const Star& star : stars_) {
        const float twinkle = 0.5F + 0.5F * std::sin(t * star.speed + star.phase);
        const float alpha = 0.18F + 0.55F * twinkle;
        canvas_.addRect(star.x, star.y, star.size, star.size, star.color.withAlpha(alpha));
        if (star.size > 1.15F && twinkle > 0.7F) {
            canvas_.addLine(star.x - 1.8F, star.y + 0.4F, star.x + 2.0F, star.y + 0.4F, 0.35F,
                            star.color.withAlpha(alpha * 0.45F));
            canvas_.addLine(star.x + 0.4F, star.y - 1.8F, star.x + 0.4F, star.y + 2.0F, 0.35F,
                            star.color.withAlpha(alpha * 0.45F));
        }
    }

    // Faint phosphor trace at the very bottom of the tube.
    canvas_.addGradientRect(0.0F, static_cast<float>(kScreenHeight) - 34.0F,
                            static_cast<float>(kScreenWidth), 34.0F,
                            Color{0.005F, 0.03F, 0.02F, 0.0F}, Color{0.02F, 0.15F, 0.11F, 0.10F});
}

void Scene::drawHud(const Game& game) {
    canvas_.addRect(0.0F, 0.0F, static_cast<float>(kScreenWidth), kHudHeight,
                    Color{0.002F, 0.008F, 0.007F, 0.62F});
    canvas_.addLine(3.0F, 25.0F, static_cast<float>(kScreenWidth) - 3.0F, 25.0F, 0.35F,
                    kFaintGreen);
    canvas_.addLine(3.0F, 25.7F, 49.0F, 25.7F, 0.25F, kGlowGreen.withAlpha(0.4F));
    canvas_.addLine(174.0F, 25.7F, 221.0F, 25.7F, 0.25F, kGlowGreen.withAlpha(0.35F));

    font_.draw(canvas_, "SCORE", 6.0F, 4.0F, 0.75F, kDimGreen);
    font_.draw(canvas_, padded(game.score(), 6), 6.0F, 12.0F, 1.35F, kWhite);
    font_.draw(canvas_, "HI-SCORE", 112.0F, 4.0F, 0.75F, kDimGreen, TextAlign::Center);
    font_.draw(canvas_, padded(game.highScore(), 6), 112.0F, 12.0F, 1.35F, kMint,
               TextAlign::Center);
    font_.draw(canvas_, "WAVE", 218.0F, 4.0F, 0.75F, kDimGreen, TextAlign::Right);
    font_.draw(canvas_, padded(game.wave(), 2), 218.0F, 12.0F, 1.35F, kWhite, TextAlign::Right);
}

void Scene::drawBunkers(const Game& game) {
    for (const Bunker& bunker : game.bunkers()) {
        for (int row = 0; row < Bunker::kRows; ++row) {
            for (int column = 0; column < Bunker::kColumns; ++column) {
                if (!bunker.isSolid(column, row)) {
                    continue;
                }
                const Rect cell = bunker.cellRect(column, row);
                canvas_.addRect(cell.x, cell.y, cell.width, cell.height, kBunker);
                if (!bunker.isSolid(column, row - 1)) {
                    canvas_.addRect(cell.x, cell.y, cell.width, 0.65F, kBunkerHighlight);
                }
                if (!bunker.isSolid(column, row + 1)) {
                    canvas_.addRect(cell.x, cell.y + cell.height - 0.55F, cell.width, 0.55F,
                                    kBunkerShadow);
                }
                if (!bunker.isSolid(column - 1, row)) {
                    canvas_.addRect(cell.x, cell.y, 0.45F, cell.height, kMint.withAlpha(0.38F));
                }
            }
        }
    }
}

void Scene::drawAliens(const Game& game) {
    for (const Alien& alien : game.aliens()) {
        if (!alien.alive) {
            continue;
        }
        const Rect bounds = game.alienRect(alien);
        const Vec2 center = bounds.center();
        const std::size_t kind = kindIndex(alien.kind);
        const AlienSprite& sprite =
            kAlienSprites[kind][static_cast<std::size_t>(game.animationFrame() & 1)];
        const Vec2 topLeft{std::floor(center.x - static_cast<float>(sprite.width) * 0.5F),
                           std::floor(center.y - 4.0F)};
        canvas_.addGlow(center.x, center.y + 0.5F, 8.2F, alienGlow(alien.kind).withAlpha(0.055F),
                        3);
        drawSprite(sprite.rows, topLeft, 1.0F, alienColor(alien.kind));
    }
}

void Scene::drawAlienSprite(AlienKind kind, Vec2 center, int frame, float scale) {
    const std::size_t index = kindIndex(kind);
    const AlienSprite& sprite = kAlienSprites[index][static_cast<std::size_t>(frame & 1)];
    const float width = static_cast<float>(sprite.width) * scale;
    const Vec2 topLeft{std::floor(center.x - width * 0.5F), std::floor(center.y - 4.0F * scale)};
    canvas_.addGlow(center.x, center.y, 9.0F * scale, alienGlow(kind).withAlpha(0.07F), 3);
    drawSprite(sprite.rows, topLeft, scale, alienColor(kind));
}

void Scene::drawUfo(const Game& game) {
    if (!game.ufo().active) {
        return;
    }
    const Vec2 center = game.ufo().position;
    canvas_.addGlow(center.x, center.y, 16.0F, kGlowMagenta.withAlpha(0.10F), 4);
    canvas_.addRect(center.x - 14.0F, center.y + 1.0F, 28.0F, 0.35F, kMagenta.withAlpha(0.25F));
    drawUfoSprite(center, 1.0F);
}

void Scene::drawUfoSprite(Vec2 center, float scale) {
    const float width = 16.0F * scale;
    const Vec2 topLeft{std::floor(center.x - width * 0.5F), std::floor(center.y - 3.5F * scale)};
    for (std::size_t row = 0; row < kUfoRows.size(); ++row) {
        const std::string_view line = kUfoRows[row];
        for (std::size_t column = 0; column < line.size(); ++column) {
            if (line[column] != '#') {
                continue;
            }
            Color color = kMagenta;
            if (row == 3 && (column % 3U) == 1U) {
                color = kAmber;
            }
            canvas_.addRect(topLeft.x + static_cast<float>(column) * scale,
                            topLeft.y + static_cast<float>(row) * scale, scale + 0.03F,
                            scale + 0.03F, color);
        }
    }
}

void Scene::drawPlayer(const Game& game) {
    if (!game.player().alive && game.phase() != Phase::Ready) {
        return;
    }
    const Vec2 center{std::floor(game.player().position.x), std::floor(game.player().position.y)};
    const bool readyPulse =
        game.phase() == Phase::Ready && std::fmod(game.elapsed(), 0.36F) < 0.18F;
    const float alpha = readyPulse ? 0.56F : 1.0F;
    canvas_.addGlow(center.x, center.y + 1.0F, 15.0F, kGlowCyan.withAlpha(0.10F), 4);
    drawCannonSprite(center, 1.0F, kWhite.withAlpha(alpha));

    if (muzzleFlash_ > 0.0F) {
        const float strength = muzzleFlash_ / 0.10F;
        canvas_.addLine(center.x, center.y - 9.0F, center.x, center.y - 18.0F * strength, 1.7F,
                        kGlowCyan.withAlpha(0.55F * strength));
        canvas_.addCircle(center.x, center.y - 8.0F, 2.2F, kWhite.withAlpha(0.65F * strength), 12);
    }
}

void Scene::drawCannonSprite(Vec2 center, float scale, Color color) {
    const int width = static_cast<int>(kCannonRows.front().size());
    const Vec2 topLeft{std::floor(center.x - static_cast<float>(width) * scale * 0.5F),
                       std::floor(center.y - 4.0F * scale)};
    for (std::size_t row = 0; row < kCannonRows.size(); ++row) {
        const std::string_view line = kCannonRows[row];
        for (std::size_t column = 0; column < line.size(); ++column) {
            if (line[column] != '#') {
                continue;
            }
            canvas_.addRect(topLeft.x + static_cast<float>(column) * scale,
                            topLeft.y + static_cast<float>(row) * scale, scale + 0.02F,
                            scale + 0.02F, color);
        }
    }
}

void Scene::drawProjectiles(const Game& game) {
    if (game.playerShot().has_value()) {
        const Projectile& shot = *game.playerShot();
        canvas_.addGlow(shot.position.x, shot.position.y, 5.5F, kGlowCyan.withAlpha(0.23F), 3);
        canvas_.addRect(shot.position.x - 1.0F, shot.position.y - 4.0F, 2.0F, 8.0F,
                        kCyan.withAlpha(0.42F));
        canvas_.addRect(shot.position.x - 0.5F, shot.position.y - 3.5F, 1.0F, 7.0F, kWhite);
    }

    for (const Projectile& shot : game.enemyShots()) {
        const float x = std::floor(shot.position.x);
        const float y = std::floor(shot.position.y);
        canvas_.addGlow(x, y, 5.0F, Color{1.0F, 0.40F, 0.16F, 0.14F}, 3);
        canvas_.addRect(x - 1.0F, y - 3.0F, 2.0F, 7.0F, kAmber.withAlpha(0.38F));
        // A tiny zig-zag gives enemy fire a distinct hostile silhouette.
        canvas_.addRect(x - 0.5F, y - 3.0F, 1.2F, 2.6F, kAmber);
        canvas_.addRect(x - 1.4F, y - 0.4F, 1.6F, 2.2F, kDanger);
        canvas_.addRect(x + 0.2F, y + 1.8F, 1.2F, 2.3F, kAmber);
    }
}

void Scene::drawParticles() {
    for (const Particle& particle : particles_) {
        const float progress = std::clamp(particle.life / particle.lifetime, 0.0F, 1.0F);
        Color color = particle.color;
        color.a *= progress;
        canvas_.addRect(particle.position.x, particle.position.y, particle.size, particle.size,
                        color);
        if (particle.size > 1.1F) {
            canvas_.addRect(particle.position.x - 0.8F, particle.position.y - 0.8F,
                            particle.size + 1.6F, particle.size + 1.6F,
                            color.withAlpha(color.a * 0.12F));
        }
    }
}

void Scene::drawPopups() {
    for (const ScorePopup& popup : popups_) {
        const float progress = 1.0F - std::clamp(popup.life / popup.lifetime, 0.0F, 1.0F);
        Color color = popup.color;
        color.a *= 1.0F - progress * 0.8F;
        font_.draw(canvas_, popup.text, popup.position.x, popup.position.y - 8.0F - progress * 9.0F,
                   1.0F, color, TextAlign::Center);
    }
}

void Scene::drawAttract(const Game& game) {
    font_.draw(canvas_, "TAITO   -   1978", 112.0F, 13.0F, 0.72F, kDimGreen, TextAlign::Center);

    font_.drawGlow(canvas_, "SPACE", 112.0F, 27.0F, 4.0F, kWhite, kGlowGreen, TextAlign::Center);
    font_.drawGlow(canvas_, "INVADERS", 112.0F, 61.0F, 3.0F, kMint, kGlowGreen, TextAlign::Center);

    canvas_.addLine(34.0F, 91.0F, 82.0F, 91.0F, 0.4F, kFaintGreen);
    canvas_.addLine(142.0F, 91.0F, 190.0F, 91.0F, 0.4F, kFaintGreen);
    canvas_.addRect(111.0F, 90.0F, 2.0F, 2.0F, kMint);

    font_.draw(canvas_, "MYSTERY SHIP", 112.0F, 98.0F, 0.72F, kDimGreen, TextAlign::Center);

    drawAlienSprite(AlienKind::Squid, {82.0F, 113.0F}, 0, 1.0F);
    font_.draw(canvas_, "= 30 PTS", 95.0F, 110.0F, 0.9F, kLime);
    drawAlienSprite(AlienKind::Crab, {81.0F, 129.0F}, 0, 1.0F);
    font_.draw(canvas_, "= 20 PTS", 95.0F, 126.0F, 0.9F, kGreen);
    drawAlienSprite(AlienKind::Octopus, {81.0F, 145.0F}, 0, 1.0F);
    font_.draw(canvas_, "= 10 PTS", 95.0F, 142.0F, 0.9F, kCyan);
    drawUfoSprite({82.0F, 161.0F}, 0.85F);
    font_.draw(canvas_, "=  ???", 95.0F, 158.0F, 0.9F, kMagenta);

    if (blink(game, 1.1F)) {
        font_.drawGlow(canvas_, "PRESS SPACE TO START", 112.0F, 179.0F, 1.12F, kWhite, kGlowGreen,
                       TextAlign::Center);
    } else {
        font_.draw(canvas_, "PRESS SPACE TO START", 112.0F, 179.0F, 1.12F,
                   kDimGreen.withAlpha(0.45F), TextAlign::Center);
    }

    font_.draw(canvas_, "MOVE  ARROWS / A D     FIRE  SPACE", 112.0F, 196.0F, 0.72F, kDimGreen,
               TextAlign::Center);
    font_.draw(canvas_, "PAUSE  P / ESC     MUTE  M", 112.0F, 206.0F, 0.72F, kDimGreen,
               TextAlign::Center);

    font_.draw(canvas_, "HIGH SCORE", 112.0F, 220.0F, 0.72F, kDimGreen, TextAlign::Center);
    font_.draw(canvas_, padded(game.highScore(), 6), 112.0F, 228.0F, 1.2F, kMint,
               TextAlign::Center);

    font_.draw(canvas_, "FAN TRIBUTE - NOT AFFILIATED WITH TAITO", 112.0F, 242.0F, 0.55F,
               kFaintGreen, TextAlign::Center);

    // Decorative cabinet corners.
    canvas_.addLine(6.0F, 6.0F, 18.0F, 6.0F, 0.45F, kDimGreen.withAlpha(0.5F));
    canvas_.addLine(6.0F, 6.0F, 6.0F, 18.0F, 0.45F, kDimGreen.withAlpha(0.5F));
    canvas_.addLine(218.0F, 6.0F, 206.0F, 6.0F, 0.45F, kDimGreen.withAlpha(0.5F));
    canvas_.addLine(218.0F, 6.0F, 218.0F, 18.0F, 0.45F, kDimGreen.withAlpha(0.5F));
}

void Scene::drawReady(const Game& game) {
    if (!blink(game, 0.9F)) {
        return;
    }
    font_.draw(canvas_, "WAVE " + padded(game.wave(), 2), 112.0F, 127.0F, 1.0F, kDimGreen,
               TextAlign::Center);
    font_.drawGlow(canvas_, "READY", 112.0F, 139.0F, 2.4F, kMint, kGlowGreen, TextAlign::Center);
    font_.draw(canvas_, "DEFEND THE BASES", 112.0F, 161.0F, 0.75F, kFaintGreen, TextAlign::Center);
}

void Scene::drawGameOver(const Game& game) {
    canvas_.addRect(0.0F, 0.0F, static_cast<float>(kScreenWidth), static_cast<float>(kScreenHeight),
                    kBlack.withAlpha(0.66F));
    canvas_.addLine(38.0F, 93.0F, 186.0F, 93.0F, 0.4F, kDanger.withAlpha(0.5F));
    font_.drawGlow(canvas_, "GAME", 112.0F, 104.0F, 3.2F, kWhite, kDanger, TextAlign::Center);
    font_.drawGlow(canvas_, "OVER", 112.0F, 132.0F, 3.2F, kWhite, kDanger, TextAlign::Center);

    font_.draw(canvas_, "FINAL SCORE", 112.0F, 169.0F, 0.72F, kDimGreen, TextAlign::Center);
    font_.draw(canvas_, padded(game.score(), 6), 112.0F, 178.0F, 1.45F, kMint, TextAlign::Center);
    font_.draw(canvas_, "HIGH SCORE  " + padded(game.highScore(), 6), 112.0F, 194.0F, 0.72F,
               kFaintGreen, TextAlign::Center);

    if (blink(game, 1.05F)) {
        font_.draw(canvas_, "PRESS SPACE TO PLAY AGAIN", 112.0F, 216.0F, 0.9F, kWhite,
                   TextAlign::Center);
    }
    font_.draw(canvas_, "WAVES SURVIVED  " + padded(game.wave(), 2), 112.0F, 230.0F, 0.62F,
               kFaintGreen, TextAlign::Center);
}

void Scene::drawWaveCleared(const Game& game) {
    canvas_.addRect(0.0F, 0.0F, static_cast<float>(kScreenWidth), static_cast<float>(kScreenHeight),
                    kBlack.withAlpha(0.48F));
    font_.draw(canvas_, "WAVE " + padded(game.wave(), 2), 112.0F, 119.0F, 0.85F, kDimGreen,
               TextAlign::Center);
    font_.drawGlow(canvas_, "CLEARED", 112.0F, 132.0F, 2.8F, kWhite, kGlowGreen, TextAlign::Center);
    const std::string nextWave = "NEXT WAVE  " + padded(game.wave() + 1, 2);
    font_.draw(canvas_, nextWave, 112.0F, 160.0F, 0.9F, kMint, TextAlign::Center);
    font_.draw(canvas_, "THE INVASION DEEPENS", 112.0F, 174.0F, 0.62F, kFaintGreen,
               TextAlign::Center);
}

void Scene::drawPaused() {
    canvas_.addRect(0.0F, 0.0F, static_cast<float>(kScreenWidth), static_cast<float>(kScreenHeight),
                    kBlack.withAlpha(0.59F));
    font_.drawGlow(canvas_, "PAUSED", 112.0F, 116.0F, 2.8F, kWhite, kGlowCyan, TextAlign::Center);
    font_.draw(canvas_, "P OR ESC TO RESUME", 112.0F, 143.0F, 0.85F, kMint, TextAlign::Center);
    font_.draw(canvas_, "M TO TOGGLE SOUND", 112.0F, 155.0F, 0.7F, kDimGreen, TextAlign::Center);
}

void Scene::drawFooter(const Game& game) {
    canvas_.addLine(4.0F, 241.0F, 220.0F, 241.0F, 0.25F, kFaintGreen.withAlpha(0.8F));
    const int reserve = std::max(0, game.lives() - 1);
    for (int index = 0; index < reserve; ++index) {
        drawCannonSprite({12.0F + static_cast<float>(index) * 13.0F, 248.0F}, 0.55F,
                         kMint.withAlpha(0.85F));
    }
    if (muted_) {
        font_.draw(canvas_, "SOUND OFF", 112.0F, 245.0F, 0.65F, kAmber, TextAlign::Center);
    } else {
        font_.draw(canvas_, "CREDIT 01", 218.0F, 245.0F, 0.65F, kDimGreen, TextAlign::Right);
    }
}

void Scene::drawScanlines() {
    for (float y = 0.0F; y < static_cast<float>(kScreenHeight); y += 3.0F) {
        canvas_.addRect(0.0F, y, static_cast<float>(kScreenWidth), 0.55F, kBlack.withAlpha(0.10F));
    }
    // Edge falloff, approximated with strips because the canvas has no shader
    // post-process step.
    for (int index = 0; index < 10; ++index) {
        const float alpha = 0.03F + static_cast<float>(index) * 0.017F;
        const float width = 2.0F;
        canvas_.addRect(static_cast<float>(index) * width, 0.0F, width,
                        static_cast<float>(kScreenHeight), kBlack.withAlpha(alpha));
        canvas_.addRect(static_cast<float>(kScreenWidth) - static_cast<float>(index + 1) * width,
                        0.0F, width, static_cast<float>(kScreenHeight), kBlack.withAlpha(alpha));
    }
    canvas_.addRect(0.0F, 0.0F, static_cast<float>(kScreenWidth), 1.0F, kBlack.withAlpha(0.5F));
    canvas_.addRect(0.0F, static_cast<float>(kScreenHeight) - 1.0F,
                    static_cast<float>(kScreenWidth), 1.0F, kBlack.withAlpha(0.5F));
}

void Scene::drawSprite(const std::array<std::string_view, 8>& rows, Vec2 topLeft, float scale,
                       Color color) {
    for (std::size_t row = 0; row < rows.size(); ++row) {
        const std::string_view line = rows[row];
        for (std::size_t column = 0; column < line.size(); ++column) {
            if (line[column] != '#') {
                continue;
            }
            canvas_.addRect(topLeft.x + static_cast<float>(column) * scale,
                            topLeft.y + static_cast<float>(row) * scale, scale + 0.03F,
                            scale + 0.03F, color);
        }
    }
}

void Scene::addScorePopup(Vec2 position, int points, Color color) {
    if (points <= 0) {
        return;
    }
    popups_.push_back(ScorePopup{position, "+" + std::to_string(points), 0.95F, 0.95F, color});
}

void Scene::addBurst(Vec2 position, Color color, int count, float speed, float lifetime) {
    for (int index = 0; index < count; ++index) {
        const float angle = effectsRng_.unit() * 6.2831853F;
        const float magnitude = speed * (0.25F + effectsRng_.unit() * 0.75F);
        const float life = lifetime * (0.45F + effectsRng_.unit() * 0.75F);
        Color tint = color;
        if (effectsRng_.unit() > 0.7F) {
            tint = kWhite;
        }
        particles_.push_back(Particle{position,
                                      {std::cos(angle) * magnitude, std::sin(angle) * magnitude},
                                      life,
                                      life,
                                      0.7F + effectsRng_.unit() * 0.9F,
                                      tint});
    }
}

} // namespace invaders::render
