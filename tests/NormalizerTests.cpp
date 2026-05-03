// Golden-value tests: inputs and expected outputs were computed by running the
// exact formulas from `d:/Git/OSCHandcontrol/oschandcontrol.py` on the same
// landmark tuples, to guarantee the C++ port matches the old Python script.

#include "TestMain.h"

#include "Tracking/Normalizer.h"

using namespace handcontrol::tracking;

namespace
{
    HandFrame makeHand(Point2D wrist, Point2D thumbTip, Point2D indexTip, Point2D pinkyTip)
    {
        HandFrame h;
        h.present = true;
        h.at(Landmark::wrist)    = wrist;
        h.at(Landmark::thumbTip) = thumbTip;
        h.at(Landmark::indexTip) = indexTip;
        h.at(Landmark::pinkyTip) = pinkyTip;
        return h;
    }
}

HC_TEST(remapDistance_matches_python)
{
    // Python: max(0, min((d - 0.5) / 0.5, 1))
    HC_CHECK_NEAR(remapDistance(0.0f),  0.0f, 1.0e-6f);
    HC_CHECK_NEAR(remapDistance(0.5f),  0.0f, 1.0e-6f);
    HC_CHECK_NEAR(remapDistance(0.75f), 0.5f, 1.0e-6f);
    HC_CHECK_NEAR(remapDistance(1.0f),  1.0f, 1.0e-6f);
    HC_CHECK_NEAR(remapDistance(1.5f),  1.0f, 1.0e-6f);  // clamped
    HC_CHECK_NEAR(remapDistance(-0.5f), 0.0f, 1.0e-6f);  // clamped
}

HC_TEST(remapAngleDeg_matches_python)
{
    // Python: if a < 30: 0 else min((a - 30) / 60, 1)
    HC_CHECK_NEAR(remapAngleDeg(0.0f),    0.0f, 1.0e-6f);
    HC_CHECK_NEAR(remapAngleDeg(29.99f),  0.0f, 1.0e-6f);  // deadzone
    HC_CHECK_NEAR(remapAngleDeg(30.0f),   0.0f, 1.0e-6f);  // boundary: (0)/60 = 0
    HC_CHECK_NEAR(remapAngleDeg(60.0f),   0.5f, 1.0e-6f);
    HC_CHECK_NEAR(remapAngleDeg(90.0f),   1.0f, 1.0e-6f);
    HC_CHECK_NEAR(remapAngleDeg(120.0f),  1.0f, 1.0e-6f); // clamped
}

HC_TEST(measure_open_hand_golden_values)
{
    // Open hand. Python reference calculation:
    //   wrist    = (0.5, 0.9)
    //   thumbTip = (0.1, 0.4)
    //   indexTip = (0.5, 0.1)
    //   pinkyTip = (0.9, 0.5)
    //
    //   hand_length = hypot(0.0, -0.8) = 0.8
    //   d_index = hypot(0.4, -0.3) = 0.5
    //   d_pinky = hypot(0.8,  0.1) = 0.8062257748
    //   ratio_index = 0.625
    //   ratio_pinky = 1.007782...
    //   remap(0.625) = 0.25
    //   remap(1.007782) = 1.0 (clamped)
    //   angle_index = deg(atan2(0.3, 0.4)) = 36.869898 -> remap = (36.8699 - 30)/60 = 0.1144983
    //   angle_pinky = deg(atan2(0.1, 0.8)) = 7.125016 -> deadzone -> 0
    const auto h = makeHand({ 0.5f, 0.9f }, { 0.1f, 0.4f }, { 0.5f, 0.1f }, { 0.9f, 0.5f });
    const auto m = measure(h);

    HC_CHECK(m.valid);
    HC_CHECK_NEAR(m.thumbIndex01,      0.25f,       1.0e-5f);
    HC_CHECK_NEAR(m.thumbPinky01,      1.0f,        1.0e-5f);
    HC_CHECK_NEAR(m.thumbIndexAngle01, 0.1144983f,  1.0e-4f);
    HC_CHECK_NEAR(m.thumbPinkyAngle01, 0.0f,        1.0e-6f);
}

