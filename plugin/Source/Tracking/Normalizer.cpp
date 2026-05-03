#include "Normalizer.h"

#include <algorithm>
#include <cmath>

namespace handcontrol::tracking
{
    namespace
    {
        constexpr float distMin = 0.5f;
        constexpr float distMax = 1.0f;
        constexpr float angleDeadzoneDeg = 30.0f;
        constexpr float angleMaxDeg = 90.0f;

        // Openness remap: average (fingertip - MCP) / hand_length over the four
        // non-thumb fingers. For real MediaPipe data, a fully closed fist gives
        // ratios near 0.05 (fingertips next to their MCPs) while a flat open
        // hand reaches roughly 0.55-0.7 because hand_length itself is the
        // wrist-to-index-tip distance and is therefore quite long.
        constexpr float opennessMin = 0.10f;
        constexpr float opennessMax = 0.65f;

        float hypot2(float dx, float dy) noexcept
        {
            return std::sqrt(dx * dx + dy * dy);
        }

        constexpr float degPerRad = 57.29577951308232f;
    }

    float remapDistance(float ratio) noexcept
    {
        const float remapped = (ratio - distMin) / (distMax - distMin);
        return std::clamp(remapped, 0.0f, 1.0f);
    }

    float remapAngleDeg(float degrees) noexcept
    {
        if (degrees < angleDeadzoneDeg)
            return 0.0f;
        const float v = (degrees - angleDeadzoneDeg) / (angleMaxDeg - angleDeadzoneDeg);
        return std::min(v, 1.0f);
    }

    HandMeasurements measure(const HandFrame& hand) noexcept
    {
        HandMeasurements m;
        if (! hand.present)
            return m;

        const auto& wrist    = hand.at(Landmark::wrist);
        const auto& thumbTip = hand.at(Landmark::thumbTip);
        const auto& indexTip = hand.at(Landmark::indexTip);
        const auto& pinkyTip = hand.at(Landmark::pinkyTip);

        const float handLength = hypot2(indexTip.x - wrist.x, indexTip.y - wrist.y);
        if (handLength <= 1.0e-6f)
            return m;

        // ---- Original Python-port measurements ---------------------------------
        const float distIndex = hypot2(indexTip.x - thumbTip.x, indexTip.y - thumbTip.y);
        const float distPinky = hypot2(pinkyTip.x - thumbTip.x, pinkyTip.y - thumbTip.y);
        const float ratioIndex = distIndex / handLength;
        const float ratioPinky = distPinky / handLength;

        m.thumbIndex01 = remapDistance(ratioIndex);
        m.thumbPinky01 = remapDistance(ratioPinky);

        const float angleIndexDeg = std::atan2(std::fabs(indexTip.y - thumbTip.y),
                                               std::fabs(indexTip.x - thumbTip.x)) * degPerRad;
        const float anglePinkyDeg = std::atan2(std::fabs(pinkyTip.y - thumbTip.y),
                                               std::fabs(pinkyTip.x - thumbTip.x)) * degPerRad;
        m.thumbIndexAngle01 = remapAngleDeg(angleIndexDeg);
        m.thumbPinkyAngle01 = remapAngleDeg(anglePinkyDeg);

        if (m.thumbIndex01 == 0.0f) m.thumbIndexAngle01 = 0.0f;
        if (m.thumbPinky01 == 0.0f) m.thumbPinkyAngle01 = 0.0f;

        // ---- New per-hand measurements (v0.2) ---------------------------------
        // Hand position: centre of the hand bounding box in normalized frame coords.
        // We use the average of wrist and middle-finger MCP as a stable proxy
        // (less noisy than fingertip positions which jitter with pinches).
        const auto& middleMcp = hand.at(Landmark::middleMcp);
        const float centerX = 0.5f * (wrist.x + middleMcp.x);
        const float centerY = 0.5f * (wrist.y + middleMcp.y);
        m.handX01 = std::clamp(centerX, 0.0f, 1.0f);
        m.handY01 = std::clamp(centerY, 0.0f, 1.0f);

        // Openness: average of (fingertip - MCP) / hand_length for the four
        // non-thumb fingers. Closed fist -> small ratios, open hand -> large.
        const auto& indexMcp  = hand.at(Landmark::indexMcp);
        const auto& middleTip = hand.at(Landmark::middleTip);
        const auto& ringMcp   = hand.at(Landmark::ringMcp);
        const auto& ringTip   = hand.at(Landmark::ringTip);
        const auto& pinkyMcp  = hand.at(Landmark::pinkyMcp);
        const float curlIndex  = hypot2(indexTip.x  - indexMcp.x,  indexTip.y  - indexMcp.y);
        const float curlMiddle = hypot2(middleTip.x - middleMcp.x, middleTip.y - middleMcp.y);
        const float curlRing   = hypot2(ringTip.x   - ringMcp.x,   ringTip.y   - ringMcp.y);
        const float curlPinky  = hypot2(pinkyTip.x  - pinkyMcp.x,  pinkyTip.y  - pinkyMcp.y);
        const float avgCurl = 0.25f * (curlIndex + curlMiddle + curlRing + curlPinky) / handLength;
        const float opennessRemap = (avgCurl - opennessMin) / (opennessMax - opennessMin);
        m.openness01 = std::clamp(opennessRemap, 0.0f, 1.0f);

        m.valid = true;
        return m;
    }
}
