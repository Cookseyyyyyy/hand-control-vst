#include "MidiCcSender.h"

#include <algorithm>
#include <cmath>

namespace handcontrol::midi
{
    namespace
    {
        constexpr int kNeverSent = -1;
    }

    MidiCcSender::MidiCcSender() noexcept
    {
        // Apply the same defaults the parameter layout uses, so the sender is
        // immediately usable before any APVTS sync happens (e.g. in unit tests).
        for (int i = 0; i < handcontrol::params::numMeasurements; ++i)
        {
            mappings[static_cast<size_t>(i)] = Mapping {
                handcontrol::params::defaultMidiChannel,
                handcontrol::params::defaultMidiCcBase + i,
                true
            };
        }
        resetLastSent();
    }

    int MidiCcSender::toCcValue(float value01) noexcept
    {
        const float v = std::clamp(value01, 0.0f, 1.0f);
        return static_cast<int>(std::lround(v * 127.0f));
    }

    void MidiCcSender::setMapping(handcontrol::params::MeasurementId id, Mapping m) noexcept
    {
        const auto idx = static_cast<size_t>(id);
        if (idx >= mappings.size()) return;
        const auto clamped = Mapping {
            std::clamp(m.channel, 1, 16),
            std::clamp(m.ccNumber, 0, 127),
            m.enabled
        };
        // If the destination changed, force the next emit to fire even if the
        // 7-bit value is the same as before.
        const auto& prev = mappings[idx];
        if (prev.channel != clamped.channel
            || prev.ccNumber != clamped.ccNumber
            || prev.enabled != clamped.enabled)
        {
            lastSent[idx] = kNeverSent;
        }
        mappings[idx] = clamped;
    }

    void MidiCcSender::emitIfChanged(handcontrol::params::MeasurementId id,
                                     float value01,
                                     juce::MidiBuffer& outBuffer,
                                     int sampleNumber) noexcept
    {
        const auto idx = static_cast<size_t>(id);
        if (idx >= mappings.size()) return;

        const auto& m = mappings[idx];
        if (! m.enabled) return;

        const int newCc = toCcValue(value01);
        if (newCc == lastSent[idx]) return;

        lastSent[idx] = newCc;
        outBuffer.addEvent(juce::MidiMessage::controllerEvent(m.channel, m.ccNumber, newCc),
                           sampleNumber);
    }

    void MidiCcSender::resetLastSent() noexcept
    {
        std::fill(lastSent.begin(), lastSent.end(), kNeverSent);
    }
}
