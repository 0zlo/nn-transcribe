# Creates imported target: onnxruntime::onnxruntime
# Expects:
#   third_party/onnxruntime/include/onnxruntime_cxx_api.h
#   third_party/onnxruntime/lib/libonnxruntime.so
#
# You can override with -DONNXRUNTIME_ROOT=/path/to/onnxruntime

set(ONNXRUNTIME_ROOT "${CMAKE_SOURCE_DIR}/third_party/onnxruntime" CACHE PATH "ONNX Runtime root")

function(nn_require_onnxruntime)
  if (NOT EXISTS "${ONNXRUNTIME_ROOT}/include/onnxruntime_cxx_api.h")
    message(FATAL_ERROR
      "ONNX Runtime headers not found at: ${ONNXRUNTIME_ROOT}/include\n"
      "Run: scripts/get_onnxruntime.sh\n"
      "Or set -DONNXRUNTIME_ROOT=... to your ORT directory.")
  endif()

  if (NOT EXISTS "${ONNXRUNTIME_ROOT}/lib/libonnxruntime.so")
    message(FATAL_ERROR
      "ONNX Runtime library not found at: ${ONNXRUNTIME_ROOT}/lib/libonnxruntime.so\n"
      "Run: scripts/get_onnxruntime.sh\n"
      "Or set -DONNXRUNTIME_ROOT=... to your ORT directory.")
  endif()
endfunction()

nn_require_onnxruntime()

add_library(onnxruntime::onnxruntime SHARED IMPORTED GLOBAL)
set_target_properties(onnxruntime::onnxruntime PROPERTIES
  IMPORTED_LOCATION "${ONNXRUNTIME_ROOT}/lib/libonnxruntime.so"
  INTERFACE_INCLUDE_DIRECTORIES "${ONNXRUNTIME_ROOT}/include"
)

function(nn_bundle_onnxruntime target_name)
  add_custom_command(TARGET ${target_name} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${ONNXRUNTIME_ROOT}/lib/libonnxruntime.so"
            "$<TARGET_FILE_DIR:${target_name}>/libonnxruntime.so"
  )
  set_target_properties(${target_name} PROPERTIES
    BUILD_RPATH "$ORIGIN"
    INSTALL_RPATH "$ORIGIN"
  )
endfunction()
