#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")/.."

CLANG_FORMAT=$(command -v clang-format 2>/dev/null || find /opt/homebrew/opt/llvm*/bin -name clang-format 2>/dev/null | head -1)

if [ -z "$CLANG_FORMAT" ]; then
    echo "Error: clang-format not found. Install with: brew install llvm"
    exit 1
fi

echo "Using: $CLANG_FORMAT"

FILES=$(find Source Tests -name '*.cpp' -o -name '*.h' | sort)

if [ "${1:-}" = "--check" ]; then
    echo "Checking format..."
    FAILED=0
    for f in $FILES; do
        if ! "$CLANG_FORMAT" --dry-run --Werror "$f" 2>/dev/null; then
            FAILED=1
        fi
    done
    if [ "$FAILED" -eq 1 ]; then
        echo "Format check failed. Run ./scripts/format.sh to fix."
        exit 1
    fi
    echo "All files formatted correctly."
else
    echo "Formatting files..."
    for f in $FILES; do
        echo "  $f"
        "$CLANG_FORMAT" -i "$f"
    done
    echo "Done."
fi