HC_TEST(measure_distance_gates_angle)
{
    // Thumb and index are nearly touching: ratio < 0.5, so remap yields 0.
    // Angle before gating would be 90 deg (vertical line) -> 1.0.
    // After gating, distance == 0 forces angle to 0 per the Python rule.
    const auto h = makeHand({ 0.5f, 0.9f }, { 0.5f, 0.15f }, { 0.5f, 0.1f }, { 0.9f, 0.5f });
    const auto m = measure(h);

    HC_CHECK(m.valid);
    HC_CHECK_NEAR(m.thumbIndex01,      0.0f, 1.0e-6f);
    HC_CHECK_NEAR(m.thumbIndexAngle01, 0.0f, 1.0e-6f);
}

HC_TEST(measure_returns_invalid_for_degenerate_hand_length)
{
    // wrist == indexTip -> hand_length = 0 -> invalid (guarded against div-by-zero).
    const auto h = makeHand({ 0.5f, 0.5f }, { 0.1f, 0.1f }, { 0.5f, 0.5f }, { 0.9f, 0.9f });
    const auto m = measure(h);
    HC_CHECK(! m.valid);
}

HC_TEST(measure_returns_invalid_for_absent_hand)
{
    HandFrame h;
    h.present = false;
    const auto m = measure(h);
    HC_CHECK(! m.valid);
}

HC_TEST(measure_angle_folds_absolute_values_like_python)
{
    // Python uses atan2(abs(dy), abs(dx)), so direction of the delta does not matter.
    // Two hand frames whose thumb-index vectors are mirror images should give the
    // same angle output.
    const auto a = makeHand({ 0.5f, 0.9f }, { 0.2f, 0.5f }, { 0.6f, 0.1f }, { 0.9f, 0.5f });
    const auto b = makeHand({ 0.5f, 0.9f }, { 0.6f, 0.5f }, { 0.2f, 0.1f }, { 0.1f, 0.5f });
    const auto ma = measure(a);
    const auto mb = measure(b);
    HC_CHECK(ma.valid && mb.valid);
    HC_CHECK_NEAR(ma.thumbIndexAngle01, mb.thumbIndexAngle01, 1.0e-5f);
}

// ---- v0.2 measurement tests --------------------------------------------------

namespace
{
    // Build a vertical, fully-open right-hand pose centred at (cx, cy).
    // Wrist at the bottom, fingers straight up, thumb out to the side.
    HandFrame makeFullHand(float cx, float cy, float scale)
    {
        HandFrame h;
        h.present = true;

        const auto setLm = [&](Landmark id, float dx, float dy)
        {
            h.at(id) = { cx + dx * scale, cy + dy * scale };
        };

        setLm(Landmark::wrist,     0.00f,  0.50f);

        // Thumb chain (out to the left, fully extended).
        setLm(Landmark::thumbCmc,  -0.10f,  0.40f);
        setLm(Landmark::thumbMcp,  -0.20f,  0.30f);
        setLm(Landmark::thumbIp,   -0.30f,  0.20f);
        setLm(Landmark::thumbTip,  -0.40f,  0.10f);

        // Index finger (vertical, fully open).
        setLm(Landmark::indexMcp,  -0.10f,  0.10f);
        setLm(Landmark::indexPip,  -0.10f, -0.10f);
        setLm(Landmark::indexDip,  -0.10f, -0.30f);
        setLm(Landmark::indexTip,  -0.10f, -0.50f);

        setLm(Landmark::middleMcp,  0.00f,  0.10f);
        setLm(Landmark::middlePip,  0.00f, -0.10f);
        setLm(Landmark::middleDip,  0.00f, -0.30f);
        setLm(Landmark::middleTip,  0.00f, -0.55f);

        setLm(Landmark::ringMcp,    0.10f,  0.10f);
        setLm(Landmark::ringPip,    0.10f, -0.10f);
        setLm(Landmark::ringDip,    0.10f, -0.30f);
        setLm(Landmark::ringTip,    0.10f, -0.50f);

        setLm(Landmark::pinkyMcp,   0.20f,  0.10f);
        setLm(Landmark::pinkyPip,   0.20f, -0.05f);
        setLm(Landmark::pinkyDip,   0.20f, -0.20f);
        setLm(Landmark::pinkyTip,   0.20f, -0.35f);

        return h;
    }

