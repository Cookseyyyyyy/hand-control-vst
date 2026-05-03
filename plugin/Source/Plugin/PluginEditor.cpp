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
          statusBar(processor.getTracker())
    {
        setLookAndFeel(&theme);
        setResizable(true, true);
        setResizeLimits(880, 620, 1800, 1300);
        setSize(980, 760);

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

        struct MeterSpec
        {
            juce::String label;
            handcontrol::params::MeasurementId id;
            juce::Colour accent;
        };

        // Layout: 2 rows (per hand) x 7 columns. Original 4 + 3 new per hand.
        const std::array<MeterSpec, 14> specs {{
            // Row 1: Hand 1
            { "H1 Thumb-Index Dist",  params::MeasurementId::hand1ThumbIndexDistance, ui::colours::accentIndex },
            { "H1 Thumb-Index Angle", params::MeasurementId::hand1ThumbIndexAngle,    ui::colours::accentIndex },
            { "H1 Thumb-Pinky Dist",  params::MeasurementId::hand1ThumbPinkyDistance, ui::colours::accentPinky },
            { "H1 Thumb-Pinky Angle", params::MeasurementId::hand1ThumbPinkyAngle,    ui::colours::accentPinky },
            { "H1 Hand X",            params::MeasurementId::hand1HandX,              ui::colours::accentHand1 },
            { "H1 Hand Y",            params::MeasurementId::hand1HandY,              ui::colours::accentHand1 },
            { "H1 Openness",          params::MeasurementId::hand1Openness,           ui::colours::accentHand1 },
            // Row 2: Hand 2
            { "H2 Thumb-Index Dist",  params::MeasurementId::hand2ThumbIndexDistance, ui::colours::accentIndex },
            { "H2 Thumb-Index Angle", params::MeasurementId::hand2ThumbIndexAngle,    ui::colours::accentIndex },
            { "H2 Thumb-Pinky Dist",  params::MeasurementId::hand2ThumbPinkyDistance, ui::colours::accentPinky },
            { "H2 Thumb-Pinky Angle", params::MeasurementId::hand2ThumbPinkyAngle,    ui::colours::accentPinky },
            { "H2 Hand X",            params::MeasurementId::hand2HandX,              ui::colours::accentHand2 },
            { "H2 Hand Y",            params::MeasurementId::hand2HandY,              ui::colours::accentHand2 },
            { "H2 Openness",          params::MeasurementId::hand2Openness,           ui::colours::accentHand2 }
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
        // Initial sync from current parameter value.
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

        // Two rows of controls: top has camera + toggles, bottom has smoothing.
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
        bounds.removeFromBottom(8);

        const bool showPreview = previewToggle.getToggleState();
        // 2 rows of 7 meters; allow more vertical space for them.
        auto metersArea = showPreview
            ? bounds.removeFromBottom(juce::jmax(170, bounds.getHeight() * 2 / 5))
            : bounds;

        if (showPreview)
        {
            bounds.removeFromBottom(10);
            preview.setBounds(bounds);
        }
        preview.setVisible(showPreview);

        const int rows = 2;
        const int cols = 7;
        const int cellW = metersArea.getWidth() / cols;
        const int cellH = metersArea.getHeight() / rows;
        for (int r = 0; r < rows; ++r)
        {
            for (int c = 0; c < cols; ++c)
            {
                const auto idx = static_cast<size_t>(r * cols + c);
                if (idx < meters.size())
                    meters[idx]->setBounds(metersArea.getX() + c * cellW,
                                           metersArea.getY() + r * cellH,
                                           cellW - 4,
                                           cellH - 4);
            }
        }
    }
}
