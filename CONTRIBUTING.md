# Contributing to SPACE INVADERS — Metal Arcade

Thanks for your interest. This is a compact, dependency-free native macOS game,
and focused contributions are welcome.

Before changing anything, read [AGENTS.md](AGENTS.md),
[Docs/PROJECT_ORIGIN.md](Docs/PROJECT_ORIGIN.md),
[Docs/CPP_DESIGN.md](Docs/CPP_DESIGN.md), and [GOVERNANCE.md](GOVERNANCE.md).

## Prerequisites

- macOS 13 or newer on a Metal-capable Mac
- Xcode or Apple Command Line Tools with a C++23-capable Clang
- CMake 3.28 or newer and Ninja
- `clang-format` (the Command Line Tools copy is fine)
- Python 3 with Pillow if you regenerate the application icon
- Git

No package-manager dependencies are required by the game. Pillow is a
development-only dependency of the icon generator and is not shipped.

## Build and run

```bash
make build
make run
```

Build the distributable application and disk image:

```bash
make app
make dmg
open dist/SPACE-INVADERS.app
```

Generated artifacts in `build/` and `dist/` are deliberately not versioned.

## Verify changes

```bash
make check
```

That runs the full gate: `clang-format --dry-run --Werror`, a Release build
with warnings as errors (which also compiles each portable header on its own),
the simulation checks, and the documentation checks. Run it before every
commit. The individual stages are also available (`make checks`,
`make docs-check`, `make format`).

The simulation checks are a dependency-free executable, not a framework test
suite. Add checks whenever you touch movement, collision, scoring, phase
transitions, or deterministic behavior. Drive only the public API on
[Game.hpp](Sources/Core/include/invaders/Game.hpp) — `Game`; do not add
test-only mutation hooks to production code.

For rendering, input, shader, or windowing changes, launching the app is
mandatory: the Metal shader is compiled at runtime, so a build alone cannot
validate it. Capture a new screenshot if a visible change invalidates the
existing documentation images.

The checks cover formation integrity, movement, one-shot firing, bunker
destruction, marching, wave progression, mystery ships, lives/game over,
determinism, and a soak run. The current baseline is:

```text
PASS: 24343 checks
```

## Where to make changes

- **Simulation:** [Game.cpp](Sources/Core/Game.cpp) — `Game::updatePlaying`;
  keep it independent of AppKit, Metal, MetalKit, and audio.
- **Bunkers:** [Bunker.cpp](Sources/Core/Bunker.cpp) — `Bunker::carve`.
- **Scene/presentation:** [Scene.cpp](Sources/Render/Scene.cpp) — `Scene::build`.
- **Metal boundary:** [MetalRenderer.mm](Sources/Render/MetalRenderer.mm) — `SIRenderer`.
- **Window/input/loop:** [GameView.mm](Sources/Platform/GameView.mm) — `drawInMTKView`.
- **Checks:** `Checks/main.cpp`.
- **Packaging:** `scripts/` and `Makefile`.
- **User docs:** `README.md`.
- **Governance docs:** `AGENTS.md`, `GOVERNANCE.md`, this file, `SECURITY.md`,
  and `Docs/PROJECT_ORIGIN.md`.

## Quality bar

A contribution is ready when:

- `make check` succeeds without warnings;
- `make checks` passes;
- `make docs-check` passes, and docs affected by the change were re-read;
- rendering changes were visually checked by running the app;
- gameplay invariants documented in `AGENTS.md` still hold, or the change
  explicitly updates them along with checks and docs;
- no new runtime dependency, telemetry, network access, or non-original asset
  was introduced; and
- user-facing documentation and screenshots are current.

## Style

- Follow the existing C++ style and [`.clang-format`](.clang-format): four-space
  indentation, 100-column lines, attached braces, and east const
  (`const Game&`).
- Keep functions and types focused. Prefer composition, value types, and
  read-only state over inheritance.
- Use `[[nodiscard]]` for pure getters, `noexcept` where it is genuinely true,
  and `std::span` for read-only views into owned storage.
- Use `enum class`, scoped constants, and explicit casts. Avoid C-style casts,
  raw owning pointers, and hidden global state.
- Preserve the fixed-step 60 Hz simulation. Do not make movement or timing
  depend on display refresh rate.
- Keep procedural art and audio procedural. Do not add arcade ROM content,
  ripped sprites, recorded arcade audio, or copyrighted fonts.
- Avoid adding dependencies. Discuss a proposed dependency with the maintainer
  before writing code around it.

## Documentation style

Cite a **file and a symbol**, never a line number:

```markdown
[`Game.cpp`](Sources/Core/Game.cpp) — `Game::updatePlaying`
```

`make docs-check` validates relative links, heading anchors, and quoted
symbols. `make docs-impact` lists documentation that references changed files.

## Workflow

1. Open an issue for a substantial behavior change or new scope.
2. Branch from `main` using a descriptive name, for example
   `feat/wave-bonus` or `fix/bunker-edge-hit`.
3. Make a coherent change with checks and docs.
4. Run the verification above.
5. Commit using a Conventional Commit message:
   - `feat: ...`
   - `fix: ...`
   - `docs: ...`
   - `perf: ...`
   - `refactor: ...`
   - `chore: ...`
6. Open a pull request describing the change, verification performed, and any
   known limitations.

Allowed direct pushes by repository agents are defined in `AGENTS.md`. Human
contributors should use pull requests unless the maintainer says otherwise.

## Reporting bugs

Use the repository issue tracker and include:

- macOS version, architecture, and compiler version;
- steps to reproduce;
- expected and observed behavior;
- a screenshot or short recording for visual issues; and
- whether the issue reproduces after `make app` and a clean launch.

Please do not include secrets, personal information, or unredacted local paths
in public reports.

## Security

Do not open a public issue for a suspected vulnerability. Follow
[SECURITY.md](SECURITY.md).

## Conduct

Be constructive, specific, and respectful. Harassment, discrimination, personal
attacks, and deliberately disruptive behavior are not welcome. The maintainer
may moderate, close, or block participation to keep the project healthy.

## Licensing and intellectual property

SPACE INVADERS is a trademark of TAITO Corporation. This is an unofficial fan
project and must not be presented as affiliated with or endorsed by TAITO.

No open-source license has been selected for this repository. Do not assume that
code, art, or audio may be reused or redistributed beyond this project. Only
submit original work or material with a documented compatible license. Adding a
license or changing the project's licensing requires explicit maintainer
approval.

For decisions about scope, releases, and repository policy, see
[GOVERNANCE.md](GOVERNANCE.md).
