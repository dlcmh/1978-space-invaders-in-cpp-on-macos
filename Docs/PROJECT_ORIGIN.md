# Project Origin and Decision Record

This document preserves the request that created SPACE INVADERS — Metal Arcade
and records the decisions that shaped the delivered v1.0. It is a historical and
governance document. The **Original request** block below is immutable.

- **Project:** SPACE INVADERS — Metal Arcade
- **Repository:** <https://github.com/dlcmh/1978-space-invaders-in-cpp-on-macos>
- **Commissioned by:** David Chin ([@dlcmh](https://github.com/dlcmh))
- **Commissioned:** 2026-10-20 18:50:39 (+08:00, Asia/Kuala_Lumpur)
- **Delivery agent:** DeepSeek Harness (deepseek-flash)
- **Initial public version:** 1.0

## Original request

The following is the human-authored request verbatim. Harness-supplied runtime
metadata and workspace instructions are summarized separately and are not part
of the quote.

```text
The approval policy changed from "ask" to "never" (changed by the user).

reckon you can one-shot this game in clean C++ with Metal? Game description: "The original Space Invaders is a landmark fixed-shooter arcade game developed by Japanese designer Tomohiro Nishikado and released by Taito in 1978.

Key Details

• Developer & Designer: Tomohiro Nishikado
• Release Year: 1978 (released in Japan by Taito and distributed overseas by Midway Games)
• Genre: Fixed shooter / Shoot 'em up
• Core Gameplay:
	• Players control a horizontal laser cannon at the bottom of the screen to shoot descending waves of 55 alien ships.
	• The aliens move side-to-side in a zig-zag pattern and step closer to the bottom with each pass.
	• Four destructible defense bunkers provide temporary cover from enemy fire.
	• As you destroy more aliens, the remaining ships speed up (originally caused by processor load decreasing as sprites were cleared).
	• A bonus "Mystery Ship" occasionally crosses the top of the screen for extra points."

Do use /Users/dlcmh/dev/pac-man-arcade-swift-macos as a reference for how i'd like this repo and game artifacts to be shaped, developed and governed; additionally for game artifacts, kindly use modern c++ swe best practices for architecture and coding style, document your c++ choices

set this repo's upstream to https://github.com/dlcmh/1978-space-invaders-in-cpp-on-macos.git
```

## Runtime context at commissioning

- Workspace: `/Users/dlcmh/dev/1978-space-invaders-in-cpp-on-macos`
- Host platform: macOS 26.6 on Apple silicon (Apple M5), Apple Clang 21
- Interaction surface: DeepSeek Harness Web GUI
- File policy: danger-full-access; approval prompts disabled
- Repository state at commissioning: empty local directory; no Git repository
- Reference repository:
  `/Users/dlcmh/dev/pac-man-arcade-swift-macos` (Swift/Metal Pac-Man)

These facts explain implementation choices and are not ongoing requirements.

## Requirements captured from the request

1. Deliver a playable, original-style **Space Invaders** arcade game in **clean
   C++** with **Metal** rendering.
2. Implement the canonical 55-alien formation, side-to-side march, downward
   steps, accelerating tempo, four destructible bunkers, and mystery ship.
3. Deliver a complete game flow rather than a mock-up or partial prototype:
   player control, shooting, enemy fire, lives, score, game over, and restart.
4. Use modern C++ software-engineering practices for architecture and style, and
   document those choices.
5. Shape the repository, governance, documentation, verification, and release
   artifacts like the supplied Pac-Man reference project.
6. Set the repository's canonical upstream to
   `https://github.com/dlcmh/1978-space-invaders-in-cpp-on-macos.git`.
7. Keep the work native to the supplied macOS workspace. The brief's historical
   details were context, not a requirement to emulate arcade hardware or use
   proprietary assets.

## Delivered implementation

- Native macOS application built with CMake 3.28+, Ninja, Apple Clang, AppKit,
  MetalKit, Metal, QuartzCore, and Core Audio.
- Headless C++23 simulation with a deterministic fixed 60 Hz step:
  - 11 × 5 alien formation, three species, 30/20/10-point scoring;
  - accelerating march and edge descent;
  - one in-flight player shot, capped enemy fire from the lowest alien in a
    column;
  - four 22 × 16 pixel bunkers with projectile carving, including erosion by the
    descending formation;
  - mystery ship with canonical 50/100/150/300-point values;
  - lives, death sequence, wave progression, game over, restart, pause, and
    persistent local high score.
- Procedural immediate-mode Metal renderer with one draw call, 4× MSAA,
  triple-buffered vertex uploads, custom 5 × 7 pixel font, CRT scanlines, glows,
  particles, score popups, and responsive letterboxing.
- Procedural Core Audio sound synthesis with a lock-free event queue; no audio
  files or third-party audio engine.
- Dependency-free simulation regression suite driven only through the public
  API. The current baseline is **24,343 checks**.
- App bundle and DMG packaging, a procedural application icon, documentation
  link/symbol validation, and governance records.

## Explicit scope decisions

| Topic | Decision |
| --- | --- |
| Platform | Native macOS on Apple silicon and Intel Macs with Metal; no browser or mobile build in v1.0. |
| Assets | No original arcade sprites, ROM data, fonts, recordings, or audio. Everything is procedural. |
| Language split | C++23 for simulation and scene composition; Objective-C++ only at the AppKit/Metal/Core Audio boundary. |
| Build system | CMake + Ninja, strict warnings, no third-party runtime dependencies. |
| Simulation rate | Fixed 60 Hz with a bounded catch-up loop; rendering can refresh independently. |
| Mystery ship | Included with the canonical score table; deterministic RNG seed. |
| Extra lives | Not awarded, matching the original arcade baseline. |
| Network | Fully offline. No leaderboards, telemetry, accounts, or downloads. |
| Persistence | Local `NSUserDefaults` only (high score and mute preference). |
| Licensing | No open-source license selected. All rights remain with the owner/contributors; see [GOVERNANCE.md](../GOVERNANCE.md). |
| Trademark | Unofficial fan tribute; not affiliated with or endorsed by TAITO. |

## Verification record

The v1.0 baseline was built and launched on the commissioning host. The
simulation suite passed **24,343 checks**, covering:

- 55 aliens across 11 columns and 5 rows with canonical point values;
- movement clamping, pause behavior, one-shot firing, and alien hits;
- bunker damage through the same bitmap used for collision and rendering;
- horizontal march, edge descent, and wave progression after all 55 kills;
- mystery-ship spawning, traversal, and a scored hit;
- player hits, lives, game over, restart, deterministic replay, and a 60-second
  movement soak; and
- high-score tracking.

Visual verification used window captures of the running app, recorded in
`Docs/`: title screen (procedural logo, point table, controls) and
gameplay (formation, carved bunkers, enemy fire, player cannon, HUD).

Audio synthesis starts through the Core Audio output unit; the engine also
fails closed to silent play if no output device is available. The repository's
automated checks do not assert audible output because that requires a physical
audio route.

## Governance directive

The request established the canonical remote and asked that the project follow
the maintenance shape of the reference Pac-Man repository. This led to
[AGENTS.md](../AGENTS.md), [CONTRIBUTING.md](../CONTRIBUTING.md),
[GOVERNANCE.md](../GOVERNANCE.md), [SECURITY.md](../SECURITY.md), the
[C++ design record](CPP_DESIGN.md), and this origin record. The canonical remote
is `origin`; `main` tracks `origin/main`.

## Record maintenance

- Do not edit the **Original request** quotation.
- Append material decisions rather than silently rewriting history.
- Update the verification section when the release baseline changes.
- Changes to governance itself follow [GOVERNANCE.md](../GOVERNANCE.md).
