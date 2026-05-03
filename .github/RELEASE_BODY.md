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

**Upgrading from v0.2:** v0.3 changes the plugin's I/O capability (it now
produces MIDI). After installing, **remove and re-add Hand Control** in any
project that previously used v0.2, otherwise Ableton may refuse to load the
new version.

If Ableton has cached a previous failed Hand Control scan, also delete its
`Hand Control` rows from `%LOCALAPPDATA%\Ableton\Live Database\Live-plugins-1.db`
(Ableton must be closed) before rescanning.

## What's new in v0.3 (vs v0.2)

### Bug fix: same hand assigned to both H1 and H2

When a single physical hand was visible, v0.2 sometimes filled both tracking
slots with that one hand, so H1 and H2 showed identical readings and the
meter grid looked "doubled". The root cause was an axis-aligned IoU check
between palm-derived and landmark-derived ROIs of different sizes: even for
the same hand, IoU often fell below the overlap threshold and the second
slot was incorrectly accepted. v0.3 replaces it with a centre-inside-ROI
test that is robust to size and rotation differences.

### MIDI CC output (the big new feature)

Each of the 14 measurements is now also emitted as a MIDI CC message.

- **Defaults**: CC 20 through CC 33 on channel 1, all enabled.
- **Configurable**: open the new **MIDI Map** panel in the plugin UI to
  change the channel and CC number for any measurement, or disable
  individual rows.
- **Hysteresis**: only sends a new CC when the 7-bit value actually
  changes. Slowly drifting measurements don't spam the buffer.
- **Persisted**: MIDI mappings save with your project.

### Universal mapping flow

In Ableton (or any DAW with a MIDI Map mode):

1. Drop Hand Control on a track.
2. Drop the target plugin on the same track.
3. Press Cmd/Ctrl+M to enter MIDI Map mode.
4. Click the target parameter.
5. Move your hand so the corresponding CC fires.
6. Mapping captured.

## Known limitations

- Installers are not yet signed or notarized (see install caveats above).
- No AAX (Pro Tools) support.
