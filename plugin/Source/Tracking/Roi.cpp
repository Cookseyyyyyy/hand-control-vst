#include "Roi.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace handcontrol::tracking
{
    RoiTransform roiFromLandmarks(const std::array<Point2D, numLandmarks>& lm) noexcept
    {
        RoiTransform roi;

        const auto& wrist     = lm[static_cast<size_t>(Landmark::wrist)];
        const auto& middleMcp = lm[static_cast<size_t>(Landmark::middleMcp)];
        const float dx = middleMcp.x - wrist.x;
        const float dy = middleMcp.y - wrist.y;
        roi.rotationRad = std::atan2(dx, -dy);

        const float cosR = std::cos(roi.rotationRad);
        const float sinR = std::sin(roi.rotationRad);

        // Project every landmark into the ROI-local frame (origin at wrist).
        // Local x =  cos*Δx + sin*Δy
        // Local y = -sin*Δx + cos*Δy
        float minX = std::numeric_limits<float>::infinity();
        float minY = std::numeric_limits<float>::infinity();
        float maxX = -std::numeric_limits<float>::infinity();
        float maxY = -std::numeric_limits<float>::infinity();
        for (const auto& p : lm)
        {
            const float relX = p.x - wrist.x;
            const float relY = p.y - wrist.y;
            const float lx =  cosR * relX + sinR * relY;
            const float ly = -sinR * relX + cosR * relY;
            minX = std::min(minX, lx); maxX = std::max(maxX, lx);
            minY = std::min(minY, ly); maxY = std::max(maxY, ly);
        }

        // Tiny epsilon just to keep the math finite if every landmark is at the
        // wrist (pathological). The clamp is unit-agnostic so the function works
        // both for pixel-space landmarks (the production path) and normalised
        // landmarks (tests).
        const float bboxW = std::max(1.0e-6f, maxX - minX);
        const float bboxH = std::max(1.0e-6f, maxY - minY);
        const float side  = std::max(bboxW, bboxH) * kHandBoxEnlarge;

        const float lcX = 0.5f * (minX + maxX);
        const float lcY = 0.5f * (minY + maxY) + kHandBoxShiftY * bboxH;

        // Rotate the local centre back into the input coordinate system.
        const float worldX = wrist.x + cosR * lcX - sinR * lcY;
        const float worldY = wrist.y + sinR * lcX + cosR * lcY;

        roi.size    = side;
        roi.centerX = worldX;
        roi.centerY = worldY;
        return roi;
    }
}
