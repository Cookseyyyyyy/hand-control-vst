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

        // Distance-gates-angle rule from the Python script.
        if (m.thumbIndex01 == 0.0f) m.thumbIndexAngle01 = 0.0f;
        if (m.thumbPinky01 == 0.0f) m.thumbPinkyAngle01 = 0.0f;

        m.valid = true;
        return m;
    }
}
