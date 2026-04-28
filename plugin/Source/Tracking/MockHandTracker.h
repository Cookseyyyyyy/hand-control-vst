#pragma once

#include "IHandTracker.h"

namespace handcontrol::tracking
{
    /** Generates a single animated hand whose thumb-index and thumb-pinky distances
        oscillate, so the whole pipeline can be exercised end-to-end without a
        webcam or a working ML runtime. Enabled by
        `HANDCONTROL_USE_MOCK_TRACKER=1` in the environment; otherwise
        `OnnxHandTracker` is used. */
    class MockHandTracker final : public IHandTracker
    {
    public:
        std::optional<std::string> initialise() override;
        TrackingResult process(const juce::Image& frame, double timestampSeconds) override;
        const char* name() const noexcept override { return "Mock"; }
    };
}
