#!/usr/bin/env bash
set -e

# Navigate from NanoOcp1Demo/Resources/Deployment/Linux/ to the repo root.
cd "$(dirname "$0")/../../../../"

echo "=== NanoOcp1Demo — CMake build (Linux) ==="
echo "Working directory: $(pwd)"

cmake -B build -S . \
      -DNANOOCP1_BUILD_DEMO=ON \
      -DCMAKE_BUILD_TYPE=Release

cmake --build build --config Release --parallel

echo "=== Build complete ==="
