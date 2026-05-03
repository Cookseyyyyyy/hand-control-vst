#pragma once

#include "Landmarks.h"

namespace handcontrol::tracking
{
    /** Per-hand measurements published as DAW parameters. The first four match
        `OSCHandcontrol.py` exactly; the last three are new in v0.2. */
    struct HandMeasurements
    {
        bool  valid             { false };

        // Original four (ported from OSCHandcontrol.py)
        float thumbIndex01      { 0.0f };  // remap_distance(distance1 / hand_length)
        float thumbIndexAngle01 { 0.0f };  // remap_angle(degrees(atan2(|dy|,|dx|)))
        float thumbPinky01      { 0.0f };
        float thumbPinkyAngle01 { 0.0f };

        // New per-hand measurements
        float handX01           { 0.0f };  // 0 = left edge of frame, 1 = right edge
        float handY01           { 0.0f };  // 0 = top edge, 1 = bottom edge
        float openness01        { 0.0f };  // 0 = closed fist, 1 = fully spread fingers
    };

    /** Remap ratio (distance/handLength) to 0..1 per the old Python script:
            min=0.5, max=1.0, clamped. */
    float remapDistance(float ratio) noexcept;

    /** Remap angle (degrees, folded 0..90) to 0..1 per the old Python script:
            deadzone=30, maps 30..90 -> 0..1, clamped. */
    float remapAngleDeg(float degrees) noexcept;

    /** Compute the per-hand measurements from a single hand frame.
        If `hand.present` is false, returns `{valid=false}` with zeros.
        Distance-gates-angle rule from the Python script is enforced:
        if a distance remaps to 0, the matching angle is also forced to 0. */
    HandMeasurements measure(const HandFrame& hand) noexcept;
}
