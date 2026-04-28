#include "OneEuroFilter.h"

#include <cmath>

namespace handcontrol::tracking
{
    double OneEuroFilter::alpha(double cutoff, double dt) noexcept
    {
        const double tau = 1.0 / (2.0 * 3.14159265358979323846 * cutoff);
        return 1.0 / (1.0 + tau / dt);
    }

    float OneEuroFilter::process(float value, double timestampSeconds) noexcept
    {
        if (! hasPrev)
        {
            hasPrev = true;
            lastTime = timestampSeconds;
            lastValue = value;
            lastDeriv = 0.0f;
            return value;
        }

        double dt = timestampSeconds - lastTime;
        if (dt <= 0.0)
            dt = 1.0 / 60.0;

        const double rawDeriv = (value - lastValue) / dt;
        const double aD = alpha(config.dCutoffHz, dt);
        const double smoothDeriv = aD * rawDeriv + (1.0 - aD) * lastDeriv;

        const double cutoff = config.minCutoffHz + config.beta * std::abs(smoothDeriv);
        const double a = alpha(cutoff, dt);
        const double smoothed = a * value + (1.0 - a) * lastValue;

        lastTime = timestampSeconds;
        lastValue = static_cast<float>(smoothed);
        lastDeriv = static_cast<float>(smoothDeriv);
        return lastValue;
    }

    void OneEuroFilter::reset() noexcept
    {
        hasPrev = false;
        lastTime = 0.0;
        lastValue = 0.0f;
        lastDeriv = 0.0f;
    }
}
