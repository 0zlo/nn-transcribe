# Architecture

## Data flow
audio file -> decode -> downmix -> resample to 22050 mono -> BasicPitch -> note events -> MIDI/JSON

## Modules
- nn_model/
  - Features: ONNX Runtime session for `features_model.onnx`
  - BasicPitchCNN: RTNeural CNN inference from JSON submodels
  - Notes: post-processing -> note events with optional bends
  - ModelAssets: loads ONNX + JSON from disk into memory

- nn_core/
  - audio: miniaudio decode + resample
  - midi_writer: convert note events -> MIDI using midifile

- apps/nn_transcribe
  - CLI wrapper around everything