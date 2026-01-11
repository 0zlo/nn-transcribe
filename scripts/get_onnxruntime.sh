#!/usr/bin/env bash
set -euo pipefail

# Usage:
#   scripts/get_onnxruntime.sh 1.17.3
# or:
#   ORT_VER=1.17.3 scripts/get_onnxruntime.sh
#
# Result:
#   third_party/onnxruntime/include/...
#   third_party/onnxruntime/lib/libonnxruntime.so

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
TP_DIR="${ROOT_DIR}/third_party"
DEST="${TP_DIR}/onnxruntime"

ORT_VER="${1:-${ORT_VER:-1.17.3}}"
ARCH="x64"

URL="https://github.com/microsoft/onnxruntime/releases/download/v${ORT_VER}/onnxruntime-linux-${ARCH}-${ORT_VER}.tgz"

mkdir -p "${TP_DIR}"
rm -rf "${DEST}"
mkdir -p "${DEST}"

echo "[*] Downloading ONNX Runtime ${ORT_VER} from:"
echo "    ${URL}"

tmp="$(mktemp -d)"
curl -L "${URL}" -o "${tmp}/ort.tgz"
tar -xzf "${tmp}/ort.tgz" -C "${tmp}"

mv "${tmp}/onnxruntime-linux-${ARCH}-${ORT_VER}/include" "${DEST}/include"
mkdir -p "${DEST}/lib"
mv "${tmp}/onnxruntime-linux-${ARCH}-${ORT_VER}/lib/libonnxruntime.so" "${DEST}/lib/libonnxruntime.so"

echo "[+] Installed to ${DEST}"
