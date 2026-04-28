#pragma once

#include "Landmarks.h"

#include <juce_graphics/juce_graphics.h>

#include <optional>
#include <string>

namespace handcontrol::tracking
{
    /** Abstract hand tracker. Implementations:
        - `MockHandTracker` (default): generates procedural landmarks for testing.
        - `TFLiteHandTracker` (optional, built when HANDCONTROL_ENABLE_TFLITE=ON):
          real MediaPipe-derived palm + landmark model via TensorFlow Lite.

        Called from a single dedicated worker thread. Not thread-safe. */
    class IHandTracker
    {
    public:
        virtual ~IHandTracker() = default;

        /** Called once before first use. Returns a human-readable error on failure. */
        virtual std::optional<std::string> initialise() = 0;

        /** Process a single RGB frame and return detected hands.
            `frame` is expected to be in RGB colour order; x/y in the returned
            Point2D are normalised to [0,1] over the frame dimensions. */
        virtual TrackingResult process(const juce::Image& frame, double timestampSeconds) = 0;

        virtual const char* name() const noexcept = 0;
    };
}
