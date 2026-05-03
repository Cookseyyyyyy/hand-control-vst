#pragma once

#include <juce_core/juce_core.h>

#include <array>
#include <string_view>

namespace handcontrol::params
{
    // ---- Single-hand measurement parameters (v0.4) -------------------------
    // We dropped two-hand support in v0.4: tracking quality of a single
    // hand is now the focus. The parameter IDs keep the "h1_" prefix so
    // existing v0.3 mappings to the H1_* params still resolve.
    inline constexpr std::string_view h1ThumbIndexDistance = "h1_thumbIndex_distance";
    inline constexpr std::string_view h1ThumbIndexAngle    = "h1_thumbIndex_angle";
    inline constexpr std::string_view h1ThumbPinkyDistance = "h1_thumbPinky_distance";
    inline constexpr std::string_view h1ThumbPinkyAngle    = "h1_thumbPinky_angle";

    inline constexpr std::string_view h1HandX     = "h1_handX";
    inline constexpr std::string_view h1HandY     = "h1_handY";
    inline constexpr std::string_view h1Openness  = "h1_openness";

    // ---- Utility (not "mappable") -------------------------------------------
    inline constexpr std::string_view cameraIndex     = "cameraIndex";
    inline constexpr std::string_view previewVisible  = "previewVisible";
    inline constexpr std::string_view smoothing       = "smoothing";
    inline constexpr std::string_view holdOnLost      = "holdOnLost";
    inline constexpr std::string_view bypass          = "bypass";
    inline constexpr std::string_view mirrorCamera    = "mirrorCamera";
    inline constexpr std::string_view roiOverlay      = "roiOverlay";

    /** Stable indices for the measurement parameters. */
    enum class MeasurementId : int
    {
        thumbIndexDistance = 0,
        thumbIndexAngle,
        thumbPinkyDistance,
        thumbPinkyAngle,
        handX,
        handY,
        openness,
        count
    };

    inline constexpr int numMeasurements = static_cast<int>(MeasurementId::count);
    static_assert(numMeasurements == 7, "7 measurement parameters expected");

    inline constexpr std::array<std::string_view, numMeasurements> measurementIds {
        h1ThumbIndexDistance, h1ThumbIndexAngle,
        h1ThumbPinkyDistance, h1ThumbPinkyAngle,
        h1HandX, h1HandY, h1Openness
    };

    inline constexpr std::array<std::string_view, numMeasurements> measurementDisplayNames {
        "Thumb-Index Distance", "Thumb-Index Angle",
        "Thumb-Pinky Distance", "Thumb-Pinky Angle",
        "Hand X", "Hand Y", "Openness"
    };

    // ---- Per-measurement MIDI CC config (non-automatable) ------------------
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

    // Default CC numbers: 20..26. CCs 20-31 are undefined per the MIDI spec
    // so they're safe targets that won't collide with standard CCs.
    inline constexpr int defaultMidiCcBase = 20;
    inline constexpr int defaultMidiChannel = 1;
}
