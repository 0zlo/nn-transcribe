#pragma once
#include <string>
#include <vector>

#include "Notes.h"

namespace nn {

// Writes a single-track MIDI file from note events.
// bpm is used to map seconds -> ticks (PPQ default 480). Tempo event is embedded at tick 0.
void writeMidiFromEvents(const std::vector<Notes::Event>& events,
                         const std::string& outPath,
                         double bpm = 120.0,
                         int ppq = 480);

} // namespace nn
