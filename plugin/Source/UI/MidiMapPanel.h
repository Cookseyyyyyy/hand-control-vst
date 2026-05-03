#pragma once

#include "../Params/ParameterIDs.h"

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include <array>
#include <memory>

namespace handcontrol::ui
{
    /** A 14-row table that lets the user assign a MIDI channel + CC number to
        each measurement parameter, and toggle sending on or off. Edits are
        bound to the underlying APVTS so they persist with the project.

        Lives next to the meter grid in the editor; visibility is toggled by
        the "MIDI Map" button. */
    class MidiMapPanel : public juce::Component
    {
    public:
        explicit MidiMapPanel(juce::AudioProcessorValueTreeState& vts);
        ~MidiMapPanel() override = default;

        void paint(juce::Graphics& g) override;
        void resized() override;

    private:
        struct Row
        {
            handcontrol::params::MeasurementId id {};
            juce::String label;
            juce::Label  nameLabel;
            juce::Slider channelSlider;
            juce::Slider ccSlider;
            juce::ToggleButton enabledToggle;

            using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
            using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

            std::unique_ptr<SliderAttachment> channelAttach;
            std::unique_ptr<SliderAttachment> ccAttach;
            std::unique_ptr<ButtonAttachment> enabledAttach;
        };

        juce::AudioProcessorValueTreeState& apvts;
        std::array<std::unique_ptr<Row>, handcontrol::params::numMeasurements> rows;

        juce::Label headerLabel;
        juce::Label headerName;
        juce::Label headerChannel;
        juce::Label headerCc;
        juce::Label headerEnabled;
        juce::Label hintLabel;

        void buildRow(int rowIndex);
    };
}
