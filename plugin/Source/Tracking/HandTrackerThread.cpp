#include "HandTrackerThread.h"

#include <juce_core/juce_core.h>

namespace handcontrol::tracking
{
    using handcontrol::params::MeasurementId;

    HandTrackerThread::HandTrackerThread(handcontrol::params::ParameterBridge& bridgeRef,
                                         std::unique_ptr<IHandTracker> trackerImpl)
        : juce::Thread("HandControl Tracker"),
          bridge(bridgeRef),
          tracker(std::move(trackerImpl))
    {
        jassert(tracker != nullptr);
    }

    HandTrackerThread::~HandTrackerThread()
    {
        stop();
    }

    std::optional<std::string> HandTrackerThread::start(int cameraIndex)
    {
        stop();

        if (auto err = tracker->initialise())
            return err;

        if (auto err = camera.open(cameraIndex))
            return err;

        identity.reset();
        for (auto& slot : landmarkFilters)
        {
            for (auto& f : slot.x) f.reset();
            for (auto& f : slot.y) f.reset();
        }
        for (auto& d : slotDiagnostics)
        {
            d.active = false;
            d.lastConfidence = 0.0f;
            d.lastSeenTime = 0.0;
        }
        updateFilterConfig();

        startThread(juce::Thread::Priority::normal);
        return std::nullopt;
    }

    void HandTrackerThread::stop()
    {
        signalThreadShouldExit();
        stopThread(1500);
        camera.close();
    }

    void HandTrackerThread::setSmoothing(float value01) noexcept
    {
        smoothing01.store(juce::jlimit(0.0f, 1.0f, value01), std::memory_order_relaxed);
        updateFilterConfig();
    }

    void HandTrackerThread::setHoldOnLost(bool shouldHold) noexcept
    {
        holdOnLost.store(shouldHold, std::memory_order_relaxed);
    }

    void HandTrackerThread::setBypass(bool shouldBypass) noexcept
    {
        bypassed.store(shouldBypass, std::memory_order_relaxed);
    }

    void HandTrackerThread::setMirrorCamera(bool shouldMirror) noexcept
    {
        camera.setMirror(shouldMirror);
    }

    void HandTrackerThread::updateFilterConfig() noexcept
    {
        // Smoothing 0..1 -> minCutoffHz in [6.0..0.6] Hz. Lower cutoff = more
        // smoothing. Beta of 0.007 keeps the filter responsive when fingers move
        // fast (high frequency content) while damping low-freq jitter.
        const float s = smoothing01.load(std::memory_order_relaxed);
        const double minCutoff = 6.0 - 5.4 * static_cast<double>(s);
        OneEuroFilter::Config cfg;
        cfg.minCutoffHz = minCutoff;
        cfg.beta        = 0.007;
        cfg.dCutoffHz   = 1.0;
        for (auto& slot : landmarkFilters)
        {
            for (auto& f : slot.x) f.setConfig(cfg);
            for (auto& f : slot.y) f.setConfig(cfg);
        }
    }

    void HandTrackerThread::run()
    {
        constexpr double targetFrameSec = 1.0 / 30.0;
        double nextWake = juce::Time::getMillisecondCounterHiRes() * 0.001;

        while (! threadShouldExit())
        {
            const auto now = juce::Time::getMillisecondCounterHiRes() * 0.001;
            auto frame = camera.snapshotLatest();

            if (frame.isValid())
                processOnce(frame, now);

            nextWake += targetFrameSec;
            const auto sleepMs = juce::jmax(0.0, (nextWake - now) * 1000.0);
            if (sleepMs > 0.0)
                wait(static_cast<int>(sleepMs));
            else
                nextWake = juce::Time::getMillisecondCounterHiRes() * 0.001;
        }
    }

