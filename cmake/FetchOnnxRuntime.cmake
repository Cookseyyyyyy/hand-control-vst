# FetchOnnxRuntime.cmake
#
# Downloads the official Microsoft ONNX Runtime prebuilt binary for the
# current platform, extracts it, and exposes:
#
#   ONNXRUNTIME_INCLUDE_DIR        - public headers (onnxruntime_cxx_api.h)
#   ONNXRUNTIME_IMPORT_LIBRARIES   - list of files to link against (import lib on Win, dylib on macOS)
#   ONNXRUNTIME_RUNTIME_LIBRARIES  - list of runtime files (.dll / .dylib + symlinks)
#                                    that must be shipped alongside the plugin.
#   ONNXRUNTIME_VERSION            - version string.
#
# Downloads go to a persistent cache directory so iteration is fast and
# FetchContent doesn't re-download on every fresh configure.

set(ONNXRUNTIME_VERSION "1.21.0")

set(_HC_DOWNLOAD_CACHE "${CMAKE_SOURCE_DIR}/build-cache/onnxruntime-${ONNXRUNTIME_VERSION}")
file(MAKE_DIRECTORY "${_HC_DOWNLOAD_CACHE}")

if(WIN32)
    set(_HC_ORT_ARCHIVE "onnxruntime-win-x64-${ONNXRUNTIME_VERSION}.zip")
    set(_HC_ORT_SHA256  "5C07BB2805CD666DDA75FA9BFA60E75F2F90D478B952298DD9D55C00740D81BF")
    set(_HC_ORT_DIRNAME "onnxruntime-win-x64-${ONNXRUNTIME_VERSION}")
elseif(APPLE)
    set(_HC_ORT_ARCHIVE "onnxruntime-osx-universal2-${ONNXRUNTIME_VERSION}.tgz")
    set(_HC_ORT_SHA256  "3C3CFC71E538E592192C14D9BAD88EC5D5D8D5D698EBC2C6B3119BE8C90B4670")
    set(_HC_ORT_DIRNAME "onnxruntime-osx-universal2-${ONNXRUNTIME_VERSION}")
else()
    message(FATAL_ERROR "Unsupported platform for ONNX Runtime prebuilt binaries. "
                        "Only Windows x64 and macOS universal2 are wired up here.")
endif()

set(_HC_ORT_ARCHIVE_PATH "${_HC_DOWNLOAD_CACHE}/${_HC_ORT_ARCHIVE}")
set(_HC_ORT_EXTRACT_DIR  "${_HC_DOWNLOAD_CACHE}/extracted")
set(_HC_ORT_ROOT         "${_HC_ORT_EXTRACT_DIR}/${_HC_ORT_DIRNAME}")
set(_HC_ORT_URL "https://github.com/microsoft/onnxruntime/releases/download/v${ONNXRUNTIME_VERSION}/${_HC_ORT_ARCHIVE}")

if(NOT EXISTS "${_HC_ORT_ROOT}/include/onnxruntime_cxx_api.h")
    if(NOT EXISTS "${_HC_ORT_ARCHIVE_PATH}")
        message(STATUS "Downloading ONNX Runtime ${ONNXRUNTIME_VERSION} (${_HC_ORT_ARCHIVE})...")
        file(DOWNLOAD
            "${_HC_ORT_URL}"
            "${_HC_ORT_ARCHIVE_PATH}"
            EXPECTED_HASH SHA256=${_HC_ORT_SHA256}
            SHOW_PROGRESS
            STATUS _hc_dl_status
            TLS_VERIFY ON)
        list(GET _hc_dl_status 0 _hc_dl_code)
        if(NOT _hc_dl_code EQUAL 0)
            file(REMOVE "${_HC_ORT_ARCHIVE_PATH}")
            message(FATAL_ERROR "Failed to download ONNX Runtime: ${_hc_dl_status}")
        endif()
    endif()

    message(STATUS "Extracting ONNX Runtime to ${_HC_ORT_EXTRACT_DIR}")
    file(MAKE_DIRECTORY "${_HC_ORT_EXTRACT_DIR}")
    file(ARCHIVE_EXTRACT
        INPUT       "${_HC_ORT_ARCHIVE_PATH}"
        DESTINATION "${_HC_ORT_EXTRACT_DIR}")
endif()

if(NOT EXISTS "${_HC_ORT_ROOT}/include/onnxruntime_cxx_api.h")
    message(FATAL_ERROR "ONNX Runtime extracted but header not found at "
                        "${_HC_ORT_ROOT}/include/onnxruntime_cxx_api.h")
endif()

set(ONNXRUNTIME_INCLUDE_DIR "${_HC_ORT_ROOT}/include")

if(WIN32)
    set(ONNXRUNTIME_IMPORT_LIBRARIES "${_HC_ORT_ROOT}/lib/onnxruntime.lib")
    set(ONNXRUNTIME_RUNTIME_LIBRARIES "${_HC_ORT_ROOT}/lib/onnxruntime.dll")
elseif(APPLE)
    set(ONNXRUNTIME_IMPORT_LIBRARIES "${_HC_ORT_ROOT}/lib/libonnxruntime.dylib")
    set(ONNXRUNTIME_RUNTIME_LIBRARIES
        "${_HC_ORT_ROOT}/lib/libonnxruntime.${ONNXRUNTIME_VERSION}.dylib"
        "${_HC_ORT_ROOT}/lib/libonnxruntime.dylib")
endif()

message(STATUS "ONNX Runtime: ${_HC_ORT_ROOT}")
