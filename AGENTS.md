
## `AGENTS.md`
```md
# Agent rules (Codex)

You are working in a CMake C++ repo for an audio→MIDI transcription CLI.

## Build command
- Configure/build:
  - `cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release`
  - `cmake --build build`

## Do not modify
- `assets/ModelData/` (user provides)
- `third_party/onnxruntime/` (downloaded by script)

## Coding rules
- C++17
- No JUCE dependency
- Avoid global state except `nn::ModelAssets` singleton (must be initialized before constructing `BasicPitch`)

## How model assets are loaded
- `nn::ModelAssets::init(model_dir)` must be called before creating `BasicPitch`.

## Output expectations
- CLI must:
  - decode any common audio file
  - resample to 22050 Hz mono float
  - run transcription
  - write MIDI
  - optionally write JSON of note events