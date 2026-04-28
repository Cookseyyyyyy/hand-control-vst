#include "Theme.h"

namespace handcontrol::ui
{
    Theme::Theme()
    {
        setColour(juce::ResizableWindow::backgroundColourId,     colours::background);
        setColour(juce::DocumentWindow::backgroundColourId,      colours::background);
        setColour(juce::Label::textColourId,                     colours::text);
        setColour(juce::Label::outlineColourId,                  juce::Colours::transparentBlack);
        setColour(juce::TextButton::buttonColourId,              colours::panelRaised);
        setColour(juce::TextButton::buttonOnColourId,            colours::accentHand1);
        setColour(juce::TextButton::textColourOffId,             colours::text);
        setColour(juce::TextButton::textColourOnId,              colours::background);
        setColour(juce::ComboBox::backgroundColourId,            colours::panelRaised);
        setColour(juce::ComboBox::outlineColourId,               colours::border);
        setColour(juce::ComboBox::textColourId,                  colours::text);
        setColour(juce::ComboBox::arrowColourId,                 colours::textDim);
        setColour(juce::PopupMenu::backgroundColourId,           colours::panel);
        setColour(juce::PopupMenu::textColourId,                 colours::text);
        setColour(juce::PopupMenu::highlightedBackgroundColourId, colours::accentHand1.withAlpha(0.25f));
        setColour(juce::PopupMenu::highlightedTextColourId,      colours::text);
        setColour(juce::Slider::backgroundColourId,              colours::panelRaised);
        setColour(juce::Slider::trackColourId,                   colours::accentHand1);
        setColour(juce::Slider::thumbColourId,                   colours::text);
        setColour(juce::ToggleButton::textColourId,              colours::text);
        setColour(juce::ToggleButton::tickColourId,              colours::accentHand1);
        setColour(juce::ToggleButton::tickDisabledColourId,      colours::textDim);
    }

    juce::Font Theme::getLabelFont(juce::Label&)
    {
        return juce::Font(juce::FontOptions(13.5f, juce::Font::plain));
    }

    juce::Font Theme::getComboBoxFont(juce::ComboBox&)
    {
        return juce::Font(juce::FontOptions(13.0f, juce::Font::plain));
    }

    juce::Font Theme::getPopupMenuFont()
    {
        return juce::Font(juce::FontOptions(13.0f, juce::Font::plain));
    }

    void Theme::drawButtonBackground(juce::Graphics& g,
                                     juce::Button& button,
                                     const juce::Colour& backgroundColour,
                                     bool highlighted, bool down)
    {
        auto bounds = button.getLocalBounds().toFloat().reduced(0.5f);
        auto bg = backgroundColour;
        if (down)        bg = bg.darker(0.15f);
        else if (highlighted) bg = bg.brighter(0.05f);

        g.setColour(bg);
        g.fillRoundedRectangle(bounds, 6.0f);
        g.setColour(colours::border);
        g.drawRoundedRectangle(bounds, 6.0f, 1.0f);
    }

    void Theme::drawComboBox(juce::Graphics& g,
                             int width, int height,
                             bool /*isButtonDown*/,
                             int buttonX, int buttonY, int buttonW, int buttonH,
                             juce::ComboBox& box)
    {
        auto bounds = juce::Rectangle<float>(0.0f, 0.0f, (float) width, (float) height).reduced(0.5f);
        g.setColour(box.findColour(juce::ComboBox::backgroundColourId));
        g.fillRoundedRectangle(bounds, 6.0f);
        g.setColour(colours::border);
        g.drawRoundedRectangle(bounds, 6.0f, 1.0f);

        juce::Rectangle<int> arrowZone(buttonX, buttonY, buttonW, buttonH);
        juce::Path path;
        path.addTriangle((float) arrowZone.getCentreX() - 4.0f, (float) arrowZone.getCentreY() - 2.0f,
                         (float) arrowZone.getCentreX() + 4.0f, (float) arrowZone.getCentreY() - 2.0f,
                         (float) arrowZone.getCentreX(),        (float) arrowZone.getCentreY() + 3.0f);
        g.setColour(colours::textDim);
        g.fillPath(path);
    }

    void Theme::drawLinearSlider(juce::Graphics& g,
                                 int x, int y, int width, int height,
                                 float sliderPos, float /*minSliderPos*/, float /*maxSliderPos*/,
                                 juce::Slider::SliderStyle /*style*/,
                                 juce::Slider& slider)
    {
        const auto bounds = juce::Rectangle<float>((float) x, (float) y, (float) width, (float) height);
        const auto trackH = 4.0f;
        const auto trackY = bounds.getCentreY() - trackH * 0.5f;

        g.setColour(slider.findColour(juce::Slider::backgroundColourId));
        g.fillRoundedRectangle({ bounds.getX(), trackY, bounds.getWidth(), trackH }, trackH * 0.5f);

        const auto activeW = juce::jmax(0.0f, sliderPos - bounds.getX());
        g.setColour(slider.findColour(juce::Slider::trackColourId));
        g.fillRoundedRectangle({ bounds.getX(), trackY, activeW, trackH }, trackH * 0.5f);

        g.setColour(slider.findColour(juce::Slider::thumbColourId));
        g.fillEllipse(sliderPos - 6.0f, bounds.getCentreY() - 6.0f, 12.0f, 12.0f);
    }

    void Theme::drawToggleButton(juce::Graphics& g,
                                 juce::ToggleButton& button,
                                 bool /*highlighted*/, bool /*down*/)
    {
        const auto bounds = button.getLocalBounds().toFloat();
        const auto boxW = 34.0f;
        const auto boxH = 18.0f;
        const auto boxX = bounds.getX();
        const auto boxY = bounds.getCentreY() - boxH * 0.5f;

        const bool on = button.getToggleState();
        g.setColour(on ? button.findColour(juce::ToggleButton::tickColourId)
                       : colours::panelRaised);
        g.fillRoundedRectangle({ boxX, boxY, boxW, boxH }, boxH * 0.5f);
        g.setColour(colours::border);
        g.drawRoundedRectangle({ boxX, boxY, boxW, boxH }, boxH * 0.5f, 1.0f);

        const auto knobD = boxH - 4.0f;
        const auto knobX = on ? boxX + boxW - knobD - 2.0f : boxX + 2.0f;
        g.setColour(juce::Colours::white);
        g.fillEllipse(knobX, boxY + 2.0f, knobD, knobD);

        g.setColour(colours::text);
        g.setFont(juce::Font(juce::FontOptions(13.5f, juce::Font::plain)));
        g.drawFittedText(button.getButtonText(),
                         bounds.withLeft(boxX + boxW + 10.0f).toNearestInt(),
                         juce::Justification::centredLeft, 1);
    }

    void drawPanel(juce::Graphics& g, juce::Rectangle<float> bounds, float r)
    {
        g.setColour(colours::panel);
        g.fillRoundedRectangle(bounds, r);
        g.setColour(colours::border);
        g.drawRoundedRectangle(bounds, r, 1.0f);
    }
}
