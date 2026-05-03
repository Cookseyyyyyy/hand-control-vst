#include "MidiMapPanel.h"
#include "Theme.h"

namespace handcontrol::ui
{
    namespace
    {
        constexpr int kRowHeight    = 24;
        constexpr int kHeaderHeight = 22;
        constexpr int kHintHeight   = 26;
        constexpr int kPadding      = 8;

        constexpr int kColNameWidth     = 200;
        constexpr int kColChannelWidth  = 80;
        constexpr int kColCcWidth       = 80;
        constexpr int kColEnabledWidth  = 70;

        const std::array<juce::String, handcontrol::params::numMeasurements> kRowLabels {
            "Thumb-Index Distance", "Thumb-Index Angle",
            "Thumb-Pinky Distance", "Thumb-Pinky Angle",
            "Hand X", "Hand Y", "Openness"
        };
    }

    MidiMapPanel::MidiMapPanel(juce::AudioProcessorValueTreeState& vts)
        : apvts(vts)
    {
        setOpaque(false);

        auto setupHeader = [this](juce::Label& l, const juce::String& text)
        {
            l.setText(text, juce::dontSendNotification);
            l.setFont(juce::Font(juce::FontOptions(11.0f, juce::Font::bold)));
            l.setColour(juce::Label::textColourId, colours::textDim);
            addAndMakeVisible(l);
        };
        headerLabel.setText("MIDI Output", juce::dontSendNotification);
        headerLabel.setFont(juce::Font(juce::FontOptions(14.0f, juce::Font::bold)));
        headerLabel.setColour(juce::Label::textColourId, colours::text);
        addAndMakeVisible(headerLabel);

        setupHeader(headerName,    "Parameter");
        setupHeader(headerChannel, "Channel");
        setupHeader(headerCc,      "CC #");
        setupHeader(headerEnabled, "Send");

        hintLabel.setText("In Ableton, press Cmd/Ctrl+M, click any plugin parameter, "
                          "then move your hand to assign that CC.",
                          juce::dontSendNotification);
        hintLabel.setFont(juce::Font(juce::FontOptions(11.5f, juce::Font::plain)));
        hintLabel.setColour(juce::Label::textColourId, colours::textDim);
        hintLabel.setJustificationType(juce::Justification::centredLeft);
        addAndMakeVisible(hintLabel);

        for (int i = 0; i < handcontrol::params::numMeasurements; ++i)
            buildRow(i);
    }

    void MidiMapPanel::buildRow(int rowIndex)
    {
        auto row = std::make_unique<Row>();
        row->id    = static_cast<handcontrol::params::MeasurementId>(rowIndex);
        row->label = kRowLabels[static_cast<size_t>(rowIndex)];

        row->nameLabel.setText(row->label, juce::dontSendNotification);
        row->nameLabel.setFont(juce::Font(juce::FontOptions(12.5f, juce::Font::plain)));
        row->nameLabel.setColour(juce::Label::textColourId, colours::text);
        addAndMakeVisible(row->nameLabel);

        auto setupIntSlider = [](juce::Slider& s, int minV, int maxV)
        {
            s.setSliderStyle(juce::Slider::IncDecButtons);
            s.setRange(minV, maxV, 1);
            s.setIncDecButtonsMode(juce::Slider::incDecButtonsDraggable_Vertical);
            s.setTextBoxStyle(juce::Slider::TextBoxLeft, false, 40, 20);
            s.setColour(juce::Slider::textBoxTextColourId, colours::text);
            s.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        };
        setupIntSlider(row->channelSlider, 1, 16);
        setupIntSlider(row->ccSlider,      0, 127);
        addAndMakeVisible(row->channelSlider);
        addAndMakeVisible(row->ccSlider);

        addAndMakeVisible(row->enabledToggle);

        row->channelAttach = std::make_unique<Row::SliderAttachment>(
            apvts, handcontrol::params::midiChannelId(row->id), row->channelSlider);
        row->ccAttach = std::make_unique<Row::SliderAttachment>(
            apvts, handcontrol::params::midiCcNumberId(row->id), row->ccSlider);
        row->enabledAttach = std::make_unique<Row::ButtonAttachment>(
            apvts, handcontrol::params::midiEnabledId(row->id), row->enabledToggle);

        rows[static_cast<size_t>(rowIndex)] = std::move(row);
    }

    void MidiMapPanel::paint(juce::Graphics& g)
    {
        const auto bounds = getLocalBounds().toFloat();
        drawPanel(g, bounds, 8.0f);
    }

    void MidiMapPanel::resized()
    {
        auto bounds = getLocalBounds().reduced(kPadding);

        headerLabel.setBounds(bounds.removeFromTop(22));
        bounds.removeFromTop(4);

        // Column headers row
        {
            auto headerRow = bounds.removeFromTop(kHeaderHeight);
            headerName.setBounds(headerRow.removeFromLeft(kColNameWidth));
            headerRow.removeFromLeft(8);
            headerChannel.setBounds(headerRow.removeFromLeft(kColChannelWidth));
            headerRow.removeFromLeft(8);
            headerCc.setBounds(headerRow.removeFromLeft(kColCcWidth));
            headerRow.removeFromLeft(8);
            headerEnabled.setBounds(headerRow.removeFromLeft(kColEnabledWidth));
        }
        bounds.removeFromTop(2);

        // Rows
        for (auto& r : rows)
        {
            if (! r) continue;
            auto rowArea = bounds.removeFromTop(kRowHeight);
            r->nameLabel.setBounds(rowArea.removeFromLeft(kColNameWidth));
            rowArea.removeFromLeft(8);
            r->channelSlider.setBounds(rowArea.removeFromLeft(kColChannelWidth));
            rowArea.removeFromLeft(8);
            r->ccSlider.setBounds(rowArea.removeFromLeft(kColCcWidth));
            rowArea.removeFromLeft(8);
            r->enabledToggle.setBounds(rowArea.removeFromLeft(kColEnabledWidth));
            bounds.removeFromTop(2);
        }

        bounds.removeFromTop(6);
        hintLabel.setBounds(bounds.removeFromTop(kHintHeight));
    }
}
