#include "nn_core/midi_writer.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>

#include "MidiFile.h"

namespace nn {

static int clampi(int v, int lo, int hi) { return std::max(lo, std::min(hi, v)); }

void writeMidiFromEvents(const std::vector<Notes::Event>& events,
                         const std::string& outPath,
                         double bpm,
                         int ppq)
{
    if (bpm <= 0.0) throw std::runtime_error("MIDI: bpm must be > 0");
    if (ppq <= 0) throw std::runtime_error("MIDI: ppq must be > 0");

    smf::MidiFile midi;
    midi.absoluteTicks();
    midi.setTicksPerQuarterNote(ppq);

    const int track = 0;
    const int channel = 0;

    // Tempo meta-event at tick 0
    midi.addTempo(track, 0, bpm);

    // Sort by start time for stable output
    std::vector<Notes::Event> sorted = events;
    std::sort(sorted.begin(), sorted.end(), [](const Notes::Event& a, const Notes::Event& b) {
        if (a.startTime != b.startTime) return a.startTime < b.startTime;
        return a.pitch < b.pitch;
    });

    auto secToTick = [&](double sec) -> int {
        // tick = seconds * (ppq * bpm / 60)
        double t = sec * ((double)ppq * bpm / 60.0);
        return (int)std::llround(t);
    };

    for (const auto& e : sorted) {
        int key = clampi(e.pitch, 0, 127);

        int startTick = secToTick(e.startTime);
        int endTick   = secToTick(e.endTime);
        if (endTick <= startTick) endTick = startTick + 1;

        int vel = clampi((int)std::lround(e.amplitude * 127.0), 1, 127);

        midi.addNoteOn(track, startTick, channel, key, vel);
        midi.addNoteOff(track, endTick, channel, key);
    }

    midi.sortTracks();
    if (!midi.write(outPath)) {
        throw std::runtime_error("MIDI: failed to write file: " + outPath);
    }
}

} // namespace nn
