// Dependency-free regression suite for the headless simulation. It drives only
// the public Game API: no test hooks, no access to private state, and no window
// server or Metal device required.

#include "invaders/Bunker.hpp"
#include "invaders/Game.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string_view>
#include <vector>

namespace {

int g_checks = 0;
int g_failures = 0;

constexpr float kStep = 1.0F / 60.0F;

void check(bool condition, std::string_view message) {
    ++g_checks;
    if (!condition) {
        ++g_failures;
        std::cerr << "FAIL: " << message << '\n';
    }
    if (g_failures > 20) {
        std::cerr << "Too many failures; stopping.\n";
        std::exit(1);
    }
}

void step(invaders::Game& game, float seconds) {
    const int steps = static_cast<int>(std::ceil(seconds / kStep));
    for (int index = 0; index < steps; ++index) {
        game.update(kStep);
    }
}

void startPlaying(invaders::Game& game) {
    game.requestStart();
    check(game.phase() == invaders::Phase::Ready, "new game enters READY");
    step(game, 1.6F);
    check(game.phase() == invaders::Phase::Playing, "READY reaches PLAYING");
}

void movePlayerTo(invaders::Game& game, float targetX, float tolerance = 1.0F) {
    for (int guard = 0; guard < 600; ++guard) {
        const float delta = targetX - game.player().position.x;
        if (std::abs(delta) <= tolerance) {
            break;
        }
        game.setMoveLeft(delta < 0.0F);
        game.setMoveRight(delta > 0.0F);
        game.update(kStep);
    }
    game.setMoveLeft(false);
    game.setMoveRight(false);
}

void checkInvariants(const invaders::Game& game) {
    const auto player = game.player().position;
    check(std::isfinite(player.x) && std::isfinite(player.y), "player position is finite");
    check(player.x >= 8.0F && player.x <= 216.0F, "player stays in bounds");
    check(player.y == invaders::kPlayerY, "player stays on its row");
    check(game.score() >= 0, "score stays non-negative");
    check(game.highScore() >= game.score(), "high score stays above score");
    check(game.lives() >= 0 && game.lives() <= 3, "lives stay within range");
    check(game.aliensRemaining() >= 0 && game.aliensRemaining() <= invaders::kAlienCount,
          "alien count stays within range");
    for (const invaders::Projectile& shot : game.enemyShots()) {
        check(std::isfinite(shot.position.x) && std::isfinite(shot.position.y),
              "enemy shot position is finite");
        check(shot.position.y >= 0.0F && shot.position.y <= 262.0F, "enemy shot y stays in range");
    }
    if (game.playerShot().has_value()) {
        const auto& shot = *game.playerShot();
        check(std::isfinite(shot.position.x) && std::isfinite(shot.position.y),
              "player shot position is finite");
        check(shot.position.y >= 0.0F && shot.position.y <= 256.0F, "player shot y stays in range");
    }
}

void testInitialState() {
    invaders::Game game(invaders::GameOptions{.seed = 1978, .startingLives = 3, .highScore = 4321});
    check(game.phase() == invaders::Phase::Attract, "game starts in attract mode");
    check(game.aliens().size() == 55, "55 aliens exist");
    check(game.aliensRemaining() == 55, "all 55 aliens begin alive");
    check(game.lives() == 3, "three lives");
    check(game.score() == 0, "score starts at zero");
    check(game.highScore() == 4321, "supplied high score is preserved");
    check(game.wave() == 1, "starts on wave one");
    check(game.bunkers().size() == 4, "four bunkers");
    check(!game.playerShot().has_value(), "no player shot in attract");
    check(game.enemyShots().empty(), "no enemy shots in attract");
    check(!game.ufo().active, "no mystery ship in attract");

    int counts[5] = {};
    for (const auto& alien : game.aliens()) {
        check(alien.alive, "initial alien is alive");
        check(alien.column >= 0 && alien.column < 11, "alien column in range");
        check(alien.row >= 0 && alien.row < 5, "alien row in range");
        ++counts[alien.row];
    }
    for (int row = 0; row < 5; ++row) {
        check(counts[row] == 11, "each row has eleven aliens");
    }
    check(game.alienPoints(invaders::AlienKind::Squid) == 30, "squid scores 30");
    check(game.alienPoints(invaders::AlienKind::Crab) == 20, "crab scores 20");
    check(game.alienPoints(invaders::AlienKind::Octopus) == 10, "octopus scores 10");
    check(game.marchInterval() > 0.0F, "march interval is positive");
}

void testStartAndMovement() {
    invaders::Game game(invaders::GameOptions{.seed = 7});
    game.requestStart();
    check(game.phase() == invaders::Phase::Ready, "start enters ready");
    check(game.score() == 0 && game.wave() == 1, "start resets score and wave");
    check(game.readyRemaining() > 0.0F, "ready timer is armed");
    step(game, 1.6F);
    check(game.phase() == invaders::Phase::Playing, "playing after ready");

    const float startX = game.player().position.x;
    game.setMoveLeft(true);
    step(game, 0.25F);
    game.setMoveLeft(false);
    check(game.player().position.x < startX, "player moves left");

    game.setMoveRight(true);
    step(game, 3.5F);
    game.setMoveRight(false);
    check(game.player().position.x <= 216.0F, "player clamps at right edge");

    game.setMoveLeft(true);
    step(game, 8.0F);
    game.setMoveLeft(false);
    check(game.player().position.x >= 8.0F, "player clamps at left edge");

    game.togglePause();
    check(game.isPaused(), "pause sets the pause flag");
    const auto frozen = game.player().position;
    const auto frozenAlien = game.aliens().front();
    step(game, 1.0F);
    check(game.player().position == frozen, "pause freezes player");
    check(game.aliens().front().column == frozenAlien.column, "pause freezes the formation");
    game.togglePause();
    check(!game.isPaused(), "pause toggles off");
}

void testPlayerFireAndAlienHit() {
    invaders::Game game(invaders::GameOptions{.seed = 99});
    startPlaying(game);
    const int initialScore = game.score();
    const int initialAliens = game.aliensRemaining();
    game.setFiring(true);
    step(game, kStep);
    check(game.playerShot().has_value(), "firing creates one player shot");
    game.setFiring(false);
    step(game, 1.2F);
    check(!game.playerShot().has_value(), "player shot is eventually consumed");
    check(game.aliensRemaining() < initialAliens, "player shot destroys an alien");
    check(game.score() > initialScore, "destroying an alien scores points");
    check(game.highScore() >= game.score(), "high score tracks the score");

    game.setFiring(true);
    step(game, kStep);
    check(game.playerShot().has_value(), "the player can fire again");
    game.setFiring(false);
    step(game, 0.1F);
    const auto onlyShot = game.playerShot();
    check(onlyShot.has_value(), "in-flight shot still exists");
    game.setFiring(true);
    step(game, 0.1F);
    check(game.playerShot().has_value(), "only one player shot may fly at once");
    game.setFiring(false);
}

void testBunkerDamage() {
    invaders::Game game(invaders::GameOptions{.seed = 123});
    startPlaying(game);
    movePlayerTo(game, 34.0F);
    const int before = game.bunkers()[0].remainingCells();
    game.setFiring(true);
    step(game, kStep);
    game.setFiring(false);
    step(game, 0.5F);
    check(game.bunkers()[0].remainingCells() < before, "player shot erodes a bunker");
    check(game.bunkers()[1].remainingCells() > 0, "other bunkers remain untouched");

    auto bunker = invaders::Bunker(0.0F, 0.0F);
    const int full = bunker.remainingCells();
    check(full > 0, "a fresh bunker has solid cells");
    check(bunker.hitTest({5.0F, 5.0F, 2.0F, 2.0F}), "bunker reports a hit on solid cells");
    check(!bunker.hitTest({5.0F, 34.0F, 2.0F, 2.0F}), "bunker reports no hit below itself");
    check(bunker.carve({22.0F, 20.0F}, 5.0F), "carving removes solid cells");
    check(bunker.remainingCells() < full, "carving reduces the cell count");
    check(!bunker.carve({22.0F, 20.0F}, 5.0F), "a second identical carve changes nothing");
}

void testFormationAndUfo() {
    invaders::Game game(invaders::GameOptions{.seed = 42});
    startPlaying(game);
    const auto origin = game.formationOrigin();
    step(game, 1.0F);
    check(game.formationOrigin().x != origin.x, "formation marches horizontally");

    float initialY = game.formationOrigin().y;
    int guard = 0;
    while (game.formationOrigin().y == initialY && guard < 60 * 45 &&
           game.phase() == invaders::Phase::Playing) {
        game.setMoveRight(guard % 120 < 60);
        game.setMoveLeft(!(guard % 120 < 60));
        game.update(kStep);
        ++guard;
    }
    game.setMoveLeft(false);
    game.setMoveRight(false);
    check(game.formationOrigin().y > initialY, "formation descends at the screen edge");
    check(game.phase() == invaders::Phase::Playing || game.phase() == invaders::Phase::Dying ||
              game.phase() == invaders::Phase::Ready,
          "edge descent does not end the game immediately");

    invaders::Game ufoGame(invaders::GameOptions{.seed = 2024});
    startPlaying(ufoGame);
    bool appeared = false;
    int ufoGuard = 0;
    float firstX = 0.0F;
    while (!appeared && ufoGuard < 60 * 35) {
        ufoGame.update(kStep);
        if (ufoGame.ufo().active) {
            appeared = true;
            firstX = ufoGame.ufo().position.x;
            check(ufoGame.ufo().points == 50 || ufoGame.ufo().points == 100 ||
                      ufoGame.ufo().points == 150 || ufoGame.ufo().points == 300,
                  "mystery ship has a canonical score value");
        }
        ++ufoGuard;
    }
    check(appeared, "mystery ship appears within 35 simulated seconds");
    if (appeared) {
        step(ufoGame, 0.4F);
        check(ufoGame.ufo().position.x != firstX || !ufoGame.ufo().active,
              "mystery ship traverses the screen");
    }
}

void testDeathAndGameOver() {
    invaders::Game game(invaders::GameOptions{.seed = 5, .startingLives = 3});
    startPlaying(game);
    bool sawPlayerHit = false;
    int guard = 0;
    while (game.phase() != invaders::Phase::GameOver && guard < 60 * 240) {
        game.update(kStep);
        for (const auto& event : game.drainEvents()) {
            if (event.type == invaders::EventType::PlayerHit) {
                sawPlayerHit = true;
            }
        }
        ++guard;
    }
    check(sawPlayerHit, "enemy fire can hit the player");
    check(game.phase() == invaders::Phase::GameOver, "losing all lives reaches game over");
    check(game.lives() == 0, "game over leaves zero lives");

    game.requestStart();
    check(game.phase() == invaders::Phase::Ready, "game over can restart");
    check(game.lives() == 3, "restart restores three lives");
    check(game.score() == 0, "restart clears the score");
    check(game.aliensRemaining() == 55, "restart restores the formation");
}

void testMysteryShipScoring() {
    invaders::Game game(invaders::GameOptions{.seed = 0x5157, .startingLives = 20, .highScore = 0});
    startPlaying(game);
    bool sawUfo = false;
    bool shotUfo = false;
    int ufoPoints = 0;
    int guard = 0;
    while (!shotUfo && game.phase() != invaders::Phase::GameOver && guard < 60 * 240) {
        if (game.ufo().active) {
            sawUfo = true;
            // The shot takes roughly 0.85 s to reach the top of the playfield,
            // so lead the target by its travel during the bullet's flight.
            const float predictedX =
                std::clamp(game.ufo().position.x + game.ufo().direction * 32.0F, 9.0F, 215.0F);
            const float delta = predictedX - game.player().position.x;
            game.setMoveLeft(delta < -1.5F);
            game.setMoveRight(delta > 1.5F);
            game.setFiring(true);
        }
        game.update(kStep);
        for (const auto& event : game.drainEvents()) {
            if (event.type == invaders::EventType::UfoHit) {
                shotUfo = true;
                ufoPoints = event.points;
            }
        }
        ++guard;
    }
    game.setMoveLeft(false);
    game.setMoveRight(false);
    game.setFiring(false);
    check(sawUfo, "mystery ship appears for the scoring test");
    check(shotUfo, "player shot can destroy the mystery ship");
    check(ufoPoints == 50 || ufoPoints == 100 || ufoPoints == 150 || ufoPoints == 300,
          "mystery ship awards a canonical score");
    check(game.score() >= ufoPoints, "mystery ship score is added to the total");
}

void testWaveProgression() {
    // Autopilot: stand under the lowest surviving alien and hold fire. A deep
    // reserve of lives makes this a rules test rather than a dodge-skill test.
    invaders::Game game(
        invaders::GameOptions{.seed = 0x1A2B3C, .startingLives = 30, .highScore = 0});
    startPlaying(game);
    int guard = 0;
    while (game.phase() != invaders::Phase::WaveCleared &&
           game.phase() != invaders::Phase::GameOver && guard < 60 * 300) {
        const invaders::Alien* target = nullptr;
        for (const invaders::Alien& alien : game.aliens()) {
            if (!alien.alive) {
                continue;
            }
            if (target == nullptr || alien.row > target->row ||
                (alien.row == target->row &&
                 std::abs(game.alienRect(alien).center().x - game.player().position.x) <
                     std::abs(game.alienRect(*target).center().x - game.player().position.x))) {
                target = &alien;
            }
        }
        if (target != nullptr && game.phase() == invaders::Phase::Playing) {
            const float targetX = game.alienRect(*target).center().x;
            const float delta = targetX - game.player().position.x;
            game.setMoveLeft(delta < -1.0F);
            game.setMoveRight(delta > 1.0F);
            game.setFiring(true);
        }
        game.update(kStep);
        ++guard;
    }
    game.setMoveLeft(false);
    game.setMoveRight(false);
    game.setFiring(false);
    check(game.phase() == invaders::Phase::WaveCleared, "all 55 kills clear the wave");
    check(game.aliensRemaining() == 0, "wave clear leaves no aliens alive");
    check(game.score() >= 55 * 10, "wave clear awards alien points");
    check(game.bunkers().size() == 4, "wave clear keeps four bunker slots");

    step(game, 2.2F);
    check(game.wave() == 2, "next wave increments the wave number");
    check(game.aliensRemaining() == 55, "next wave restores the formation");
    check(game.score() >= 550, "wave transition keeps score");
    check(game.bunkers()[0].remainingCells() > 0, "next wave restores the bunkers");
    step(game, 1.4F);
    check(game.phase() == invaders::Phase::Playing, "next wave reaches playing");
}

void testDeterminismAndSoak() {
    invaders::Game first(invaders::GameOptions{.seed = 0xABC});
    invaders::Game second(invaders::GameOptions{.seed = 0xABC});
    startPlaying(first);
    startPlaying(second);
    for (int frame = 0; frame < 60 * 30; ++frame) {
        const bool left = (frame / 73) % 2 == 0;
        const bool right = !left;
        first.setMoveLeft(left);
        first.setMoveRight(right);
        second.setMoveLeft(left);
        second.setMoveRight(right);
        if (frame % 45 == 0) {
            first.setFiring(true);
            second.setFiring(true);
        }
        if (frame % 45 == 20) {
            first.setFiring(false);
            second.setFiring(false);
        }
        first.update(kStep);
        second.update(kStep);
        check(first.player().position == second.player().position,
              "deterministic runs keep identical player positions");
        check(first.score() == second.score(), "deterministic runs keep identical scores");
        check(first.aliensRemaining() == second.aliensRemaining(),
              "deterministic runs keep identical alien counts");
        checkInvariants(first);
    }

    const auto position = first.player().position;
    first.setMoveLeft(true);
    first.setMoveRight(false);
    first.setFiring(true);
    for (int frame = 0; frame < 60; ++frame) {
        first.update(kStep);
        check(first.player().position.x >= 8.0F && first.player().position.x <= 216.0F,
              "soak player remains inside the playfield");
        check(std::isfinite(first.player().position.x), "soak player x remains finite");
        check(std::isfinite(first.formationOrigin().x), "formation origin remains finite");
        check(std::isfinite(first.formationOrigin().y), "formation y remains finite");
    }
    check(position.x != first.player().position.x || first.phase() != invaders::Phase::Playing,
          "soak updates gameplay");
}

} // namespace

int main() {
    testInitialState();
    testStartAndMovement();
    testPlayerFireAndAlienHit();
    testBunkerDamage();
    testFormationAndUfo();
    testDeathAndGameOver();
    testWaveProgression();
    testMysteryShipScoring();
    testDeterminismAndSoak();

    if (g_failures != 0) {
        std::cerr << "FAIL: " << g_failures << " of " << g_checks << " checks failed\n";
        return EXIT_FAILURE;
    }
    std::cout << "PASS: " << g_checks << " checks\n";
    return EXIT_SUCCESS;
}
