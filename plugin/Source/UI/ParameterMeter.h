#pragma once

#include "../Params/ParameterBridge.h"

#include <juce_gui_basics/juce_gui_basics.h>

#include <deque>

namespace handcontrol::ui
{
    /** A compact meter for a single 0..1 measurement parameter.
        Shows the name, the current value numerically, a horizontal bar, and
        a small sparkline of recent history. */
    class ParameterMeter : public juce::Component,
                           private juce::Timer
    {
    public:
        ParameterMeter(const juce::String& labelText,
                       handcontrol::params::ParameterBridge& bridge,
                       handcontrol::params::MeasurementId id,
                       juce::Colour accent);
        ~ParameterMeter() override = default;

        void paint(juce::Graphics& g) override;

    private:
        void timerCallback() override;

        juce::String label;
        handcontrol::params::ParameterBridge& bridge;
        handcontrol::params::MeasurementId id;
        juce::Colour accent;
        float value { 0.0f };
        std::deque<float> history;
        static constexpr size_t historyMax = 120;
    };
}
