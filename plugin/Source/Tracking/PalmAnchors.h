#pragma once

#include <array>
#include <vector>

namespace handcontrol::tracking
{
    /** MediaPipe palm detector SSD anchors for a 192x192 input.

        The palm detector outputs 2016 candidate boxes, each tied to a fixed
        anchor (cx, cy) at unit scale [0, 1]. Deltas from the model are added
        to these anchors to produce final boxes.

        We regenerate the list procedurally rather than hardcoding 2016 values.
        The formula exactly matches the anchor table in OpenCV Zoo's
        `mp_palmdet.py`:
          - Layer 0: stride 8,  feature map 24x24, 2 anchors per cell  -> 1152
          - Layer 1: stride 16, feature map 12x12, 6 anchors per cell  ->  864
        Total: 2016, which matches the model's output shape. */
    struct Anchor { float cx; float cy; };

    constexpr int kNumPalmAnchors = 2016;

    /** Build the anchor table once at startup. */
    std::vector<Anchor> buildPalmAnchors();
}
