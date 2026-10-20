#include "invaders/Game.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <utility>

namespace invaders {
namespace {

constexpr float kPlayerSpeed = 92.0F;
constexpr float kPlayerShotSpeed = 214.0F;
constexpr float kDeathDuration = 1.7F;
constexpr float kWaveClearDuration = 2.0F;
constexpr float kFormationLeft = 4.0F;
constexpr float kFormationRight = 220.0F;
constexpr float kFormationWidth = 174.0F;
constexpr float kMarchStep = 2.0F;
constexpr float kMarchDescent = 8.0F;
constexpr float kUfoSpeed = 38.0F;
constexpr float kUfoY = 36.0F;

[[nodiscard]] constexpr auto projectileRect(const Projectile& projectile) noexcept -> Rect {
    return {projectile.position.x - projectile.size.x * 0.5F,
            projectile.position.y - projectile.size.y * 0.5F, projectile.size.x, projectile.size.y};
}

[[nodiscard]] constexpr auto playerRect(const Player& player) noexcept -> Rect {
    return {player.position.x - 6.5F, player.position.y - 4.0F, 13.0F, 8.0F};
}

[[nodiscard]] constexpr auto ufoRect(const Ufo& ufo) noexcept -> Rect {
    return {ufo.position.x - 10.0F, ufo.position.y - 4.0F, 20.0F, 8.0F};
}

} // namespace

Game::Game(GameOptions options)
    : options_(options), rng_(options.seed), highScore_(std::max(0, options.highScore)),
      lives_(std::max(1, options.startingLives)) {
    resetField();
}

void Game::requestStart() {
    if (phase_ == Phase::Attract || phase_ == Phase::GameOver) {
        startNewGame();
    }
}

void Game::startNewGame() {
    score_ = 0;
    wave_ = 1;
    lives_ = std::max(1, options_.startingLives);
    paused_ = false;
    leftHeld_ = false;
    rightHeld_ = false;
    fireHeld_ = false;
    resetField();
    beginReady(1.5F);
    postEvent(EventType::GameStarted, {kScreenWidth * 0.5F, kPlayerY});
}

void Game::togglePause() {
    if (phase_ != Phase::Playing) {
        return;
    }
    paused_ = !paused_;
    leftHeld_ = false;
    rightHeld_ = false;
    fireHeld_ = false;
}

void Game::pause() {
    if (phase_ == Phase::Playing && !paused_) {
        paused_ = true;
        leftHeld_ = false;
        rightHeld_ = false;
        fireHeld_ = false;
    }
}

void Game::setMoveLeft(bool held) noexcept {
    leftHeld_ = held;
}

void Game::setMoveRight(bool held) noexcept {
    rightHeld_ = held;
}

void Game::setFiring(bool held) noexcept {
    fireHeld_ = held;
}

void Game::update(float deltaSeconds) {
    if (paused_ || deltaSeconds <= 0.0F) {
        return;
    }
    const float delta = std::min(deltaSeconds, 0.05F);
    elapsed_ += delta;
    fireCooldown_ = std::max(0.0F, fireCooldown_ - delta);

    switch (phase_) {
    case Phase::Attract:
    case Phase::GameOver:
        break;
    case Phase::Ready:
        readyTimer_ -= delta;
        if (readyTimer_ <= 0.0F) {
            readyTimer_ = 0.0F;
            phase_ = Phase::Playing;
            enemyFireTimer_ = 0.85F + rng_.unit() * 0.5F;
        }
        break;
    case Phase::Playing:
        updatePlaying(delta);
        break;
    case Phase::Dying:
        dyingTimer_ -= delta;
        if (dyingTimer_ <= 0.0F) {
            finishDeath();
        }
        break;
    case Phase::WaveCleared:
        waveClearTimer_ -= delta;
        if (waveClearTimer_ <= 0.0F) {
            advanceWave();
        }
        break;
    }
}

auto Game::drainEvents() -> std::vector<Event> {
    auto drained = std::move(events_);
    events_.clear();
    return drained;
}

auto Game::deathProgress() const noexcept -> float {
    if (phase_ != Phase::Dying) {
        return 0.0F;
    }
    return std::clamp(1.0F - dyingTimer_ / kDeathDuration, 0.0F, 1.0F);
}

auto Game::waveClearProgress() const noexcept -> float {
    if (phase_ != Phase::WaveCleared) {
        return 0.0F;
    }
    return std::clamp(1.0F - waveClearTimer_ / kWaveClearDuration, 0.0F, 1.0F);
}

auto Game::marchInterval() const noexcept -> float {
    const float density =
        static_cast<float>(std::max(aliensRemaining_, 0)) / static_cast<float>(kAlienCount);
    const float base = 0.075F + 0.68F * std::pow(density, 1.35F);
    const float waveScale = 1.0F + 0.16F * static_cast<float>(wave_ - 1);
    return std::max(0.055F, base / waveScale);
}

auto Game::playerSpeed() const noexcept -> float {
    return kPlayerSpeed;
}

auto Game::alienPoints(AlienKind kind) const noexcept -> int {
    switch (kind) {
    case AlienKind::Squid:
        return 30;
    case AlienKind::Crab:
        return 20;
    case AlienKind::Octopus:
        return 10;
    }
    return 10;
}

auto Game::alienRect(const Alien& alien) const noexcept -> Rect {
    return {formationOrigin_.x + static_cast<float>(alien.column) * 16.0F + 2.0F,
            formationOrigin_.y + static_cast<float>(alien.row) * 12.0F + 2.0F, 12.0F, 8.0F};
}

void Game::resetField() {
    aliens_.clear();
    aliens_.reserve(kAlienCount);
    for (int row = 0; row < kAlienRows; ++row) {
        AlienKind kind = AlienKind::Octopus;
        if (row == 0) {
            kind = AlienKind::Squid;
        } else if (row <= 2) {
            kind = AlienKind::Crab;
        }
        for (int column = 0; column < kAlienColumns; ++column) {
            aliens_.push_back(Alien{kind, column, row, true});
        }
    }

    formationOrigin_ = {25.0F, 44.0F};
    formationDirection_ = -1.0F;
    marchTimer_ = 0.62F;
    animationFrame_ = 0;
    aliensRemaining_ = kAlienCount;

    player_ = Player{{static_cast<float>(kScreenWidth) * 0.5F, kPlayerY}, true};
    playerShot_.reset();
    enemyShots_.clear();
    bunkers_ = makeBunkers();
    ufo_ = Ufo{};
    ufoTimer_ = 11.0F + rng_.unit() * 10.0F;
    enemyFireTimer_ = 0.85F;
    fireCooldown_ = 0.0F;
    readyTimer_ = 0.0F;
    dyingTimer_ = 0.0F;
    waveClearTimer_ = 0.0F;
}

void Game::beginReady(float seconds) {
    phase_ = Phase::Ready;
    readyTimer_ = seconds;
    player_.alive = true;
    player_.position = {static_cast<float>(kScreenWidth) * 0.5F, kPlayerY};
    playerShot_.reset();
    enemyShots_.clear();
    marchTimer_ = 0.62F;
    enemyFireTimer_ = 0.9F;
    fireCooldown_ = 0.0F;
}

void Game::updatePlaying(float deltaSeconds) {
    updatePlayer(deltaSeconds);
    if (phase_ != Phase::Playing) {
        return;
    }

    updateMarch(deltaSeconds);
    if (phase_ != Phase::Playing) {
        return;
    }

    updatePlayerShot(deltaSeconds);
    if (phase_ != Phase::Playing) {
        return;
    }

    updateEnemyShots(deltaSeconds);
    if (phase_ != Phase::Playing) {
        return;
    }

    updateEnemyFire(deltaSeconds);
    updateUfo(deltaSeconds);
}

void Game::updatePlayer(float deltaSeconds) {
    int direction = 0;
    if (leftHeld_) {
        --direction;
    }
    if (rightHeld_) {
        ++direction;
    }
    if (direction != 0) {
        player_.position.x += static_cast<float>(direction) * kPlayerSpeed * deltaSeconds;
        player_.position.x =
            std::clamp(player_.position.x, 8.0F, static_cast<float>(kScreenWidth) - 8.0F);
    }

    if (fireHeld_ && !playerShot_ && fireCooldown_ <= 0.0F) {
        firePlayerShot();
    }
}

void Game::firePlayerShot() {
    playerShot_ = Projectile{{player_.position.x, player_.position.y - 9.0F}, {1.5F, 6.0F}};
    fireCooldown_ = 0.16F;
    postEvent(EventType::PlayerFired, playerShot_->position);
}

void Game::updatePlayerShot(float deltaSeconds) {
    if (!playerShot_) {
        return;
    }

    playerShot_->position.y -= kPlayerShotSpeed * deltaSeconds;
    if (playerShot_->position.y < kHudHeight + 1.0F) {
        playerShot_.reset();
        return;
    }

    const Rect shotRect = projectileRect(*playerShot_);
    for (auto& bunker : bunkers_) {
        if (const auto point = bunker.hitPoint(shotRect); point.has_value()) {
            hitBunker(bunker, *point, 4.4F);
            playerShot_.reset();
            return;
        }
    }

    if (ufo_.active && shotRect.intersects(ufoRect(ufo_))) {
        const Vec2 position = ufo_.position;
        const int points = ufo_.points;
        ufo_.active = false;
        ufoTimer_ = 14.0F + rng_.unit() * 9.0F;
        addScore(points);
        postEvent(EventType::UfoHit, position, points);
        playerShot_.reset();
        return;
    }

    for (std::size_t index = 0; index < aliens_.size(); ++index) {
        const Alien& alien = aliens_[index];
        if (alien.alive && shotRect.intersects(alienRect(alien))) {
            const Vec2 hitPosition = alienRect(alien).center();
            playerShot_.reset();
            hitAlien(index, hitPosition);
            return;
        }
    }
}

void Game::hitAlien(std::size_t alienIndex, Vec2 position) {
    Alien& alien = aliens_[alienIndex];
    if (!alien.alive) {
        return;
    }
    alien.alive = false;
    --aliensRemaining_;
    const int points = alienPoints(alien.kind);
    addScore(points);
    postEvent(EventType::AlienHit, position, points);

    if (aliensRemaining_ <= 0) {
        completeWave();
    }
}

void Game::hitBunker(Bunker& bunker, Vec2 position, float radius) {
    if (bunker.carve(position, radius)) {
        postEvent(EventType::BunkerHit, position);
    }
}

void Game::updateEnemyShots(float deltaSeconds) {
    const float speed = enemyShotSpeed();
    const Rect playerBounds = playerRect(player_);

    for (std::size_t index = 0; index < enemyShots_.size();) {
        Projectile& shot = enemyShots_[index];
        shot.position.y += speed * deltaSeconds;

        if (shot.position.y > static_cast<float>(kScreenHeight) + 6.0F) {
            enemyShots_.erase(enemyShots_.begin() + static_cast<std::ptrdiff_t>(index));
            continue;
        }

        const Rect shotRect = projectileRect(shot);
        bool consumed = false;
        for (auto& bunker : bunkers_) {
            if (const auto point = bunker.hitPoint(shotRect); point.has_value()) {
                hitBunker(bunker, *point, 3.2F);
                consumed = true;
                break;
            }
        }
        if (consumed) {
            enemyShots_.erase(enemyShots_.begin() + static_cast<std::ptrdiff_t>(index));
            continue;
        }

        if (player_.alive && shotRect.intersects(playerBounds)) {
            enemyShots_.erase(enemyShots_.begin() + static_cast<std::ptrdiff_t>(index));
            playerHit();
            return;
        }
        ++index;
    }
}

void Game::updateEnemyFire(float deltaSeconds) {
    enemyFireTimer_ -= deltaSeconds;
    if (enemyFireTimer_ > 0.0F) {
        return;
    }
    if (static_cast<int>(enemyShots_.size()) < maximumEnemyShots() && aliensRemaining_ > 0) {
        spawnEnemyShot();
    }
    const float wavePressure = std::min(0.3F, 0.05F * static_cast<float>(wave_ - 1));
    enemyFireTimer_ = (0.62F + rng_.unit() * 0.9F) * (1.0F - wavePressure);
}

void Game::spawnEnemyShot() {
    std::array<int, kAlienColumns> columns{};
    int columnCount = 0;
    for (int column = 0; column < kAlienColumns; ++column) {
        if (lowestAlienInColumn(column) != nullptr) {
            columns[static_cast<std::size_t>(columnCount)] = column;
            ++columnCount;
        }
    }
    if (columnCount == 0) {
        return;
    }
    const int column = columns[static_cast<std::size_t>(rng_.range(0, columnCount - 1))];
    const Alien* alien = lowestAlienInColumn(column);
    if (alien == nullptr) {
        return;
    }
    const Rect bounds = alienRect(*alien);
    enemyShots_.push_back(Projectile{{bounds.center().x, bounds.bottom() + 4.0F}, {3.0F, 6.0F}});
}

auto Game::lowestAlienInColumn(int column) const noexcept -> const Alien* {
    const Alien* result = nullptr;
    for (const Alien& alien : aliens_) {
        if (alien.alive && alien.column == column &&
            (result == nullptr || alien.row > result->row)) {
            result = &alien;
        }
    }
    return result;
}

void Game::updateMarch(float deltaSeconds) {
    if (aliensRemaining_ <= 0) {
        return;
    }
    marchTimer_ -= deltaSeconds;
    if (marchTimer_ > 0.0F) {
        return;
    }
    stepFormation();
    if (phase_ != Phase::Playing) {
        return;
    }
    marchTimer_ = marchInterval();
}

void Game::stepFormation() {
    animationFrame_ ^= 1;
    const float nextX = formationOrigin_.x + formationDirection_ * kMarchStep;
    const bool atEdge = nextX < kFormationLeft || nextX + kFormationWidth > kFormationRight;

    if (atEdge) {
        formationOrigin_.y += kMarchDescent;
        formationDirection_ = -formationDirection_;
    } else {
        formationOrigin_.x = nextX;
    }

    postEvent(EventType::AlienMarch,
              {formationOrigin_.x + kFormationWidth * 0.5F, formationOrigin_.y});

    // The falling formation erodes shields it touches, just as the original's
    // aliens did while marching through the player's cover.
    if (atEdge) {
        for (const Alien& alien : aliens_) {
            if (!alien.alive) {
                continue;
            }
            const Rect bounds = alienRect(alien);
            for (auto& bunker : bunkers_) {
                if (bunker.hitTest(bounds)) {
                    bunker.carve(bounds.center(), 6.8F);
                }
            }
        }
    }

    for (const Alien& alien : aliens_) {
        if (alien.alive && alienRect(alien).bottom() >= kInvasionY) {
            endGame();
            return;
        }
    }
}

void Game::updateUfo(float deltaSeconds) {
    if (ufo_.active) {
        ufo_.position.x += ufo_.direction * kUfoSpeed * deltaSeconds;
        if (ufo_.position.x < -14.0F ||
            ufo_.position.x > static_cast<float>(kScreenWidth) + 14.0F) {
            ufo_.active = false;
            ufoTimer_ = 15.0F + rng_.unit() * 10.0F;
        }
        return;
    }

    ufoTimer_ -= deltaSeconds;
    if (ufoTimer_ > 0.0F) {
        return;
    }

    const bool fromLeft = rng_.range(0, 1) == 0;
    ufo_.active = true;
    ufo_.direction = fromLeft ? 1.0F : -1.0F;
    ufo_.position = {fromLeft ? -12.0F : static_cast<float>(kScreenWidth) + 12.0F, kUfoY};
    ufo_.points = nextUfoScore();
    postEvent(EventType::UfoAppeared, ufo_.position, ufo_.points);
}

auto Game::nextUfoScore() noexcept -> int {
    constexpr std::array<int, 4> scores = {50, 100, 150, 300};
    return scores[static_cast<std::size_t>(rng_.range(0, static_cast<int>(scores.size()) - 1))];
}

void Game::playerHit() {
    if (!player_.alive) {
        return;
    }
    player_.alive = false;
    postEvent(EventType::PlayerHit, player_.position);
    loseLife();
}

void Game::loseLife() {
    lives_ = std::max(0, lives_ - 1);
    phase_ = Phase::Dying;
    dyingTimer_ = kDeathDuration;
    playerShot_.reset();
    enemyShots_.clear();
    fireHeld_ = false;
    ufo_.active = false;
}

void Game::finishDeath() {
    dyingTimer_ = 0.0F;
    if (lives_ > 0) {
        beginReady(1.25F);
        return;
    }
    endGame();
}

void Game::endGame() {
    if (phase_ == Phase::GameOver) {
        return;
    }
    phase_ = Phase::GameOver;
    paused_ = false;
    lives_ = 0;
    player_.alive = false;
    playerShot_.reset();
    enemyShots_.clear();
    ufo_.active = false;
    leftHeld_ = false;
    rightHeld_ = false;
    fireHeld_ = false;
    postEvent(EventType::GameOver, player_.position);
}

void Game::completeWave() {
    phase_ = Phase::WaveCleared;
    waveClearTimer_ = kWaveClearDuration;
    playerShot_.reset();
    enemyShots_.clear();
    ufo_.active = false;
    fireHeld_ = false;
    postEvent(EventType::WaveCleared, {kScreenWidth * 0.5F, kScreenHeight * 0.5F});
}

void Game::advanceWave() {
    ++wave_;
    player_.alive = true;
    resetField();
    beginReady(1.2F);
}

void Game::addScore(int points) {
    if (points <= 0) {
        return;
    }
    score_ += points;
    highScore_ = std::max(highScore_, score_);
}

void Game::postEvent(EventType type, Vec2 position, int points) {
    events_.push_back(Event{type, position, points});
}

auto Game::enemyShotSpeed() const noexcept -> float {
    return std::min(98.0F, 54.0F + 5.0F * static_cast<float>(wave_ - 1));
}

auto Game::maximumEnemyShots() const noexcept -> int {
    return std::min(5, 3 + (wave_ - 1) / 2);
}

} // namespace invaders
