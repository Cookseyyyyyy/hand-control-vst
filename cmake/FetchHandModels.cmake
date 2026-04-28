# FetchHandModels.cmake
#
# Downloads the MediaPipe palm detection + hand landmark ONNX models from
# OpenCV Zoo (Apache 2.0). Exposes:
#
#   HANDCONTROL_PALM_MODEL      - absolute path to palm detection ONNX
#   HANDCONTROL_HANDPOSE_MODEL  - absolute path to hand landmark ONNX
#
# Both files are expected to be embedded into the plugin binary via
# juce_add_binary_data so end users never need to download anything.

set(_HC_MODELS_VERSION "2023feb")
set(_HC_MODEL_CACHE "${CMAKE_SOURCE_DIR}/build-cache/models-${_HC_MODELS_VERSION}")
file(MAKE_DIRECTORY "${_HC_MODEL_CACHE}")

function(_handcontrol_download_model name url sha256)
    set(dest "${_HC_MODEL_CACHE}/${name}")
    if(NOT EXISTS "${dest}")
        message(STATUS "Downloading ${name}...")
        file(DOWNLOAD
            "${url}"
            "${dest}"
            EXPECTED_HASH SHA256=${sha256}
            SHOW_PROGRESS
            STATUS _dl_status
            TLS_VERIFY ON)
        list(GET _dl_status 0 _dl_code)
        if(NOT _dl_code EQUAL 0)
            file(REMOVE "${dest}")
            message(FATAL_ERROR "Failed to download ${name}: ${_dl_status}")
        endif()
    endif()
endfunction()

_handcontrol_download_model(
    "palm_detection_mediapipe_${_HC_MODELS_VERSION}.onnx"
    "https://huggingface.co/opencv/palm_detection_mediapipe/resolve/main/palm_detection_mediapipe_${_HC_MODELS_VERSION}.onnx?download=true"
    "78FF51C38496B7FC8B8EBDB6CC8C1ABB02FA6C38427C6848254CDABA57FCCE7C")

_handcontrol_download_model(
    "handpose_estimation_mediapipe_${_HC_MODELS_VERSION}.onnx"
    "https://huggingface.co/opencv/handpose_estimation_mediapipe/resolve/main/handpose_estimation_mediapipe_${_HC_MODELS_VERSION}.onnx?download=true"
    "DB0898AE717B76B075D9BF563AF315B29562E11F8DF5027A1EF07B02BEF6D81C")

set(HANDCONTROL_PALM_MODEL     "${_HC_MODEL_CACHE}/palm_detection_mediapipe_${_HC_MODELS_VERSION}.onnx")
set(HANDCONTROL_HANDPOSE_MODEL "${_HC_MODEL_CACHE}/handpose_estimation_mediapipe_${_HC_MODELS_VERSION}.onnx")

message(STATUS "Hand models: ${_HC_MODEL_CACHE}")