    // Same skeleton but every fingertip retracted to its MCP -> closed fist.
    HandFrame makeFist(float cx, float cy, float scale)
    {
        auto h = makeFullHand(cx, cy, scale);
        // Move every tip and DIP back to the corresponding MCP location.
        for (auto pair : std::initializer_list<std::pair<Landmark, Landmark>> {
                 { Landmark::indexTip,  Landmark::indexMcp  },
                 { Landmark::indexDip,  Landmark::indexMcp  },
                 { Landmark::middleTip, Landmark::middleMcp },
                 { Landmark::middleDip, Landmark::middleMcp },
                 { Landmark::ringTip,   Landmark::ringMcp   },
                 { Landmark::ringDip,   Landmark::ringMcp   },
                 { Landmark::pinkyTip,  Landmark::pinkyMcp  },
                 { Landmark::pinkyDip,  Landmark::pinkyMcp  } })
        {
            h.at(pair.first) = h.at(pair.second);
        }
        return h;
    }
}

HC_TEST(handX_handY_track_palm_centre)
{
    // Hand centred at (0.3, 0.6) -> handX ≈ midpoint(wrist.x, middleMcp.x),
    // handY ≈ midpoint(wrist.y, middleMcp.y).
    const auto h = makeFullHand(0.3f, 0.6f, 0.4f);
    const auto m = measure(h);
    HC_CHECK(m.valid);

    // wrist.x = 0.3 + 0   * 0.4 = 0.30
    // middleMcp.x = 0.3 + 0 * 0.4 = 0.30 -> handX = 0.30
    HC_CHECK_NEAR(m.handX01, 0.30f, 1.0e-4f);
    // wrist.y = 0.6 + 0.5*0.4 = 0.80, middleMcp.y = 0.6 + 0.1*0.4 = 0.64
    // handY = (0.80 + 0.64) / 2 = 0.72
    HC_CHECK_NEAR(m.handY01, 0.72f, 1.0e-4f);
}

HC_TEST(handX_handY_clamped_to_unit_range)
{
    // Hand placed off-frame should clamp to [0,1].
    const auto h = makeFullHand(-0.5f, 1.5f, 0.4f);
    const auto m = measure(h);
    HC_CHECK(m.valid);
    HC_CHECK_NEAR(m.handX01, 0.0f, 1.0e-6f);
    HC_CHECK_NEAR(m.handY01, 1.0f, 1.0e-6f);
}

HC_TEST(openness_is_high_for_open_hand)
{
    const auto h = makeFullHand(0.5f, 0.5f, 0.4f);
    const auto m = measure(h);
    HC_CHECK(m.valid);
    // Fingers fully extended -> openness should be near the top of the range.
    HC_CHECK(m.openness01 > 0.7f);
}

HC_TEST(openness_is_zero_for_closed_fist)
{
    const auto h = makeFist(0.5f, 0.5f, 0.4f);
    const auto m = measure(h);
    HC_CHECK(m.valid);
    // All fingertips coincide with their MCPs -> openness floors to 0.
    HC_CHECK_NEAR(m.openness01, 0.0f, 1.0e-3f);
}

int main() { return handcontrol::test::runAll(); }
