#include "PluginEditor.h"

#include "../Params/ParameterIDs.h"
#include "../Tracking/CameraSource.h"

namespace handcontrol
{
    namespace
    {
        juce::String pid(std::string_view sv)
        {
            return juce::String(sv.data(), sv.size());
        }
    }

    PluginEditor::PluginEditor(PluginProcessor& p)
        : juce::AudioProcessorEditor(&p),
          processor(p),
          preview(processor.getTracker()),
          statusBar(processor.getTracker()),
          midiMapPanel(processor.getValueTreeState())
    {
        setLookAndFeel(&theme);
        setResizable(true, true);
        setResizeLimits(820, 540, 1800, 1300);
        setSize(900, 660);

        buildControls();
        processor.restartTracker();

        title.setText("Hand Control", juce::dontSendNotification);
        title.setFont(juce::Font(juce::FontOptions(20.0f, juce::Font::bold)));
        title.setColour(juce::Label::textColourId, ui::colours::text);
        addAndMakeVisible(title);

        subtitle.setText(juce::String("v") + JUCE_STRINGIFY(JucePlugin_VersionString)
                             + "  \u2022  Right-click any parameter in Ableton to Map",
                         juce::dontSendNotification);
        subtitle.setFont(juce::Font(juce::FontOptions(12.0f, juce::Font::plain)));
        subtitle.setColour(juce::Label::textColourId, ui::colours::textDim);
        addAndMakeVisible(subtitle);

        addAndMakeVisible(preview);
        addAndMakeVisible(statusBar);
        addChildComponent(midiMapPanel);  // hidden by default

        struct MeterSpec
        {
            juce::String label;
            handcontrol::params::MeasurementId id;
            juce::Colour accent;
        };
        const std::array<MeterSpec, handcontrol::params::numMeasurements> specs {{
            { "Thumb-Index Dist",  params::MeasurementId::thumbIndexDistance, ui::colours::accentIndex },
            { "Thumb-Index Angle", params::MeasurementId::thumbIndexAngle,    ui::colours::accentIndex },
            { "Thumb-Pinky Dist",  params::MeasurementId::thumbPinkyDistance, ui::colours::accentPinky },
            { "Thumb-Pinky Angle", params::MeasurementId::thumbPinkyAngle,    ui::colours::accentPinky },
            { "Hand X",            params::MeasurementId::handX,              ui::colours::accentHand1 },
            { "Hand Y",            params::MeasurementId::handY,              ui::colours::accentHand1 },
            { "Openness",          params::MeasurementId::openness,           ui::colours::accentHand1 }
        }};
        for (const auto& spec : specs)
        {
            auto m = std::make_unique<ui::ParameterMeter>(spec.label, processor.getBridge(), spec.id, spec.accent);
            addAndMakeVisible(*m);
            meters.push_back(std::move(m));
        }
    }

    PluginEditor::~PluginEditor()
    {
        setLookAndFeel(nullptr);
    }

    void PluginEditor::buildControls()
    {
        populateCameraBox();
        cameraBox.setJustificationType(juce::Justification::centredLeft);
        addAndMakeVisible(cameraBox);
        cameraLabel.setFont(juce::Font(juce::FontOptions(12.0f, juce::Font::plain)));
        cameraLabel.setColour(juce::Label::textColourId, ui::colours::textDim);
        addAndMakeVisible(cameraLabel);

        addAndMakeVisible(previewToggle);
        addAndMakeVisible(holdToggle);
        addAndMakeVisible(mirrorToggle);
        addAndMakeVisible(roiToggle);
        addAndMakeVisible(midiMapButton);
        midiMapButton.setClickingTogglesState(true);
        midiMapButton.onClick = [this]()
        {
            midiMapVisible = midiMapButton.getToggleState();
            midiMapPanel.setVisible(midiMapVisible);
            for (auto& m : meters) m->setVisible(! midiMapVisible);
            resized();
        };

        smoothingSlider.setSliderStyle(juce::Slider::LinearHorizontal);
        smoothingSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 52, 18);
        smoothingSlider.setColour(juce::Slider::textBoxTextColourId, ui::colours::text);
        smoothingSlider.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        smoothingSlider.setNumDecimalPlacesToDisplay(2);
        addAndMakeVisible(smoothingSlider);
        smoothingLabel.setFont(juce::Font(juce::FontOptions(12.0f, juce::Font::plain)));
        smoothingLabel.setColour(juce::Label::textColourId, ui::colours::textDim);
        addAndMakeVisible(smoothingLabel);

