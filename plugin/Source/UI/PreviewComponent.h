#pragma once

#include "../Tracking/HandTrackerThread.h"

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_opengl/juce_opengl.h>

namespace handcontrol::ui
{
    /** Draws the camera frame with hand landmark overlays on top.
        Updates at ~30 Hz by pulling snapshots from the tracker thread. */
    class PreviewComponent : public juce::Component,
                             private juce::Timer
    {
    public:
        explicit PreviewComponent(handcontrol::tracking::HandTrackerThread& source);
        ~PreviewComponent() override;

        void paint(juce::Graphics& g) override;
        void resized() override;

        /** Show / hide the oriented ROI rectangles drawn around each tracked hand.
            Useful diagnostic when tracking is misbehaving. Off by default. */
        void setShowRoi(bool shouldShow) { showRoi = shouldShow; repaint(); }

    private:
        void timerCallback() override;
        void drawLandmarks(juce::Graphics& g,
                           const handcontrol::tracking::HandFrame& hand,
                           const juce::Rectangle<float>& frameRect,
                           juce::Colour accent,
                           const juce::String& label);
        void drawRoi(juce::Graphics& g,
                     const handcontrol::tracking::RoiTransform& roi,
                     const juce::Rectangle<float>& frameRect,
                     juce::Colour accent);

        handcontrol::tracking::HandTrackerThread& source;
        handcontrol::tracking::HandTrackerThread::UISnapshot snapshot;
        juce::OpenGLContext gl;
        bool showRoi { false };
    };
}
