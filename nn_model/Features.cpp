#include "Features.h"
#include "ModelAssets.h"

#include <stdexcept>

Features::Features()
    : mMemoryInfo(nullptr)
    , mSession(nullptr)
{
    mMemoryInfo = Ort::MemoryInfo::CreateCpu(OrtDeviceAllocator, OrtMemTypeCPU);

    mSessionOptions.SetInterOpNumThreads(1);
    mSessionOptions.SetIntraOpNumThreads(1);

    // Load ONNX model bytes from disk (must init ModelAssets before constructing Features)
    const auto& modelBytes = nn::ModelAssets::instance().featuresModelOnnx();
    if (modelBytes.empty()) {
        throw std::runtime_error("Features: features_model.onnx is empty");
    }

    mSession = Ort::Session(mEnv,
                            modelBytes.data(),
                            modelBytes.size(),
                            mSessionOptions);
}

const float* Features::computeFeatures(float* inAudio, int inNumSamples, size_t& outNumFrames)
{
    assert(inNumSamples > 0);

    // shape: [1, N]
    mInputShape = {1, (int64_t)inNumSamples, 1};

    mInput.push_back(
        Ort::Value::CreateTensor<float>(mMemoryInfo, inAudio, (size_t)inNumSamples, mInputShape.data(), mInputShape.size()));

    mOutput = mSession.Run(mRunOptions, mInputNames, mInput.data(), 1, mOutputNames, 1);

    auto out_shape = mOutput[0].GetTensorTypeAndShapeInfo().GetShape();
    assert(out_shape[0] == 1 && out_shape[2] == NUM_FREQ_IN && out_shape[3] == NUM_HARMONICS);

    outNumFrames = static_cast<size_t>(out_shape[1]);
    mInput.clear();

    return mOutput[0].GetTensorData<float>();
}
