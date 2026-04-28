# Porting the `OSCHandcontrol.py` math

The new VST preserves the exact measurement formulas from the original Python
script (`d:/Git/OSCHandcontrol/oschandcontrol.py`). This document is the spec
that `plugin/Source/Tracking/Normalizer.{h,cpp}` implements, and it is what
`tests/NormalizerTests.cpp` verifies.

## Landmarks

MediaPipe Hand Landmarker indices, using the Python script's names:

| Python                                | Index |
|---------------------------------------|-------|
| `HandLandmark.WRIST`                  | 0     |
| `HandLandmark.THUMB_TIP`              | 4     |
| `HandLandmark.INDEX_FINGER_TIP`       | 8     |
| `HandLandmark.PINKY_TIP`              | 20    |

All coordinates are MediaPipe's normalised frame coordinates in `[0, 1]`. The
C++ port consumes the same units, so no rescaling is needed.

## Distance measurement

```python
hand_length       = hypot(index.x - wrist.x, index.y - wrist.y)
distance_index    = hypot(index.x - thumb.x, index.y - thumb.y)
distance_pinky    = hypot(pinky.x - thumb.x, pinky.y - thumb.y)
ratio_index       = distance_index / hand_length
ratio_pinky       = distance_pinky / hand_length
```

Notes:

- `hand_length` uses the **wrist-to-index-tip** distance (not wrist to middle
  knuckle). This is stylistic to the old script and is preserved verbatim.
  Using the index tip means `hand_length` shrinks slightly as the index curls
  inward, which is part of the "feel" existing users have calibrated to.
- The C++ port guards against `hand_length == 0` (degenerate landmarks) and
  returns an invalid measurement in that case.

## Distance remap to `[0, 1]`

```python
def remap_distance(d):
    return max(0, min((d - 0.5) / 0.5, 1))
```

Equivalent to: treat `0.5` as the minimum of the visible range and `1.0` as
the maximum, linearly interpolate, then clamp. Any ratio at or below 0.5 maps
to 0; any ratio at or above 1.0 maps to 1.

## Angle measurement

```python
dx, dy = tip.x - thumb.x, tip.y - thumb.y
angle_deg = degrees(atan2(abs(dy), abs(dx)))  # folded to [0, 90]
```

Taking the absolute values of `dx` and `dy` before `atan2` folds the angle
into the first quadrant, so hand orientation does not matter (a vector at
`+45` and `-135` degrees both produce `45` degrees).

## Angle remap to `[0, 1]`

```python
def remap_angle(a):
    if a < 30:
        return 0
    return min((a - 30) / 60, 1)
```

30 deg deadzone below which the output is exactly 0; everything from 30 to 90
maps linearly to `[0, 1]`.

## Distance-gates-angle rule

If a distance parameter remaps to 0, the matching angle parameter is forced
to 0 too. This comes from the Python script's explicit post-process:

```python
if remapped_distance1 == 0:
    max_normalized_angle1 = 0
```

The rationale: when the fingers are too close together to register a
meaningful distance, the direction vector is also meaningless, so silencing
the angle avoids jumpy values that would otherwise follow tiny noise.

## What the new VST changes

- **Per-hand output**: the Python script output only one set of four values,
  derived from whichever hand had the largest `relative_distance1`. The
  plugin outputs per-hand values for up to two hands, stabilised by
  `HandIdentityTracker` so slot assignments don't flicker.
- **Smoothing**: added post-normalization via `OneEuroFilter` with a
  user-controllable cutoff (`Smoothing` parameter).
- **Hold-on-lost**: when a hand disappears, the Python script snapped its
  outputs to 0. The plugin lets the user choose via the `HoldOnLost`
  parameter (default: hold, which is nicer for live performance).
- **Transport**: OSC is replaced with native DAW parameters. Addresses
  `/hand/distance1`, `/hand/rotation1`, `/hand/distance2`, `/hand/rotation2`
  no longer exist.
