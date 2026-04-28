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

int main() { return handcontrol::test::runAll(); }
