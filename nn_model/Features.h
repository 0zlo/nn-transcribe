//
// Created by Damien Ronssin on 04.03.23.
// Patched: remove BinaryData; load ONNX bytes via nn::ModelAssets.
//

#ifndef Features_h
#define Features_h

#include <cassert>
#include <array>
#include <vector>
#include <onnxruntime_cxx_api.h>

#include "BasicPitchConstants.h"

/**
 * Class to compute the CQT and harmonically stack those. Output of this can be given as input to Basic Pitch cnn.
 */
class Features
{
public:
    Features();
    const float* computeFeatures(float* inAudio, int inNumSamples, size_t& outNumFrames);

private:
    std::vector<Ort::Value> mInput;
    std::vector<Ort::Value> mOutput;

    std::array<int64_t, 3> mInputShape;

    const char* mInputNames[1] = {"input_1"};
    const char* mOutputNames[1] = {"harmonic_stacking"};

    Ort::MemoryInfo mMemoryInfo;
    Ort::SessionOptions mSessionOptions;
    Ort::Env mEnv;
    Ort::Session mSession;
    Ort::RunOptions mRunOptions;
};

#endif // Features_h
