#include "PluginProcessor.h"
#include "PluginEditor.h"

#include "../Params/ParameterIDs.h"
#include "../Params/ParameterLayout.h"
#include "../Tracking/MockHandTracker.h"
#include "../Tracking/OnnxHandTracker.h"

#include <cstdlib>

namespace handcontrol
{
    namespace
    {
        /** Select which tracker implementation to use.
            Default: real MediaPipe-via-ONNX tracker.
            Override for development or headless testing by setting the
            environment variable HANDCONTROL_USE_MOCK_TRACKER=1 before loading
            the plugin. */
        std::unique_ptr<tracking::IHandTracker> makeTrackerImpl()
        {
            if (const char* env = std::getenv("HANDCONTROL_USE_MOCK_TRACKER"))
            {
                const juce::String v (env);
                if (v == "1" || v.equalsIgnoreCase("true") || v.equalsIgnoreCase("yes"))
                    return std::make_unique<tracking::MockHandTracker>();
            }
            return std::make_unique<tracking::OnnxHandTracker>();
        }
    }

    PluginProcessor::PluginProcessor()
        : juce::AudioProcessor(BusesProperties()
                                   .withInput("Input",  juce::AudioChannelSet::stereo(), true)
                                   .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
          apvts(*this, nullptr, "HandControl", params::createLayout()),
          bridge(apvts),
          tracker(std::make_unique<tracking::HandTrackerThread>(bridge, makeTrackerImpl()))
    {
        apvts.addParameterListener(juce::String(params::cameraIndex.data(),  params::cameraIndex.size()),  this);
        apvts.addParameterListener(juce::String(params::smoothing.data(),    params::smoothing.size()),    this);
        apvts.addParameterListener(juce::String(params::holdOnLost.data(),   params::holdOnLost.size()),   this);
        apvts.addParameterListener(juce::String(params::bypass.data(),       params::bypass.size()),       this);
        apvts.addParameterListener(juce::String(params::mirrorCamera.data(), params::mirrorCamera.size()), this);

        // Listen on every per-measurement MIDI config param so the sender stays
        // in sync with the user's edits in the MIDI Map panel (or with values
        // restored from a saved project state).
        for (int i = 0; i < params::numMeasurements; ++i)
        {
            const auto id = static_cast<params::MeasurementId>(i);
            apvts.addParameterListener(params::midiChannelId(id),  this);
            apvts.addParameterListener(params::midiCcNumberId(id), this);
            apvts.addParameterListener(params::midiEnabledId(id),  this);
        }

        syncMidiMappings();
    }

    PluginProcessor::~PluginProcessor()
    {
        apvts.removeParameterListener(juce::String(params::cameraIndex.data(),  params::cameraIndex.size()),  this);
        apvts.removeParameterListener(juce::String(params::smoothing.data(),    params::smoothing.size()),    this);
        apvts.removeParameterListener(juce::String(params::holdOnLost.data(),   params::holdOnLost.size()),   this);
        apvts.removeParameterListener(juce::String(params::bypass.data(),       params::bypass.size()),       this);
        apvts.removeParameterListener(juce::String(params::mirrorCamera.data(), params::mirrorCamera.size()), this);

        for (int i = 0; i < params::numMeasurements; ++i)
        {
            const auto id = static_cast<params::MeasurementId>(i);
            apvts.removeParameterListener(params::midiChannelId(id),  this);
            apvts.removeParameterListener(params::midiCcNumberId(id), this);
            apvts.removeParameterListener(params::midiEnabledId(id),  this);
        }
    }

    void PluginProcessor::prepareToPlay(double, int) {}
    void PluginProcessor::releaseResources() {}

    bool PluginProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
    {
        const auto in  = layouts.getMainInputChannelSet();
        const auto out = layouts.getMainOutputChannelSet();
        if (in.isDisabled() || out.isDisabled()) return false;
        return in == out;
    }

    void PluginProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiBuffer)
    {
        juce::ScopedNoDenormals noDenormals;

        // This is a control plugin: it doesn't transform audio, it just lets the
        // DAW see parameter automation generated from the tracker thread.
        bridge.flushToParameters();

        // Emit MIDI CCs for any measurement whose 7-bit value changed since the
        // last block. Single sample-0 event per param so any DAW MIDI-map mode
        // captures the source CC reliably.
        for (int i = 0; i < params::numMeasurements; ++i)
        {
            const auto id = static_cast<params::MeasurementId>(i);
            midiSender.emitIfChanged(id, bridge.snapshot(id), midiBuffer, 0);
        }

        // Pass audio through untouched.
        const auto numIn  = getTotalNumInputChannels();
        const auto numOut = getTotalNumOutputChannels();
        for (int ch = numIn; ch < numOut; ++ch)
            buffer.clear(ch, 0, buffer.getNumSamples());
    }

