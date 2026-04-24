#include <algorithm>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>
#include <tuple>
#include <vector>

#include "MidiFile.h"

namespace {

struct Options {
    std::string inputPath;
    std::string outputPath;
    std::string quantizeSpec;
    bool quantizeEnds = true;
    double swing = 0.0;
    double bpm = 0.0;
    double minNoteMs = 0.0;
    double mergeMs = 0.0;
    int maxPolyphony = 0;
};

struct Note {
    int channel = 0;
    int key = 0;
    int velocity = 0;
    int start = 0;
    int end = 0;
    bool dropped = false;
};

struct Stats {
    int inputNotes = 0;
    int quantizedNotes = 0;
    int mergedNotes = 0;
    int shortNotesDropped = 0;
    int polyphonyDropped = 0;
    int outputNotes = 0;
};

void usage()
{
    std::cout <<
R"(nn_midi_clean <input.mid> <output.mid> [options]

Options:
  --quantize <grid>       Quantize starts/ends to grid, e.g. 1/8, 1/16, 1/32, 1/8t, ticks:120
  --quantize-ends         Quantize note ends too (default)
  --no-quantize-ends      Keep original note end offsets after start quantization
  --swing <0-1>           Delay odd grid positions by this fraction of half a grid (default 0)
  --min-note-ms <ms>      Drop notes shorter than this duration after quantize/merge
  --merge-ms <ms>         Merge same-pitch notes separated by at most this gap
  --max-polyphony <n>     Limit overlapping notes; 0 means unlimited (default 0)
  --bpm <num>             Tempo used for ms options if MIDI has no tempo event

Examples:
  nn_midi_clean in.mid out.mid --quantize 1/16 --min-note-ms 80 --merge-ms 40
  nn_midi_clean in.mid out.mid --quantize 1/8t --max-polyphony 1
)";
}

bool getArg(int& i, int argc, char** argv, std::string& out)
{
    if (i + 1 >= argc) return false;
    out = argv[++i];
    return true;
}

double parseDouble(const std::string& text, const std::string& name)
{
    try {
        size_t idx = 0;
        const double value = std::stod(text, &idx);
        if (idx != text.size()) throw std::invalid_argument("trailing chars");
        return value;
    } catch (const std::exception&) {
        throw std::runtime_error("Invalid " + name + ": " + text);
    }
}

int parseInt(const std::string& text, const std::string& name)
{
    try {
        size_t idx = 0;
        const int value = std::stoi(text, &idx);
        if (idx != text.size()) throw std::invalid_argument("trailing chars");
        return value;
    } catch (const std::exception&) {
        throw std::runtime_error("Invalid " + name + ": " + text);
    }
}

int clampMidiValue(int value)
{
    return std::max(0, std::min(127, value));
}

int roundToInt(double value)
{
    return static_cast<int>(std::llround(value));
}

int parseQuantizeGridTicks(std::string spec, int tpq)
{
    if (spec.empty() || spec == "off" || spec == "none" || spec == "0") {
        return 0;
    }

    constexpr const char* ticksPrefix = "ticks:";
    if (spec.rfind(ticksPrefix, 0) == 0) {
        const int ticks = parseInt(spec.substr(std::string(ticksPrefix).size()), "quantize ticks");
        if (ticks < 0) throw std::runtime_error("Quantize ticks must be >= 0");
        return ticks;
    }

    bool triplet = false;
    if (!spec.empty() && (spec.back() == 't' || spec.back() == 'T')) {
        triplet = true;
        spec.pop_back();
    }

    double numerator = 1.0;
    double denominator = 0.0;
    const auto slash = spec.find('/');
    if (slash == std::string::npos) {
        denominator = parseDouble(spec, "quantize denominator");
    } else {
        numerator = parseDouble(spec.substr(0, slash), "quantize numerator");
        denominator = parseDouble(spec.substr(slash + 1), "quantize denominator");
    }

    if (numerator <= 0.0 || denominator <= 0.0) {
        throw std::runtime_error("Quantize grid must be positive: " + spec);
    }

    double ticks = (static_cast<double>(tpq) * 4.0) * numerator / denominator;
    if (triplet) {
        ticks *= 2.0 / 3.0;
    }

    const int rounded = std::max(1, roundToInt(ticks));
    return rounded;
}

