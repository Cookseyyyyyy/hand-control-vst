#pragma once

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
}
