#pragma once

#include "Landmarks.h"

namespace handcontrol::tracking
{
    /** Compute an ROI for the next landmark inference from the current frame's
        21 landmarks (in pixel coordinates of the camera frame).

        Matches MediaPipe's mp_handpose.py post-processing:
          - Orient along wrist (lm[0]) -> middle MCP (lm[9]).
          - Take oriented bbox enclosing all 21 landmarks.
          - Shift along rotated y by HAND_BOX_SHIFT_Y * bbox height.
          - Enlarge by HAND_BOX_ENLARGE.

        Returned ROI is in the same coordinate space as the input landmarks
        (typically camera-frame pixels). */
    RoiTransform roiFromLandmarks(const std::array<Point2D, numLandmarks>& lm) noexcept;

    inline constexpr float kHandBoxShiftY  = -0.1f;
    inline constexpr float kHandBoxEnlarge = 1.65f;

    /** Returns true if the point (cx, cy) falls inside the oriented ROI.
        Used by the tracker to decide whether a fresh palm detection refers
        to a hand that's already being tracked, robust to size / rotation
        differences between palm-derived and landmark-derived ROIs. */
    bool pointInsideRoi(float cx, float cy, const RoiTransform& roi) noexcept;
}
