#pragma once

#include "IHandTracker.h"

#if HANDCONTROL_HAS_TFLITE

#include <memory>

namespace handcontrol::tracking
{
    /** Real hand tracker using MediaPipe's two-stage pipeline (palm detector +
        hand landmark) compiled as TensorFlow Lite models. See
        `docs/MEDIAPIPE_INTEGRATION.md` for the model download + build setup.

        Only compiled when `HANDCONTROL_ENABLE_TFLITE=ON`. */
    class TFLiteHandTracker final : public IHandTracker
    {
    public:
        TFLiteHandTracker();
        ~TFLiteHandTracker() override;

        std::optional<std::string> initialise() override;
        TrackingResult process(const juce::Image& frame, double timestampSeconds) override;
        const char* name() const noexcept override { return "TFLite"; }

    private:
        struct Impl;
        std::unique_ptr<Impl> impl;
    };
}

#endif
