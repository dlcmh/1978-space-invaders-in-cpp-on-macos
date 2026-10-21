#pragma once

#include "invaders/Bunker.hpp"
#include "invaders/Rng.hpp"
#include "invaders/Types.hpp"

#include <array>
#include <cstddef>
#include <optional>
#include <span>
#include <vector>

namespace invaders {

/// The authoritative, headless simulation. It owns every gameplay rule and
/// has no knowledge of AppKit, Metal, audio, or wall-clock time. Presentation
/// may read its state but can never mutate it; all input enters through the
/// explicit command methods below.
///
/// Update with a fixed 1/60 s step. That keeps the march cadence, projectile
/// collision, and enemy-fire cadence deterministic across displays.
class Game final {
public:
    explicit Game(GameOptions options = {});

    // Commands. Commands that don't apply to the current phase are no-ops.
    void requestStart();
    void startNewGame();
    void togglePause();
    void pause();
    void setMoveLeft(bool held) noexcept;
    void setMoveRight(bool held) noexcept;
    void setFiring(bool held) noexcept;

    /// Advance one fixed simulation step. Clamped to 50 ms defensively, though
    /// the application always supplies exactly 1/60 s.
    void update(float deltaSeconds);

    [[nodiscard]] auto drainEvents() -> std::vector<Event>;

    // Read-only state for presentation and checks.
    [[nodiscard]] auto phase() const noexcept -> Phase { return phase_; }
    [[nodiscard]] auto isPaused() const noexcept -> bool { return paused_; }
    [[nodiscard]] auto score() const noexcept -> int { return score_; }
    [[nodiscard]] auto highScore() const noexcept -> int { return highScore_; }
    [[nodiscard]] auto wave() const noexcept -> int { return wave_; }
    [[nodiscard]] auto lives() const noexcept -> int { return lives_; }
    [[nodiscard]] auto elapsed() const noexcept -> float { return elapsed_; }
    [[nodiscard]] auto readyRemaining() const noexcept -> float { return readyTimer_; }
    [[nodiscard]] auto deathProgress() const noexcept -> float;
    [[nodiscard]] auto waveClearProgress() const noexcept -> float;
    [[nodiscard]] auto player() const noexcept -> const Player& { return player_; }
    [[nodiscard]] auto playerShot() const noexcept -> const std::optional<Projectile>& {
        return playerShot_;
    }
    [[nodiscard]] auto enemyShots() const noexcept -> std::span<const Projectile> {
        return enemyShots_;
    }
    [[nodiscard]] auto aliens() const noexcept -> std::span<const Alien> { return aliens_; }
    [[nodiscard]] auto bunkers() const noexcept -> const std::array<Bunker, 4>& { return bunkers_; }
    [[nodiscard]] auto ufo() const noexcept -> const Ufo& { return ufo_; }
    [[nodiscard]] auto formationOrigin() const noexcept -> Vec2 { return formationOrigin_; }
    [[nodiscard]] auto animationFrame() const noexcept -> int { return animationFrame_; }
    [[nodiscard]] auto aliensRemaining() const noexcept -> int { return aliensRemaining_; }
    [[nodiscard]] auto marchInterval() const noexcept -> float;
    [[nodiscard]] auto playerSpeed() const noexcept -> float;
    [[nodiscard]] auto alienPoints(AlienKind kind) const noexcept -> int;
    [[nodiscard]] auto alienRect(const Alien& alien) const noexcept -> Rect;

private:
    void resetField();
    void beginReady(float seconds);
    void updatePlaying(float deltaSeconds);
    void updatePlayer(float deltaSeconds);
    void updatePlayerShot(float deltaSeconds);
    void updateEnemyShots(float deltaSeconds);
    void updateMarch(float deltaSeconds);
    void updateEnemyFire(float deltaSeconds);
    void updateUfo(float deltaSeconds);
    void firePlayerShot();
    void spawnEnemyShot();
    void stepFormation();
    void hitAlien(std::size_t alienIndex, Vec2 position);
    void hitBunker(Bunker& bunker, Vec2 position, float radius);
    void playerHit();
    void loseLife();
    void finishDeath();
    void endGame();
    void completeWave();
    void advanceWave();
    void addScore(int points);
    void postEvent(EventType type, Vec2 position = {}, int points = 0);
    [[nodiscard]] auto lowestAlienInColumn(int column) const noexcept -> const Alien*;
    [[nodiscard]] auto nextUfoScore() noexcept -> int;
    [[nodiscard]] auto enemyShotSpeed() const noexcept -> float;
    [[nodiscard]] auto maximumEnemyShots() const noexcept -> int;

    GameOptions options_;
    SplitMix64 rng_;
    Phase phase_ = Phase::Attract;
    bool paused_ = false;
    bool leftHeld_ = false;
    bool rightHeld_ = false;
    bool fireHeld_ = false;

    int score_ = 0;
    int highScore_ = 0;
    int wave_ = 1;
    int lives_ = 3;
    int aliensRemaining_ = kAlienCount;
    int animationFrame_ = 0;

    Player player_{};
    std::optional<Projectile> playerShot_;
    std::vector<Projectile> enemyShots_;
    std::vector<Alien> aliens_;
    std::array<Bunker, 4> bunkers_{};
    Ufo ufo_{};

    Vec2 formationOrigin_{};
    float formationDirection_ = -1.0F;
    float marchTimer_ = 0.0F;
    float fireCooldown_ = 0.0F;
    float enemyFireTimer_ = 0.0F;
    float ufoTimer_ = 0.0F;
    float readyTimer_ = 0.0F;
    float dyingTimer_ = 0.0F;
    float waveClearTimer_ = 0.0F;
    float elapsed_ = 0.0F;

    std::vector<Event> events_;
};

} // namespace invaders
