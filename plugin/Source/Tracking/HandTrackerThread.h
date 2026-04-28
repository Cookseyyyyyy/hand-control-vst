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
          - Computes `HandMeasurements` via `measure()`,
          - Smooths each output via `OneEuroFilter`,
          - Publishes the eight values into `ParameterBridge`.

        Also exposes a snapshot of the latest result for the UI thread. */
    class HandTrackerThread : private juce::Thread
    {
    public:
        HandTrackerThread(handcontrol::params::ParameterBridge& bridgeRef,
                          std::unique_ptr<IHandTracker> trackerImpl);
        ~HandTrackerThread() override;

        /** Starts/restarts the camera on the given device and kicks off the worker. */
        std::optional<std::string> start(int cameraIndex);
        void stop();

        /** Safe from any thread. Used by the UI to draw the preview. */
        struct UISnapshot
        {
            juce::Image frame;
            TrackingResult result;
            HandIdentityTracker::Assignment assignment;
            bool trackerHealthy { false };
            std::string statusMessage;
        };
        UISnapshot latestSnapshotForUI() const;

        /** Smoothing 0..1. Mapped to One-Euro `minCutoffHz`. */
        void setSmoothing(float value01) noexcept;
        void setHoldOnLost(bool shouldHold) noexcept;
        void setBypass(bool shouldBypass) noexcept;

    private:
        void run() override;
        void processOnce(const juce::Image& frame, double timestampSeconds);
        void publishMeasurements(int slotIndex, const HandMeasurements& m, double ts);
        void publishLost(int slotIndex);
        void updateFilterConfig() noexcept;

        handcontrol::params::ParameterBridge& bridge;
        std::unique_ptr<IHandTracker> tracker;
        CameraSource camera;
        HandIdentityTracker identity;

        struct SlotFilters
        {
            OneEuroFilter thumbIndex;
            OneEuroFilter thumbIndexAngle;
            OneEuroFilter thumbPinky;
            OneEuroFilter thumbPinkyAngle;
        };
        std::array<SlotFilters, 2> filters;

        struct LastValues
        {
            float thumbIndex { 0.0f };
            float thumbIndexAngle { 0.0f };
            float thumbPinky { 0.0f };
            float thumbPinkyAngle { 0.0f };
        };
        std::array<LastValues, 2> lastPublished;

        mutable std::mutex snapshotMutex;
        UISnapshot uiSnapshot;

        std::atomic<float> smoothing01 { 0.35f };
        std::atomic<bool> holdOnLost { true };
        std::atomic<bool> bypassed { false };
    };
}
