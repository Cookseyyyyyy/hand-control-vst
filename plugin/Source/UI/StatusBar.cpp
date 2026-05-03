#include "StatusBar.h"
#include "Theme.h"

namespace handcontrol::ui
{
    StatusBar::StatusBar(handcontrol::tracking::HandTrackerThread& src)
        : source(src)
    {
        setOpaque(false);
        startTimerHz(8);
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
            const auto& d = snap.slotDiagnostics;

            juce::String text = "Hand ";
            if (d.active)
            {
                text << juce::String(d.lastConfidence, 2);
            }
            else if (d.lastSeenTime > 0.0)
            {
                const double age = juce::jmax(0.0, snap.currentTime - d.lastSeenTime);
                text << "lost (" << juce::String(age, 1) << "s)";
            }
            else
            {
                text << "-";
            }
            text << "    Palm " << juce::String(snap.result.diagnostics.lastPalmScore, 2);
            text << "    " << (snap.result.diagnostics.palmRanThisFrame
                                   ? juce::String("[palm]")
                                   : juce::String("[cached]"));

            currentText = text;
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
        g.setFont(juce::Font(juce::FontOptions("Menlo", 12.0f, juce::Font::plain)));
        g.drawFittedText(currentText, inner.toNearestInt(),
                         juce::Justification::centredLeft, 1);
    }
}
