#include "Roi.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace handcontrol::tracking
{
    bool pointInsideRoi(float cx, float cy, const RoiTransform& roi) noexcept
    {
        const float dx = cx - roi.centerX;
        const float dy = cy - roi.centerY;
        const float c =  std::cos(roi.rotationRad);
        const float s =  std::sin(roi.rotationRad);
        const float lx =  c * dx + s * dy;
        const float ly = -s * dx + c * dy;
        const float half = roi.size * 0.5f;
        return std::abs(lx) <= half && std::abs(ly) <= half;
    }

    float bboxIou(float ax1, float ay1, float ax2, float ay2,
                  float bx1, float by1, float bx2, float by2) noexcept
    {
        const float ix1 = std::max(ax1, bx1);
        const float iy1 = std::max(ay1, by1);
        const float ix2 = std::min(ax2, bx2);
        const float iy2 = std::min(ay2, by2);
        const float iw = std::max(0.0f, ix2 - ix1);
        const float ih = std::max(0.0f, iy2 - iy1);
        const float inter = iw * ih;
        const float aArea = std::max(0.0f, ax2 - ax1) * std::max(0.0f, ay2 - ay1);
        const float bArea = std::max(0.0f, bx2 - bx1) * std::max(0.0f, by2 - by1);
        const float un = aArea + bArea - inter;
        return un > 0.0f ? inter / un : 0.0f;
    }

    LandmarkBbox computeLandmarkBbox(const std::array<Point2D, numLandmarks>& lm) noexcept
    {
        LandmarkBbox b;
        b.x1 = b.y1 =  std::numeric_limits<float>::infinity();
        b.x2 = b.y2 = -std::numeric_limits<float>::infinity();
        for (const auto& p : lm)
        {
            b.x1 = std::min(b.x1, p.x); b.x2 = std::max(b.x2, p.x);
            b.y1 = std::min(b.y1, p.y); b.y2 = std::max(b.y2, p.y);
        }
        b.valid = std::isfinite(b.x1) && std::isfinite(b.y2);
        return b;
    }

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
