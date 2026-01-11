#include "BasicPitchCNN.h"
#include "ModelAssets.h"

#include <cassert>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

BasicPitchCNN::BasicPitchCNN()
{
    const auto& assets = nn::ModelAssets::instance();

    json json_cnn_contour = json::parse(assets.cnnContourJson());
    mCNNContour.parseJson(json_cnn_contour);

    json json_cnn_note = json::parse(assets.cnnNoteJson());
    mCNNNote.parseJson(json_cnn_note);

    json json_cnn_onset_1 = json::parse(assets.cnnOnset1Json());
    mCNNOnsetInput.parseJson(json_cnn_onset_1);

    json json_cnn_onset_2 = json::parse(assets.cnnOnset2Json());
    mCNNOnsetOutput.parseJson(json_cnn_onset_2);
}

int BasicPitchCNN::getNumFramesLookahead()
{
    return mTotalLookahead;
}

void BasicPitchCNN::reset()
{
    mCountFrames = 0;
    mContoursCircularBuffer = {};
    mNotesCircularBuffer = {};
    mConcat2CircularBuffer = {};
}

void BasicPitchCNN::_concat()
{
    // Concat:
    // [notes(t-6), notes(t-5), ..., notes(t)] + contours(t-3)
    auto idx_c = _wrapIndex(mCountFrames - mLookaheadCNNContour, mNumContourStored);
    auto idx_n = _wrapIndex(mCountFrames - (mLookaheadCNNContour + mLookaheadCNNNote), mNumNoteStored);

    // First 32*NUM_FREQ_OUT comes from stored concat2 (from onset input)
    // Last NUM_FREQ_OUT from current notes frame (stored in notes buffer)
    // mConcatArray layout: 33 x NUM_FREQ_OUT flattened
    for (int i = 0; i < 32 * NUM_FREQ_OUT; ++i) {
        // pick the right concat2 frame stored for onset
        auto idx_concat2 = _wrapIndex(mCountFrames - mLookaheadCNNOnsetInput, mNumConcat2Stored);
        mConcatArray[i] = mConcat2CircularBuffer[idx_concat2][i];
    }

    for (int i = 0; i < NUM_FREQ_OUT; ++i) {
        mConcatArray[32 * NUM_FREQ_OUT + i] = mNotesCircularBuffer[idx_n][i];
    }

    (void)idx_c; // idx_c used in main inference path when building notes/contours buffers
}

void BasicPitchCNN::frameInference(const float* inData,
                                   std::vector<float>& outContours,
                                   std::vector<float>& outNotes,
                                   std::vector<float>& outOnsets)
{
    assert(outContours.size() == NUM_FREQ_IN);
    assert(outNotes.size() == NUM_FREQ_OUT);
    assert(outOnsets.size() == NUM_FREQ_OUT);

    // Copy input frame
    for (int i = 0; i < NUM_FREQ_IN * NUM_HARMONICS; ++i) {
        mInputArray[i] = inData[i];
    }

    // Contours
    mCNNContour.forward(mInputArray.data());
    auto idx_c = _wrapIndex(mCountFrames - mLookaheadCNNContour, mNumContourStored);
    for (int i = 0; i < NUM_FREQ_IN; ++i) {
        mContoursCircularBuffer[idx_c][i] = mCNNContour.getOutputs()[i];
    }

    // Notes
    mCNNNote.forward(mContoursCircularBuffer[idx_c].data());
    auto idx_n = _wrapIndex(mCountFrames - (mLookaheadCNNContour + mLookaheadCNNNote), mNumNoteStored);
    for (int i = 0; i < NUM_FREQ_OUT; ++i) {
        mNotesCircularBuffer[idx_n][i] = mCNNNote.getOutputs()[i];
    }

    // Onset input
    mCNNOnsetInput.forward(mInputArray.data());
    auto idx_concat2 = _wrapIndex(mCountFrames - mLookaheadCNNOnsetInput, mNumConcat2Stored);
    for (int i = 0; i < 32 * NUM_FREQ_OUT; ++i) {
        mConcat2CircularBuffer[idx_concat2][i] = mCNNOnsetInput.getOutputs()[i];
    }

    // Onset output: concat + forward
    _concat();
    mCNNOnsetOutput.forward(mConcatArray.data());

    // Output current frame (time-aligned)
    for (int i = 0; i < NUM_FREQ_IN; ++i) outContours[i] = mContoursCircularBuffer[idx_c][i];
    for (int i = 0; i < NUM_FREQ_OUT; ++i) outNotes[i] = mNotesCircularBuffer[idx_n][i];
    for (int i = 0; i < NUM_FREQ_OUT; ++i) outOnsets[i] = mCNNOnsetOutput.getOutputs()[i];

    mCountFrames++;
}
