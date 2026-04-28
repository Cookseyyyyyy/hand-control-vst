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

    /** Result of a single tracking iteration over a camera frame.
        Up to two hands, in the order MediaPipe returned them (unstable).
        The `HandIdentityTracker` stabilises these into Hand 1 / Hand 2. */
    struct TrackingResult
    {
        double timestampSeconds { 0.0 };
        std::array<HandFrame, 2> hands {};
    };
}
