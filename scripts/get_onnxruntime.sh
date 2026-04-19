#!/usr/bin/env bash
set -euo pipefail

# Usage:
#   scripts/get_onnxruntime.sh 1.17.3
# or:
#   ORT_VER=1.17.3 scripts/get_onnxruntime.sh
#
# Result:
#   third_party/onnxruntime/include/...
#   third_party/onnxruntime/lib/libonnxruntime.(so|dylib)

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
TP_DIR="${ROOT_DIR}/third_party"
DEST="${TP_DIR}/onnxruntime"

ORT_VER="${1:-${ORT_VER:-1.17.3}}"

case "$(uname -s)" in
  Darwin)
    PLATFORM="osx"
    case "$(uname -m)" in
      arm64) ARCH="arm64" ;;
      x86_64) ARCH="x86_64" ;;
      *) echo "[!] Unsupported macOS architecture: $(uname -m)" >&2; exit 1 ;;
    esac
    ;;
  Linux)
    PLATFORM="linux"
    case "$(uname -m)" in
      x86_64) ARCH="x64" ;;
      aarch64|arm64) ARCH="aarch64" ;;
      *) echo "[!] Unsupported Linux architecture: $(uname -m)" >&2; exit 1 ;;
    esac
    ;;
  *)
    echo "[!] Unsupported OS: $(uname -s)" >&2
    exit 1
    ;;
esac

PACKAGE="onnxruntime-${PLATFORM}-${ARCH}-${ORT_VER}"
URL="https://github.com/microsoft/onnxruntime/releases/download/v${ORT_VER}/${PACKAGE}.tgz"

mkdir -p "${TP_DIR}"
rm -rf "${DEST}"
mkdir -p "${DEST}"

echo "[*] Downloading ONNX Runtime ${ORT_VER} from:"
echo "    ${URL}"

tmp="$(mktemp -d)"
curl -L "${URL}" -o "${tmp}/ort.tgz"
tar -xzf "${tmp}/ort.tgz" -C "${tmp}"

mv "${tmp}/${PACKAGE}/include" "${DEST}/include"
mkdir -p "${DEST}/lib"
find "${tmp}/${PACKAGE}/lib" -maxdepth 1 -name 'libonnxruntime*' -exec cp -R {} "${DEST}/lib/" \;

echo "[+] Installed to ${DEST}"
