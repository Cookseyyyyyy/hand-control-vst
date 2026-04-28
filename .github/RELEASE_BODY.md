## Downloads

Grab the zip for your platform below, then follow the install steps.

### macOS

1. Download `HandControl-macos-universal.zip`.
2. Unzip. You get `Hand Control.vst3` and `Hand Control.component`.
3. Drop them into:
   - `~/Library/Audio/Plug-Ins/VST3/` (per-user) or `/Library/Audio/Plug-Ins/VST3/` (system)
   - `~/Library/Audio/Plug-Ins/Components/` or `/Library/Audio/Plug-Ins/Components/`
4. Rescan plugins in your DAW.

**Note:** these builds are **unsigned**. macOS will refuse to load them from
Ableton / Logic until you either right-click -> Open the standalone app once,
or remove the quarantine attribute:

```
xattr -dr com.apple.quarantine "~/Library/Audio/Plug-Ins/VST3/Hand Control.vst3"
xattr -dr com.apple.quarantine "~/Library/Audio/Plug-Ins/Components/Hand Control.component"
```

Signed/notarized `.pkg` installers are planned for v1.0.

### Windows

1. Download `HandControl-windows-x64.zip`.
2. Unzip. You get `Hand Control.vst3` (a folder despite the `.vst3` name) and
   `Hand Control.exe` (a standalone test app).
3. Copy `Hand Control.vst3` into `C:\Program Files\Common Files\VST3\`.
4. Rescan plugins in your DAW.

## What works

- Eight mappable DAW parameters per-hand (thumb-index + thumb-pinky distance
  and angle, for up to two hands), math ported verbatim from the original
  `OSCHandcontrol.py`.
- In-plugin camera preview with landmark overlay.
- One-Euro filter smoothing, `HoldOnLost` behaviour, hand identity stabilisation.

## Known limitations

- Default tracker is the **mock tracker** that generates an animated synthetic
  hand, so out of the box the parameters oscillate instead of following a real
  hand. Wiring in real TFLite-based tracking is in progress - see
  `docs/MEDIAPIPE_INTEGRATION.md`.
- No AAX (Pro Tools) support.
