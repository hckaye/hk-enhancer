#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")/.."

# Find clang-tidy
CLANG_TIDY=$(command -v clang-tidy 2>/dev/null || find /opt/homebrew/opt/llvm*/bin -name clang-tidy 2>/dev/null | head -1)

if [ -z "$CLANG_TIDY" ]; then
    echo "Error: clang-tidy not found. Install with: brew install llvm"
    exit 1
fi

echo "Using: $CLANG_TIDY"

# Ensure compile_commands.json exists
if [ ! -f build/compile_commands.json ]; then
    echo "Generating compile_commands.json..."
    cmake -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DCMAKE_BUILD_TYPE=Release
fi

# Run clang-tidy on our source files only
SOURCE_FILES=$(find Source -name '*.cpp' -o -name '*.h' | sort)

echo "Linting files:"
echo "$SOURCE_FILES"
echo "---"

ERRORS=0
for f in $SOURCE_FILES; do
    echo "Checking $f..."
    if ! "$CLANG_TIDY" -p build "$f" 2>&1 | grep -v "^$" | grep -v "warnings generated" | grep -v "Use -header-filter"; then
        true
    fi
done

echo "---"
echo "Lint complete."
