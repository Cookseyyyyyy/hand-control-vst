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

**Note:** these builds are **unsigned**. Remove the quarantine attribute
before loading in a DAW:

```bash
xattr -dr com.apple.quarantine "~/Library/Audio/Plug-Ins/VST3/Hand Control.vst3"
xattr -dr com.apple.quarantine "~/Library/Audio/Plug-Ins/Components/Hand Control.component"
```

### Windows

1. Download `HandControl-windows-x64.zip`.
2. Unzip. You get `Hand Control.vst3` (a folder despite the `.vst3` name).
3. Copy `Hand Control.vst3` into `C:\Program Files\Common Files\VST3\` as-is.
   It already contains the bundled ML runtime and models - nothing else to install.
4. Rescan plugins in your DAW.
5. Windows will prompt for camera permission the first time the plugin
   opens a webcam.

**Upgrading from v0.3:** the parameter set changed (now 7 single-hand
measurements instead of 14). Existing mappings to `H1_*` parameters in
v0.3 will resolve to the new equivalents (the parameter IDs kept the
`h1_` prefix), but mappings to `H2_*` will be lost since two-hand
tracking has been removed. Re-add Hand Control in any saved projects
to pick up the new defaults.

## What's new in v0.4

This release deliberately reverses two ambitious changes from v0.2-v0.3
that turned out to make tracking *less* reliable, and fixes the pipeline
to match Google's reference MediaPipe implementation more closely.

### Single-hand tracking (was: two-hand)

Two-hand tracking was a nice-sounding feature but the dual-slot
disambiguation was fundamentally hard - a single physical hand often
filled both slots, producing duplicate readings, and our v0.3 fix had
edge cases of its own. v0.4 tracks one hand at a time. If you need to
swap hands, just put the other one in front of the camera; the tracker
will pick it up.

### Inter-frame tracking confidence (matches MediaPipe)

Added an IoU-based "did the hand drift between frames?" check. After
running the landmark model on the cached ROI, we compute the bbox of
the new landmarks and compare to the previous frame's bbox. If overlap
drops below 0.5, we drop the cached track and force a fresh palm
detection. Catches drift before it compounds into the model looking at
the wrong region.

### Faster palm refresh + matched MediaPipe defaults

- Palm re-detection runs every 10 frames (was 30) - cheap and recovers
  fast if the cached ROI subtly latches onto the wrong region (face,
  background hand-like blob).
- Confidence thresholds match MediaPipe defaults: 0.5 for palm score
  and landmark presence. v0.3's hysteresis (0.55/0.4) held on with
  low-confidence landmarks too long.
- Default `Smoothing` slider is now 0 (very light per-landmark
  filtering). v0.3's 0.35 default added perceptible lag.

### Diagnostic info in the status bar

Status bar now shows: `Hand 0.92    Palm 0.81    [palm]` or `[cached]`.
You can see live whether the palm detector is being triggered each
frame and what scores both models are reporting. Useful when something
feels off.

### MIDI CC output (unchanged from v0.3)

Each of the 7 measurements emits as a MIDI CC. Defaults: CC 20-26 on
channel 1. Per-measurement channel + CC# + on/off configurable in the
**MIDI Map** panel inside the plugin.

In Ableton: drop Hand Control on any track, drop your target plugin on
the same track, press Cmd/Ctrl+M, click the target parameter, move
your hand to fire a CC, mapping captured.

## Known limitations

- Installers are not signed or notarized.
- No AAX (Pro Tools) support.
- Single hand only.
