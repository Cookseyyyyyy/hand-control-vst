#pragma once

#include "IHandTracker.h"

namespace handcontrol::tracking
{
    /** Generates a single animated hand whose thumb-index and thumb-pinky distances
        oscillate, so the whole pipeline can be exercised end-to-end without ML.
        Useful as a placeholder until the TFLite integration is wired in, and as a
        deterministic source for UI smoke tests. */
    class MockHandTracker final : public IHandTracker
    {
    public:
        std::optional<std::string> initialise() override;
        TrackingResult process(const juce::Image& frame, double timestampSeconds) override;
        const char* name() const noexcept override { return "Mock"; }
    };
}