int quantizeTick(int tick, int gridTicks, double swing)
{
    if (gridTicks <= 0) return tick;

    int q = roundToInt(static_cast<double>(tick) / static_cast<double>(gridTicks)) * gridTicks;
    if (swing > 0.0) {
        const int slot = q / gridTicks;
        if (slot % 2 != 0) {
            q += roundToInt(swing * static_cast<double>(gridTicks) * 0.5);
        }
    }
    return std::max(0, q);
}

double detectFirstTempoBpm(const smf::MidiFile& midi)
{
    for (int track = 0; track < midi.getTrackCount(); ++track) {
        for (int i = 0; i < midi.getEventCount(track); ++i) {
            const auto& event = midi[track][i];
            if (event.isTempo()) {
                return event.getTempoBPM();
            }
        }
    }
    return 120.0;
}

int msToTicks(double ms, double bpm, int tpq)
{
    if (ms <= 0.0) return 0;
    const double ticksPerMs = (static_cast<double>(tpq) * bpm / 60.0) / 1000.0;
    return std::max(1, roundToInt(ms * ticksPerMs));
}

std::vector<Note> extractNotes(smf::MidiFile& midi)
{
    std::vector<Note> notes;
    midi.linkNotePairs();

    for (int track = 0; track < midi.getTrackCount(); ++track) {
        for (int i = 0; i < midi.getEventCount(track); ++i) {
            auto& event = midi[track][i];
            if (!event.isNoteOn()) continue;

            auto* linked = event.getLinkedEvent();
            if (!linked || linked->tick <= event.tick) continue;

            Note note;
            note.channel = event.getChannel();
            note.key = event.getKeyNumber();
            note.velocity = event.getVelocity();
            note.start = event.tick;
            note.end = linked->tick;
            notes.push_back(note);
        }
    }

    return notes;
}

void quantizeNotes(std::vector<Note>& notes, int gridTicks, bool quantizeEnds, double swing, Stats& stats)
{
    if (gridTicks <= 0) return;

    for (auto& note : notes) {
        const int originalLength = std::max(1, note.end - note.start);
        note.start = quantizeTick(note.start, gridTicks, swing);
        if (quantizeEnds) {
            note.end = quantizeTick(note.end, gridTicks, swing);
        } else {
            note.end = note.start + originalLength;
        }
        if (note.end <= note.start) {
            note.end = note.start + std::max(1, std::min(originalLength, gridTicks));
        }
        ++stats.quantizedNotes;
    }
}

void mergeAdjacentNotes(std::vector<Note>& notes, int mergeTicks, Stats& stats)
{
    if (mergeTicks <= 0 || notes.empty()) return;

    std::sort(notes.begin(), notes.end(), [](const Note& a, const Note& b) {
        return std::tie(a.channel, a.key, a.start, a.end) < std::tie(b.channel, b.key, b.start, b.end);
    });

    std::vector<Note> merged;
    merged.reserve(notes.size());

    for (const auto& note : notes) {
        if (!merged.empty()) {
            auto& prev = merged.back();
            if (prev.channel == note.channel && prev.key == note.key && note.start <= prev.end + mergeTicks) {
                prev.end = std::max(prev.end, note.end);
                prev.velocity = std::max(prev.velocity, note.velocity);
                ++stats.mergedNotes;
                continue;
            }
        }
        merged.push_back(note);
    }

    notes = std::move(merged);
}

