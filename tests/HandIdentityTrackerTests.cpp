#include "TestMain.h"

#include "Tracking/HandIdentityTracker.h"
#include "Tracking/Landmarks.h"

using namespace handcontrol::tracking;

namespace
{
    HandFrame makeHand(float wristX, float wristY, Handedness h = Handedness::unknown)
    {
        HandFrame f;
        f.present = true;
        f.handedness = h;
        f.at(Landmark::wrist) = { wristX, wristY };
        return f;
    }
}

HC_TEST(identity_assigns_left_to_slot0_right_to_slot1)
{
    HandIdentityTracker t;
    std::array<HandFrame, 2> hands {
        makeHand(0.2f, 0.5f, Handedness::right),
        makeHand(0.8f, 0.5f, Handedness::left)
    };
    auto a = t.assign(hands, 0.0);
    HC_CHECK(a.slotToInputIndex[0].has_value());
    HC_CHECK(a.slotToInputIndex[1].has_value());
    HC_CHECK(*a.slotToInputIndex[0] == 1);  // left hand -> slot 0
    HC_CHECK(*a.slotToInputIndex[1] == 0);  // right hand -> slot 1
}

HC_TEST(identity_keeps_hand_in_same_slot_across_frames_when_input_order_flips)
{
    HandIdentityTracker t;
    std::array<HandFrame, 2> hands1 {
        makeHand(0.2f, 0.5f, Handedness::left),
        makeHand(0.8f, 0.5f, Handedness::right)
    };
    auto a1 = t.assign(hands1, 0.0);
    HC_CHECK(*a1.slotToInputIndex[0] == 0);
    HC_CHECK(*a1.slotToInputIndex[1] == 1);

    // Next frame: the tracker returns the hands in the opposite order, but
    // handedness still maps the left hand to slot 0.
    std::array<HandFrame, 2> hands2 {
        makeHand(0.8f, 0.5f, Handedness::right),
        makeHand(0.2f, 0.5f, Handedness::left)
    };
    auto a2 = t.assign(hands2, 0.033);
    HC_CHECK(*a2.slotToInputIndex[0] == 1);  // left -> slot 0
    HC_CHECK(*a2.slotToInputIndex[1] == 0);  // right -> slot 1
}

HC_TEST(identity_nearest_neighbour_when_handedness_ambiguous)
{
    HandIdentityTracker t;
    // Seed the tracker with a hand near (0.2, 0.5) in slot 0.
    {
        std::array<HandFrame, 2> hands {
            makeHand(0.2f, 0.5f, Handedness::unknown),
            HandFrame {}
        };
        (void) t.assign(hands, 0.0);
    }
    // New frame, no handedness info, one hand near (0.21, 0.51) and one far away.
    std::array<HandFrame, 2> hands {
        makeHand(0.75f, 0.5f, Handedness::unknown),
        makeHand(0.21f, 0.51f, Handedness::unknown)
    };
    auto a = t.assign(hands, 0.033);
    HC_CHECK(*a.slotToInputIndex[0] == 1);  // the near-neighbour wins slot 0
}

HC_TEST(identity_lost_slot_clears_after_timeout)
{
    HandIdentityTracker t;
    {
        std::array<HandFrame, 2> hands { makeHand(0.2f, 0.5f, Handedness::left), HandFrame {} };
        (void) t.assign(hands, 0.0);
    }
    // Long gap; no hands visible.
    {
        std::array<HandFrame, 2> empty {};
        (void) t.assign(empty, 5.0);
    }
    // New right hand shows up.
    {
        std::array<HandFrame, 2> hands { makeHand(0.8f, 0.5f, Handedness::right), HandFrame {} };
        auto a = t.assign(hands, 5.033);
        // It should land in slot 1 (right convention), not reuse slot 0.
        HC_CHECK(! a.slotToInputIndex[0].has_value());
        HC_CHECK(*a.slotToInputIndex[1] == 0);
    }
}
