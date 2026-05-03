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
            // Per-slot block: "H1 0.92" or "H1 lost (1.4s)" so users can see
            // exactly when and why a meter has stopped moving.
            juce::String text;
            for (int slot = 0; slot < 2; ++slot)
            {
                const auto& d = snap.slotDiagnostics[static_cast<size_t>(slot)];
                if (text.isNotEmpty()) text << "   ";
                text << "H" << (slot + 1) << " ";
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
            }
            text << "    Palm " << juce::String(snap.result.diagnostics.lastPalmScore, 2);

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
