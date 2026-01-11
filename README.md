# nn-transcribe (NeuralNote core, standalone CLI)

This repo wraps the NeuralNote transcription core (Basic Pitch style) into a standalone CLI:
- ONNX Runtime for feature model (`features_model.onnx`)
- RTNeural for CNN submodels (JSON weights)
- miniaudio for decode + resample to 22050 Hz mono
- midifile for MIDI output

## Requirements
Ubuntu 20.04+ (WSL is fine), gcc, cmake, ninja, curl.

```bash
sudo apt-get update
sudo apt-get install -y build-essential cmake ninja-build git curl
