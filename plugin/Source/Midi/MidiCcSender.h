#pragma once

#include "../Params/ParameterIDs.h"

#include <juce_audio_basics/juce_audio_basics.h>

#include <array>

namespace handcontrol::midi
{
    /** Generates MIDI CC events from the 14 normalised measurement values
        published by the tracker.

        For each measurement we hold a per-instance Mapping (channel, CC#,
        enabled) and the last-sent 7-bit CC value. A new event is only emitted
        when the 7-bit-quantised value actually changes, so a slowly drifting
        measurement won't spam the buffer with redundant CCs.

        Designed to be called once per audio block from `processBlock`. All
        events are placed at sample 0 of the block, since the tracker thread
        runs at ~30 Hz and there's no value in trying to interpolate within a
        ~5-10ms audio buffer. */
    class MidiCcSender
    {
    public:
        struct Mapping
        {
            int  channel  { 1 };       // 1..16
            int  ccNumber { 0 };       // 0..127
            bool enabled  { true };
        };

        MidiCcSender() noexcept;

        /** Replace the mapping for one measurement. Safe from any thread. */
        void setMapping(handcontrol::params::MeasurementId id, Mapping m) noexcept;

        /** Compares `value01` against the last sent 7-bit CC value for this
            measurement. If different (and the mapping is enabled), writes a
            controller-change event into `outBuffer` at the given sample. */
        void emitIfChanged(handcontrol::params::MeasurementId id,
                           float value01,
                           juce::MidiBuffer& outBuffer,
                           int sampleNumber = 0) noexcept;

        /** Forces every enabled mapping to re-emit its current value on the
            next `emitIfChanged`. Use this after a project load so the host can
            see the values immediately. */
        void resetLastSent() noexcept;

        /** Quantise [0,1] to a 0..127 integer. Public for testing. */
        static int toCcValue(float value01) noexcept;

    private:
        std::array<Mapping, handcontrol::params::numMeasurements> mappings;
        std::array<int,     handcontrol::params::numMeasurements> lastSent;
    };
}
