# Security Policy

## Supported versions

| Version | Supported |
| --- | --- |
| 1.0 | Yes |

This is a small, locally distributed macOS game. Security fixes are applied to
the current `main` branch and the latest release.

## Reporting a vulnerability

Please do not open a public issue for a suspected vulnerability. Use GitHub's
private vulnerability reporting for the canonical repository if it is enabled,
or contact the maintainer [@dlcmh](https://github.com/dlcmh) privately through
GitHub with:

- a description of the issue and its impact;
- the affected version and macOS build;
- reproduction steps or a minimal proof of concept; and
- any suggested mitigation.

The maintainer will acknowledge reports as time permits, investigate, and
coordinate a fix and disclosure when appropriate. This project has no bug
bounty program.

## Security posture

- **Offline by design.** The game makes no network requests and has no
  telemetry, analytics, accounts, remote configuration, or crash uploaders.
- **No third-party runtime dependencies.** The game links only Apple frameworks
  (AppKit, Metal, MetalKit, QuartzCore, AudioToolbox, Foundation).
- **Minimal persistence.** Only the high score and mute preference are stored
  in local `NSUserDefaults`:
  - `SpaceInvadersCPPHighScore`
  - `SpaceInvadersCPPMuted`
- **Generated assets only.** No downloaded assets, ROM content, executable
  plug-ins, or external update mechanism exists.
- **No privileged operations.** The app does not request camera, microphone,
  contacts, location, automation, or full-disk access. It uses the default audio
  output device.
- **Local bundle only.** Release bundles are ad-hoc signed for local use. No
  Developer ID signing, notarization, or distribution identity is claimed
  unless the maintainer explicitly publishes such a build.
- **Build reproducibility.** CMake, Ninja, the Apple toolchain, and the checked-in
  sources are the build inputs. The game itself has no package-manager fetch
  step.

## Out of scope

- Reports requiring a malicious local process with the same user privileges to
  modify the app bundle or preferences.
- Issues in macOS, Metal, AppKit, Core Audio, or third-party tooling that are
  not caused by this repository's code.
- Social-engineering or lost-device scenarios.

## Secrets

The repository contains no secrets, credentials, or signing identities. Never
commit signing certificates, provisioning profiles, API keys, or personal data
to the repository or issue tracker.
