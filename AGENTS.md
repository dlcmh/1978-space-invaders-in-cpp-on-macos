# AGENTS.md

Operating instructions for AI coding agents working in this repository. These
rules are mandatory unless the maintainer gives a direct instruction that
supersedes them. Update this file when project policy changes.

- **Project:** SPACE INVADERS — Metal Arcade
- **Canonical repository:** <https://github.com/dlcmh/1978-space-invaders-in-cpp-on-macos>
- **Maintainer:** [@dlcmh](https://github.com/dlcmh)
- **Governance baseline:** 2026-10-20
- **Current release:** 1.0

## Read first

1. [Docs/PROJECT_ORIGIN.md](Docs/PROJECT_ORIGIN.md) — why this project exists,
   including the original request. Its quoted request is a historical record
   and must not be edited.
2. [GOVERNANCE.md](GOVERNANCE.md) — decision-making, releases, licensing, and
   amendment rules.
3. [CONTRIBUTING.md](CONTRIBUTING.md) — build, test, and contribution workflow.
4. [SECURITY.md](SECURITY.md) — vulnerability reporting and privacy posture.
5. [Docs/CPP_DESIGN.md](Docs/CPP_DESIGN.md) — the C++ architecture and style
   decisions that are considered settled architecture.

## Project layout

| Path | Responsibility |
| --- | --- |
| `Sources/Core/` | Portable C++23 simulation. No AppKit, Metal, AudioToolbox, or windowing imports. The public API starts at [Game.hpp](Sources/Core/include/invaders/Game.hpp) — `Game::update`. |
| `Sources/Render/` | Portable C++ scene composition plus the Objective-C++ Metal boundary. [MetalRenderer.mm](Sources/Render/MetalRenderer.mm) — `SIRenderer` owns GPU state; [Scene.cpp](Sources/Render/Scene.cpp) — `Scene::build` composes every frame. |
| `Sources/Platform/` | AppKit `MTKView`, input, fixed-step driver, and procedural Core Audio. [GameView.mm](Sources/Platform/GameView.mm) — `drawInMTKView` runs the loop. |
| `Sources/App/` | `NSApplication` entry point and menus. [AppDelegate.mm](Sources/App/AppDelegate.mm) — `createWindow` builds the window. |
| `Checks/` | Dependency-free regression executable. |
| `Resources/` | `Info.plist` and the generated `.icns`/`.png` app icon. |
| `Docs/` | Screenshots, origin record, architecture, Q&A, and refactoring records. |
| `scripts/` | App-bundle packaging, icon generation, source formatting, docs checking, and the `make check` gate. |
| `build/`, `dist/` | Generated and git-ignored. Never commit either directory. |

## Prerequisites

- macOS 13 or newer on a Metal-capable Mac.
- Xcode or Apple Command Line Tools with a C++23-capable Clang.
- CMake 3.28+ and Ninja.
- `clang-format` (the copy shipped with the Command Line Tools is sufficient).
- Python 3 with Pillow for regenerating the icon.
- No third-party runtime dependencies, package managers, or network services.

The project is built with CMake directly. There is no CocoaPods, SwiftPM,
Homebrew runtime dependency, or downloaded game asset. Do not add one without
explicit maintainer approval.

## Build, run, and verify

Run these from the repository root:

```bash
make build                  # debug CMake build in build/debug
make run                    # build and launch the app binary
make checks                 # build and run the simulation checks
make app                    # release .app bundle in dist/
make dmg                    # app bundle and dist/SPACE-INVADERS-1.0.dmg
make check                  # full gate: format, release build, checks, docs
make docs-check             # links, anchors, and cited symbols
make docs-impact            # docs that reference files changed against HEAD
make format                 # apply .clang-format
```

`cmake --build` is necessary but not sufficient for rendering changes. The MSL
shader is compiled at runtime, so a successful compile can still launch into an
error alert if the shader is invalid. After touching shaders, renderer code,
windowing, input, or audio, launch the app and confirm it actually renders and
responds.

## Non-negotiable architecture

1. **Keep simulation and presentation separate.** `Sources/Core/` must remain
   free of AppKit, Metal, MetalKit, AudioToolbox, CoreAudio, and window-server
   dependencies. It must build and run headlessly.
2. **Keep Metal.** Use the existing immediate-mode triangle renderer and
   `MTKView` path. Do not replace it with SpriteKit, SceneKit, Core Animation,
   a game engine, or a browser runtime.
3. **Keep assets procedural.** Aliens, cannon, bunkers, star field, pixel font,
   logo, app icon, and audio are generated in code or by a checked-in generator
   script. Do not add ripped arcade sprites, ROM data, scans, or copyrighted
   audio.
4. **Keep the game offline.** No network calls, telemetry, analytics, remote
   configuration, accounts, or crash uploaders. The only persisted state is
   local `NSUserDefaults`.
5. **No new dependencies** without explicit maintainer approval.
6. **Authoritative simulation is `invaders::Game`.** Do not duplicate gameplay
   rules in the renderer or app layer. Presentation state must be read-only with
   respect to the simulation.
7. **Keep the game loop deterministic and fixed-step.** The app runs simulation
   updates at exactly 60 Hz through a bounded catch-up loop. Do not make
   movement, collision, marching, or randomness depend on display refresh rate
   or wall-clock time.
8. **Preserve public read-only state.** Simulation state is exposed through
   getters and `std::span` views. All input enters through explicit command
   methods such as `setMoveLeft`, `setMoveRight`, `setFiring`, `requestStart`,
   and `togglePause`. Prefer this over mutable public fields.
9. **Confine Objective-C++ to the Apple boundary.** [MetalRenderer.mm](Sources/Render/MetalRenderer.mm) — `SIRenderer`, `GameView.mm`, `AppDelegate.mm`, and `main.mm` may be Objective-C++. Do not leak Objective-C types or Cocoa ownership into `Sources/Core/` or `Sources/Render/Scene.cpp`.
10. **Warnings are errors.** Fix warnings; do not suppress them wholesale or
    lower the standard.

## Gameplay invariants

Changes that affect movement, collisions, scoring, or wave flow must preserve
or deliberately update these known invariants and their checks:

- The formation contains 11 × 5 = 55 aliens. Their row values are 30 (squid),
  20 (crab), and 10 (octopus) points.
- The player fires at most one shot at a time; enemy fire is capped and comes
  from the lowest surviving alien in a chosen column.
- The formation moves horizontally in fixed steps, descends and reverses at the
  boundaries, and accelerates as `aliensRemaining` decreases.
- Four bunkers begin each wave. Their collision surface and visible pixels are
  the same 22 × 16 grid, and shots carve deterministic holes.
- The mystery ship crosses the top of the screen for 50, 100, 150, or 300
  points, selected deterministically by the seeded RNG.
- Losing a life freezes play for the death sequence, then returns to READY with
  the formation intact. The third death reaches GAME OVER.
- Clearing every alien moves through WAVE CLEARED, increments the wave,
  restores the formation and bunkers, and preserves score and lives.
- High score never falls below the current score and is persisted locally.
- The README's controls, scoring, and deliverables remain accurate.

Do not add the historical arcade ROM, hardware quirks that require undocumented
memory corruption, or a kill screen as a drive-by change. Scope substantial
gameplay additions with the maintainer first.

## Static checks

`make check` runs [`scripts/check.sh`](scripts/check.sh) — `CLANG_FORMAT`, the
gate every change must pass:

1. `clang-format --dry-run --Werror` over `Sources/` and `Checks/`;
2. a Release build with `-Werror` and the project warning set;
3. `spaceinvaders_checks` — behavior; and
4. `python3 scripts/check_docs.py` — documentation integrity.

The format style lives in [`.clang-format`](.clang-format). It is close to LLVM
with four-space indentation, 100 columns, attached braces, and left-aligned
pointers/references. Apply it with `make format`.

Do not add SwiftLint, clang-tidy plugins, or package-based linters to obtain
stronger analysis without approval. The compiler's warning set and the checks
executable are the dependency-free quality bar.

## Testing standard

`Checks/main.cpp` is the regression suite. It intentionally uses only the
public game API. Add checks whenever gameplay, collision, scoring, phase
transitions, or deterministic behavior changes.

The current green run ends with:

```text
PASS: 24343 checks
```

Counts may grow. The important requirement is that all checks pass. The suite
covers formation integrity, movement clamps, paused simulation, one-shot
firing, alien and bunker hits, edge descent, wave progression, mystery-ship
spawning and scoring, lives/game-over, deterministic replay, and a soak run.
For pure visual or packaging changes, also rebuild and open the resulting
`.app`.

## Documentation and code references

A line number in prose is wrong the moment ten lines are inserted above it, and
nothing fails. Documentation therefore cites a **file and a symbol**, never a
line range:

```markdown
[`Game.cpp`](Sources/Core/Game.cpp) — `Game::updatePlaying`
```

- Link the file, then name the symbol in backticks on the same line or in the
  same sentence.
- Prefer the most specific symbol — a function, method, type, or constant — over
  a general word such as a state name.
- Link Markdown by heading anchor (`../AGENTS.md#non-negotiable-architecture`),
  never by line.
- When a symbol is renamed or removed, update every document that cites it.

`make docs-check` enforces this: every relative link must resolve, every heading
anchor must exist, every cited symbol must still occur in the linked file, and
`#L` line anchors are rejected.

`make docs-impact` lists the docs that reference files changed against `HEAD`
(or `--base <rev>`) and warns when the diff removes a cited symbol. Treat that
list as the re-read queue after changing code.

`Docs/PROJECT_ORIGIN.md` is a historical record: its quotation is immutable, so
correct a stale reference there by annotating outside the quote rather than
rewriting it. A single line can opt out of the symbol rule with
`<!-- docs-check: ignore -->`.

## Git and release workflow

- `main` is the canonical trunk.
- Use Conventional Commit messages, for example:
  - `feat: add bunker pixel destruction`
  - `fix: prevent shots from skipping edge cells`
  - `docs: record C++ architecture`
  - `chore: refresh the application icon`
- Keep commits coherent and reviewable. Do not mix unrelated refactors with
  gameplay changes.
- Run `make check` before committing.
- Push to `origin/main`. The maintainer has authorized normal agent commits and
  pushes in this repository.
- Never force-push `main`, rewrite published history, or change remotes without
  explicit maintainer approval.
- Do not commit `build/`, `dist/`, `.DS_Store`, or other generated artifacts.
  Distribution binaries belong in a release artifact, not source history.
- Do not tag or publish a GitHub Release unless the maintainer asks.

## Rendering and audio guidance

- Keep the presentation in the **224 × 256 logical portrait space** and
  letterbox it cleanly when the window aspect changes.
- Maintain 4× MSAA where supported and triple-buffered vertex uploads.
- Preserve the established palette, phosphor HUD, pixel font, star field, and
  scanline look.
- Screenshots in `Docs/` are user-facing documentation. Refresh them when a
  material visual change invalidates them.
- Sound effects are synthesized at launch through Core Audio. Keep the mute
  setting persisted and do not introduce audio files or streamed audio.
- Presentation effects (particles, score popups, flashes) may live in
  `Scene`; they must never alter simulation state.

## Persistence and privacy

The app stores only two local preferences:

| Key | Purpose |
| --- | --- |
| `SpaceInvadersCPPHighScore` | Persistent local high score |
| `SpaceInvadersCPPMuted` | Sound preference |

Do not add analytics, identifiers, crash uploaders, remote persistence, or
network access. If a new stored preference is unavoidable, document it here and
in [SECURITY.md](SECURITY.md).

## Legal and licensing

- SPACE INVADERS is a trademark of TAITO Corporation. This is an unofficial fan
  tribute and must never be presented as affiliated with or endorsed by TAITO.
- No open-source license has been selected for this repository. Do not add a
  license, relicense code, or claim reuse rights without explicit maintainer
  approval.
- Only contribute original code and generated art/audio, or material whose
  license is compatible and documented.

## Agent operating protocol

1. Read `AGENTS.md`, the origin record, and the relevant source before editing.
2. Inspect `git status` and preserve unrelated local work.
3. Make the smallest coherent change that satisfies the request.
4. Add or update checks for behavioral changes.
5. Build, run `make check`, and launch the app for presentation changes.
6. Update README/governance docs when behavior, controls, build steps, or
   policy changes, then run `make docs-check` and resolve the docs listed by
   `make docs-impact`.
7. Commit with a Conventional Commit message and push to `origin/main` when
   authorized.
8. Report what changed, what was verified, and any remaining risks or follow-up
   work.

Do not claim to have run something you did not run. If the environment prevents
a verification step, say exactly what was blocked.

## Definition of done

- [ ] `make check` passes: format, Release build with warnings as errors,
      checks, and docs.
- [ ] `make checks` passes.
- [ ] `make docs-check` passes, and docs listed by `make docs-impact` were
      re-read after code changes.
- [ ] The app launches and renders correctly when presentation changed.
- [ ] Gameplay/controls documentation is current.
- [ ] Governance and origin records remain accurate.
- [ ] No generated binaries, build products, DMGs, or secrets are staged.
- [ ] Change is committed and pushed, if authorized.

## When to stop and ask the maintainer

Ask before:

- changing licensing, repository visibility, or the canonical remote;
- adding third-party dependencies, assets, telemetry, or network access;
- making incompatible gameplay changes or altering the established scoring;
- cutting a release, tag, or public distribution build;
- force-pushing or rewriting published history; or
- implementing large optional scope such as original-ROM emulation or a
  two-player mode.
