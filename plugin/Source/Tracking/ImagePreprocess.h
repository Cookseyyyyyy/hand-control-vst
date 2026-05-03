#pragma once

#include "Landmarks.h"

#include <juce_graphics/juce_graphics.h>

#include <vector>

namespace handcontrol::tracking
{
    /** Pads `src` with black borders on the shorter axis, resizes to the given
        square target, and writes a float NHWC RGB tensor (values 0..1) suitable
        for feeding the palm detector.

        `outPadBiasX` and `outPadBiasY` receive the top-left pad size in input
        image pixels (so the model's outputs can be translated back to the
        original frame coordinates).

        The output buffer is laid out as a flat `target*target*3` float array. */
    void letterboxToTensor(const juce::Image& src,
                           int target,
                           std::vector<float>& outTensor,
                           int& outPadBiasX,
                           int& outPadBiasY);

    /** Crop, rotate, and pad a region of interest around a hand,
        then resize to the target square and normalise into a float NHWC RGB
        tensor 0..1, ready for the hand landmark model.

        `RoiTransform` is defined in Landmarks.h and shared with the rest of the
        tracker so the same struct can be used for diagnostics + overlay. */
    void cropRotateResizeToTensor(const juce::Image& src,
                                  const RoiTransform& roi,
                                  int target,
                                  std::vector<float>& outTensor);
}
