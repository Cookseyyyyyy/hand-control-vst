#pragma once

#include "../Tracking/HandTrackerThread.h"

#include <juce_gui_basics/juce_gui_basics.h>

namespace handcontrol::ui
{
    /** Bottom strip: shows tracker status and a concise summary of which hands
        are being detected. */
    class StatusBar : public juce::Component, private juce::Timer
    {
    public:
        explicit StatusBar(handcontrol::tracking::HandTrackerThread& src);
        ~StatusBar() override = default;

        void paint(juce::Graphics& g) override;

    private:
        void timerCallback() override;

        handcontrol::tracking::HandTrackerThread& source;
        juce::String currentText { "Starting..." };
        bool healthy { false };
    };
}
