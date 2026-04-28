cmake_minimum_required(VERSION 3.22)

project(HandControl
    VERSION 0.1.0
    DESCRIPTION "Hand-tracking VST3/AU plugin for mapping gestures to DAW parameters"
    LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

if(APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "macOS deployment target")
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "macOS architectures")
endif()

if(MSVC)
    add_compile_options(/W4 /permissive- /utf-8 /Zc:__cplusplus)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS NOMINMAX)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

option(HANDCONTROL_BUILD_TESTS "Build unit tests" ON)
# Copy plug-in artefacts into the system plug-in folder after build. Nice for
# local dev on a workstation where you have permission; turn off in CI or
# restricted environments.
option(HANDCONTROL_COPY_PLUGIN_AFTER_BUILD "Copy built plugin into system plugin folder" ON)
if(DEFINED ENV{CI})
    set(HANDCONTROL_COPY_PLUGIN_AFTER_BUILD OFF CACHE BOOL "" FORCE)
endif()

# ML runtime + models (downloaded & cached on first configure).
include(FetchOnnxRuntime)
include(FetchHandModels)

include(FetchContent)

set(JUCE_VERSION "8.0.4" CACHE STRING "JUCE version to fetch")
message(STATUS "Fetching JUCE ${JUCE_VERSION}...")
FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        ${JUCE_VERSION}
    GIT_SHALLOW    TRUE)
FetchContent_MakeAvailable(JUCE)

add_subdirectory(plugin)

if(HANDCONTROL_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
