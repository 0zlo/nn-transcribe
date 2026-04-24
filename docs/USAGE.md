# nn-transcribe Usage

`nn_transcribe` converts an audio file to MIDI note events using the bundled model assets.

```bash
build/nn_transcribe input.wav output.mid --json output.json
```

## Current Options

```text
--model-dir <dir>     Path to ModelData directory
--json <path>         Write note events JSON dump
--settings <path>     Load JSON settings file
--preset <path>       Alias for --settings
--bpm <num>           Tempo for MIDI mapping
--note-sens <0-1>     Higher means more notes
--split-sens <0-1>    Higher means more note splits
--min-note-ms <ms>    Minimum note duration
```

CLI flags override values loaded from `--settings` or `--preset`.

## Settings File

The settings file is JSON so a future GUI can save it directly.

```json
{
  "bpm": 120,
  "note_sens": 0.5,
  "split_sens": 0.5,
  "min_note_ms": 80
}
```

The same values can also live under a `transcribe` object, which leaves room for future modular tools to share one project file.

```json
{
  "transcribe": {
    "bpm": 120,
    "note_sens": 0.5,
    "split_sens": 0.5,
    "min_note_ms": 80
  }
}
```

Dash-style keys are accepted too: `note-sens`, `split-sens`, and `min-note-ms`.

## Useful Starting Points

More conservative:

```json
{
  "note_sens": 0.4,
  "split_sens": 0.35,
  "min_note_ms": 120
}
```

More detailed:

```json
{
  "note_sens": 0.65,
  "split_sens": 0.6,
  "min_note_ms": 50
}
```

Run with a settings file:

```bash
build/nn_transcribe input.wav output.mid \
  --settings my-settings.json \
  --json output.json
```

Override one saved value from the command line:

```bash
build/nn_transcribe input.wav output.mid \
  --settings my-settings.json \
  --note-sens 0.7
```

## JSON Output

When `--json` is provided, the output includes the resolved settings:

```json
{
  "settings_path": "my-settings.json",
  "settings": {
    "bpm": 120,
    "note_sens": 0.5,
    "split_sens": 0.5,
    "min_note_ms": 80
  },
  "events": []
}
```

Each event includes note timing, MIDI pitch, amplitude, frame indexes, and bend data.

## MIDI Cleanup

`nn_midi_clean` is a separate tool for post-transcription MIDI cleanup.

```bash
build/nn_midi_clean input.mid output.mid \
  --quantize 1/16 \
  --min-note-ms 80 \
  --merge-ms 40
```

Current options:

```text
--quantize <grid>       Quantize starts/ends to grid: 1/8, 1/16, 1/32, 1/8t, ticks:120
--quantize-ends         Quantize note ends too
--no-quantize-ends      Keep original note durations after moving starts
--swing <0-1>           Delay odd grid positions by a fraction of half a grid
--min-note-ms <ms>      Drop notes shorter than this duration
--merge-ms <ms>         Merge same-pitch notes separated by this gap or less
--max-polyphony <n>     Limit overlapping notes; 0 means unlimited
--bpm <num>             Tempo used for ms options if MIDI has no tempo event
```

The first version writes one cleaned MIDI track, preserving non-note events such as tempo and controller messages.

Run the hardcoded MIDI cleanup smoke test:

```bash
scripts/test_midi_clean.sh
```

It reads the tuning corpus MIDI files from `build/tuning_corpus/no_drums_instrumental_10/midi` and writes cleaned outputs/logs to `build/test_runs/midi_clean/latest`.