    juce::AudioProcessorEditor* PluginProcessor::createEditor()
    {
        return new PluginEditor(*this);
    }

    void PluginProcessor::getStateInformation(juce::MemoryBlock& dest)
    {
        auto state = apvts.copyState();
        if (auto xml = state.createXml())
            copyXmlToBinary(*xml, dest);
    }

    void PluginProcessor::setStateInformation(const void* data, int size)
    {
        if (auto xml = getXmlFromBinary(data, size))
            apvts.replaceState(juce::ValueTree::fromXml(*xml));
    }

    void PluginProcessor::parameterChanged(const juce::String& id, float newValue)
    {
        const juce::String cameraId (params::cameraIndex.data(),   params::cameraIndex.size());
        const juce::String smoothId (params::smoothing.data(),     params::smoothing.size());
        const juce::String holdId   (params::holdOnLost.data(),    params::holdOnLost.size());
        const juce::String bypassId (params::bypass.data(),        params::bypass.size());
        const juce::String mirrorId (params::mirrorCamera.data(),  params::mirrorCamera.size());

        if (id == cameraId)
        {
            restartTracker();
        }
        else if (id == smoothId)
        {
            tracker->setSmoothing(newValue);
        }
        else if (id == holdId)
        {
            tracker->setHoldOnLost(newValue > 0.5f);
        }
        else if (id == bypassId)
        {
            tracker->setBypass(newValue > 0.5f);
        }
        else if (id == mirrorId)
        {
            tracker->setMirrorCamera(newValue > 0.5f);
        }
        else if (id.endsWith("_midi_ch") || id.endsWith("_midi_cc") || id.endsWith("_midi_en"))
        {
            // Any of the 42 MIDI config params changed - cheaper to re-sync all
            // 14 mappings than parse the suffix and update one slot.
            syncMidiMappings();
        }
    }

    void PluginProcessor::syncMidiMappings()
    {
        for (int i = 0; i < params::numMeasurements; ++i)
        {
            const auto id = static_cast<params::MeasurementId>(i);

            const auto* chParam = dynamic_cast<juce::AudioParameterInt*>(
                apvts.getParameter(params::midiChannelId(id)));
            const auto* ccParam = dynamic_cast<juce::AudioParameterInt*>(
                apvts.getParameter(params::midiCcNumberId(id)));
            const auto* enParam = dynamic_cast<juce::AudioParameterBool*>(
                apvts.getParameter(params::midiEnabledId(id)));

            handcontrol::midi::MidiCcSender::Mapping m;
            m.channel  = chParam ? chParam->get() : params::defaultMidiChannel;
            m.ccNumber = ccParam ? ccParam->get() : params::defaultMidiCcBase + i;
            m.enabled  = enParam ? enParam->get() : true;
            midiSender.setMapping(id, m);
        }
    }

    void PluginProcessor::restartTracker()
    {
        lastStartError.clear();

        auto* cameraParam = dynamic_cast<juce::AudioParameterInt*>(
            apvts.getParameter(juce::String(params::cameraIndex.data(), params::cameraIndex.size())));
        auto* smoothingParam = dynamic_cast<juce::AudioParameterFloat*>(
            apvts.getParameter(juce::String(params::smoothing.data(), params::smoothing.size())));
        auto* holdParam = dynamic_cast<juce::AudioParameterBool*>(
            apvts.getParameter(juce::String(params::holdOnLost.data(), params::holdOnLost.size())));
        auto* bypassParam = dynamic_cast<juce::AudioParameterBool*>(
            apvts.getParameter(juce::String(params::bypass.data(), params::bypass.size())));
        auto* mirrorParam = dynamic_cast<juce::AudioParameterBool*>(
            apvts.getParameter(juce::String(params::mirrorCamera.data(), params::mirrorCamera.size())));

        if (smoothingParam) tracker->setSmoothing(smoothingParam->get());
        if (holdParam)      tracker->setHoldOnLost(holdParam->get());
        if (bypassParam)    tracker->setBypass(bypassParam->get());
        if (mirrorParam)    tracker->setMirrorCamera(mirrorParam->get());

        const int cameraIdx = cameraParam ? cameraParam->get() : 0;
        if (auto err = tracker->start(cameraIdx))
            lastStartError = juce::String(*err);
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new handcontrol::PluginProcessor();
}
