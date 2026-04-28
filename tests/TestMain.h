#pragma once

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <string>
#include <vector>

namespace handcontrol::test
{
    struct TestCase
    {
        const char* name;
        std::function<void()> fn;
    };

    inline std::vector<TestCase>& registry()
    {
        static std::vector<TestCase> cases;
        return cases;
    }

    inline int& failureCounter()
    {
        static int c = 0;
        return c;
    }

    struct Registrar
    {
        Registrar(const char* name, std::function<void()> fn)
        {
            registry().push_back({ name, std::move(fn) });
        }
    };

    inline void reportFailure(const char* file, int line, const char* msg)
    {
        std::fprintf(stderr, "  FAIL: %s:%d  %s\n", file, line, msg);
        ++failureCounter();
    }

    inline int runAll()
    {
        int totalFailures = 0;
        for (auto& tc : registry())
        {
            const int before = failureCounter();
            std::printf("[RUN ] %s\n", tc.name);
            try { tc.fn(); }
            catch (const std::exception& e)
            {
                std::fprintf(stderr, "  EXCEPTION: %s\n", e.what());
                ++failureCounter();
            }
            const int after = failureCounter();
            if (after == before)
                std::printf("[ OK ] %s\n", tc.name);
            else
            {
                std::printf("[FAIL] %s (%d failures)\n", tc.name, after - before);
                totalFailures += (after - before);
            }
        }
        std::printf("\n%d test(s) run, %d failure(s).\n",
                    (int) registry().size(), totalFailures);
        return totalFailures == 0 ? 0 : 1;
    }
}

#define HC_TEST(name) \
    static void name(); \
    static ::handcontrol::test::Registrar registrar_##name(#name, name); \
    static void name()

#define HC_CHECK(cond) \
    do { if (! (cond)) ::handcontrol::test::reportFailure(__FILE__, __LINE__, #cond); } while (0)

#define HC_CHECK_NEAR(a, b, eps) \
    do { \
        const double _aa = static_cast<double>(a); \
        const double _bb = static_cast<double>(b); \
        if (std::fabs(_aa - _bb) > (eps)) { \
            char _buf[256]; \
            std::snprintf(_buf, sizeof(_buf), "|%g - %g| > %g  (%s vs %s)", \
                          _aa, _bb, (double)(eps), #a, #b); \
            ::handcontrol::test::reportFailure(__FILE__, __LINE__, _buf); \
        } \
    } while (0)
