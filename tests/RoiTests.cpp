#include "TestMain.h"

#include "Tracking/Roi.h"

#include <cmath>

using namespace handcontrol::tracking;

namespace
{
    // Build a vertical-up open-hand pose: wrist at the bottom, middle MCP above
    // it, fingertips above that, thumb out to the side. Hand size and centre
    // are configurable so we can exercise translation independence.
    std::array<Point2D, numLandmarks> verticalHand(float cx, float cy, float scale)
    {
        std::array<Point2D, numLandmarks> lm {};

        auto put = [&](Landmark id, float dx, float dy)
        {
            lm[static_cast<size_t>(id)] = { cx + dx * scale, cy + dy * scale };
        };

        put(Landmark::wrist,     0.0f,  1.0f);

        put(Landmark::thumbCmc, -0.2f,  0.8f);
        put(Landmark::thumbMcp, -0.4f,  0.6f);
        put(Landmark::thumbIp,  -0.5f,  0.4f);
        put(Landmark::thumbTip, -0.6f,  0.2f);

        put(Landmark::indexMcp, -0.2f,  0.0f);
        put(Landmark::indexPip, -0.2f, -0.4f);
        put(Landmark::indexDip, -0.2f, -0.7f);
        put(Landmark::indexTip, -0.2f, -1.0f);

        put(Landmark::middleMcp, 0.0f,  0.0f);
        put(Landmark::middlePip, 0.0f, -0.4f);
        put(Landmark::middleDip, 0.0f, -0.7f);
        put(Landmark::middleTip, 0.0f, -1.1f);

        put(Landmark::ringMcp,   0.2f,  0.0f);
        put(Landmark::ringPip,   0.2f, -0.4f);
        put(Landmark::ringDip,   0.2f, -0.7f);
        put(Landmark::ringTip,   0.2f, -1.0f);

        put(Landmark::pinkyMcp,  0.4f,  0.1f);
        put(Landmark::pinkyPip,  0.4f, -0.2f);
        put(Landmark::pinkyDip,  0.4f, -0.5f);
        put(Landmark::pinkyTip,  0.4f, -0.8f);

        return lm;
    }

    // Rotate every point in a landmark set about (cx, cy) by `angleRad`.
    std::array<Point2D, numLandmarks> rotateAbout(
        const std::array<Point2D, numLandmarks>& lm,
        float cx, float cy, float angleRad)
    {
        const float c = std::cos(angleRad);
        const float s = std::sin(angleRad);
        std::array<Point2D, numLandmarks> out;
        for (size_t i = 0; i < lm.size(); ++i)
        {
            const float dx = lm[i].x - cx;
            const float dy = lm[i].y - cy;
            out[i] = { cx + c * dx - s * dy, cy + s * dx + c * dy };
        }
        return out;
    }
}

HC_TEST(roiFromLandmarks_vertical_hand_has_zero_rotation)
{
    // For a vertical-up hand (middle MCP directly above wrist), the rotation
    // should be (close to) zero so the ROI is axis-aligned.
    const auto lm = verticalHand(0.5f, 0.5f, 0.2f);
    const auto roi = roiFromLandmarks(lm);
    HC_CHECK_NEAR(roi.rotationRad, 0.0f, 1.0e-4f);
}

HC_TEST(roiFromLandmarks_centre_is_near_palm)
{
    // ROI centre should sit roughly between wrist and fingertips, biased
    // slightly *toward* the fingertips by the MediaPipe shift (kHandBoxShiftY).
    const auto lm = verticalHand(0.5f, 0.5f, 0.2f);
    const auto roi = roiFromLandmarks(lm);

    // Wrist is at y = 0.5 + 1.0*0.2 = 0.7.
    // Top of fingers is at y = 0.5 + (-1.1)*0.2 = 0.28. Mid = ~0.49.
    // The shift moves the centre slightly higher (smaller y) than the mid.
    HC_CHECK_NEAR(roi.centerX, 0.5f, 0.1f);
    HC_CHECK(roi.centerY < 0.50f);
    HC_CHECK(roi.centerY > 0.30f);
}

HC_TEST(roiFromLandmarks_size_grows_with_hand_size)
{
    const auto smallHand = verticalHand(0.5f, 0.5f, 0.10f);
    const auto bigHand   = verticalHand(0.5f, 0.5f, 0.30f);
    const auto smallRoi = roiFromLandmarks(smallHand);
    const auto bigRoi   = roiFromLandmarks(bigHand);
    HC_CHECK(bigRoi.size > smallRoi.size * 2.0f);
}

