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
        for (auto& f : filters)
        {
            f.thumbIndex.reset();
            f.thumbIndexAngle.reset();
            f.thumbPinky.reset();
            f.thumbPinkyAngle.reset();
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

    void HandTrackerThread::updateFilterConfig() noexcept
    {
        // Map 0..1 smoothing to minCutoffHz in [6.0 .. 0.6] Hz (higher cutoff -> less smoothing).
        const float s = smoothing01.load(std::memory_order_relaxed);
        const double minCutoff = 6.0 - 5.4 * static_cast<double>(s);
        OneEuroFilter::Config cfg;
        cfg.minCutoffHz = minCutoff;
        cfg.beta = 0.007;
        cfg.dCutoffHz = 1.0;
        for (auto& f : filters)
        {
            f.thumbIndex.setConfig(cfg);
            f.thumbIndexAngle.setConfig(cfg);
            f.thumbPinky.setConfig(cfg);
            f.thumbPinkyAngle.setConfig(cfg);
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
                publishLost(slot);
                continue;
            }
            const auto& hand = result.hands[static_cast<size_t>(*maybeIdx)];
            const auto m = measure(hand);
            publishMeasurements(slot, m, ts);
        }

        {
            std::lock_guard lock(snapshotMutex);
            uiSnapshot.frame = frame;
            uiSnapshot.result = result;
            uiSnapshot.assignment = assignment;
            uiSnapshot.trackerHealthy = true;
            uiSnapshot.statusMessage = {};
        }
    }

    void HandTrackerThread::publishMeasurements(int slot, const HandMeasurements& m, double ts)
    {
        auto& f = filters[static_cast<size_t>(slot)];
        auto& last = lastPublished[static_cast<size_t>(slot)];

        if (! m.valid)
        {
            publishLost(slot);
            return;
        }

        last.thumbIndex      = f.thumbIndex.process(m.thumbIndex01, ts);
        last.thumbIndexAngle = f.thumbIndexAngle.process(m.thumbIndexAngle01, ts);
        last.thumbPinky      = f.thumbPinky.process(m.thumbPinky01, ts);
        last.thumbPinkyAngle = f.thumbPinkyAngle.process(m.thumbPinkyAngle01, ts);

        const int baseId = slot * 4;
        bridge.publish(static_cast<MeasurementId>(baseId + 0), last.thumbIndex);
        bridge.publish(static_cast<MeasurementId>(baseId + 1), last.thumbIndexAngle);
        bridge.publish(static_cast<MeasurementId>(baseId + 2), last.thumbPinky);
        bridge.publish(static_cast<MeasurementId>(baseId + 3), last.thumbPinkyAngle);
    }

    void HandTrackerThread::publishLost(int slot)
    {
        if (holdOnLost.load(std::memory_order_relaxed))
            return;

        auto& f = filters[static_cast<size_t>(slot)];
        f.thumbIndex.reset();
        f.thumbIndexAngle.reset();
        f.thumbPinky.reset();
        f.thumbPinkyAngle.reset();

        auto& last = lastPublished[static_cast<size_t>(slot)];
        last = {};

        const int baseId = slot * 4;
        bridge.publish(static_cast<MeasurementId>(baseId + 0), 0.0f);
        bridge.publish(static_cast<MeasurementId>(baseId + 1), 0.0f);
        bridge.publish(static_cast<MeasurementId>(baseId + 2), 0.0f);
        bridge.publish(static_cast<MeasurementId>(baseId + 3), 0.0f);
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
        return copy;
    }
}
