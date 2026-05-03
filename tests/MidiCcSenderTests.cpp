#include "TestMain.h"

#include "Midi/MidiCcSender.h"
#include "Params/ParameterIDs.h"

using namespace handcontrol::midi;
using handcontrol::params::MeasurementId;

namespace
{
    // Convenience: pull all CC events out of a buffer for assertion.
    struct Cc { int channel; int cc; int value; int sample; };
    std::vector<Cc> ccEvents(const juce::MidiBuffer& buf)
    {
        std::vector<Cc> out;
        for (auto m : buf)
        {
            const auto& msg = m.getMessage();
            if (! msg.isController()) continue;
            out.push_back({ msg.getChannel(), msg.getControllerNumber(), msg.getControllerValue(), m.samplePosition });
        }
        return out;
    }
}

HC_TEST(midi_first_value_emits_event)
{
    MidiCcSender sender;
    juce::MidiBuffer buf;
    sender.emitIfChanged(MeasurementId::hand1ThumbIndexDistance, 0.5f, buf, 0);
    const auto evs = ccEvents(buf);
    HC_CHECK(evs.size() == 1);
    HC_CHECK(evs[0].channel == 1);          // default channel
    HC_CHECK(evs[0].cc == 20);              // first measurement default CC
    HC_CHECK(evs[0].value == 64);           // 0.5 -> ~64 on 0..127
}

HC_TEST(midi_unchanged_value_does_not_re_emit)
{
    MidiCcSender sender;
    juce::MidiBuffer buf;
    sender.emitIfChanged(MeasurementId::hand1ThumbIndexDistance, 0.5f, buf);
    sender.emitIfChanged(MeasurementId::hand1ThumbIndexDistance, 0.5f, buf);
    const auto evs = ccEvents(buf);
    HC_CHECK(evs.size() == 1);  // only the first call emits
}

HC_TEST(midi_sub_lsb_change_does_not_re_emit)
{
    // 0.500 and 0.503 both round to CC value 64. Should not produce two events.
    MidiCcSender sender;
    juce::MidiBuffer buf;
    sender.emitIfChanged(MeasurementId::hand1ThumbIndexDistance, 0.500f, buf);
    sender.emitIfChanged(MeasurementId::hand1ThumbIndexDistance, 0.503f, buf);
    const auto evs = ccEvents(buf);
    HC_CHECK(evs.size() == 1);
}

HC_TEST(midi_meaningful_change_re_emits)
{
    MidiCcSender sender;
    juce::MidiBuffer buf;
    sender.emitIfChanged(MeasurementId::hand1ThumbIndexDistance, 0.0f, buf);
    sender.emitIfChanged(MeasurementId::hand1ThumbIndexDistance, 1.0f, buf);
    const auto evs = ccEvents(buf);
    HC_CHECK(evs.size() == 2);
    HC_CHECK(evs[0].value == 0);
    HC_CHECK(evs[1].value == 127);
}

HC_TEST(midi_disabled_mapping_emits_nothing)
{
    MidiCcSender sender;
    sender.setMapping(MeasurementId::hand1ThumbIndexDistance,
                      MidiCcSender::Mapping { 1, 20, false });
    juce::MidiBuffer buf;
    sender.emitIfChanged(MeasurementId::hand1ThumbIndexDistance, 0.7f, buf);
    HC_CHECK(buf.isEmpty());
}

HC_TEST(midi_uses_configured_channel_and_cc)
{
    MidiCcSender sender;
    sender.setMapping(MeasurementId::hand2HandY,
                      MidiCcSender::Mapping { 9, 74, true });
    juce::MidiBuffer buf;
    sender.emitIfChanged(MeasurementId::hand2HandY, 0.25f, buf, 17);
    const auto evs = ccEvents(buf);
    HC_CHECK(evs.size() == 1);
    HC_CHECK(evs[0].channel == 9);
    HC_CHECK(evs[0].cc == 74);
    HC_CHECK(evs[0].sample == 17);
}

HC_TEST(midi_remapping_destination_re_emits_immediately)
{
    // After the user changes channel or CC, the sender should fire a fresh
    // event on the next emit so the new target sees the current value.
    MidiCcSender sender;
    juce::MidiBuffer buf;
    sender.emitIfChanged(MeasurementId::hand1ThumbIndexDistance, 0.5f, buf);
    sender.setMapping(MeasurementId::hand1ThumbIndexDistance,
                      MidiCcSender::Mapping { 5, 41, true });
    sender.emitIfChanged(MeasurementId::hand1ThumbIndexDistance, 0.5f, buf);
    const auto evs = ccEvents(buf);
    HC_CHECK(evs.size() == 2);
    HC_CHECK(evs[1].channel == 5);
    HC_CHECK(evs[1].cc == 41);
}

HC_TEST(midi_to_cc_value_clamps_and_rounds)
{
    HC_CHECK(MidiCcSender::toCcValue(0.0f)   == 0);
    HC_CHECK(MidiCcSender::toCcValue(1.0f)   == 127);
    HC_CHECK(MidiCcSender::toCcValue(0.5f)   == 64);
    HC_CHECK(MidiCcSender::toCcValue(-0.5f)  == 0);    // clamped low
    HC_CHECK(MidiCcSender::toCcValue(2.0f)   == 127);  // clamped high
}
