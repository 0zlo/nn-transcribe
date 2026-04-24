#include <iostream>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include "nn_core/audio.h"
#include "nn_core/midi_writer.h"

#include "ModelAssets.h"
#include "BasicPitch.h"

static void usage()
{
    std::cout <<
R"(nn_transcribe <input_audio> <output.mid> [options]

Options:
  --model-dir <dir>     Path to ModelData directory (default: assets/ModelData, or env NN_MODEL_DIR)
  --json <path>         Write note events JSON dump
  --settings <path>     Load JSON settings file (CLI flags override it)
  --preset <path>       Alias for --settings
  --bpm <num>           Tempo for MIDI mapping (default 120)
  --note-sens <0-1>     Higher => more notes (default 0.50)
  --split-sens <0-1>    Higher => more splits (default 0.50)
  --min-note-ms <ms>    Minimum note duration in ms (default 80)

Example:
  nn_transcribe in.wav out.mid --model-dir assets/ModelData --json out.json --bpm 140
)";
}

struct TranscribeSettings {
    double bpm = 120.0;
    float noteSens = 0.50f;
    float splitSens = 0.50f;
    float minNoteMs = 80.0f;
};

static const nlohmann::json& settingsRoot(const nlohmann::json& j)
{
    if (j.contains("transcribe") && j["transcribe"].is_object()) {
        return j["transcribe"];
    }
    return j;
}

static bool getJsonNumber(const nlohmann::json& j, const char* snakeName, const char* dashName, double& out)
{
    if (j.contains(snakeName)) {
        out = j.at(snakeName).get<double>();
        return true;
    }
    if (j.contains(dashName)) {
        out = j.at(dashName).get<double>();
        return true;
    }
    return false;
}

static void loadSettingsFile(const std::string& path, TranscribeSettings& settings)
{
    std::ifstream in(path);
    if (!in) {
        throw std::runtime_error("Failed to open settings file: " + path);
    }

    nlohmann::json j;
    in >> j;
    const auto& root = settingsRoot(j);

    double value = 0.0;
    if (getJsonNumber(root, "bpm", "bpm", value)) settings.bpm = value;
    if (getJsonNumber(root, "note_sens", "note-sens", value)) settings.noteSens = static_cast<float>(value);
    if (getJsonNumber(root, "split_sens", "split-sens", value)) settings.splitSens = static_cast<float>(value);
    if (getJsonNumber(root, "min_note_ms", "min-note-ms", value)) settings.minNoteMs = static_cast<float>(value);
}

static bool getArg(int& i, int argc, char** argv, std::string& out)
{
    if (i + 1 >= argc) return false;
    out = argv[++i];
    return true;
}

int main(int argc, char** argv)
{
    if (argc == 2 && (std::string(argv[1]) == "--help" || std::string(argv[1]) == "-h")) {
        usage();
        return 0;
    }

    if (argc < 3) {
        usage();
        return 1;
    }

    std::string inPath = argv[1];
    std::string outMidi = argv[2];

    std::string modelDir = "assets/ModelData";
    std::string outJson;
    std::string settingsPath;
    TranscribeSettings settings;

    try {
        for (int i = 3; i < argc; ++i) {
            std::string a = argv[i];
            if (a == "--settings" || a == "--preset") {
                if (!getArg(i, argc, argv, settingsPath)) {
                    std::cerr << "Missing value for " << a << "\n";
                    return 2;
                }
                loadSettingsFile(settingsPath, settings);
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "[!] Fatal: " << e.what() << "\n";
        return 10;
    }

    for (int i = 3; i < argc; ++i) {
        std::string a = argv[i];
        if (a == "--help" || a == "-h") {
            usage();
            return 0;
        } else if (a == "--model-dir") {
            if (!getArg(i, argc, argv, modelDir)) { std::cerr << "Missing value for --model-dir\n"; return 2; }
        } else if (a == "--json") {
            if (!getArg(i, argc, argv, outJson)) { std::cerr << "Missing value for --json\n"; return 2; }
        } else if (a == "--settings" || a == "--preset") {
            if (!getArg(i, argc, argv, settingsPath)) { std::cerr << "Missing value for " << a << "\n"; return 2; }
        } else if (a == "--bpm") {
            std::string v; if (!getArg(i, argc, argv, v)) { std::cerr << "Missing value for --bpm\n"; return 2; }
            settings.bpm = std::stod(v);
        } else if (a == "--note-sens") {
            std::string v; if (!getArg(i, argc, argv, v)) { std::cerr << "Missing value for --note-sens\n"; return 2; }
            settings.noteSens = std::stof(v);
        } else if (a == "--split-sens") {
            std::string v; if (!getArg(i, argc, argv, v)) { std::cerr << "Missing value for --split-sens\n"; return 2; }
            settings.splitSens = std::stof(v);
        } else if (a == "--min-note-ms") {
            std::string v; if (!getArg(i, argc, argv, v)) { std::cerr << "Missing value for --min-note-ms\n"; return 2; }
            settings.minNoteMs = std::stof(v);
        } else {
            std::cerr << "Unknown option: " << a << "\n";
            return 2;
        }
    }

    try {
        // Initialize model assets BEFORE constructing BasicPitch (it default-constructs Features/CNN internally)
        nn::ModelAssets::instance().initFromEnvOrDefault(modelDir);

        // Load and resample audio
        auto audio = nn::loadAudioMonoResampled(inPath, 22050);
        if (audio.sampleRate != 22050) {
            std::cerr << "Internal error: expected 22050 after resample\n";
            return 3;
        }

        // Transcribe
        BasicPitch bp;
        bp.reset();
        bp.setParameters(settings.noteSens, settings.splitSens, settings.minNoteMs);

        // BasicPitch expects float* (non-const)
        bp.transcribeToMIDI(audio.samples.data(), (int)audio.samples.size());
        const auto& events = bp.getNoteEvents();

        // MIDI
        nn::writeMidiFromEvents(events, outMidi, settings.bpm, 480);

        // Optional JSON
        if (!outJson.empty()) {
            nlohmann::json j;
            j["input"] = inPath;
            j["output_midi"] = outMidi;
            j["settings_path"] = settingsPath;
            j["settings"] = {
                {"bpm", settings.bpm},
                {"note_sens", settings.noteSens},
                {"split_sens", settings.splitSens},
                {"min_note_ms", settings.minNoteMs}
            };
            j["bpm"] = settings.bpm;
            j["events"] = nlohmann::json::array();

            for (const auto& e : events) {
                nlohmann::json je;
                je["start_time"] = e.startTime;
                je["end_time"] = e.endTime;
                je["pitch_midi"] = e.pitch;
                je["amplitude"] = e.amplitude;
                je["start_frame"] = e.startFrame;
                je["end_frame"] = e.endFrame;
                je["bends"] = e.bends; // can be big; keep it if you want future pitch-bend MIDI
                j["events"].push_back(std::move(je));
            }

            std::ofstream out(outJson);
            out << j.dump(2);
        }

        std::cout << "[+] Wrote: " << outMidi << "\n";
        if (!outJson.empty()) std::cout << "[+] Wrote: " << outJson << "\n";
        std::cout << "[+] Notes: " << events.size() << "\n";
        return 0;

    } catch (const std::exception& e) {
        std::cerr << "[!] Fatal: " << e.what() << "\n";
        return 10;
    }
}
