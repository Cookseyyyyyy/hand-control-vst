#pragma once

#include "IHandTracker.h"

#include <memory>

namespace handcontrol::tracking
{
    /** Real hand tracker that runs two ONNX models:

        1. Palm detector (192x192 input)  - finds up to two hands in the frame.
        2. Hand landmark  (224x224 input) - produces 21 landmarks per detection.

        Models and ONNX Runtime are fetched by CMake; models are embedded in
        the plugin binary via `HandControlModels` (juce_add_binary_data). End
        users never download anything. */
    class OnnxHandTracker final : public IHandTracker
    {
    public:
        OnnxHandTracker();
        ~OnnxHandTracker() override;

        std::optional<std::string> initialise() override;
        TrackingResult process(const juce::Image& frame, double timestampSeconds) override;
        const char* name() const noexcept override { return "ONNX / MediaPipe"; }

    private:
        struct Impl;
        std::unique_ptr<Impl> impl;
    };
}
