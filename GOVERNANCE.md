# Governance

This document defines how SPACE INVADERS — Metal Arcade is maintained, how
decisions are made, and how releases and legal obligations are handled. It is
intentionally lightweight for a small fan project.

- **Canonical repository:** <https://github.com/dlcmh/1978-space-invaders-in-cpp-on-macos>
- **Canonical branch:** `main`
- **Maintainer:** David Chin ([@dlcmh](https://github.com/dlcmh))
- **Governance baseline:** 2026-10-20
- **Version covered:** 1.0

## Mission and scope

The project is a native, offline, procedurally generated macOS tribute to the
1978 Space Invaders arcade game. Its quality priorities are faithful arcade
gameplay, clean modern C++ architecture, a dependency-free Metal renderer,
deterministic behavior, and reproducible builds.

### In scope

- The 55-alien formation, march, downward steps, acceleration, and species
  scoring.
- Player movement and firing, enemy fire, destructible bunkers, mystery ships,
  lives, waves, high scores, and fixed-step deterministic simulation.
- Native macOS presentation, input, procedural sound, and local persistence.
- Procedural art, audio, fonts, and app packaging.
- Simulation checks, documentation, and release tooling.

### Explicitly excluded unless re-scoped

- Network play, online leaderboards, accounts, telemetry, and remote content.
- Web, iOS, or other platform ports.
- Original arcade ROMs, sprites, fonts, audio, or other proprietary assets.
- Two-player mode, original-ROM emulation, and hardware-level quirks that
  depend on undocumented memory corruption.
- Paid distribution or any claim of official affiliation with TAITO.

Scope changes require the process described under **Decision making**.

## Roles

### Maintainer

The maintainer has final responsibility for:

- repository direction, roadmap, and scope;
- merging and publishing changes;
- releases, tags, signing, and distribution;
- licensing and trademark decisions;
- security response; and
- amendments to governance documents.

The current sole maintainer is [@dlcmh](https://github.com/dlcmh).

### Contributors

Contributors propose fixes, features, documentation, and tests through issues,
pull requests, or maintainer-authorized direct pushes. Contributions are
reviewed against the quality bar in [CONTRIBUTING.md](CONTRIBUTING.md) and the
architectural rules in [AGENTS.md](AGENTS.md).

### Users

Users may report bugs and request features through the GitHub issue tracker.
Security reports must follow [SECURITY.md](SECURITY.md).

## Decision making

The project uses a **maintainer-led** model:

- Day-to-day fixes and documentation improvements proceed through normal review.
- Substantial gameplay changes, new dependencies, platform changes, licensing
  changes, and releases require explicit maintainer approval.
- Discussion happens in the relevant GitHub issue or pull request so the record
  remains public where appropriate.
- When consensus is unclear, the maintainer makes the final decision and records
  the rationale.

Architectural invariants in [AGENTS.md](AGENTS.md) are considered settled policy
until changed through the governance amendment process.

## Quality gates

Changes to `main` are expected to pass:

1. `make check`, which includes formatting, a warnings-as-errors Release build,
   the simulation checks, and documentation validation.
2. A manual launch for any rendering, input, audio, shader, or windowing change.
3. Review of user-facing documentation and governance accuracy.

Generated binaries are not committed to source control. Distribution artifacts
are produced through `make app` and `make dmg` and published only when the
maintainer authorizes a release.

## Agent commit and push authority

The maintainer has pre-authorized repository agents to commit and push normal
maintenance and documentation work to `origin/main` without a separate request
for each change. Completing a task includes landing it: an agent runs the
quality gates, commits with a Conventional Commit message, and pushes.

The limits of that authority:

- The quality gates must pass first. A failing `make check` is reported, not
  committed around silently.
- Agents stage explicit paths, and never commit generated artifacts, build
  products, DMGs, or secrets.
- Force-pushing, rewriting published history, and changing remotes remain
  prohibited.
- Dependent, unrelated, or maintainer-approval work does not become authorized
  merely because an agent can push. Agents still stop for the cases listed in
  [AGENTS.md](AGENTS.md#when-to-stop-and-ask-the-maintainer): licensing,
  dependencies and network access, incompatible gameplay or scoring changes,
  releases and tags, history rewrites, and large optional scope.

The operating procedure lives in
[AGENTS.md](AGENTS.md#committing-and-pushing-are-the-default).

## Releases

Release versions follow semantic versioning for the application and its public
documentation.

A release requires the maintainer to:

1. confirm the checks and manual verification pass;
2. update the version in `Resources/Info.plist` and any `dist/` artifact naming;
3. confirm README, governance, and provenance records are current;
4. commit and tag the release if a tag is requested;
5. attach any `.app`/`.dmg` artifacts to a GitHub Release when distribution is
   intended; and
6. record notable changes and any compatibility notes.

Version 1.0 corresponds to the initial implementation commit and its immediately
following governance baseline. No Git tag or public GitHub Release was created
as part of the governance setup.

## Licensing and intellectual property

- **No open-source license has been selected for this repository.** Absent a
  separate license, default copyright applies and no permission to reuse or
  redistribute is granted.
- The maintainer reserves all licensing decisions. A license must not be added,
  changed, or implied by an agent or contributor without explicit approval.
- All committed code and generated art/audio must be original or properly
  licensed with attribution.
- Ripped or extracted assets from Space Invaders arcade hardware, ROMs, or
  official products are prohibited.
- SPACE INVADERS is a trademark of TAITO Corporation. This project is an
  unofficial fan tribute and is not affiliated with or endorsed by TAITO.
- Public descriptions must retain this disclaimer and must not imply official
  status.

See [Docs/PROJECT_ORIGIN.md](Docs/PROJECT_ORIGIN.md) for the commissioning record
and scope decisions.

## Security and privacy

The game is offline, has no accounts, and does not collect telemetry. Only a
local high score and mute preference are stored. Security handling is defined in
[SECURITY.md](SECURITY.md).

## Repository and remote policy

- `origin` is the canonical GitHub remote:
  <https://github.com/dlcmh/1978-space-invaders-in-cpp-on-macos.git>
- `main` tracks `origin/main`.
- Force-pushing, rewriting published history, changing the canonical remote, or
  adding another release remote requires maintainer approval.
- Agents follow the commit and push rules in [AGENTS.md](AGENTS.md); ordinary
  commits and pushes are pre-authorized under
  [Agent commit and push authority](#agent-commit-and-push-authority).

## Amendments

Governance changes are proposed as a pull request or explicit maintainer
instruction and require maintainer approval. The change should state why the
current rule is insufficient, what replaces it, and any migration impact.

| Date | Change | Authority |
| --- | --- | --- |
| 2026-10-20 | Initial governance baseline for version 1.0. | Maintainer |
| 2026-10-21 | Committing and pushing ordinary maintenance work became the agent default rather than a per-change authorization. Reason: the earlier rule, "push when authorized", was ambiguous and led agents to hand back finished, verified work and wait. Replaced by [Agent commit and push authority](#agent-commit-and-push-authority); the protected operations listed in [AGENTS.md](AGENTS.md#when-to-stop-and-ask-the-maintainer) still require approval. Migration impact: none — no history was rewritten, and the policy only widens agent authority for ordinary work. | Maintainer instruction |

The **Original request** quotation in
[Docs/PROJECT_ORIGIN.md](Docs/PROJECT_ORIGIN.md) is immutable. Historical
decisions may be annotated but must not be silently rewritten.

## Contact

Use the GitHub issue tracker for project questions and
[SECURITY.md](SECURITY.md) for vulnerabilities.
