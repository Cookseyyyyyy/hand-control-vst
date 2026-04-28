#include "StatusBar.h"
#include "Theme.h"

namespace handcontrol::ui
{
    StatusBar::StatusBar(handcontrol::tracking::HandTrackerThread& src)
        : source(src)
    {
        setOpaque(false);
        startTimerHz(6);
    }

    void StatusBar::timerCallback()
    {
        auto snap = source.latestSnapshotForUI();
        healthy = snap.trackerHealthy;

        if (! snap.statusMessage.empty())
        {
            currentText = juce::String(snap.statusMessage);
        }
        else
        {
            int count = 0;
            juce::String which;
            for (int slot = 0; slot < 2; ++slot)
            {
                const auto idx = snap.assignment.slotToInputIndex[static_cast<size_t>(slot)];
                if (idx.has_value() && snap.result.hands[static_cast<size_t>(*idx)].present)
                {
                    ++count;
                    if (which.isNotEmpty()) which << ", ";
                    which << (slot == 0 ? "Hand 1" : "Hand 2");
                }
            }
            if (count == 0)
                currentText = "Tracker ready. No hands detected.";
            else
                currentText = "Tracking: " + which;
        }
        repaint();
    }

    void StatusBar::paint(juce::Graphics& g)
    {
        auto bounds = getLocalBounds().toFloat();
        drawPanel(g, bounds, 6.0f);

        auto inner = bounds.reduced(10.0f, 4.0f);

        auto dotRect = juce::Rectangle<float>(inner.getX(), inner.getCentreY() - 4.0f, 8.0f, 8.0f);
        g.setColour(healthy ? colours::ok : colours::warn);
        g.fillEllipse(dotRect);
        inner.removeFromLeft(14.0f);

        g.setColour(colours::textDim);
        g.setFont(juce::Font(juce::FontOptions(12.5f, juce::Font::plain)));
        g.drawFittedText(currentText, inner.toNearestInt(),
                         juce::Justification::centredLeft, 1);
    }
}
