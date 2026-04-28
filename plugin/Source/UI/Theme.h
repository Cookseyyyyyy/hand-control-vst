#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace handcontrol::ui
{
    namespace colours
    {
        const juce::Colour background       { 0xff101318 };
        const juce::Colour panel            { 0xff161a22 };
        const juce::Colour panelRaised      { 0xff1d2230 };
        const juce::Colour border           { 0xff262d3c };
        const juce::Colour text             { 0xffe6e8ef };
        const juce::Colour textDim          { 0xff828aa0 };
        const juce::Colour accentHand1      { 0xff35d1c7 };  // teal
        const juce::Colour accentHand2      { 0xffff8a4c };  // warm orange
        const juce::Colour accentIndex      { 0xff6effa8 };  // green (matches old script)
        const juce::Colour accentPinky      { 0xff8d6cff };  // violet (modernised red)
        const juce::Colour ok               { 0xff6effa8 };
        const juce::Colour warn             { 0xffffd166 };
        const juce::Colour err              { 0xffff5c7a };
    }

    class Theme : public juce::LookAndFeel_V4
    {
    public:
        Theme();

        juce::Font getLabelFont(juce::Label&) override;
        juce::Font getComboBoxFont(juce::ComboBox&) override;
        juce::Font getPopupMenuFont() override;

        void drawButtonBackground(juce::Graphics&,
                                  juce::Button&,
                                  const juce::Colour& backgroundColour,
                                  bool shouldDrawButtonAsHighlighted,
                                  bool shouldDrawButtonAsDown) override;

        void drawComboBox(juce::Graphics&,
                          int width, int height,
                          bool isButtonDown,
                          int buttonX, int buttonY, int buttonW, int buttonH,
                          juce::ComboBox&) override;

        void drawLinearSlider(juce::Graphics&,
                              int x, int y, int width, int height,
                              float sliderPos, float minSliderPos, float maxSliderPos,
                              juce::Slider::SliderStyle,
                              juce::Slider&) override;

        void drawToggleButton(juce::Graphics&,
                              juce::ToggleButton&,
                              bool shouldDrawButtonAsHighlighted,
                              bool shouldDrawButtonAsDown) override;
    };

    /** Utility: draw a rounded panel with border. */
    void drawPanel(juce::Graphics& g, juce::Rectangle<float> bounds,
                   float cornerRadius = 8.0f);
}