HC_TEST(roiFromLandmarks_size_includes_enlarge_factor)
{
    const auto lm = verticalHand(0.5f, 0.5f, 0.2f);
    const auto roi = roiFromLandmarks(lm);

    // Hand bounding box height in input coords: y range = [0.28, 0.7] = 0.42.
    // Width: x range = [0.5 - 0.6*0.2, 0.5 + 0.4*0.2] = [0.38, 0.58] = 0.20.
    // Side = max(width, height) * 1.65 = 0.42 * 1.65 = 0.693.
    HC_CHECK_NEAR(roi.size, 0.693f, 0.01f);
}

// Regression test for the v0.2 dual-slot bug: palm-derived ROI (large, ~2.6x)
// and landmark-derived ROI (smaller, ~1.65x) for the same hand had IoU below
// 0.5 and were treated as separate hands, filling both H1 and H2 slots with
// the same physical hand. Centre-inside check fixes that.
HC_TEST(pointInsideRoi_detects_same_hand_size_mismatch)
{
    // A landmark-derived ROI: small, axis-aligned, centred at (300, 400).
    RoiTransform landmarkRoi;
    landmarkRoi.centerX = 300.0f;
    landmarkRoi.centerY = 400.0f;
    landmarkRoi.size    = 200.0f;
    landmarkRoi.rotationRad = 0.0f;

    // A palm-derived ROI for the *same* hand would have its centre roughly
    // inside the landmark ROI (palm is part of the hand) but be much larger.
    // Pick a centre slightly offset from landmark centre but still well inside.
    const float palmCentreX = 320.0f;
    const float palmCentreY = 380.0f;

    HC_CHECK(pointInsideRoi(palmCentreX, palmCentreY, landmarkRoi));

    // A palm centre well outside the landmark ROI (e.g. 600 px away on the x
    // axis) is correctly rejected.
    HC_CHECK(! pointInsideRoi(900.0f, 400.0f, landmarkRoi));
}

HC_TEST(pointInsideRoi_handles_rotation)
{
    // A 45-degree rotated 200x200 ROI centred at the origin. Its corners
    // reach to about (+/-141, 0) and (0, +/-141) in image coords - so
    // (140, 0) is inside the rotated box but well outside an axis-aligned
    // 200-square (which would only contain x in [-100, 100]).
    RoiTransform axisAligned;
    axisAligned.centerX = 0.0f; axisAligned.centerY = 0.0f;
    axisAligned.size = 200.0f;
    axisAligned.rotationRad = 0.0f;

    RoiTransform rotated45 = axisAligned;
    rotated45.rotationRad = 0.7853982f;  // pi/4

    // Discriminating point on the rotated box's local x axis (a corner).
    HC_CHECK(! pointInsideRoi(140.0f, 0.0f, axisAligned));   // outside axis-aligned
    HC_CHECK(  pointInsideRoi(140.0f, 0.0f, rotated45));     // inside rotated

    // A point well past the corner of either box: outside both.
    HC_CHECK(! pointInsideRoi(200.0f, 200.0f, axisAligned));
    HC_CHECK(! pointInsideRoi(200.0f, 200.0f, rotated45));
}

HC_TEST(roiFromLandmarks_rotation_tracks_hand_orientation)
{
    // Vertical hand (fingers up in image coords, i.e. wrist has larger y than
    // middle MCP): rotation = atan2(0, +1) = 0.
    const auto upright = verticalHand(0.5f, 0.5f, 0.2f);
    HC_CHECK_NEAR(roiFromLandmarks(upright).rotationRad, 0.0f, 1.0e-4f);

    // Rotate the whole hand by +pi/2 about the wrist. The wrist -> middle MCP
    // vector starts at (0, -1) in image-coord deltas (because wrist has larger y).
    // After +pi/2 rotation in image coords this becomes (+1, 0).
    // The ROI's reported rotation should be atan2(+1, 0) = +pi/2.
    const auto wrist = upright[static_cast<size_t>(Landmark::wrist)];
    const auto rotated = rotateAbout(upright, wrist.x, wrist.y, 1.5707963f);
    HC_CHECK_NEAR(roiFromLandmarks(rotated).rotationRad, 1.5707963f, 1.0e-3f);
}
