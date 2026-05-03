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

    /** Axis-aligned IoU between two boxes, given as min/max corners. */
    float bboxIou(float ax1, float ay1, float ax2, float ay2,
                  float bx1, float by1, float bx2, float by2) noexcept;

    /** Axis-aligned bounding box of all 21 hand landmarks, used by the
        tracker for inter-frame IoU tracking confidence. */
    struct LandmarkBbox { float x1, y1, x2, y2; bool valid; };
    LandmarkBbox computeLandmarkBbox(const std::array<Point2D, numLandmarks>& lm) noexcept;
}
