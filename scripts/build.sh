#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${ROOT_DIR}/build"

cmake -S "${ROOT_DIR}" -B "${BUILD_DIR}" -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DFETCHCONTENT_UPDATES_DISCONNECTED=ON
cmake --build "${BUILD_DIR}"

echo
echo "[+] Built:"
echo "    ${BUILD_DIR}/nn_transcribe"
