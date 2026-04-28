# MediaPipe / TFLite integration

Hand Control ships with a `MockHandTracker` that generates synthetic landmarks,
so the plugin loads, runs, and can be mapped in Ableton without any ML
dependencies. The real hand tracking path is behind the
`HANDCONTROL_ENABLE_TFLITE=ON` CMake flag and uses MediaPipe's published
TensorFlow Lite models.

## 1. Download the models

MediaPipe's hand models are Apache 2.0 and can be redistributed. Grab the
latest `palm_detection_full.tflite` and `hand_landmark_full.tflite` from
MediaPipe's model card pages and drop them into `resources/`:

```
resources/
├── palm_detection_full.tflite
└── hand_landmark_full.tflite
```

These files are bundled with the plugin by CMake's resource step (the current
scaffold does not yet copy them automatically; see TODO below).

## 2. Prebuild TensorFlow Lite

You need a TensorFlow Lite static or dynamic library that exposes the `c_api.h`
headers. Easiest options, by platform:

### macOS / Linux

Build TFLite via Bazel from the TensorFlow source tree, or grab a prebuilt from
the MediaPipe `mediapipe_tasks` release wheels. Lay it out as:

```
<TFLITE_ROOT>/include/tensorflow/lite/c/c_api.h
<TFLITE_ROOT>/lib/libtensorflowlite_c.dylib   (or .so)
```

### Windows

Build TFLite with CMake from the TensorFlow source
(`tensorflow/lite/c/CMakeLists.txt`), then:

```
<TFLITE_ROOT>\include\tensorflow\lite\c\c_api.h
<TFLITE_ROOT>\lib\tensorflowlite_c.dll
<TFLITE_ROOT>\lib\tensorflowlite_c.lib
```

## 3. Configure and build

```bash
cmake -S . -B build \
    -DHANDCONTROL_ENABLE_TFLITE=ON \
    -DTFLITE_ROOT=/path/to/tflite

cmake --build build --config Release
```

On Windows, `tensorflowlite_c.dll` must live next to the `.vst3` (or in the
system plugin folder), or in a folder on the DLL search path.

## 4. Fill in `TFLiteHandTracker`

`plugin/Source/Tracking/TFLiteHandTracker.cpp` currently stubs out
`initialise()` / `process()`. The proven MediaPipe pipeline is:

1. **Palm detector** on a 192x192 RGB input tensor. Output: a set of anchor
   boxes with scores; pick the highest-confidence anchor and use its box to
   crop the hand ROI.
2. **Hand landmark** on a 224x224 crop around the ROI. Output: 21 normalised
   (x, y, z) landmarks plus a handedness score and a presence score.
3. **Temporal ROI reuse**: if the landmark model reports high confidence,
   skip the palm detector next frame and reuse the ROI derived from the
   landmarks. This is what brings the tracker to real-time on CPU.

MediaPipe's reference C++ implementation (under
`mediapipe/tasks/cc/vision/hand_landmarker`) is the canonical reference;
you generally want a simplified port that uses only the two `.tflite` models
above without the full MediaPipe graph runtime.

## TODO for v1.0

- [ ] Automatic resource bundling (`juce_add_binary_data` for the `.tflite`s).
- [ ] Real `TFLiteHandTracker::initialise()` / `process()`.
- [ ] macOS CoreML delegate when available.
- [ ] Windows DirectML delegate when available.
