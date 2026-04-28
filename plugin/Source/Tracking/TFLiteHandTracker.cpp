#include "TFLiteHandTracker.h"

#if HANDCONTROL_HAS_TFLITE

// This file is the integration point for TFLite-based MediaPipe hand tracking.
//
// Integration checklist (see docs/MEDIAPIPE_INTEGRATION.md for full instructions):
//
//   1. Download the MediaPipe hand landmark models (Apache 2.0):
//        - palm_detection_full.tflite
//        - hand_landmark_full.tflite
//      and place them in `resources/`. They will be bundled with the plugin.
//
//   2. Provide prebuilt TensorFlow Lite headers and libs (tensorflowlite_c) via
//      the `TFLITE_ROOT` CMake variable when configuring.
//
//   3. Fill in the Impl struct below. The tested approach is:
//        a. Palm detector on a 192x192 RGB tensor to find ROI.
//        b. Hand landmark model on a 224x224 crop around the ROI to get 21 landmarks.
//        c. Temporal ROI reuse when confidence is high (skip detector most frames).
//
// Until (1)-(3) are done, this file intentionally returns an error from
// `initialise()` and falls back through to the mock tracker elsewhere.

#include <tensorflow/lite/c/c_api.h>

namespace handcontrol::tracking
{
    struct TFLiteHandTracker::Impl
    {
        TfLiteModel* palmModel { nullptr };
        TfLiteModel* landmarkModel { nullptr };
        TfLiteInterpreter* palmInterp { nullptr };
        TfLiteInterpreter* landmarkInterp { nullptr };

        ~Impl()
        {
            if (palmInterp)     TfLiteInterpreterDelete(palmInterp);
            if (landmarkInterp) TfLiteInterpreterDelete(landmarkInterp);
            if (palmModel)      TfLiteModelDelete(palmModel);
            if (landmarkModel)  TfLiteModelDelete(landmarkModel);
        }
    };

    TFLiteHandTracker::TFLiteHandTracker() : impl(std::make_unique<Impl>()) {}
    TFLiteHandTracker::~TFLiteHandTracker() = default;

    std::optional<std::string> TFLiteHandTracker::initialise()
    {
        // TODO(handcontrol): load palm_detection_full.tflite and
        // hand_landmark_full.tflite from the plugin bundle resources.
        return std::string("TFLiteHandTracker: not yet implemented. "
                           "See docs/MEDIAPIPE_INTEGRATION.md.");
    }

    TrackingResult TFLiteHandTracker::process(const juce::Image&, double timestampSeconds)
    {
        TrackingResult r;
        r.timestampSeconds = timestampSeconds;
        return r;
    }
}

#endif
