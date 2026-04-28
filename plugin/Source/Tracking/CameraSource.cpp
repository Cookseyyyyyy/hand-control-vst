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
        std::lock_guard lock(latestMutex);
        return latest.createCopy();
    }

    void CameraSource::setFrameCallback(FrameCallback cb)
    {
        callback = std::move(cb);
    }
}
