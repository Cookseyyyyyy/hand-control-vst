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

If you previously installed v0.1 and Ableton blacklisted it, also delete the
`Hand Control` row from `%LOCALAPPDATA%\Ableton\Live Database\Live-plugins-1.db`
(Ableton must be closed) before rescanning.

## What's new in v0.2 (vs v0.1)

### Tracking quality
- **MediaPipe-style ROI reuse**: the landmark model now keeps tracking once
  it has a lock; the palm detector only runs to find new hands. Fixes the
  "drops in and out" behaviour from v0.1.
- **Confidence hysteresis** (enter 0.55, exit 0.4): single-frame conf dips
  no longer kill the hand.
- **Per-landmark One-Euro smoothing** (42 filters per slot) instead of
  per-output: jitter is removed at the source so derived measurements
  (especially angles) get dramatically smoother.

### New mappable parameters (additive, originals unchanged)
- `H1_HandX` / `H1_HandY`: hand centre position 0..1 in frame
- `H1_Openness`: closed fist = 0, fully spread = 1
- `H2_*` equivalents
- Total mappable params: **14** (was 8). Existing v0.1 mappings still work.

### UX
- **Mirror Camera** toggle (default on): webcam users see themselves
  mirror-image, as expected.
- **Status bar diagnostics**: per-slot landmark confidence and last palm
  detector score live, plus `Hand 1 lost (1.4s)` when a slot drops, so
  you can see exactly why a meter has frozen.
- **Show ROI** toggle: draws the oriented region the model is looking at
  for each tracked hand. Useful debugging when tracking misbehaves.

### Plugin loading fix
- Replaced delay-loaded ONNX Runtime with manual runtime resolution, which
  removes onnxruntime.dll from the import table entirely. Fixes the
  "scanner rejects the plugin" issue some users hit on v0.1.

## Known limitations

- Installers are not yet signed or notarized (see install caveats above).
- No AAX (Pro Tools) support.
