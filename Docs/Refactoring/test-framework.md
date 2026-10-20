# Keep the checks dependency-free instead of adopting a test framework

- **Date:** 2026-10-20
- **Status:** accepted
- **Scope:** verification tooling

## Proposal

Replace the custom `check(bool, message)` executable with XCTest or GoogleTest
so tests are discoverable, parametrised, and integrated with CI reporting.

## Why it was considered

Test frameworks provide fixtures, assertions, filtering, and reporting. A serious
project often benefits from these. The reference repository also used a
standalone checks executable because the bootstrap toolchain lacked XCTest.

## Options

1. **XCTest.** Rejected for v1: it would tie the headless simulation checks to an
   Xcode test host/project, while the application builds with CMake and the
   command line. The current checks must run with only the Apple toolchain.
2. **GoogleTest.** Rejected: a third-party dependency and package-fetch step for
   a few hundred lines of checks.
3. **Custom executable with a tiny assertion helper.** Chosen.
4. **No framework, but a shell script of one-off binaries.** Rejected: loses
   shared autopilot and invariant helpers.

## Evidence

- [Checks/main.cpp](../../Checks/main.cpp) — `testWaveProgression` and
  `testMysteryShipScoring` use reusable autopilot logic; the entire binary is the
  test runner and exits non-zero on failure.
- The suite currently reports **24,343** checks and runs in under a second,
  including a 60-second simulated soak. No filter or reporting features are
  currently needed to locate failures; messages include the invariant.
- The production API is intentionally sufficient for checks. There are no test
  hooks or conditional compilation branches for testing.
- Adding a framework would also add a build-system dependency, conflicting with
  the project's "no new dependencies" rule absent explicit approval.

## Decision

Keep the custom executable. It is not an argument against test frameworks in
general; it reflects the small surface area and the constraint that the game and
checks build from the same dependency-free CMake project.

## Consequences

- Tests run from a clean checkout with no package resolution.
- The check suite owns its small autopilot helpers and invariant printer.
- If CI reporting, parameterised fixtures, or parallel suites become necessary,
  this decision should be revisited with the maintainer, not changed silently.
