#pragma once

#include <juce_core/juce_core.h>

#include <array>
#include <string_view>

namespace handcontrol::params
{
    // ---- Original Python-port measurements (v0.1) ---------------------------
    inline constexpr std::string_view h1ThumbIndexDistance = "h1_thumbIndex_distance";
    inline constexpr std::string_view h1ThumbIndexAngle    = "h1_thumbIndex_angle";
    inline constexpr std::string_view h1ThumbPinkyDistance = "h1_thumbPinky_distance";
    inline constexpr std::string_view h1ThumbPinkyAngle    = "h1_thumbPinky_angle";

    inline constexpr std::string_view h2ThumbIndexDistance = "h2_thumbIndex_distance";
    inline constexpr std::string_view h2ThumbIndexAngle    = "h2_thumbIndex_angle";
    inline constexpr std::string_view h2ThumbPinkyDistance = "h2_thumbPinky_distance";
    inline constexpr std::string_view h2ThumbPinkyAngle    = "h2_thumbPinky_angle";

    // ---- New per-hand measurements (v0.2) -----------------------------------
    inline constexpr std::string_view h1HandX     = "h1_handX";
    inline constexpr std::string_view h1HandY     = "h1_handY";
    inline constexpr std::string_view h1Openness  = "h1_openness";
    inline constexpr std::string_view h2HandX     = "h2_handX";
    inline constexpr std::string_view h2HandY     = "h2_handY";
    inline constexpr std::string_view h2Openness  = "h2_openness";

    // ---- Utility (not "mappable") -------------------------------------------
    inline constexpr std::string_view cameraIndex     = "cameraIndex";
    inline constexpr std::string_view previewVisible  = "previewVisible";
    inline constexpr std::string_view smoothing       = "smoothing";
    inline constexpr std::string_view holdOnLost      = "holdOnLost";
    inline constexpr std::string_view bypass          = "bypass";
    inline constexpr std::string_view mirrorCamera    = "mirrorCamera";
    inline constexpr std::string_view roiOverlay      = "roiOverlay";

    /** Stable indices for the measurement parameters. The ORIGINAL EIGHT are
        kept at their v0.1 positions so existing user mappings still work. New
        v0.2 measurements are appended. */
    enum class MeasurementId : int
    {
        // v0.1 - do not reorder
        hand1ThumbIndexDistance = 0,
        hand1ThumbIndexAngle,
        hand1ThumbPinkyDistance,
        hand1ThumbPinkyAngle,
        hand2ThumbIndexDistance,
        hand2ThumbIndexAngle,
        hand2ThumbPinkyDistance,
        hand2ThumbPinkyAngle,

        // v0.2 - append-only
        hand1HandX,
        hand1HandY,
        hand1Openness,
        hand2HandX,
        hand2HandY,
        hand2Openness,

        count
    };

    inline constexpr int numMeasurements = static_cast<int>(MeasurementId::count);
    static_assert(numMeasurements == 14, "14 measurement parameters expected");

    inline constexpr std::array<std::string_view, numMeasurements> measurementIds {
        h1ThumbIndexDistance, h1ThumbIndexAngle,
        h1ThumbPinkyDistance, h1ThumbPinkyAngle,
        h2ThumbIndexDistance, h2ThumbIndexAngle,
        h2ThumbPinkyDistance, h2ThumbPinkyAngle,
        h1HandX, h1HandY, h1Openness,
        h2HandX, h2HandY, h2Openness
    };

    // ---- Per-measurement MIDI CC config (v0.3, non-automatable) -------------
    //
    // Three parameters per measurement: channel (1..16), CC number (0..127),
    // and an enabled toggle. Stored in APVTS so they persist with the project.
    // None are exposed for DAW automation (withAutomatable(false)) so they
    // don't pollute the host's parameter list - they're only meant to be
    // edited from the plugin's MIDI Map panel.
    inline juce::String midiChannelId(MeasurementId id)
    {
        const auto base = measurementIds[static_cast<size_t>(id)];
        return juce::String(base.data(), base.size()) + "_midi_ch";
    }
    inline juce::String midiCcNumberId(MeasurementId id)
    {
        const auto base = measurementIds[static_cast<size_t>(id)];
        return juce::String(base.data(), base.size()) + "_midi_cc";
    }
    inline juce::String midiEnabledId(MeasurementId id)
    {
        const auto base = measurementIds[static_cast<size_t>(id)];
        return juce::String(base.data(), base.size()) + "_midi_en";
    }

    // Default CC numbers: 20..33 (the MIDI spec leaves these undefined, so
    // they're safe to use without colliding with standard CCs like volume,
    // pan, modulation etc.). All start on channel 1 with sending enabled.
    inline constexpr int defaultMidiCcBase = 20;
    inline constexpr int defaultMidiChannel = 1;
}
