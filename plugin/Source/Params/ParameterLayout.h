#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

namespace handcontrol::params
{
    juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
}
