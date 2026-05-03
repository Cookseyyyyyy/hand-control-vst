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

        // v0.1 originals
        layout.add(makeMeasurement(h1ThumbIndexDistance, "H1 Thumb-Index Distance"));
        layout.add(makeMeasurement(h1ThumbIndexAngle,    "H1 Thumb-Index Angle"));
        layout.add(makeMeasurement(h1ThumbPinkyDistance, "H1 Thumb-Pinky Distance"));
        layout.add(makeMeasurement(h1ThumbPinkyAngle,    "H1 Thumb-Pinky Angle"));
        layout.add(makeMeasurement(h2ThumbIndexDistance, "H2 Thumb-Index Distance"));
        layout.add(makeMeasurement(h2ThumbIndexAngle,    "H2 Thumb-Index Angle"));
        layout.add(makeMeasurement(h2ThumbPinkyDistance, "H2 Thumb-Pinky Distance"));
        layout.add(makeMeasurement(h2ThumbPinkyAngle,    "H2 Thumb-Pinky Angle"));

        // v0.2 new measurements
        layout.add(makeMeasurement(h1HandX,    "H1 Hand X"));
        layout.add(makeMeasurement(h1HandY,    "H1 Hand Y"));
        layout.add(makeMeasurement(h1Openness, "H1 Openness"));
        layout.add(makeMeasurement(h2HandX,    "H2 Hand X"));
        layout.add(makeMeasurement(h2HandY,    "H2 Hand Y"));
        layout.add(makeMeasurement(h2Openness, "H2 Openness"));

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
            0.35f));

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

        return layout;
    }
}
