#pragma once

#include <array>
#include <string_view>

namespace handcontrol::params
{
    inline constexpr std::string_view h1ThumbIndexDistance = "h1_thumbIndex_distance";
    inline constexpr std::string_view h1ThumbIndexAngle    = "h1_thumbIndex_angle";
    inline constexpr std::string_view h1ThumbPinkyDistance = "h1_thumbPinky_distance";
    inline constexpr std::string_view h1ThumbPinkyAngle    = "h1_thumbPinky_angle";

    inline constexpr std::string_view h2ThumbIndexDistance = "h2_thumbIndex_distance";
    inline constexpr std::string_view h2ThumbIndexAngle    = "h2_thumbIndex_angle";
    inline constexpr std::string_view h2ThumbPinkyDistance = "h2_thumbPinky_distance";
    inline constexpr std::string_view h2ThumbPinkyAngle    = "h2_thumbPinky_angle";

    inline constexpr std::string_view cameraIndex   = "cameraIndex";
    inline constexpr std::string_view previewVisible = "previewVisible";
    inline constexpr std::string_view smoothing     = "smoothing";
    inline constexpr std::string_view holdOnLost    = "holdOnLost";
    inline constexpr std::string_view bypass        = "bypass";

    enum class MeasurementId : int
    {
        hand1ThumbIndexDistance = 0,
        hand1ThumbIndexAngle,
        hand1ThumbPinkyDistance,
        hand1ThumbPinkyAngle,
        hand2ThumbIndexDistance,
        hand2ThumbIndexAngle,
        hand2ThumbPinkyDistance,
        hand2ThumbPinkyAngle,
        count
    };

    inline constexpr int numMeasurements = static_cast<int>(MeasurementId::count);
    static_assert(numMeasurements == 8, "8 measurement parameters expected");

    inline constexpr std::array<std::string_view, numMeasurements> measurementIds {
        h1ThumbIndexDistance, h1ThumbIndexAngle,
        h1ThumbPinkyDistance, h1ThumbPinkyAngle,
        h2ThumbIndexDistance, h2ThumbIndexAngle,
        h2ThumbPinkyDistance, h2ThumbPinkyAngle
    };
}
