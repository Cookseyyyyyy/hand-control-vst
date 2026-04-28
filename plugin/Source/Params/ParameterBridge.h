#pragma once

#include "ParameterIDs.h"

#include <juce_audio_processors/juce_audio_processors.h>

#include <array>
#include <atomic>

namespace handcontrol::params
{
    /**
        Lock-free bridge between the tracking thread (producer) and the audio thread
        (which owns the DAW-visible parameters).

        The tracking thread publishes normalized [0,1] values via `publish()`. The audio
        thread calls `flushToParameters()` once per block to push those values into the
        associated `AudioParameterFloat`s so the DAW sees automation-rate changes.

        A version counter lets the audio thread skip work when nothing changed.
    */
    class ParameterBridge
    {
    public:
        explicit ParameterBridge(juce::AudioProcessorValueTreeState& apvtsRef);

        void publish(MeasurementId id, float value01) noexcept;

        /** Copies any newly published shadow values into the APVTS parameters.
            Safe to call from the audio thread. */
        void flushToParameters() noexcept;

        /** Snapshot of the last published value for UI display. Safe from any thread. */
        float snapshot(MeasurementId id) const noexcept;

    private:
        juce::AudioProcessorValueTreeState& apvts;
        std::array<std::atomic<float>, numMeasurements> shadows {};
        std::array<juce::AudioParameterFloat*, numMeasurements> cachedParams {};
        std::atomic<uint32_t> publishVersion { 0 };
        uint32_t lastFlushedVersion { 0 };
    };
}
