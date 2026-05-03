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

        for (auto& f : landmarkFilters.x) f.reset();
        for (auto& f : landmarkFilters.y) f.reset();
        slotDiagnostics = {};
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
        // Smoothing 0..1 -> minCutoffHz in [10..0.6] Hz. Default 0 means a very
        // high cutoff (10 Hz) so the filter barely modifies fast hand motion;
        // dialling toward 1 trades responsiveness for stability.
        const float s = smoothing01.load(std::memory_order_relaxed);
        const double minCutoff = 10.0 - 9.4 * static_cast<double>(s);
        OneEuroFilter::Config cfg;
        cfg.minCutoffHz = minCutoff;
        cfg.beta        = 0.007;
        cfg.dCutoffHz   = 1.0;
        for (auto& f : landmarkFilters.x) f.setConfig(cfg);
        for (auto& f : landmarkFilters.y) f.setConfig(cfg);
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

        const auto& hand = result.hands[0];
        if (! hand.present)
        {
            slotDiagnostics.active = false;
            publishLost();
        }
        else
        {
            // Per-landmark One-Euro smoothing before measurement.
            HandFrame smoothed = hand;
            for (int i = 0; i < numLandmarks; ++i)
            {
                smoothed.landmarks[static_cast<size_t>(i)].x =
                    landmarkFilters.x[static_cast<size_t>(i)].process(
                        smoothed.landmarks[static_cast<size_t>(i)].x, ts);
                smoothed.landmarks[static_cast<size_t>(i)].y =
                    landmarkFilters.y[static_cast<size_t>(i)].process(
                        smoothed.landmarks[static_cast<size_t>(i)].y, ts);
            }

            const auto m = measure(smoothed);

            slotDiagnostics.active = true;
            slotDiagnostics.lastConfidence = hand.confidence;
            slotDiagnostics.lastSeenTime = ts;

            if (m.valid)
                publishMeasurements(m);
            else
                publishLost();
        }

        {
            std::lock_guard lock(snapshotMutex);
            uiSnapshot.frame = frame;
            uiSnapshot.result = result;
            uiSnapshot.trackerHealthy = true;
            uiSnapshot.statusMessage = {};
            uiSnapshot.slotDiagnostics = slotDiagnostics;
            uiSnapshot.currentTime = ts;
        }
    }

    void HandTrackerThread::publishMeasurements(const HandMeasurements& m)
    {
        bridge.publish(MeasurementId::thumbIndexDistance, m.thumbIndex01);
        bridge.publish(MeasurementId::thumbIndexAngle,    m.thumbIndexAngle01);
        bridge.publish(MeasurementId::thumbPinkyDistance, m.thumbPinky01);
        bridge.publish(MeasurementId::thumbPinkyAngle,    m.thumbPinkyAngle01);
        bridge.publish(MeasurementId::handX,              m.handX01);
        bridge.publish(MeasurementId::handY,              m.handY01);
        bridge.publish(MeasurementId::openness,           m.openness01);
    }

    void HandTrackerThread::publishLost()
    {
        if (holdOnLost.load(std::memory_order_relaxed))
            return;

        for (auto& f : landmarkFilters.x) f.reset();
        for (auto& f : landmarkFilters.y) f.reset();

        for (int i = 0; i < handcontrol::params::numMeasurements; ++i)
            bridge.publish(static_cast<MeasurementId>(i), 0.0f);
    }

    HandTrackerThread::UISnapshot HandTrackerThread::latestSnapshotForUI() const
    {
        std::lock_guard lock(snapshotMutex);
        UISnapshot copy;
        copy.frame = uiSnapshot.frame.createCopy();
        copy.result = uiSnapshot.result;
        copy.trackerHealthy = uiSnapshot.trackerHealthy;
        copy.statusMessage = uiSnapshot.statusMessage;
        copy.slotDiagnostics = uiSnapshot.slotDiagnostics;
        copy.currentTime = uiSnapshot.currentTime;
        return copy;
    }
}
