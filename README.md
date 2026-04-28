# Hand Control VST

A webcam-driven VST3 / AU plugin that turns pinching gestures into eight
DAW-mappable parameters. Port of the original
[OSCHandcontrol](https://github.com/) Python script, rebuilt as a self-
contained plugin so it installs in one double-click and maps in Ableton with
a right-click.

- **Inputs**: any USB or built-in webcam.
- **Outputs**: eight `AudioParameterFloat`s in `[0, 1]`:
  - `H1_ThumbIndex_Distance`, `H1_ThumbIndex_Angle`
  - `H1_ThumbPinky_Distance`, `H1_ThumbPinky_Angle`
  - `H2_*` (same four for the second hand)
- **Platforms**: macOS (Apple Silicon + Intel) and Windows x64.
- **Formats**: VST3, AU, Standalone.

## Installing (end users)

### macOS

1. Download `HandControl-x.y.z.pkg`.
2. Double-click to install. The plugin lands in
   `/Library/Audio/Plug-Ins/VST3` and `/Library/Audio/Plug-Ins/Components`.
3. Launch your DAW. The first time the plugin loads, macOS will ask for
   camera permission.

### Windows

1. Download `HandControl-x.y.z.msi`.
2. Double-click. The plugin installs to `C:\Program Files\Common Files\VST3`.
3. Rescan plugins in your DAW.

## Mapping in Ableton Live

1. Add **Hand Control** to any track (Audio Effects -> Hand Control).
2. Enter Map mode (top-right, "MIDI" / "Key" toggle has a Map equivalent,
   or right-click any control and choose _Map to..._).
3. Right-click a parameter on another device (e.g. the cutoff of an Auto
   Filter). Choose **Map to MIDI...** - actually no, for plugin parameters
   use **Configure** on the Hand Control device header, then map from Macro
   knobs, or expose the parameter into a Rack and assign a Macro.

The simplest workflow:

1. Drop Hand Control on a track.
2. Unfold the device to expose the eight measurement parameters.
3. Click the configure (wrench) icon on any other device and map one of
   its controls via an **Audio Effect Rack** Macro linked to a Hand Control
   parameter.

## Building from source

Requirements:

- CMake 3.22+
- A C++17 compiler (MSVC 19.32+, Clang 13+, Xcode 14+)
- Git (JUCE is fetched via `FetchContent`)

```bash
git clone https://github.com/YOURORG/hand-control-vst
cd hand-control-vst

cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --parallel
```

Artifacts land in:

```
build/plugin/HandControlPlugin_artefacts/Release/VST3/Hand Control.vst3
build/plugin/HandControlPlugin_artefacts/Release/AU/Hand Control.component   (macOS)
build/plugin/HandControlPlugin_artefacts/Release/Standalone/Hand Control     (test harness)
```

### Tests

```bash
cmake --build build --target HandControlTests
ctest --test-dir build --output-on-failure
```

The tests include golden-value checks that the C++ normaliser produces the
same numeric outputs as the original Python script for the same landmark
inputs - see `tests/NormalizerTests.cpp` and `docs/OSCHANDCONTROL_MATH.md`.

### How the tracker is built

Real hand tracking ships by default. On first configure CMake downloads:

- [ONNX Runtime 1.21.0](https://github.com/microsoft/onnxruntime/releases/tag/v1.21.0)
  prebuilt binaries from Microsoft (macOS universal2 or Windows x64),
- The [MediaPipe palm detection + hand landmark ONNX models](https://huggingface.co/opencv/handpose_estimation_mediapipe)
  from OpenCV Zoo (Apache 2.0).

All of the above is cached in `build-cache/` so subsequent configures are
fast. SHA256 hashes are enforced for every download.

Models are embedded into the plugin binary via `juce_add_binary_data`. The
ONNX Runtime shared library is copied into the plugin bundle at build time
and bound via a linker flag (`/DEPENDENTLOADFLAG:0x100` on Windows,
`@loader_path/../Resources` rpath on macOS), so the plugin is a single
self-contained artefact that loads cleanly in any DAW.

### Debugging with a mock tracker

If you want to iterate on UI or bridge code without a webcam, set
`HANDCONTROL_USE_MOCK_TRACKER=1` in the environment before launching the
DAW or the Standalone. The plugin will switch to a deterministic mock
tracker whose fingers oscillate predictably.

## Repository layout

```
CMakeLists.txt                 # root: pulls in JUCE, wires subprojects
plugin/
  CMakeLists.txt               # juce_add_plugin target
  Source/
    Plugin/                    # AudioProcessor + Editor
    Params/                    # Parameter IDs, layout, tracker->audio bridge
    Tracking/                  # Camera, hand tracker, math, smoothing
    UI/                        # Theme, preview, meters, status bar
tests/                         # Unit tests (minimal harness)
installer/
  macos/                       # pkgbuild + notarization driver
  windows/                     # WiX installer definition
docs/
  OSCHANDCONTROL_MATH.md       # Spec the Normalizer implements
  ML_PIPELINE.md               # How the real hand tracker works end-to-end
cmake/
  FetchOnnxRuntime.cmake       # Downloads + caches ONNX Runtime
  FetchHandModels.cmake        # Downloads + caches MediaPipe hand models
.github/workflows/build.yml    # CI: builds + tests on Mac and Windows
```

## Status

- [x] JUCE skeleton with 8 mappable params
- [x] Camera capture + preview via JUCE
- [x] Real MediaPipe-based hand tracker via ONNX Runtime
- [x] Ported OSCHandcontrol math + One-Euro smoothing
- [x] Custom dark theme + landmark overlay + meters
- [x] Installer scaffolds (WiX + pkgbuild)
- [ ] Signed + notarized release binaries
