# Hand Control VST

A webcam-driven VST3 / AU plugin that turns hand gestures into 14
DAW-mappable parameters and 14 MIDI CC messages. Spiritual successor to
the original [OSCHandcontrol](https://github.com/) Python script, rebuilt
as a self-contained plugin that installs in one double-click and maps in
any DAW via standard MIDI Map mode.

- **Inputs**: any USB or built-in webcam.
- **Outputs (14 measurements per hand pair)**:
  - `H1_ThumbIndex_Distance`, `H1_ThumbIndex_Angle`
  - `H1_ThumbPinky_Distance`, `H1_ThumbPinky_Angle`
  - `H1_HandX`, `H1_HandY`, `H1_Openness`
  - `H2_*` (same seven for the second hand)
  - Each value also emitted as MIDI CC (default CC 20-33 on channel 1, configurable in the plugin).
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

## Mapping to plugin parameters in Ableton Live

The recommended workflow uses MIDI Map mode, which works for any plugin
on any track.

1. Drop **Hand Control** on a track.
2. Open Hand Control's UI and click the **MIDI Map** button to see (and
   optionally change) the channel + CC number assigned to each measurement.
3. Drop the target plugin on the same track or another track (e.g. Transit).
4. Press **Cmd+M** (macOS) or **Ctrl+M** (Windows) to enter Ableton's MIDI
   Map mode.
5. Click the parameter you want to control (e.g. Transit's _Mix_ knob). It
   highlights blue.
6. Move your hand so the source measurement actually changes value
   (e.g. pinch your thumb and index finger). Hand Control sends a CC,
   Ableton captures the mapping.
7. Press **Cmd/Ctrl+M** again to leave Map mode. The target parameter now
   follows your hand in real time.

### Alternate workflow using DAW automation parameters

Hand Control's measurements are also exposed as native DAW automation
parameters. To use those instead of MIDI:

1. Group both devices into an **Audio Effect Rack** (Cmd/Ctrl+G).
2. In the rack, click the wrench (Configure) icon on the rack header.
3. Click a Hand Control measurement and the target parameter to add both
   to the rack's parameter list.
4. Map both to the same Macro knob.

Note that Macro routing is one-way (Macro drives params), so you'd usually
prefer the MIDI flow for live performance.

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
