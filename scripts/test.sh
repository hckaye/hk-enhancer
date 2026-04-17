#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")/.."

# Build tests if needed
if [ ! -f build/HKEnhancerTests ]; then
    echo "Building tests..."
    cmake -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=ON
    cmake --build build --target HKEnhancerTests --config Release
fi

echo "Running tests..."
./build/HKEnhancerTests "$@"
