#!/usr/bin/env bash
set -e

# Navigate from NanoOcp1Demo/Resources/Deployment/macOS/ to the repo root.
cd "$(dirname "$0")/../../../../"

echo "=== NanoOcp1Demo — CMake build (macOS) ==="
echo "Working directory: $(pwd)"

cmake -B build -S . \
      -DNANOOCP1_BUILD_DEMO=ON \
      -DCMAKE_BUILD_TYPE=Release

cmake --build build --config Release

echo "=== Build complete ==="
