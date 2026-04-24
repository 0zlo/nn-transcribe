#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
INPUT_DIR="${ROOT_DIR}/build/tuning_corpus/no_drums_instrumental_10/midi"
OUT_DIR="${ROOT_DIR}/build/test_runs/midi_clean/latest"

mkdir -p "${OUT_DIR}"

if [[ ! -x "${ROOT_DIR}/build/nn_midi_clean" ]]; then
  cmake --build "${ROOT_DIR}/build" --target nn_midi_clean
fi

if [[ ! -d "${INPUT_DIR}" ]]; then
  echo "[!] Missing MIDI corpus: ${INPUT_DIR}" >&2
  exit 1
fi

echo "input	output	status"
for input in "${INPUT_DIR}"/*.mid; do
  name="$(basename "${input}" .mid)"
  output="${OUT_DIR}/${name}.clean.mid"
  "${ROOT_DIR}/build/nn_midi_clean" "${input}" "${output}" \
    --quantize 1/16 \
    --min-note-ms 70 \
    --merge-ms 40 \
    > "${OUT_DIR}/${name}.log"
  echo "${input}	${output}	ok"
done

echo
echo "[+] MIDI clean test outputs:"
echo "    ${OUT_DIR}"