void dropShortNotes(std::vector<Note>& notes, int minTicks, Stats& stats)
{
    if (minTicks <= 0) return;

    auto it = std::remove_if(notes.begin(), notes.end(), [&](const Note& note) {
        const bool drop = (note.end - note.start) < minTicks;
        if (drop) ++stats.shortNotesDropped;
        return drop;
    });
    notes.erase(it, notes.end());
}

std::tuple<int, int, int> notePriority(const Note& note)
{
    return {note.velocity, note.end - note.start, note.key};
}

void limitPolyphony(std::vector<Note>& notes, int maxPolyphony, Stats& stats)
{
    if (maxPolyphony <= 0 || notes.empty()) return;

    std::sort(notes.begin(), notes.end(), [](const Note& a, const Note& b) {
        return std::tie(a.start, a.end, a.key, a.channel) < std::tie(b.start, b.end, b.key, b.channel);
    });

    std::vector<size_t> active;
    for (size_t i = 0; i < notes.size(); ++i) {
        active.erase(std::remove_if(active.begin(), active.end(), [&](size_t idx) {
            return notes[idx].dropped || notes[idx].end <= notes[i].start;
        }), active.end());

        if (static_cast<int>(active.size()) < maxPolyphony) {
            active.push_back(i);
            continue;
        }

        auto weakest = std::min_element(active.begin(), active.end(), [&](size_t a, size_t b) {
            return notePriority(notes[a]) < notePriority(notes[b]);
        });

        if (weakest != active.end() && notePriority(notes[i]) > notePriority(notes[*weakest])) {
            notes[*weakest].dropped = true;
            *weakest = i;
            ++stats.polyphonyDropped;
        } else {
            notes[i].dropped = true;
            ++stats.polyphonyDropped;
        }
    }

    notes.erase(std::remove_if(notes.begin(), notes.end(), [](const Note& note) {
        return note.dropped;
    }), notes.end());
}

void copyNonNoteEvents(const smf::MidiFile& input, smf::MidiFile& output)
{
    for (int track = 0; track < input.getTrackCount(); ++track) {
        for (int i = 0; i < input.getEventCount(track); ++i) {
            const auto& event = input[track][i];
            if (event.isNoteOn() || event.isNoteOff() || event.isEndOfTrack()) {
                continue;
            }
            smf::MidiEvent copy = event;
            output.addEvent(0, copy);
        }
    }
}

void writeCleanMidi(const smf::MidiFile& input, const std::vector<Note>& notes, const Options& options)
{
    smf::MidiFile output;
    output.absoluteTicks();
    output.setTicksPerQuarterNote(input.getTicksPerQuarterNote());

    copyNonNoteEvents(input, output);

    for (const auto& note : notes) {
        output.addNoteOn(0, note.start, note.channel, clampMidiValue(note.key), clampMidiValue(note.velocity));
        output.addNoteOff(0, note.end, note.channel, clampMidiValue(note.key));
    }

    output.sortTracksNoteOffsBeforeOns();
    if (!output.write(options.outputPath)) {
        throw std::runtime_error("Failed to write MIDI: " + options.outputPath);
    }
}

