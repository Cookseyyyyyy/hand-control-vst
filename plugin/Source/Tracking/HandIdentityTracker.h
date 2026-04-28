#pragma once

#include "Landmarks.h"

#include <array>
#include <optional>

namespace handcontrol::tracking
{
    /** Stabilises which detected hand is "Hand 1" vs "Hand 2" frame-to-frame.

        Strategy:
          1. If both candidates report a clean Left/Right handedness, use that.
             (By convention: Left -> slot 0 (Hand 1), Right -> slot 1 (Hand 2).)
          2. Otherwise, nearest-neighbour match against the last known wrist
             position for each slot.
          3. Fresh hands get the first empty slot.

        Keeps a short timeout: if a hand is absent for more than `lostTimeoutSec`,
        its slot is cleared so a new hand can take it. */
    class HandIdentityTracker
    {
    public:
        struct Assignment
        {
            std::array<std::optional<int>, 2> slotToInputIndex {};  // which input hand fills each slot
        };

        struct Config
        {
            double lostTimeoutSec { 0.75 };
        };

        void setConfig(Config c) noexcept { config = c; }

        /** Given up to two detected hands (in arbitrary order from the tracker),
            return a stable assignment of them to slots 0 and 1. */
        Assignment assign(const std::array<HandFrame, 2>& hands,
                          double timestampSeconds) noexcept;

        void reset() noexcept;

    private:
        struct SlotState
        {
            bool occupied { false };
            double lastSeenTime { 0.0 };
            Handedness handedness { Handedness::unknown };
            Point2D lastWrist {};
        };

        Config config {};
        std::array<SlotState, 2> slots {};
    };
}
