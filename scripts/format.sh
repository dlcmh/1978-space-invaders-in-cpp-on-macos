#!/bin/sh
set -eu
cd "$(dirname "$0")/.."

if command -v clang-format >/dev/null 2>&1; then
    CLANG_FORMAT="$(command -v clang-format)"
else
    CLANG_FORMAT="$(xcrun --find clang-format)"
fi

find Sources Checks -type f \( -name '*.cpp' -o -name '*.hpp' -o -name '*.mm' -o -name '*.h' \) \
    -print0 | xargs -0 "$CLANG_FORMAT" -i