        auto& vts = processor.getValueTreeState();
        cameraAttach    = std::make_unique<ComboAttachment>(vts, pid(params::cameraIndex), cameraBox);
        previewAttach   = std::make_unique<ButtonAttachment>(vts, pid(params::previewVisible), previewToggle);
        holdAttach      = std::make_unique<ButtonAttachment>(vts, pid(params::holdOnLost),     holdToggle);
        mirrorAttach    = std::make_unique<ButtonAttachment>(vts, pid(params::mirrorCamera),   mirrorToggle);
        roiAttach       = std::make_unique<ButtonAttachment>(vts, pid(params::roiOverlay),     roiToggle);
        smoothingAttach = std::make_unique<SliderAttachment>(vts, pid(params::smoothing),      smoothingSlider);

        previewToggle.onStateChange = [this]()
        {
            preview.setVisible(previewToggle.getToggleState());
            resized();
        };
        roiToggle.onStateChange = [this]()
        {
            preview.setShowRoi(roiToggle.getToggleState());
        };
        preview.setShowRoi(roiToggle.getToggleState());
    }

    void PluginEditor::populateCameraBox()
    {
        cameraBox.clear(juce::dontSendNotification);
        auto devices = tracking::CameraSource::enumerateDevices();
        if (devices.isEmpty())
        {
            for (int i = 0; i < 10; ++i)
                cameraBox.addItem("Camera " + juce::String(i), i + 1);
        }
        else
        {
            for (int i = 0; i < devices.size(); ++i)
                cameraBox.addItem(devices[i], i + 1);
        }
    }

    void PluginEditor::paint(juce::Graphics& g)
    {
        g.fillAll(ui::colours::background);
    }

    void PluginEditor::resized()
    {
        auto bounds = getLocalBounds().reduced(14);

        auto headerRow = bounds.removeFromTop(36);
        title.setBounds(headerRow.removeFromLeft(180));
        subtitle.setBounds(headerRow.withTrimmedLeft(4));

        bounds.removeFromTop(6);

        auto statusRow = bounds.removeFromBottom(26);
        statusBar.setBounds(statusRow);
        bounds.removeFromBottom(6);

        auto controls2 = bounds.removeFromBottom(32);
        smoothingLabel.setBounds(controls2.removeFromLeft(80));
        smoothingSlider.setBounds(controls2.reduced(4, 6));

        auto controls1 = bounds.removeFromBottom(32);
        cameraLabel.setBounds(controls1.removeFromLeft(58));
        controls1.removeFromLeft(4);
        cameraBox.setBounds(controls1.removeFromLeft(220).reduced(0, 4));
        controls1.removeFromLeft(12);
        previewToggle.setBounds(controls1.removeFromLeft(110));
        mirrorToggle.setBounds (controls1.removeFromLeft(100));
        holdToggle.setBounds   (controls1.removeFromLeft(140));
        roiToggle.setBounds    (controls1.removeFromLeft(110));
        controls1.removeFromLeft(4);
        midiMapButton.setBounds(controls1.removeFromLeft(110).reduced(0, 4));
        bounds.removeFromBottom(8);

        const bool showPreview = previewToggle.getToggleState();
        // Single row of 7 meters (was 2x7 in v0.3).
        // Bottom area is smaller now since we only need one row.
        auto bottomArea = showPreview
            ? bounds.removeFromBottom(juce::jmax(110, bounds.getHeight() / 4))
            : bounds;

        if (showPreview)
        {
            bounds.removeFromBottom(10);
            preview.setBounds(bounds);
        }
        preview.setVisible(showPreview);

        if (midiMapVisible)
        {
            midiMapPanel.setBounds(bottomArea);
        }
        else
        {
            const int cols = handcontrol::params::numMeasurements;
            const int cellW = bottomArea.getWidth() / cols;
            for (int c = 0; c < cols; ++c)
            {
                const auto idx = static_cast<size_t>(c);
                if (idx < meters.size())
                    meters[idx]->setBounds(bottomArea.getX() + c * cellW,
                                           bottomArea.getY(),
                                           cellW - 4,
                                           bottomArea.getHeight() - 4);
            }
        }
    }
}
