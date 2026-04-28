#include "ImagePreprocess.h"

#include <algorithm>
#include <cmath>

namespace handcontrol::tracking
{
    namespace
    {
        // Sample an RGB pixel by nearest-neighbour. Bilinear would be slightly
        // better but nearest is fast and good enough for the detector resolution.
        inline void sampleRGB(const juce::Image::BitmapData& bmp,
                              int x, int y,
                              uint8_t& r, uint8_t& g, uint8_t& b) noexcept
        {
            x = juce::jlimit(0, bmp.width  - 1, x);
            y = juce::jlimit(0, bmp.height - 1, y);
            const auto* pixel = bmp.getPixelPointer(x, y);
            const auto fmt = bmp.pixelFormat;
            if (fmt == juce::Image::PixelFormat::RGB)
            {
                r = pixel[0]; g = pixel[1]; b = pixel[2];
            }
            else // ARGB - JUCE stores as BGRA in memory on both Windows and macOS.
            {
                b = pixel[0]; g = pixel[1]; r = pixel[2];
            }
        }

        inline void sampleRGBBilinear(const juce::Image::BitmapData& bmp,
                                      float x, float y,
                                      uint8_t& r, uint8_t& g, uint8_t& b) noexcept
        {
            const float xf = juce::jlimit(0.0f, static_cast<float>(bmp.width)  - 1.0f, x);
            const float yf = juce::jlimit(0.0f, static_cast<float>(bmp.height) - 1.0f, y);
            const int x0 = static_cast<int>(std::floor(xf));
            const int y0 = static_cast<int>(std::floor(yf));
            const int x1 = std::min(x0 + 1, bmp.width  - 1);
            const int y1 = std::min(y0 + 1, bmp.height - 1);
            const float wx = xf - static_cast<float>(x0);
            const float wy = yf - static_cast<float>(y0);

            uint8_t r00, g00, b00, r10, g10, b10, r01, g01, b01, r11, g11, b11;
            sampleRGB(bmp, x0, y0, r00, g00, b00);
            sampleRGB(bmp, x1, y0, r10, g10, b10);
            sampleRGB(bmp, x0, y1, r01, g01, b01);
            sampleRGB(bmp, x1, y1, r11, g11, b11);

            auto lerp = [&](uint8_t a0, uint8_t a1, uint8_t b0, uint8_t b1) -> uint8_t
            {
                const float top    = static_cast<float>(a0) * (1.0f - wx) + static_cast<float>(a1) * wx;
                const float bot    = static_cast<float>(b0) * (1.0f - wx) + static_cast<float>(b1) * wx;
                const float mixed  = top * (1.0f - wy) + bot * wy;
                return static_cast<uint8_t>(juce::jlimit(0.0f, 255.0f, mixed));
            };
            r = lerp(r00, r10, r01, r11);
            g = lerp(g00, g10, g01, g11);
            b = lerp(b00, b10, b01, b11);
        }
    }

    void letterboxToTensor(const juce::Image& src,
                           int target,
                           std::vector<float>& outTensor,
                           int& outPadBiasX,
                           int& outPadBiasY)
    {
        outTensor.assign(static_cast<size_t>(target * target * 3), 0.0f);
        outPadBiasX = 0;
        outPadBiasY = 0;

        if (! src.isValid() || src.getWidth() <= 0 || src.getHeight() <= 0)
            return;

        const auto srcW = src.getWidth();
        const auto srcH = src.getHeight();
        const float ratio = std::min(static_cast<float>(target) / static_cast<float>(srcW),
                                     static_cast<float>(target) / static_cast<float>(srcH));
        const int newW = static_cast<int>(std::round(static_cast<float>(srcW) * ratio));
        const int newH = static_cast<int>(std::round(static_cast<float>(srcH) * ratio));
        const int padX = (target - newW) / 2;
        const int padY = (target - newH) / 2;

        // Pad bias expressed in original image pixels.
        outPadBiasX = static_cast<int>(std::round(static_cast<float>(padX) / ratio));
        outPadBiasY = static_cast<int>(std::round(static_cast<float>(padY) / ratio));

        juce::Image::BitmapData bmp(src, juce::Image::BitmapData::readOnly);

        for (int y = 0; y < target; ++y)
        {
            for (int x = 0; x < target; ++x)
            {
                float* dst = outTensor.data() + (static_cast<size_t>(y) * target + x) * 3;

                const int relX = x - padX;
                const int relY = y - padY;
                if (relX < 0 || relY < 0 || relX >= newW || relY >= newH)
                {
                    dst[0] = dst[1] = dst[2] = 0.0f;  // letterbox padding
                    continue;
                }

                const float sx = static_cast<float>(relX) / ratio;
                const float sy = static_cast<float>(relY) / ratio;

                uint8_t r, g, b;
                sampleRGBBilinear(bmp, sx, sy, r, g, b);
                dst[0] = static_cast<float>(r) / 255.0f;
                dst[1] = static_cast<float>(g) / 255.0f;
                dst[2] = static_cast<float>(b) / 255.0f;
            }
        }
    }

    void cropRotateResizeToTensor(const juce::Image& src,
                                  const RoiTransform& roi,
                                  int target,
                                  std::vector<float>& outTensor)
    {
        outTensor.assign(static_cast<size_t>(target * target * 3), 0.0f);

        if (! src.isValid() || roi.size <= 0.0f)
            return;

        juce::Image::BitmapData bmp(src, juce::Image::BitmapData::readOnly);

        const float cosR = std::cos(roi.rotationRad);
        const float sinR = std::sin(roi.rotationRad);
        const float halfSize = roi.size * 0.5f;
        const float invTarget = 1.0f / static_cast<float>(target);

        for (int y = 0; y < target; ++y)
        {
            // normalised [-0.5, 0.5]
            const float ny = (static_cast<float>(y) + 0.5f) * invTarget - 0.5f;
            const float scaledY = ny * roi.size;
            for (int x = 0; x < target; ++x)
            {
                const float nx = (static_cast<float>(x) + 0.5f) * invTarget - 0.5f;
                const float scaledX = nx * roi.size;

                // Rotate around ROI centre, then translate to original image coords.
                const float srcX = roi.centerX + (scaledX * cosR - scaledY * sinR);
                const float srcY = roi.centerY + (scaledX * sinR + scaledY * cosR);

                float* dst = outTensor.data() + (static_cast<size_t>(y) * target + x) * 3;

                if (srcX < 0.0f || srcY < 0.0f
                    || srcX >= static_cast<float>(bmp.width)
                    || srcY >= static_cast<float>(bmp.height))
                {
                    dst[0] = dst[1] = dst[2] = 0.0f;
                    continue;
                }

                uint8_t r, g, b;
                sampleRGBBilinear(bmp, srcX, srcY, r, g, b);
                dst[0] = static_cast<float>(r) / 255.0f;
                dst[1] = static_cast<float>(g) / 255.0f;
                dst[2] = static_cast<float>(b) / 255.0f;
            }
        }

        juce::ignoreUnused(halfSize);
    }
}
