#pragma once

#include "CameraSource.h"
#include "HandIdentityTracker.h"
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
          - Stabilises hand identity with `HandIdentityTracker`,
          - Smooths each landmark x/y via `OneEuroFilter` (per-landmark, not per-output),
          - Computes `HandMeasurements` via `measure()`,
          - Publishes the values into `ParameterBridge`.

        Also exposes a snapshot of the latest result for the UI thread. */
    class HandTrackerThread : private juce::Thread
    {
    public:
        HandTrackerThread(handcontrol::params::ParameterBridge& bridgeRef,
                          std::unique_ptr<IHandTracker> trackerImpl);
        ~HandTrackerThread() override;

        std::optional<std::string> start(int cameraIndex);
        void stop();

        /** Per-slot diagnostic data, surfaced to the UI for the status bar. */
        struct SlotDiagnostics
        {
            float lastConfidence { 0.0f };
            double lastSeenTime  { 0.0 };
            bool active          { false };
        };

        /** Safe from any thread. Used by the UI to draw the preview. */
        struct UISnapshot
        {
            juce::Image frame;
            TrackingResult result;
            HandIdentityTracker::Assignment assignment;
            bool trackerHealthy { false };
            std::string statusMessage;
            std::array<SlotDiagnostics, 2> slotDiagnostics {};
            double currentTime { 0.0 };
        };
        UISnapshot latestSnapshotForUI() const;

        /** Smoothing 0..1. Mapped to One-Euro `minCutoffHz`. */
        void setSmoothing(float value01) noexcept;
        void setHoldOnLost(bool shouldHold) noexcept;
        void setBypass(bool shouldBypass) noexcept;
        void setMirrorCamera(bool shouldMirror) noexcept;

    private:
        void run() override;
        void processOnce(const juce::Image& frame, double timestampSeconds);
        void publishMeasurements(int slotIndex, const handcontrol::params::MeasurementId firstId,
                                  float v0, float v1, float v2, float v3,
                                  float vHandX, float vHandY, float vOpenness);
        void publishLost(int slotIndex);
        void updateFilterConfig() noexcept;

        handcontrol::params::ParameterBridge& bridge;
        std::unique_ptr<IHandTracker> tracker;
        CameraSource camera;
        HandIdentityTracker identity;

        // 21 landmarks * 2 axes = 42 filters per slot. We smooth at the source
        // so all derived measurements (distances, angles, openness, position)
        // benefit equally.
        struct LandmarkFilters
        {
            std::array<OneEuroFilter, numLandmarks> x {};
            std::array<OneEuroFilter, numLandmarks> y {};
        };
        std::array<LandmarkFilters, 2> landmarkFilters;

        std::array<SlotDiagnostics, 2> slotDiagnostics {};

        mutable std::mutex snapshotMutex;
        UISnapshot uiSnapshot;

        std::atomic<float> smoothing01 { 0.35f };
        std::atomic<bool> holdOnLost { true };
        std::atomic<bool> bypassed { false };
    };
}
