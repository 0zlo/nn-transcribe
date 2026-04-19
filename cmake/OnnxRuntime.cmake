# Creates imported target: onnxruntime::onnxruntime
# Expects:
#   third_party/onnxruntime/include/onnxruntime_cxx_api.h
#   third_party/onnxruntime/lib/libonnxruntime.(so|dylib)
#
# You can override with -DONNXRUNTIME_ROOT=/path/to/onnxruntime

set(ONNXRUNTIME_ROOT "${CMAKE_SOURCE_DIR}/third_party/onnxruntime" CACHE PATH "ONNX Runtime root")
set(ONNXRUNTIME_LIBRARY
    "${ONNXRUNTIME_ROOT}/lib/${CMAKE_SHARED_LIBRARY_PREFIX}onnxruntime${CMAKE_SHARED_LIBRARY_SUFFIX}"
    CACHE FILEPATH "ONNX Runtime shared library")

function(nn_require_onnxruntime)
  if (NOT EXISTS "${ONNXRUNTIME_ROOT}/include/onnxruntime_cxx_api.h")
    message(FATAL_ERROR
      "ONNX Runtime headers not found at: ${ONNXRUNTIME_ROOT}/include\n"
      "Run: scripts/get_onnxruntime.sh\n"
      "Or set -DONNXRUNTIME_ROOT=... to your ORT directory.")
  endif()

  if (NOT EXISTS "${ONNXRUNTIME_LIBRARY}")
    message(FATAL_ERROR
      "ONNX Runtime library not found at: ${ONNXRUNTIME_LIBRARY}\n"
      "Run: scripts/get_onnxruntime.sh\n"
      "Or set -DONNXRUNTIME_ROOT=... and -DONNXRUNTIME_LIBRARY=... to your ORT directory.")
  endif()
endfunction()

nn_require_onnxruntime()

add_library(onnxruntime::onnxruntime SHARED IMPORTED GLOBAL)
set_target_properties(onnxruntime::onnxruntime PROPERTIES
  IMPORTED_LOCATION "${ONNXRUNTIME_LIBRARY}"
  INTERFACE_INCLUDE_DIRECTORIES "${ONNXRUNTIME_ROOT}/include"
)

function(nn_bundle_onnxruntime target_name)
  file(GLOB NN_ORT_RUNTIME_LIBS
       "${ONNXRUNTIME_ROOT}/lib/${CMAKE_SHARED_LIBRARY_PREFIX}onnxruntime*${CMAKE_SHARED_LIBRARY_SUFFIX}*")

  foreach(NN_ORT_RUNTIME_LIB IN LISTS NN_ORT_RUNTIME_LIBS)
    get_filename_component(NN_ORT_RUNTIME_LIB_NAME "${NN_ORT_RUNTIME_LIB}" NAME)
    add_custom_command(TARGET ${target_name} POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different
              "${NN_ORT_RUNTIME_LIB}"
              "$<TARGET_FILE_DIR:${target_name}>/${NN_ORT_RUNTIME_LIB_NAME}"
    )
  endforeach()

  if (APPLE)
    set(NN_ORT_RPATH "@loader_path")
  else()
    set(NN_ORT_RPATH "$ORIGIN")
  endif()

  set_target_properties(${target_name} PROPERTIES
    BUILD_RPATH "${NN_ORT_RPATH}"
    INSTALL_RPATH "${NN_ORT_RPATH}"
  )
endfunction()
