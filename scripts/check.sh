#!/bin/sh
# The verification gate for SPACE INVADERS — Metal Arcade.
#
# Stages:
#   1. clang-format lint       — house style and line length
#   2. release build           — warnings are errors
#   3. spaceinvaders_checks    — headless simulation regression suite
#   4. docs-check              — links, anchors, and C++ symbol references
#
# Ninja and clang-format ship with Xcode/Command Line Tools or Homebrew. No
# third-party package manager or runtime dependency is required.

set -eu
cd "$(dirname "$0")/.."

if command -v clang-format >/dev/null 2>&1; then
    CLANG_FORMAT="$(command -v clang-format)"
elif command -v xcrun >/dev/null 2>&1; then
    CLANG_FORMAT="$(xcrun --find clang-format 2>/dev/null || true)"
else
    CLANG_FORMAT=""
fi
if [ -z "$CLANG_FORMAT" ] || [ ! -x "$CLANG_FORMAT" ]; then
    echo "clang-format not found. Install Xcode/Command Line Tools or clang-format." >&2
    exit 1
fi

echo "==> clang-format lint"
# shellcheck disable=SC2046
"$CLANG_FORMAT" --dry-run --Werror $(find Sources Checks -type f \( -name '*.cpp' -o -name '*.hpp' -o -name '*.mm' -o -name '*.h' \) | sort)

BUILD_DIR="${BUILD_DIR:-build/check}"
echo "==> configure release build with warnings as errors"
cmake -S . -B "$BUILD_DIR" -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DSPACE_INVADERS_WARNINGS_AS_ERRORS=ON >/dev/null
echo "==> build"
cmake --build "$BUILD_DIR"

echo "==> simulation checks"
"$BUILD_DIR/bin/spaceinvaders_checks"

echo "==> docs-check"
python3 scripts/check_docs.py

echo "==> all checks passed"
