#include "nn_core/audio.h"

#include <stdexcept>
#include <cmath>
#include <algorithm>

#include "miniaudio.h"

namespace nn {

static std::vector<float> downmixToMono(const float* interleaved, ma_uint64 frames, ma_uint32 channels)
{
    std::vector<float> mono;
    mono.resize((size_t)frames);

    if (channels == 1) {
        std::copy(interleaved, interleaved + frames, mono.begin());
        return mono;
    }

    for (ma_uint64 i = 0; i < frames; ++i) {
        double acc = 0.0;
        const ma_uint64 base = i * channels;
        for (ma_uint32 c = 0; c < channels; ++c) acc += interleaved[base + c];
        mono[(size_t)i] = (float)(acc / (double)channels);
    }
    return mono;
}

AudioMono loadAudioMonoResampled(const std::string& path, int targetSampleRate)
{
    ma_decoder_config cfg = ma_decoder_config_init(ma_format_f32, 0, 0);
    ma_decoder dec;
    if (ma_decoder_init_file(path.c_str(), &cfg, &dec) != MA_SUCCESS) {
        throw std::runtime_error("miniaudio: failed to open/decode file: " + path);
    }

    ma_uint64 frameCount = 0;
    if (ma_decoder_get_length_in_pcm_frames(&dec, &frameCount) != MA_SUCCESS || frameCount == 0) {
        ma_decoder_uninit(&dec);
        throw std::runtime_error("miniaudio: failed to get length (or empty): " + path);
    }

    const ma_uint32 ch = dec.outputChannels;
    const ma_uint32 sr = dec.outputSampleRate;

    std::vector<float> interleaved;
    interleaved.resize((size_t)frameCount * (size_t)ch);

    ma_uint64 framesRead = 0;
    if (ma_decoder_read_pcm_frames(&dec, interleaved.data(), frameCount, &framesRead) != MA_SUCCESS || framesRead == 0) {
        ma_decoder_uninit(&dec);
        throw std::runtime_error("miniaudio: failed to read frames: " + path);
    }
    ma_decoder_uninit(&dec);

    auto mono = downmixToMono(interleaved.data(), framesRead, ch);

    // Resample to targetSampleRate (mono f32)
    if ((int)sr == targetSampleRate) {
        return AudioMono{ (int)sr, std::move(mono) };
    }

    ma_resampler_config rc = ma_resampler_config_init(
        ma_format_f32, 1, sr, (ma_uint32)targetSampleRate, ma_resample_algorithm_linear
    );
    ma_resampler res;
    if (ma_resampler_init(&rc, nullptr, &res) != MA_SUCCESS) {
        throw std::runtime_error("miniaudio: failed to init resampler");
    }

    // Output capacity (ceil) + pad
    const double ratio = (double)targetSampleRate / (double)sr;
    ma_uint64 inFramesRemaining = (ma_uint64)mono.size();
    ma_uint64 outCap = (ma_uint64)std::ceil((double)mono.size() * ratio) + 256;

    std::vector<float> out;
    out.resize((size_t)outCap);

    ma_uint64 outFrames = outCap;
    ma_uint64 inFrames = inFramesRemaining;

    // One-shot process (miniaudio generally consumes all if enough output space)
    if (ma_resampler_process_pcm_frames(&res, mono.data(), &inFrames, out.data(), &outFrames) != MA_SUCCESS) {
        ma_resampler_uninit(&res);
        throw std::runtime_error("miniaudio: resample failed");
    }

    ma_resampler_uninit(&res);
    out.resize((size_t)outFrames);

    return AudioMono{ targetSampleRate, std::move(out) };
}

} // namespace nn
