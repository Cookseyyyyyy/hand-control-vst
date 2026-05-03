#pragma once

#include "PluginProcessor.h"

#include "../UI/ParameterMeter.h"
#include "../UI/PreviewComponent.h"
#include "../UI/StatusBar.h"
#include "../UI/Theme.h"

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include <memory>
#include <vector>

namespace handcontrol
{
    class PluginEditor : public juce::AudioProcessorEditor
    {
    public:
        explicit PluginEditor(PluginProcessor& p);
        ~PluginEditor() override;

        void paint(juce::Graphics& g) override;
        void resized() override;

    private:
        void buildControls();
        void populateCameraBox();

        PluginProcessor& processor;
        ui::Theme theme;

        ui::PreviewComponent preview;
        ui::StatusBar statusBar;

        std::vector<std::unique_ptr<ui::ParameterMeter>> meters;

        juce::Label title;
        juce::Label subtitle;

        juce::ComboBox cameraBox;
        juce::Label cameraLabel { {}, "Camera" };

        juce::ToggleButton previewToggle { "Preview" };
        juce::ToggleButton holdToggle    { "Hold On Lost" };
        juce::ToggleButton mirrorToggle  { "Mirror" };
        juce::ToggleButton roiToggle     { "Show ROI" };

        juce::Slider smoothingSlider;
        juce::Label smoothingLabel { {}, "Smoothing" };

        using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
        using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;
        using ComboAttachment  = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

        std::unique_ptr<SliderAttachment> smoothingAttach;
        std::unique_ptr<ButtonAttachment> previewAttach;
        std::unique_ptr<ButtonAttachment> holdAttach;
        std::unique_ptr<ButtonAttachment> mirrorAttach;
        std::unique_ptr<ButtonAttachment> roiAttach;
        std::unique_ptr<ComboAttachment>  cameraAttach;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginEditor)
    };
}