    void HandTrackerThread::processOnce(const juce::Image& frame, double ts)
    {
        if (bypassed.load(std::memory_order_relaxed))
            return;

        auto result = tracker->process(frame, ts);
        auto assignment = identity.assign(result.hands, ts);

        for (int slot = 0; slot < 2; ++slot)
        {
            const auto maybeIdx = assignment.slotToInputIndex[static_cast<size_t>(slot)];
            if (! maybeIdx.has_value())
            {
                slotDiagnostics[static_cast<size_t>(slot)].active = false;
                publishLost(slot);
                continue;
            }

            // Apply per-landmark One-Euro smoothing BEFORE measurement so that
            // distance, angle, position and openness all benefit equally.
            HandFrame smoothedHand = result.hands[static_cast<size_t>(*maybeIdx)];
            auto& filters = landmarkFilters[static_cast<size_t>(slot)];
            for (int i = 0; i < numLandmarks; ++i)
            {
                smoothedHand.landmarks[static_cast<size_t>(i)].x =
                    filters.x[static_cast<size_t>(i)].process(smoothedHand.landmarks[static_cast<size_t>(i)].x, ts);
                smoothedHand.landmarks[static_cast<size_t>(i)].y =
                    filters.y[static_cast<size_t>(i)].process(smoothedHand.landmarks[static_cast<size_t>(i)].y, ts);
            }

            const auto m = measure(smoothedHand);

            slotDiagnostics[static_cast<size_t>(slot)].active = true;
            slotDiagnostics[static_cast<size_t>(slot)].lastConfidence = smoothedHand.confidence;
            slotDiagnostics[static_cast<size_t>(slot)].lastSeenTime = ts;

            if (! m.valid)
            {
                publishLost(slot);
                continue;
            }

            const auto base = static_cast<MeasurementId>(slot * 4);
            const auto extras = static_cast<MeasurementId>(
                static_cast<int>(MeasurementId::hand1HandX) + slot * 3);
            juce::ignoreUnused(extras);

            publishMeasurements(slot, base,
                                m.thumbIndex01, m.thumbIndexAngle01,
                                m.thumbPinky01, m.thumbPinkyAngle01,
                                m.handX01, m.handY01, m.openness01);
        }

        {
            std::lock_guard lock(snapshotMutex);
            uiSnapshot.frame = frame;
            uiSnapshot.result = result;
            uiSnapshot.assignment = assignment;
            uiSnapshot.trackerHealthy = true;
            uiSnapshot.statusMessage = {};
            uiSnapshot.slotDiagnostics = slotDiagnostics;
            uiSnapshot.currentTime = ts;
        }
    }

    void HandTrackerThread::publishMeasurements(int slot, const MeasurementId firstId,
                                                 float v0, float v1, float v2, float v3,
                                                 float vHandX, float vHandY, float vOpenness)
    {
        // First four are at slot*4..slot*4+3 (matches v0.1 layout).
        bridge.publish(static_cast<MeasurementId>(static_cast<int>(firstId) + 0), v0);
        bridge.publish(static_cast<MeasurementId>(static_cast<int>(firstId) + 1), v1);
        bridge.publish(static_cast<MeasurementId>(static_cast<int>(firstId) + 2), v2);
        bridge.publish(static_cast<MeasurementId>(static_cast<int>(firstId) + 3), v3);

        // New v0.2 measurements: per-hand block of three.
        const int extrasBase = static_cast<int>(MeasurementId::hand1HandX) + slot * 3;
        bridge.publish(static_cast<MeasurementId>(extrasBase + 0), vHandX);
        bridge.publish(static_cast<MeasurementId>(extrasBase + 1), vHandY);
        bridge.publish(static_cast<MeasurementId>(extrasBase + 2), vOpenness);
    }

    void HandTrackerThread::publishLost(int slot)
    {
        if (holdOnLost.load(std::memory_order_relaxed))
            return;

        auto& filters = landmarkFilters[static_cast<size_t>(slot)];
        for (auto& f : filters.x) f.reset();
        for (auto& f : filters.y) f.reset();

        const int baseId = slot * 4;
        bridge.publish(static_cast<MeasurementId>(baseId + 0), 0.0f);
        bridge.publish(static_cast<MeasurementId>(baseId + 1), 0.0f);
        bridge.publish(static_cast<MeasurementId>(baseId + 2), 0.0f);
        bridge.publish(static_cast<MeasurementId>(baseId + 3), 0.0f);

        const int extrasBase = static_cast<int>(MeasurementId::hand1HandX) + slot * 3;
        bridge.publish(static_cast<MeasurementId>(extrasBase + 0), 0.0f);
        bridge.publish(static_cast<MeasurementId>(extrasBase + 1), 0.0f);
        bridge.publish(static_cast<MeasurementId>(extrasBase + 2), 0.0f);
    }

    HandTrackerThread::UISnapshot HandTrackerThread::latestSnapshotForUI() const
    {
        std::lock_guard lock(snapshotMutex);
        UISnapshot copy;
        copy.frame = uiSnapshot.frame.createCopy();
        copy.result = uiSnapshot.result;
        copy.assignment = uiSnapshot.assignment;
        copy.trackerHealthy = uiSnapshot.trackerHealthy;
        copy.statusMessage = uiSnapshot.statusMessage;
        copy.slotDiagnostics = uiSnapshot.slotDiagnostics;
        copy.currentTime = uiSnapshot.currentTime;
        return copy;
    }
}
