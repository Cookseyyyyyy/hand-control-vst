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
        setResizeLimits(780, 580, 1600, 1200);
        setSize(880, 680);

        buildControls();

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
        const std::array<MeterSpec, 8> specs {{
            { "H1 Thumb-Index Distance",  params::MeasurementId::hand1ThumbIndexDistance, ui::colours::accentIndex },
            { "H1 Thumb-Index Angle",     params::MeasurementId::hand1ThumbIndexAngle,    ui::colours::accentIndex },
            { "H1 Thumb-Pinky Distance",  params::MeasurementId::hand1ThumbPinkyDistance, ui::colours::accentPinky },
            { "H1 Thumb-Pinky Angle",     params::MeasurementId::hand1ThumbPinkyAngle,    ui::colours::accentPinky },
            { "H2 Thumb-Index Distance",  params::MeasurementId::hand2ThumbIndexDistance, ui::colours::accentIndex },
            { "H2 Thumb-Index Angle",     params::MeasurementId::hand2ThumbIndexAngle,    ui::colours::accentIndex },
            { "H2 Thumb-Pinky Distance",  params::MeasurementId::hand2ThumbPinkyDistance, ui::colours::accentPinky },
            { "H2 Thumb-Pinky Angle",     params::MeasurementId::hand2ThumbPinkyAngle,    ui::colours::accentPinky }
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
        cameraAttach = std::make_unique<ComboAttachment>(vts, pid(params::cameraIndex), cameraBox);
        previewAttach = std::make_unique<ButtonAttachment>(vts, pid(params::previewVisible), previewToggle);
        holdAttach    = std::make_unique<ButtonAttachment>(vts, pid(params::holdOnLost),     holdToggle);
        smoothingAttach = std::make_unique<SliderAttachment>(vts, pid(params::smoothing),    smoothingSlider);

        previewToggle.onStateChange = [this]()
        {
            preview.setVisible(previewToggle.getToggleState());
            resized();
        };
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

        auto controlsRow = bounds.removeFromBottom(36);
        auto left = controlsRow.removeFromLeft(controlsRow.getWidth() / 2);
        cameraLabel.setBounds(left.removeFromLeft(58));
        left.removeFromLeft(4);
        cameraBox.setBounds(left.removeFromLeft(220).reduced(0, 4));
        left.removeFromLeft(12);
        previewToggle.setBounds(left.removeFromLeft(120));
        holdToggle.setBounds(controlsRow.removeFromLeft(140));
        smoothingLabel.setBounds(controlsRow.removeFromLeft(80));
        smoothingSlider.setBounds(controlsRow.reduced(4, 6));
        bounds.removeFromBottom(8);

        const bool showPreview = previewToggle.getToggleState();
        auto metersArea = showPreview
            ? bounds.removeFromBottom(juce::jmax(180, bounds.getHeight() / 3))
            : bounds;

        if (showPreview)
        {
            bounds.removeFromBottom(10);
            preview.setBounds(bounds);
        }
        preview.setVisible(showPreview);

        // 2 rows of 4 meters, grouped by hand.
        const int rows = 2;
        const int cols = 4;
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
                                           cellW - 6,
                                           cellH - 6);
            }
        }
    }
}
