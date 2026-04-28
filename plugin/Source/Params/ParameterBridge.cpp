#include "ParameterBridge.h"

namespace handcontrol::params
{
    ParameterBridge::ParameterBridge(juce::AudioProcessorValueTreeState& apvtsRef)
        : apvts(apvtsRef)
    {
        for (int i = 0; i < numMeasurements; ++i)
        {
            shadows[static_cast<size_t>(i)].store(0.0f, std::memory_order_relaxed);

            auto id = juce::String(measurementIds[static_cast<size_t>(i)].data(),
                                   measurementIds[static_cast<size_t>(i)].size());
            auto* param = apvts.getParameter(id);
            cachedParams[static_cast<size_t>(i)] =
                dynamic_cast<juce::AudioParameterFloat*>(param);
            jassert(cachedParams[static_cast<size_t>(i)] != nullptr);
        }
    }

    void ParameterBridge::publish(MeasurementId id, float value01) noexcept
    {
        const auto idx = static_cast<size_t>(id);
        jassert(idx < shadows.size());
        shadows[idx].store(juce::jlimit(0.0f, 1.0f, value01), std::memory_order_release);
        publishVersion.fetch_add(1, std::memory_order_release);
    }

    void ParameterBridge::flushToParameters() noexcept
    {
        const auto current = publishVersion.load(std::memory_order_acquire);
        if (current == lastFlushedVersion)
            return;

        for (int i = 0; i < numMeasurements; ++i)
        {
            auto* p = cachedParams[static_cast<size_t>(i)];
            if (p == nullptr)
                continue;

            const auto v = shadows[static_cast<size_t>(i)].load(std::memory_order_acquire);
            if (std::abs(p->get() - v) > 1.0e-5f)
                p->setValueNotifyingHost(v);
        }

        lastFlushedVersion = current;
    }

    float ParameterBridge::snapshot(MeasurementId id) const noexcept
    {
        return shadows[static_cast<size_t>(id)].load(std::memory_order_acquire);
    }
}
