cmake_minimum_required(VERSION 3.16)
project(nn_transcribe LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Build our deps static where the platform supports it; ONNX Runtime stays shared
# and is bundled next to the executable.
set(BUILD_SHARED_LIBS OFF)

option(NN_MOSTLY_STATIC "Static link libstdc++/libgcc; deps static; ORT shared" ON)
option(NN_FULL_STATIC "Attempt fully static link (usually painful on glibc)" OFF)

if (NN_MOSTLY_STATIC AND NOT APPLE)
  add_link_options(-static-libstdc++ -static-libgcc)
elseif (NN_MOSTLY_STATIC AND APPLE)
  message(STATUS "NN_MOSTLY_STATIC is ignored on Apple platforms")
endif()

include(FetchContent)
set(FETCHCONTENT_UPDATES_DISCONNECTED ON CACHE BOOL
    "Do not contact remotes to update already-populated FetchContent dependencies")

# --- nlohmann/json (RTNeural parseJson uses it) ---
FetchContent_Declare(
  nlohmann_json
  GIT_REPOSITORY https://github.com/nlohmann/json.git
  GIT_TAG        v3.11.3
)
FetchContent_MakeAvailable(nlohmann_json)

# --- RTNeural (header-only) ---
FetchContent_Declare(
  RTNeural
  GIT_REPOSITORY https://github.com/jatinchowdhury18/RTNeural.git
  GIT_TAG        main
  SOURCE_SUBDIR  cmake/skip-add-subdirectory
)
FetchContent_MakeAvailable(RTNeural)

# --- miniaudio (single-header; header-only fetch) ---
FetchContent_Declare(
  miniaudio
  GIT_REPOSITORY https://github.com/mackron/miniaudio.git
  GIT_TAG        master
  SOURCE_SUBDIR  cmake/skip-add-subdirectory
)
FetchContent_MakeAvailable(miniaudio)

# --- midifile (no consistent CMake; build a tiny static lib ourselves) ---
FetchContent_Declare(
  midifile_src
  GIT_REPOSITORY https://github.com/craigsapp/midifile.git
  GIT_TAG        master
  SOURCE_SUBDIR  cmake/skip-add-subdirectory
)
FetchContent_MakeAvailable(midifile_src)

add_library(midifile STATIC
  ${midifile_src_SOURCE_DIR}/src/Binasc.cpp
  ${midifile_src_SOURCE_DIR}/src/MidiEvent.cpp
  ${midifile_src_SOURCE_DIR}/src/MidiEventList.cpp
  ${midifile_src_SOURCE_DIR}/src/MidiFile.cpp
  ${midifile_src_SOURCE_DIR}/src/MidiMessage.cpp
)
target_include_directories(midifile PUBLIC ${midifile_src_SOURCE_DIR}/include)

# --- ONNX Runtime imported target ---
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(OnnxRuntime)

# --- nn_model (ported NeuralNote model code + our ModelAssets) ---
add_library(nn_model STATIC
  nn_model/ModelAssets.cpp
  nn_model/BasicPitch.cpp
  nn_model/BasicPitchCNN.cpp
  nn_model/Features.cpp
  nn_model/Notes.cpp
)

target_include_directories(nn_model PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/nn_model
)

target_include_directories(nn_model SYSTEM PUBLIC
  ${rtneural_SOURCE_DIR}
)

target_link_libraries(nn_model PUBLIC
  onnxruntime::onnxruntime
  nlohmann_json::nlohmann_json
)

# Force-disable JUCE debug audio code paths
target_compile_definitions(nn_model PUBLIC
  SAVE_DOWNSAMPLED_AUDIO=0
  RTNEURAL_DEFAULT_ALIGNMENT=16
  RTNEURAL_USE_STL=1
)

# --- nn_core (audio decode/resample + midi writer) ---
add_library(nn_core STATIC
  nn_core/src/audio.cpp
  nn_core/src/midi_writer.cpp
  nn_core/src/miniaudio_impl.cpp
)

target_include_directories(nn_core PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/nn_core/include
  ${CMAKE_CURRENT_SOURCE_DIR}/nn_model
  ${miniaudio_SOURCE_DIR}
  ${midifile_src_SOURCE_DIR}/include
)

target_link_libraries(nn_core PUBLIC
  midifile
  nlohmann_json::nlohmann_json
)

# --- app ---
add_executable(nn_transcribe apps/nn_transcribe/main.cpp)
target_link_libraries(nn_transcribe PRIVATE nn_model nn_core)

if (NN_FULL_STATIC)
  target_link_options(nn_transcribe PRIVATE -static)
endif()

# Bundle ONNX Runtime next to executable & set rpath so it finds it.
nn_bundle_onnxruntime(nn_transcribe)
