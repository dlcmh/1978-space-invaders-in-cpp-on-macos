#!/bin/bash
# Build a locally runnable, ad-hoc-signed macOS application bundle.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

BUILD_DIR="${BUILD_DIR:-build/release}"
APP_NAME="SPACE-INVADERS"
APP_DIR="$ROOT/dist/$APP_NAME.app"
CONFIGURATION="Release"

if [[ ! -f "$ROOT/Resources/AppIcon.icns" ]]; then
    python3 scripts/make_icon.py
fi

echo "→ Configuring release build ($(uname -m))…"
cmake -S "$ROOT" -B "$ROOT/$BUILD_DIR" -G Ninja \
    -DCMAKE_BUILD_TYPE="$CONFIGURATION" \
    -DSPACE_INVADERS_WARNINGS_AS_ERRORS=ON
cmake --build "$ROOT/$BUILD_DIR" --target spaceinvaders_app

BINARY="$ROOT/$BUILD_DIR/bin/spaceinvaders_app"
if [[ ! -x "$BINARY" ]]; then
    echo "Build did not produce $BINARY" >&2
    exit 1
fi

rm -rf "$APP_DIR"
mkdir -p "$APP_DIR/Contents/MacOS" "$APP_DIR/Contents/Resources"

cp "$BINARY" "$APP_DIR/Contents/MacOS/$APP_NAME"
cp "$ROOT/Resources/Info.plist" "$APP_DIR/Contents/Info.plist"
cp "$ROOT/Resources/AppIcon.icns" "$APP_DIR/Contents/Resources/AppIcon.icns"
printf 'APPL????' > "$APP_DIR/Contents/PkgInfo"
chmod +x "$APP_DIR/Contents/MacOS/$APP_NAME"

# An ad-hoc signature is sufficient for a locally built, non-distributed app.
codesign --force --deep --sign - "$APP_DIR" >/dev/null 2>&1 || true

echo "→ App bundle: $APP_DIR"
echo "  open \"$APP_DIR\""