Options parseOptions(int argc, char** argv)
{
    if (argc == 2 && (std::string(argv[1]) == "--help" || std::string(argv[1]) == "-h")) {
        usage();
        std::exit(0);
    }
    if (argc < 3) {
        usage();
        std::exit(1);
    }

    Options options;
    options.inputPath = argv[1];
    options.outputPath = argv[2];

    for (int i = 3; i < argc; ++i) {
        const std::string arg = argv[i];
        std::string value;
        if (arg == "--help" || arg == "-h") {
            usage();
            std::exit(0);
        } else if (arg == "--quantize") {
            if (!getArg(i, argc, argv, options.quantizeSpec)) throw std::runtime_error("Missing value for --quantize");
        } else if (arg == "--quantize-ends") {
            options.quantizeEnds = true;
        } else if (arg == "--no-quantize-ends") {
            options.quantizeEnds = false;
        } else if (arg == "--swing") {
            if (!getArg(i, argc, argv, value)) throw std::runtime_error("Missing value for --swing");
            options.swing = parseDouble(value, "--swing");
        } else if (arg == "--min-note-ms") {
            if (!getArg(i, argc, argv, value)) throw std::runtime_error("Missing value for --min-note-ms");
            options.minNoteMs = parseDouble(value, "--min-note-ms");
        } else if (arg == "--merge-ms") {
            if (!getArg(i, argc, argv, value)) throw std::runtime_error("Missing value for --merge-ms");
            options.mergeMs = parseDouble(value, "--merge-ms");
        } else if (arg == "--max-polyphony") {
            if (!getArg(i, argc, argv, value)) throw std::runtime_error("Missing value for --max-polyphony");
            options.maxPolyphony = parseInt(value, "--max-polyphony");
        } else if (arg == "--bpm") {
            if (!getArg(i, argc, argv, value)) throw std::runtime_error("Missing value for --bpm");
            options.bpm = parseDouble(value, "--bpm");
        } else {
            throw std::runtime_error("Unknown option: " + arg);
        }
    }

    if (options.swing < 0.0 || options.swing > 1.0) {
        throw std::runtime_error("--swing must be between 0 and 1");
    }
    if (options.minNoteMs < 0.0) throw std::runtime_error("--min-note-ms must be >= 0");
    if (options.mergeMs < 0.0) throw std::runtime_error("--merge-ms must be >= 0");
    if (options.maxPolyphony < 0) throw std::runtime_error("--max-polyphony must be >= 0");
    if (options.bpm < 0.0) throw std::runtime_error("--bpm must be >= 0");

    return options;
}

} // namespace

int main(int argc, char** argv)
{
    try {
        const Options options = parseOptions(argc, argv);

        smf::MidiFile midi;
        if (!midi.read(options.inputPath)) {
            throw std::runtime_error("Failed to read MIDI: " + options.inputPath);
        }
        midi.absoluteTicks();

        const int tpq = midi.getTicksPerQuarterNote();
        if (tpq <= 0) {
            throw std::runtime_error("Only PPQ MIDI timing is supported right now");
        }

        const int gridTicks = parseQuantizeGridTicks(options.quantizeSpec, tpq);
        const double bpm = options.bpm > 0.0 ? options.bpm : detectFirstTempoBpm(midi);
        const int minTicks = msToTicks(options.minNoteMs, bpm, tpq);
        const int mergeTicks = msToTicks(options.mergeMs, bpm, tpq);

        Stats stats;
        auto notes = extractNotes(midi);
        stats.inputNotes = static_cast<int>(notes.size());

        quantizeNotes(notes, gridTicks, options.quantizeEnds, options.swing, stats);
        mergeAdjacentNotes(notes, mergeTicks, stats);
        dropShortNotes(notes, minTicks, stats);
        limitPolyphony(notes, options.maxPolyphony, stats);

        std::sort(notes.begin(), notes.end(), [](const Note& a, const Note& b) {
            return std::tie(a.start, a.end, a.key, a.channel) < std::tie(b.start, b.end, b.key, b.channel);
        });
        stats.outputNotes = static_cast<int>(notes.size());

        writeCleanMidi(midi, notes, options);

        std::cout << "[+] Wrote: " << options.outputPath << "\n";
        std::cout << "[+] Notes: " << stats.inputNotes << " -> " << stats.outputNotes << "\n";
        std::cout << "[+] Grid ticks: " << gridTicks << " (TPQ " << tpq << ")\n";
        std::cout << "[+] Merged: " << stats.mergedNotes
                  << ", short dropped: " << stats.shortNotesDropped
                  << ", poly dropped: " << stats.polyphonyDropped << "\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "[!] Fatal: " << e.what() << "\n";
        return 10;
    }
}
