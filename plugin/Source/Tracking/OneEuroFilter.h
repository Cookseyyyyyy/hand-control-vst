#pragma once

namespace handcontrol::tracking
{
    /** One-Euro filter for low-latency, jitter-robust smoothing of 1D signals.
        Reference: Casiez et al., "1 Euro Filter" (CHI 2012). */
    class OneEuroFilter
    {
    public:
        struct Config
        {
            double minCutoffHz { 1.0 };
            double beta        { 0.007 };
            double dCutoffHz   { 1.0 };
        };

        OneEuroFilter() = default;
        explicit OneEuroFilter(Config c) : config(c) {}

        void setConfig(Config c) noexcept { config = c; }

        /** Apply the filter. `value` is the new measurement, `timestampSeconds`
            is the time it was taken. Returns the smoothed value. */
        float process(float value, double timestampSeconds) noexcept;

        void reset() noexcept;

    private:
        static double alpha(double cutoff, double dt) noexcept;

        Config config {};
        bool hasPrev { false };
        double lastTime { 0.0 };
        float lastValue { 0.0f };
        float lastDeriv { 0.0f };
    };
}
