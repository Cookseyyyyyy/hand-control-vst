#pragma once

#include <array>
#include <cstdint>

namespace handcontrol::tracking
{
    /** MediaPipe Hand Landmarker indices (21 landmarks per hand). */
    enum class Landmark : int
    {
        wrist = 0,
        thumbCmc = 1, thumbMcp = 2, thumbIp = 3, thumbTip = 4,
        indexMcp = 5, indexPip = 6, indexDip = 7, indexTip = 8,
        middleMcp = 9, middlePip = 10, middleDip = 11, middleTip = 12,
        ringMcp = 13, ringPip = 14, ringDip = 15, ringTip = 16,
        pinkyMcp = 17, pinkyPip = 18, pinkyDip = 19, pinkyTip = 20
    };

    inline constexpr int numLandmarks = 21;

    struct Point2D
    {
        float x { 0.0f };
        float y { 0.0f };
    };

    /** Oriented region of interest in the camera frame, used to crop a hand
        sub-image for the landmark model. */
    struct RoiTransform
    {
        float centerX     { 0.0f };  // ROI centre in original image pixels
        float centerY     { 0.0f };
        float size        { 0.0f };  // square side length in original pixels
        float rotationRad { 0.0f };
    };

    enum class Handedness : uint8_t { unknown, left, right };

    struct HandFrame
    {
        bool present { false };
        Handedness handedness { Handedness::unknown };
        float confidence { 0.0f };
        std::array<Point2D, numLandmarks> landmarks {};

        const Point2D& at(Landmark lm) const noexcept
        {
            return landmarks[static_cast<size_t>(lm)];
        }
        Point2D& at(Landmark lm) noexcept
        {
            return landmarks[static_cast<size_t>(lm)];
        }
    };

    /** Optional per-frame diagnostics emitted by the tracker so the UI can
        show what the model is doing internally. None of these affect the
        published parameter values; they are pure observability.

        v0.4 reduced tracking to one hand only; the size-2 arrays here are
        kept for transitional simplicity but only index 0 is populated. */
    struct TrackerDiagnostics
    {
        float lastPalmScore { 0.0f };           // best palm-detector score this frame
        bool  palmRanThisFrame { false };       // true if the palm detector ran
        std::array<RoiTransform, 2> activeRois {};
        std::array<bool, 2>         roiActive  { false, false };
    };

    /** Result of a single tracking iteration over a camera frame.

        v0.4: single hand only. `hands[1]` is unused but the array stays
        size 2 so that downstream code (UI overlay, etc.) doesn't need to
        be rewritten if multi-hand returns. */
    struct TrackingResult
    {
        double timestampSeconds { 0.0 };
        std::array<HandFrame, 2> hands {};
        TrackerDiagnostics diagnostics {};
    };
}
