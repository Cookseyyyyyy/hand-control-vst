#include "PalmAnchors.h"

namespace handcontrol::tracking
{
    std::vector<Anchor> buildPalmAnchors()
    {
        std::vector<Anchor> anchors;
        anchors.reserve(kNumPalmAnchors);

        auto appendLayer = [&](int featureMapSize, int anchorsPerCell)
        {
            const float inv = 1.0f / static_cast<float>(featureMapSize);
            for (int y = 0; y < featureMapSize; ++y)
            {
                for (int x = 0; x < featureMapSize; ++x)
                {
                    const float cx = (static_cast<float>(x) + 0.5f) * inv;
                    const float cy = (static_cast<float>(y) + 0.5f) * inv;
                    for (int a = 0; a < anchorsPerCell; ++a)
                        anchors.push_back({ cx, cy });
                }
            }
        };

        appendLayer(24, 2);  // stride 8:  1152 anchors
        appendLayer(12, 6);  // stride 16:  864 anchors

        return anchors;
    }
}
