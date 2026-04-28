# ML pipeline

The plugin tracks hands with two ONNX models running through
[ONNX Runtime](https://onnxruntime.ai/). Everything is embedded in the
plugin binary; end users install nothing.

## Models

Both are provided by the OpenCV Zoo
(<https://huggingface.co/opencv>) and are Apache 2.0 licensed:

| Model | File | Input | Output |
|-------|------|-------|--------|
| Palm detector | `palm_detection_mediapipe_2023feb.onnx` | 192x192 RGB, `[0,1]` | 2016 anchor boxes + 7 palm landmarks per anchor + scores |
| Hand landmark | `handpose_estimation_mediapipe_2023feb.onnx` | 224x224 RGB, `[0,1]` | 21 landmarks, confidence, handedness |

Both are originally converted from MediaPipe's TFLite models. We download
them at configure time via `cmake/FetchHandModels.cmake`, verify SHA256
hashes, and embed them into the plugin via `juce_add_binary_data` so they
ship as part of the `.vst3` file itself.

## Runtime

[ONNX Runtime 1.21.0](https://github.com/microsoft/onnxruntime/releases/tag/v1.21.0)
is downloaded as a prebuilt binary from Microsoft's GitHub releases by
`cmake/FetchOnnxRuntime.cmake`. We ship the shared library inside the
plugin bundle:

- **Windows:** `Hand Control.vst3/Contents/x86_64-win/onnxruntime.dll`
  The plugin binary is linked with `/DEPENDENTLOADFLAG:0x100`
  (`LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR`) so the Windows loader finds the
  sibling DLL regardless of which process loads the plugin.

- **macOS:** `Hand Control.vst3/Contents/Resources/libonnxruntime.*.dylib`
  The plugin binary has install rpath `@loader_path/../Resources` so the
  runtime is found at load time.

## Pipeline (`OnnxHandTracker`)

See [`plugin/Source/Tracking/OnnxHandTracker.cpp`](../plugin/Source/Tracking/OnnxHandTracker.cpp).

1. **Camera frame** arrives from `CameraSource` at ~30 fps, RGB.
2. **Letterbox** to 192x192, normalise `[0,1]`, feed the palm detector.
3. **Decode** 2016 candidate boxes via hardcoded anchor grid
   (`PalmAnchors.cpp` regenerates the exact MediaPipe anchor table
   procedurally), apply sigmoid to scores, threshold at 0.5.
4. **NMS** with IoU threshold 0.3, keep top two detections (up to 2 hands).
5. For each detection:
   - Compute ROI: orient palm vertically using palm landmarks 0 (wrist) and
     2 (middle knuckle), enlarge ~2.6x around the palm bbox.
   - Crop-rotate-resize the original frame into a 224x224 tensor.
   - Run the hand landmark model, threshold confidence at 0.55.
   - Transform the 21 landmarks back to original-frame normalised coords
     via the inverse ROI transform.
6. Pass results to `HandIdentityTracker` which stabilises slot assignment.
7. `Normalizer` (unchanged from the Python port) computes the four per-hand
   measurement values from landmarks 0, 4, 8, 20.
8. `OneEuroFilter` smooths them and writes to `ParameterBridge` for the
   audio thread to publish as DAW automation.

## Licensing

- ONNX Runtime: MIT license (Microsoft).
- MediaPipe palm / hand landmark models: Apache 2.0 (via OpenCV Zoo).
- Both are redistributable, which is why we can bundle them with the plugin.
