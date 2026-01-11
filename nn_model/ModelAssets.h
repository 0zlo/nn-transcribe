#pragma once

#include <cstdint>
#include <filesystem>
#include <mutex>
#include <string>
#include <vector>

namespace nn {

class ModelAssets {
public:
    static ModelAssets& instance();

    // Must be called before constructing Features/BasicPitchCNN/BasicPitch.
    void init(const std::filesystem::path& modelDir);

    // Convenience: if NN_MODEL_DIR env exists, uses it; otherwise uses defaultDir.
    void initFromEnvOrDefault(const std::filesystem::path& defaultDir);

    bool isInitialized() const;

    const std::vector<std::uint8_t>& featuresModelOnnx() const;
    const std::string& cnnContourJson() const;
    const std::string& cnnNoteJson() const;
    const std::string& cnnOnset1Json() const;
    const std::string& cnnOnset2Json() const;

    std::filesystem::path modelDir() const;

private:
    ModelAssets() = default;
    void loadAllLocked();

    static std::vector<std::uint8_t> readBytes(const std::filesystem::path& p);
    static std::string readText(const std::filesystem::path& p);

    mutable std::mutex mMutex;
    bool mInitialized {false};
    std::filesystem::path mModelDir;

    std::vector<std::uint8_t> mFeaturesOnnx;
    std::string mCnnContour;
    std::string mCnnNote;
    std::string mCnnOnset1;
    std::string mCnnOnset2;
};

} // namespace nn
