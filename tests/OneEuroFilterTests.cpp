#include "TestMain.h"

#include "Tracking/OneEuroFilter.h"

using handcontrol::tracking::OneEuroFilter;

HC_TEST(oneEuro_first_sample_passes_through)
{
    OneEuroFilter f;
    HC_CHECK_NEAR(f.process(0.7f, 0.0), 0.7f, 1.0e-6f);
}

HC_TEST(oneEuro_converges_to_constant_input)
{
    OneEuroFilter f;
    f.setConfig({ /*minCutoffHz*/ 1.0, /*beta*/ 0.0, /*dCutoff*/ 1.0 });
    float v = 0.0f;
    double t = 0.0;
    for (int i = 0; i < 200; ++i)
    {
        t += 1.0 / 30.0;
        v = f.process(0.5f, t);
    }
    HC_CHECK_NEAR(v, 0.5f, 1.0e-3f);
}

HC_TEST(oneEuro_higher_cutoff_means_less_lag)
{
    OneEuroFilter slow, fast;
    slow.setConfig({ 0.5, 0.0, 1.0 });
    fast.setConfig({ 10.0, 0.0, 1.0 });

    // Step response: signal jumps from 0 to 1 at t=0.1.
    double t = 0.0;
    for (int i = 0; i < 3; ++i) { t += 0.033; slow.process(0.0f, t); fast.process(0.0f, t); }
    float slowOut = 0, fastOut = 0;
    for (int i = 0; i < 3; ++i)
    {
        t += 0.033;
        slowOut = slow.process(1.0f, t);
        fastOut = fast.process(1.0f, t);
    }
    HC_CHECK(fastOut > slowOut);
}

HC_TEST(oneEuro_reset_clears_history)
{
    OneEuroFilter f;
    f.process(1.0f, 0.0);
    f.process(1.0f, 0.033);
    f.reset();
    HC_CHECK_NEAR(f.process(0.25f, 0.066), 0.25f, 1.0e-6f);
}
