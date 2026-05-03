#pragma once

#include "../Midi/MidiCcSender.h"
#include "../Params/ParameterBridge.h"
#include "../Tracking/HandTrackerThread.h"

#include <juce_audio_processors/juce_audio_processors.h>

#include <memory>

namespace handcontrol
{
    class PluginProcessor : public juce::AudioProcessor,
                            private juce::AudioProcessorValueTreeState::Listener
    {
    public:
        PluginProcessor();
        ~PluginProcessor() override;

        void prepareToPlay(double sampleRate, int samplesPerBlock) override;
        void releaseResources() override;
        bool isBusesLayoutSupported(const BusesLayout&) const override;

        void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

        juce::AudioProcessorEditor* createEditor() override;
        bool hasEditor() const override { return true; }

        const juce::String getName() const override { return JucePlugin_Name; }
        bool acceptsMidi() const override { return false; }
        bool producesMidi() const override { return true; }
        bool isMidiEffect() const override { return false; }
        double getTailLengthSeconds() const override { return 0.0; }

        int getNumPrograms() override { return 1; }
        int getCurrentProgram() override { return 0; }
        void setCurrentProgram(int) override {}
        const juce::String getProgramName(int) override { return {}; }
        void changeProgramName(int, const juce::String&) override {}

        void getStateInformation(juce::MemoryBlock&) override;
        void setStateInformation(const void*, int) override;

        juce::AudioProcessorValueTreeState& getValueTreeState() noexcept { return apvts; }
        handcontrol::params::ParameterBridge& getBridge() noexcept { return bridge; }
        handcontrol::tracking::HandTrackerThread& getTracker() noexcept { return *tracker; }
        handcontrol::midi::MidiCcSender& getMidiSender() noexcept { return midiSender; }

        juce::String getLastStartError() const { return lastStartError; }
        void restartTracker();

        /** Pulls the current MIDI mapping from APVTS into the sender. Called when
            any midi_ch / midi_cc / midi_en parameter changes. Safe from any
            thread - only writes atomic shadows in the sender. */
        void syncMidiMappings();

    private:
        void parameterChanged(const juce::String& id, float newValue) override;

        juce::AudioProcessorValueTreeState apvts;
        handcontrol::params::ParameterBridge bridge;
        handcontrol::midi::MidiCcSender midiSender;
        std::unique_ptr<handcontrol::tracking::HandTrackerThread> tracker;
        juce::String lastStartError;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginProcessor)
    };
}
