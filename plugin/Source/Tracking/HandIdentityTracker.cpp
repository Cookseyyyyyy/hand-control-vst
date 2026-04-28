#include "HandIdentityTracker.h"

#include <cmath>
#include <limits>

namespace handcontrol::tracking
{
    namespace
    {
        float dist2(const Point2D& a, const Point2D& b) noexcept
        {
            const float dx = a.x - b.x;
            const float dy = a.y - b.y;
            return dx * dx + dy * dy;
        }
    }

    void HandIdentityTracker::reset() noexcept
    {
        slots = {};
    }

    HandIdentityTracker::Assignment HandIdentityTracker::assign(
        const std::array<HandFrame, 2>& hands,
        double timestampSeconds) noexcept
    {
        // Expire stale slots.
        for (auto& s : slots)
        {
            if (s.occupied && (timestampSeconds - s.lastSeenTime) > config.lostTimeoutSec)
                s.occupied = false;
        }

        Assignment out;

        // Collect candidate input indices that are present.
        std::array<int, 2> candidates { -1, -1 };
        int nCandidates = 0;
        for (int i = 0; i < 2; ++i)
            if (hands[static_cast<size_t>(i)].present)
                candidates[static_cast<size_t>(nCandidates++)] = i;

        if (nCandidates == 0)
            return out;

        // Step 1: handedness match where possible.
        auto assignByHandedness = [&](Handedness h, int slot)
        {
            for (int c = 0; c < nCandidates; ++c)
            {
                const int idx = candidates[static_cast<size_t>(c)];
                if (idx < 0)
                    continue;
                if (hands[static_cast<size_t>(idx)].handedness == h
                    && ! out.slotToInputIndex[static_cast<size_t>(slot)].has_value())
                {
                    out.slotToInputIndex[static_cast<size_t>(slot)] = idx;
                    candidates[static_cast<size_t>(c)] = -1;
                    return true;
                }
            }
            return false;
        };

        // Convention: Left hand -> slot 0, Right hand -> slot 1.
        assignByHandedness(Handedness::left, 0);
        assignByHandedness(Handedness::right, 1);

        // Step 2: nearest-neighbour for remaining candidates against any still-live slot.
        for (int slot = 0; slot < 2; ++slot)
        {
            if (out.slotToInputIndex[static_cast<size_t>(slot)].has_value())
                continue;
            if (! slots[static_cast<size_t>(slot)].occupied)
                continue;

            int bestC = -1;
            float bestD2 = std::numeric_limits<float>::infinity();
            for (int c = 0; c < nCandidates; ++c)
            {
                const int idx = candidates[static_cast<size_t>(c)];
                if (idx < 0)
                    continue;
                const auto d2 = dist2(slots[static_cast<size_t>(slot)].lastWrist,
                                      hands[static_cast<size_t>(idx)].at(Landmark::wrist));
                if (d2 < bestD2)
                {
                    bestD2 = d2;
                    bestC = c;
                }
            }
            if (bestC >= 0)
            {
                out.slotToInputIndex[static_cast<size_t>(slot)] = candidates[static_cast<size_t>(bestC)];
                candidates[static_cast<size_t>(bestC)] = -1;
            }
        }

        // Step 3: any leftover candidates go into empty slots.
        for (int c = 0; c < nCandidates; ++c)
        {
            const int idx = candidates[static_cast<size_t>(c)];
            if (idx < 0)
                continue;
            for (int slot = 0; slot < 2; ++slot)
            {
                if (! out.slotToInputIndex[static_cast<size_t>(slot)].has_value())
                {
                    out.slotToInputIndex[static_cast<size_t>(slot)] = idx;
                    candidates[static_cast<size_t>(c)] = -1;
                    break;
                }
            }
        }

        // Update slot state.
        for (int slot = 0; slot < 2; ++slot)
        {
            const auto maybeIdx = out.slotToInputIndex[static_cast<size_t>(slot)];
            if (! maybeIdx.has_value())
                continue;
            const auto& hand = hands[static_cast<size_t>(*maybeIdx)];
            auto& s = slots[static_cast<size_t>(slot)];
            s.occupied = true;
            s.lastSeenTime = timestampSeconds;
            s.handedness = hand.handedness;
            s.lastWrist = hand.at(Landmark::wrist);
        }

        return out;
    }
}
