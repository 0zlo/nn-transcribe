#pragma once
#include <string>
#include <vector>

namespace nn {

struct AudioMono {
    int sampleRate = 0;
    std::vector<float> samples;
};

AudioMono loadAudioMonoResampled(const std::string& path, int targetSampleRate = 22050);

} // namespace nn
