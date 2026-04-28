#include "MockHandTracker.h"

#include <cmath>

namespace handcontrol::tracking
{
    std::optional<std::string> MockHandTracker::initialise()
    {
        return std::nullopt;
    }

    TrackingResult MockHandTracker::process(const juce::Image& /*frame*/, double timestampSeconds)
    {
        TrackingResult result;
        result.timestampSeconds = timestampSeconds;

        auto& hand = result.hands[0];
        hand.present = true;
        hand.handedness = Handedness::right;
        hand.confidence = 0.95f;

        // Build a plausible "open hand" skeleton centred at (0.5, 0.55).
        const float cx = 0.5f;
        const float cy = 0.55f;
        const float scale = 0.22f;

        // Wrist.
        hand.at(Landmark::wrist) = { cx, cy + scale * 0.9f };

        // Pinch oscillation for thumb and fingertips (drives the distance parameters).
        const float t = static_cast<float>(timestampSeconds);
        const float pinchIndex = 0.5f + 0.45f * std::sin(t * 1.3f);
        const float pinchPinky = 0.5f + 0.45f * std::sin(t * 0.9f + 1.2f);
        const float rotate     = 0.35f * std::sin(t * 0.7f);

        auto radial = [&](float angleRad, float dist) -> Point2D
        {
            return { cx + std::cos(angleRad) * dist,
                     cy - std::sin(angleRad) * dist };
        };

        // Thumb chain.
        hand.at(Landmark::thumbCmc) = radial(-2.3f + rotate, scale * 0.45f);
        hand.at(Landmark::thumbMcp) = radial(-2.3f + rotate, scale * 0.75f);
        hand.at(Landmark::thumbIp)  = radial(-2.3f + rotate + 0.4f, scale * 0.95f);
        hand.at(Landmark::thumbTip) = radial(-2.3f + rotate + 0.9f, scale * (0.6f + 0.4f * (1.0f - pinchIndex)));

        // Index chain (closes toward the thumb as pinchIndex drops).
        const float indexBase = -1.2f + rotate;
        hand.at(Landmark::indexMcp) = radial(indexBase, scale * 0.7f);
        hand.at(Landmark::indexPip) = radial(indexBase, scale * 1.0f);
        hand.at(Landmark::indexDip) = radial(indexBase, scale * (1.15f * pinchIndex));
        hand.at(Landmark::indexTip) = radial(indexBase, scale * (1.3f * pinchIndex));

        // Middle.
        hand.at(Landmark::middleMcp) = radial(-0.8f + rotate, scale * 0.75f);
        hand.at(Landmark::middlePip) = radial(-0.8f + rotate, scale * 1.05f);
        hand.at(Landmark::middleDip) = radial(-0.8f + rotate, scale * 1.25f);
        hand.at(Landmark::middleTip) = radial(-0.8f + rotate, scale * 1.45f);

        // Ring.
        hand.at(Landmark::ringMcp) = radial(-0.4f + rotate, scale * 0.7f);
        hand.at(Landmark::ringPip) = radial(-0.4f + rotate, scale * 1.0f);
        hand.at(Landmark::ringDip) = radial(-0.4f + rotate, scale * 1.2f);
        hand.at(Landmark::ringTip) = radial(-0.4f + rotate, scale * 1.35f);

        // Pinky.
        const float pinkyBase = 0.0f + rotate;
        hand.at(Landmark::pinkyMcp) = radial(pinkyBase, scale * 0.65f);
        hand.at(Landmark::pinkyPip) = radial(pinkyBase, scale * 0.9f);
        hand.at(Landmark::pinkyDip) = radial(pinkyBase, scale * (1.0f * pinchPinky));
        hand.at(Landmark::pinkyTip) = radial(pinkyBase, scale * (1.15f * pinchPinky));

        return result;
    }
}
