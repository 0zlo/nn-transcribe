# Codex workflow (WSL container)

You said the CLI supports:
- `codex --cd <path>`
- `@` fuzzy file search
- `!` run local shell commands and feed output back as "user-provided result"

## Recommended "2-shot" sequence

### Shot 1 (build sanity)
Prompt:
- "Build the repo, confirm `nn_transcribe` compiles, run `! ./build/nn_transcribe --help` and fix any compile errors."

Commands:
- `! cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release`
- `! cmake --build build -j`

### Shot 2 (end-to-end)
Prompt:
- "Run transcription on a test wav, emit MIDI + JSON, confirm no runtime missing libs with `! ldd build/nn_transcribe`."

Commands:
- `! ldd build/nn_transcribe`
- `! ./build/nn_transcribe test.wav out.mid --model-dir assets/ModelData --json out.json --bpm 140`
