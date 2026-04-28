## Downloads

Grab the zip for your platform below, then follow the install steps.

### macOS

1. Download `HandControl-macos-universal.zip`.
2. Unzip. You get `Hand Control.vst3` and `Hand Control.component`.
3. Drop them into:
   - `~/Library/Audio/Plug-Ins/VST3/` (per-user) or `/Library/Audio/Plug-Ins/VST3/` (system)
   - `~/Library/Audio/Plug-Ins/Components/` or `/Library/Audio/Plug-Ins/Components/`
4. Rescan plugins in your DAW.
5. The first time you open the plugin in a track, macOS will prompt for
   camera permission.

**Note:** these builds are **unsigned**. macOS will refuse to load them from
Ableton / Logic until you remove the quarantine attribute:

```bash
xattr -dr com.apple.quarantine "~/Library/Audio/Plug-Ins/VST3/Hand Control.vst3"
xattr -dr com.apple.quarantine "~/Library/Audio/Plug-Ins/Components/Hand Control.component"
```

Signed/notarized `.pkg` installers are planned for v1.0.

### Windows

1. Download `HandControl-windows-x64.zip`.
2. Unzip. You get `Hand Control.vst3` (a folder despite the `.vst3` name).
3. Copy `Hand Control.vst3` into `C:\Program Files\Common Files\VST3\` as-is.
   It already contains the bundled ML runtime and models - nothing else to install.
4. Rescan plugins in your DAW.
5. Windows will prompt for camera permission the first time the plugin
   opens a webcam.

## What's in this release

- **Real hand tracking** out of the box: MediaPipe palm detection +
  hand-landmark ONNX models run inside the plugin via ONNX Runtime.
  No downloads, no setup - everything is embedded in the plugin binary.
- **Eight mappable DAW parameters per-hand** (thumb-index + thumb-pinky
  distance and angle, up to two hands). Right-click -> Map in Ableton to
  assign any parameter to a finger pinch.
- **Math ported verbatim** from the original `OSCHandcontrol.py` so the
  feel is identical for existing users.
- **Live landmark preview** with per-hand accent colours in the plugin window.
- **One-Euro smoothing** + hand identity stabilisation so Hand 1 / Hand 2 stay
  pinned to the same physical hand across frames.

## Known limitations

- Installers are not yet signed or notarized (see install caveats above).
- No AAX (Pro Tools) support.
