#pragma once

#include "CameraSource.h"
#include "IHandTracker.h"
#include "Normalizer.h"
#include "OneEuroFilter.h"

#include "../Params/ParameterBridge.h"

#include <juce_core/juce_core.h>

#include <array>
#include <atomic>
#include <memory>
#include <mutex>

namespace handcontrol::tracking
{
    /** Owns the camera, the hand tracker, and the normalization pipeline.

        Lifecycle:
          - The thread pulls the latest frame from `CameraSource`,
          - Runs `IHandTracker::process()` on it,
          - Smooths each landmark x/y via `OneEuroFilter`,
          - Computes `HandMeasurements` via `measure()`,
          - Publishes the values into `ParameterBridge`.

        Single-hand tracking only as of v0.4. */
    class HandTrackerThread : private juce::Thread
    {
    public:
        HandTrackerThread(handcontrol::params::ParameterBridge& bridgeRef,
                          std::unique_ptr<IHandTracker> trackerImpl);
        ~HandTrackerThread() override;

        std::optional<std::string> start(int cameraIndex);
        void stop();

        struct SlotDiagnostics
        {
            float lastConfidence { 0.0f };
            double lastSeenTime  { 0.0 };
            bool active          { false };
        };

        struct UISnapshot
        {
            juce::Image frame;
            TrackingResult result;
            bool trackerHealthy { false };
            std::string statusMessage;
            SlotDiagnostics slotDiagnostics {};
            double currentTime { 0.0 };
        };
        UISnapshot latestSnapshotForUI() const;

        void setSmoothing(float value01) noexcept;
        void setHoldOnLost(bool shouldHold) noexcept;
        void setBypass(bool shouldBypass) noexcept;
        void setMirrorCamera(bool shouldMirror) noexcept;

    private:
        void run() override;
        void processOnce(const juce::Image& frame, double timestampSeconds);
        void publishMeasurements(const HandMeasurements& m);
        void publishLost();
        void updateFilterConfig() noexcept;

        handcontrol::params::ParameterBridge& bridge;
        std::unique_ptr<IHandTracker> tracker;
        CameraSource camera;

        // 21 landmarks * 2 axes = 42 filters. Smoothing applied at the source
        // so all derived measurements benefit equally.
        struct LandmarkFilters
        {
            std::array<OneEuroFilter, numLandmarks> x {};
            std::array<OneEuroFilter, numLandmarks> y {};
        };
        LandmarkFilters landmarkFilters;

        SlotDiagnostics slotDiagnostics {};

        mutable std::mutex snapshotMutex;
        UISnapshot uiSnapshot;

        // Default smoothing now 0 (very light) to avoid the perceived lag we
        // had at v0.3's 0.35 default. Users can dial in heavier filtering.
        std::atomic<float> smoothing01 { 0.0f };
        std::atomic<bool> holdOnLost { true };
        std::atomic<bool> bypassed { false };
    };
}
