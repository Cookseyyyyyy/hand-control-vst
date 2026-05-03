#include "CameraSource.h"

namespace handcontrol::tracking
{
    class CameraSource::Listener : public juce::CameraDevice::Listener
    {
    public:
        explicit Listener(CameraSource& ownerRef) : owner(ownerRef) {}

        void imageReceived(const juce::Image& image) override
        {
            if (! image.isValid())
                return;

            const auto now = juce::Time::getMillisecondCounterHiRes() * 0.001;

            {
                std::lock_guard lock(owner.latestMutex);
                owner.latest = image.createCopy();
            }

            if (owner.callback)
                owner.callback(image, now);
        }

    private:
        CameraSource& owner;
    };

    CameraSource::CameraSource() = default;

    CameraSource::~CameraSource()
    {
        close();
    }

    juce::StringArray CameraSource::enumerateDevices()
    {
        return juce::CameraDevice::getAvailableDevices();
    }

    std::optional<std::string> CameraSource::open(int newDeviceIndex)
    {
        close();

        const auto devices = enumerateDevices();
        if (newDeviceIndex < 0 || newDeviceIndex >= devices.size())
            return std::string("Camera index out of range (")
                   + std::to_string(newDeviceIndex) + ")";

        device.reset(juce::CameraDevice::openDevice(newDeviceIndex));
        if (device == nullptr)
            return std::string("Failed to open camera: ")
                   + devices[newDeviceIndex].toStdString();

        listener = std::make_unique<Listener>(*this);
        device->addListener(listener.get());
        deviceIndex = newDeviceIndex;
        return std::nullopt;
    }

    void CameraSource::close()
    {
        if (device != nullptr && listener != nullptr)
            device->removeListener(listener.get());
        listener.reset();
        device.reset();
        deviceIndex = -1;
        {
            std::lock_guard lock(latestMutex);
            latest = {};
        }
    }

    bool CameraSource::isOpen() const noexcept
    {
        return device != nullptr;
    }

    juce::Image CameraSource::snapshotLatest() const
    {
        juce::Image src;
        {
            std::lock_guard lock(latestMutex);
            src = latest.createCopy();
        }
        if (! src.isValid() || ! mirrorEnabled.load(std::memory_order_relaxed))
            return src;

        // Horizontal flip for the mirror display + matching tracker input.
        // Implemented as a pixel copy in BitmapData; for ARGB this is a 4-byte
        // memmove per scanline pair.
        juce::Image flipped(src.getFormat(), src.getWidth(), src.getHeight(), false);
        const int w = src.getWidth();
        const int h = src.getHeight();
        juce::Image::BitmapData srcBmp(src,    juce::Image::BitmapData::readOnly);
        juce::Image::BitmapData dstBmp(flipped, juce::Image::BitmapData::writeOnly);
        const int bytesPerPixel = srcBmp.pixelStride;
        for (int y = 0; y < h; ++y)
        {
            const auto* srcRow = srcBmp.getLinePointer(y);
            auto*       dstRow = dstBmp.getLinePointer(y);
            for (int x = 0; x < w; ++x)
            {
                const auto* sp = srcRow + (w - 1 - x) * bytesPerPixel;
                auto*       dp = dstRow + x * bytesPerPixel;
                for (int b = 0; b < bytesPerPixel; ++b)
                    dp[b] = sp[b];
            }
        }
        return flipped;
    }

    void CameraSource::setFrameCallback(FrameCallback cb)
    {
        callback = std::move(cb);
    }
}
