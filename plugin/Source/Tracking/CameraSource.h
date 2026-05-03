#pragma once

#include <juce_video/juce_video.h>
#include <juce_graphics/juce_graphics.h>

#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace handcontrol::tracking
{
    /** Thin RAII wrapper around `juce::CameraDevice` that keeps only the latest
        frame available for pull-based consumers (the tracker thread).

        On macOS this is AVFoundation, on Windows it's Media Foundation, both
        transparently via JUCE. */
    class CameraSource
    {
    public:
        CameraSource();
        ~CameraSource();

        /** Returns a list of human-readable camera names available on this system. */
        static juce::StringArray enumerateDevices();

        /** Opens the device at index `deviceIndex`. Returns an error string on failure. */
        std::optional<std::string> open(int deviceIndex);
        void close();
        bool isOpen() const noexcept;

        /** Copy out the latest frame. Returns an invalid Image if nothing has been
            captured yet. If mirror mode is enabled, the returned frame is
            horizontally flipped. */
        juce::Image snapshotLatest() const;

        using FrameCallback = std::function<void(const juce::Image& frame, double timestampSeconds)>;
        /** Optional push-based callback invoked on JUCE's camera thread when a
            new frame arrives. */
        void setFrameCallback(FrameCallback cb);

        /** Index passed to `open()`, or -1 if closed. */
        int currentDeviceIndex() const noexcept { return deviceIndex; }

        /** When true (default), `snapshotLatest()` returns a horizontally
            flipped image so the user sees a mirror image of themselves and
            tracking matches their kinaesthetic expectation. */
        void setMirror(bool shouldMirror) noexcept { mirrorEnabled.store(shouldMirror); }
        bool isMirrored() const noexcept { return mirrorEnabled.load(); }

    private:
        class Listener;
        std::unique_ptr<juce::CameraDevice> device;
        std::unique_ptr<Listener> listener;
        mutable std::mutex latestMutex;
        juce::Image latest;
        FrameCallback callback;
        int deviceIndex { -1 };
        std::atomic<bool> mirrorEnabled { true };
    };
}
