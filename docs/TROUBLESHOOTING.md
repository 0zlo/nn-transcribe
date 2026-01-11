# Troubleshooting

## "libonnxruntime.so not found"
We copy `libonnxruntime.so` next to the executable and set rpath to `$ORIGIN`.
Check:
- `ldd build/nn_transcribe | grep onnx`

## "ModelAssets not initialized"
You must pass `--model-dir assets/ModelData` (or set env `NN_MODEL_DIR`).

## Bad transcription / empty notes
Try tweaking:
- `--note-sens` higher => more notes
- `--split-sens` higher => more splits
- `--min-note-ms` lower => shorter notes allowed
