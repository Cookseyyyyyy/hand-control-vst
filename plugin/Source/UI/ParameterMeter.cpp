#include "ParameterMeter.h"
#include "Theme.h"

namespace handcontrol::ui
{
    ParameterMeter::ParameterMeter(const juce::String& labelText,
                                   handcontrol::params::ParameterBridge& bridgeRef,
                                   handcontrol::params::MeasurementId measurementId,
                                   juce::Colour accentColour)
        : label(labelText), bridge(bridgeRef), id(measurementId), accent(accentColour)
    {
        setOpaque(false);
        startTimerHz(30);
    }

    void ParameterMeter::timerCallback()
    {
        value = bridge.snapshot(id);
        history.push_back(value);
        while (history.size() > historyMax)
            history.pop_front();
        repaint();
    }

    void ParameterMeter::paint(juce::Graphics& g)
    {
        auto bounds = getLocalBounds().toFloat().reduced(1.0f);
        drawPanel(g, bounds, 6.0f);

        auto inner = bounds.reduced(8.0f, 6.0f);

        // Label + value
        auto headerRow = inner.removeFromTop(16.0f);
        g.setColour(colours::textDim);
        g.setFont(juce::Font(juce::FontOptions(11.5f, juce::Font::plain)));
        g.drawFittedText(label,
                         headerRow.removeFromLeft(headerRow.getWidth() * 0.6f).toNearestInt(),
                         juce::Justification::centredLeft, 1);

        g.setColour(colours::text);
        g.setFont(juce::Font(juce::FontOptions("Menlo", 12.0f, juce::Font::plain)));
        g.drawFittedText(juce::String(value, 3),
                         headerRow.toNearestInt(),
                         juce::Justification::centredRight, 1);

        inner.removeFromTop(4.0f);

        // Bar
        auto barRect = inner.removeFromTop(6.0f);
        g.setColour(colours::panelRaised);
        g.fillRoundedRectangle(barRect, 3.0f);
        auto filled = barRect.withWidth(barRect.getWidth() * value);
        g.setColour(accent);
        g.fillRoundedRectangle(filled, 3.0f);

        inner.removeFromTop(4.0f);

        // Sparkline
        auto spark = inner.reduced(0.0f, 1.0f);
        if (history.size() > 1)
        {
            juce::Path p;
            const auto step = spark.getWidth() / static_cast<float>(historyMax - 1);
            for (size_t i = 0; i < history.size(); ++i)
            {
                const auto v = history[i];
                const auto x = spark.getX() + (float) i * step;
                const auto y = spark.getBottom() - v * spark.getHeight();
                if (i == 0)
                    p.startNewSubPath(x, y);
                else
                    p.lineTo(x, y);
            }
            g.setColour(accent.withAlpha(0.55f));
            g.strokePath(p, juce::PathStrokeType(1.4f));
        }
    }
}
