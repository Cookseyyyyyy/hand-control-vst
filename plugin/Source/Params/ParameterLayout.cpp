#include "ParameterLayout.h"
#include "ParameterIDs.h"

namespace handcontrol::params
{
    static juce::String toId(std::string_view sv)
    {
        return juce::String(sv.data(), sv.size());
    }

    static std::unique_ptr<juce::AudioParameterFloat> makeMeasurement(std::string_view id,
                                                                     const juce::String& label)
    {
        return std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { toId(id), 1 },
            label,
            juce::NormalisableRange<float> { 0.0f, 1.0f, 0.0f, 1.0f },
            0.0f,
            juce::AudioParameterFloatAttributes {}.withStringFromValueFunction(
                [](float v, int) { return juce::String(v, 3); }));
    }

    juce::AudioProcessorValueTreeState::ParameterLayout createLayout()
    {
        juce::AudioProcessorValueTreeState::ParameterLayout layout;

        layout.add(makeMeasurement(h1ThumbIndexDistance, "Thumb-Index Distance"));
        layout.add(makeMeasurement(h1ThumbIndexAngle,    "Thumb-Index Angle"));
        layout.add(makeMeasurement(h1ThumbPinkyDistance, "Thumb-Pinky Distance"));
        layout.add(makeMeasurement(h1ThumbPinkyAngle,    "Thumb-Pinky Angle"));
        layout.add(makeMeasurement(h1HandX,    "Hand X"));
        layout.add(makeMeasurement(h1HandY,    "Hand Y"));
        layout.add(makeMeasurement(h1Openness, "Openness"));

        layout.add(std::make_unique<juce::AudioParameterInt>(
            juce::ParameterID { toId(cameraIndex), 1 },
            "Camera",
            0, 9, 0));

        layout.add(std::make_unique<juce::AudioParameterBool>(
            juce::ParameterID { toId(previewVisible), 1 },
            "Preview Visible",
            true));

        layout.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { toId(smoothing), 1 },
            "Smoothing",
            juce::NormalisableRange<float> { 0.0f, 1.0f, 0.0f, 1.0f },
            // Default is now 0 (very light) - per-landmark filters at the
            // previous 0.35 default added perceptible lag. Users can dial in
            // more smoothing if they need it.
            0.0f));

        layout.add(std::make_unique<juce::AudioParameterBool>(
            juce::ParameterID { toId(holdOnLost), 1 },
            "Hold On Lost Hand",
            true));

        layout.add(std::make_unique<juce::AudioParameterBool>(
            juce::ParameterID { toId(bypass), 1 },
            "Bypass",
            false));

        layout.add(std::make_unique<juce::AudioParameterBool>(
            juce::ParameterID { toId(mirrorCamera), 1 },
            "Mirror Camera",
            true));

        layout.add(std::make_unique<juce::AudioParameterBool>(
            juce::ParameterID { toId(roiOverlay), 1 },
            "Show ROI Overlay",
            false));

        // ---- Per-measurement MIDI CC config (non-automatable) --------------
        for (int i = 0; i < numMeasurements; ++i)
        {
            const auto id = static_cast<MeasurementId>(i);

            layout.add(std::make_unique<juce::AudioParameterInt>(
                juce::ParameterID { midiChannelId(id), 1 },
                "MIDI Ch " + juce::String(i + 1),
                1, 16, defaultMidiChannel,
                juce::AudioParameterIntAttributes {}.withAutomatable(false)));

            layout.add(std::make_unique<juce::AudioParameterInt>(
                juce::ParameterID { midiCcNumberId(id), 1 },
                "MIDI CC " + juce::String(i + 1),
                0, 127, defaultMidiCcBase + i,
                juce::AudioParameterIntAttributes {}.withAutomatable(false)));

            layout.add(std::make_unique<juce::AudioParameterBool>(
                juce::ParameterID { midiEnabledId(id), 1 },
                "MIDI On " + juce::String(i + 1),
                true,
                juce::AudioParameterBoolAttributes {}.withAutomatable(false)));
        }

        return layout;
    }
}
