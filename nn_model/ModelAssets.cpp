#include "ModelAssets.h"

#include <cstdlib>
#include <fstream>
#include <sstream>
#include <stdexcept>

namespace nn {

static constexpr const char* ENV_MODEL_DIR = "NN_MODEL_DIR";

ModelAssets& ModelAssets::instance()
{
    static ModelAssets inst;
    return inst;
}

bool ModelAssets::isInitialized() const
{
    std::lock_guard<std::mutex> lk(mMutex);
    return mInitialized;
}

std::filesystem::path ModelAssets::modelDir() const
{
    std::lock_guard<std::mutex> lk(mMutex);
    return mModelDir;
}

void ModelAssets::initFromEnvOrDefault(const std::filesystem::path& defaultDir)
{
    const char* env = std::getenv(ENV_MODEL_DIR);
    if (env && env[0] != '\0') {
        init(std::filesystem::path(env));
        return;
    }
    init(defaultDir);
}

void ModelAssets::init(const std::filesystem::path& modelDir)
{
    std::lock_guard<std::mutex> lk(mMutex);
    if (mInitialized) return;

    mModelDir = modelDir;
    loadAllLocked();
    mInitialized = true;
}

void ModelAssets::loadAllLocked()
{
    auto f = [&](const char* name) { return mModelDir / name; };

    const auto features = f("features_model.onnx");
    const auto contour  = f("cnn_contour_model.json");
    const auto note     = f("cnn_note_model.json");
    const auto onset1   = f("cnn_onset_1_model.json");
    const auto onset2   = f("cnn_onset_2_model.json");

    mFeaturesOnnx = readBytes(features);
    mCnnContour   = readText(contour);
    mCnnNote      = readText(note);
    mCnnOnset1    = readText(onset1);
    mCnnOnset2    = readText(onset2);
}

std::vector<std::uint8_t> ModelAssets::readBytes(const std::filesystem::path& p)
{
    std::ifstream in(p, std::ios::binary);
    if (!in) {
        throw std::runtime_error("ModelAssets: failed to open file: " + p.string());
    }
    in.seekg(0, std::ios::end);
    const std::streamsize size = in.tellg();
    in.seekg(0, std::ios::beg);

    std::vector<std::uint8_t> buf(static_cast<size_t>(size));
    if (size > 0) {
        in.read(reinterpret_cast<char*>(buf.data()), size);
    }
    return buf;
}

std::string ModelAssets::readText(const std::filesystem::path& p)
{
    std::ifstream in(p);
    if (!in) {
        throw std::runtime_error("ModelAssets: failed to open file: " + p.string());
    }
    std::ostringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

const std::vector<std::uint8_t>& ModelAssets::featuresModelOnnx() const
{
    std::lock_guard<std::mutex> lk(mMutex);
    if (!mInitialized) throw std::runtime_error("ModelAssets not initialized");
    return mFeaturesOnnx;
}

const std::string& ModelAssets::cnnContourJson() const
{
    std::lock_guard<std::mutex> lk(mMutex);
    if (!mInitialized) throw std::runtime_error("ModelAssets not initialized");
    return mCnnContour;
}

const std::string& ModelAssets::cnnNoteJson() const
{
    std::lock_guard<std::mutex> lk(mMutex);
    if (!mInitialized) throw std::runtime_error("ModelAssets not initialized");
    return mCnnNote;
}

const std::string& ModelAssets::cnnOnset1Json() const
{
    std::lock_guard<std::mutex> lk(mMutex);
    if (!mInitialized) throw std::runtime_error("ModelAssets not initialized");
    return mCnnOnset1;
}

const std::string& ModelAssets::cnnOnset2Json() const
{
    std::lock_guard<std::mutex> lk(mMutex);
    if (!mInitialized) throw std::runtime_error("ModelAssets not initialized");
    return mCnnOnset2;
}

} // namespace nn
