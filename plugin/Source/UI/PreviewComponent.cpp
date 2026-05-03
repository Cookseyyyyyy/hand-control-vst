#include "PreviewComponent.h"
#include "Theme.h"

namespace handcontrol::ui
{
    using handcontrol::tracking::Landmark;

    namespace
    {
        // MediaPipe skeleton connections (pairs of landmark indices).
        constexpr std::array<std::pair<int, int>, 20> kConnections = {{
            {0, 1}, {1, 2}, {2, 3}, {3, 4},          // thumb
            {0, 5}, {5, 6}, {6, 7}, {7, 8},          // index
            {5, 9}, {9, 10}, {10, 11}, {11, 12},      // middle
            {9, 13}, {13, 14}, {14, 15}, {15, 16},    // ring
            {13, 17}, {17, 18}, {18, 19}, {19, 20}    // pinky + palm
        }};
    }

    PreviewComponent::PreviewComponent(handcontrol::tracking::HandTrackerThread& src)
        : source(src)
    {
        setOpaque(true);
        gl.attachTo(*this);
        startTimerHz(30);
    }

    PreviewComponent::~PreviewComponent()
    {
        gl.detach();
        stopTimer();
    }

    void PreviewComponent::timerCallback()
    {
        snapshot = source.latestSnapshotForUI();
        repaint();
    }

    void PreviewComponent::resized() {}

    void PreviewComponent::paint(juce::Graphics& g)
    {
        const auto bounds = getLocalBounds().toFloat();
        drawPanel(g, bounds, 10.0f);

        auto inner = bounds.reduced(6.0f);

        juce::Rectangle<float> frameRect;
        if (snapshot.frame.isValid())
        {
            const auto aspect = (float) snapshot.frame.getWidth()
                              / (float) juce::jmax(1, snapshot.frame.getHeight());
            const auto innerAspect = inner.getWidth() / juce::jmax(1.0f, inner.getHeight());
            if (aspect > innerAspect)
            {
                const auto h = inner.getWidth() / aspect;
                frameRect = { inner.getX(), inner.getCentreY() - h * 0.5f, inner.getWidth(), h };
            }
            else
            {
                const auto w = inner.getHeight() * aspect;
                frameRect = { inner.getCentreX() - w * 0.5f, inner.getY(), w, inner.getHeight() };
            }

            g.drawImage(snapshot.frame, frameRect, juce::RectanglePlacement::stretchToFit);
            g.setColour(colours::background.withAlpha(0.25f));
            g.fillRect(frameRect);
        }
        else
        {
            frameRect = inner;
            g.setColour(colours::panelRaised);
            g.fillRoundedRectangle(frameRect, 8.0f);
            g.setColour(colours::textDim);
            g.setFont(juce::Font(juce::FontOptions(14.0f, juce::Font::plain)));
            g.drawFittedText("No camera feed", frameRect.toNearestInt(),
                             juce::Justification::centred, 1);
            return;
        }

        // Single-hand only (v0.4): just slot 0.
        if (showRoi && snapshot.result.diagnostics.roiActive[0])
        {
            drawRoi(g, snapshot.result.diagnostics.activeRois[0],
                    frameRect, colours::accentHand1);
        }

        const auto& hand = snapshot.result.hands[0];
        if (hand.present)
            drawLandmarks(g, hand, frameRect, colours::accentHand1, "Hand");
    }

    void PreviewComponent::drawLandmarks(juce::Graphics& g,
                                         const handcontrol::tracking::HandFrame& hand,
                                         const juce::Rectangle<float>& frameRect,
                                         juce::Colour accent,
                                         const juce::String& label)
    {
        auto toScreen = [&](const handcontrol::tracking::Point2D& p) -> juce::Point<float>
        {
            return { frameRect.getX() + p.x * frameRect.getWidth(),
                     frameRect.getY() + p.y * frameRect.getHeight() };
        };

        g.setColour(accent.withAlpha(0.85f));
        for (const auto& c : kConnections)
        {
            const auto a = toScreen(hand.landmarks[static_cast<size_t>(c.first)]);
            const auto b = toScreen(hand.landmarks[static_cast<size_t>(c.second)]);
            g.drawLine({ a, b }, 1.6f);
        }

        auto highlight = [&](int a, int b, juce::Colour colour, float thickness)
        {
            const auto pa = toScreen(hand.landmarks[static_cast<size_t>(a)]);
            const auto pb = toScreen(hand.landmarks[static_cast<size_t>(b)]);

            auto glow = colour.withAlpha(0.35f);
            g.setColour(glow);
            g.drawLine({ pa, pb }, thickness * 2.5f);
            g.setColour(colour);
            g.drawLine({ pa, pb }, thickness);
        };
        highlight((int) Landmark::thumbTip, (int) Landmark::indexTip, colours::accentIndex, 3.0f);
        highlight((int) Landmark::thumbTip, (int) Landmark::pinkyTip, colours::accentPinky, 3.0f);

        for (const auto& lm : hand.landmarks)
        {
            auto p = toScreen(lm);
            g.setColour(accent);
            g.fillEllipse(p.x - 2.5f, p.y - 2.5f, 5.0f, 5.0f);
        }

        const auto wristPt = toScreen(hand.at(Landmark::wrist));
        g.setColour(colours::background.withAlpha(0.7f));
        const auto labelRect = juce::Rectangle<float>(wristPt.x - 18.0f, wristPt.y + 6.0f, 36.0f, 18.0f);
        g.fillRoundedRectangle(labelRect, 4.0f);
        g.setColour(accent);
        g.setFont(juce::Font(juce::FontOptions(11.5f, juce::Font::bold)));
        g.drawFittedText(label, labelRect.toNearestInt(), juce::Justification::centred, 1);
    }

    void PreviewComponent::drawRoi(juce::Graphics& g,
                                   const handcontrol::tracking::RoiTransform& roi,
                                   const juce::Rectangle<float>& frameRect,
                                   juce::Colour accent)
    {
        if (! snapshot.frame.isValid()) return;
        const float frameW = static_cast<float>(snapshot.frame.getWidth());
        const float frameH = static_cast<float>(snapshot.frame.getHeight());
        if (frameW <= 0.0f || frameH <= 0.0f) return;

        // Convert ROI corners (pixel coords in the camera frame) into the
        // displayed frameRect's coords.
        auto cameraToScreen = [&](float cx, float cy) -> juce::Point<float>
        {
            return { frameRect.getX() + (cx / frameW) * frameRect.getWidth(),
                     frameRect.getY() + (cy / frameH) * frameRect.getHeight() };
        };

        const float half = roi.size * 0.5f;
        const float c = std::cos(roi.rotationRad);
        const float s = std::sin(roi.rotationRad);

        // Four ROI corners in local frame, rotated into camera coords.
        std::array<juce::Point<float>, 4> corners {{
            cameraToScreen(roi.centerX + (-half * c - -half * s),
                           roi.centerY + (-half * s + -half * c)),
            cameraToScreen(roi.centerX + ( half * c - -half * s),
                           roi.centerY + ( half * s + -half * c)),
            cameraToScreen(roi.centerX + ( half * c -  half * s),
                           roi.centerY + ( half * s +  half * c)),
            cameraToScreen(roi.centerX + (-half * c -  half * s),
                           roi.centerY + (-half * s +  half * c))
        }};

        juce::Path p;
        p.startNewSubPath(corners[0]);
        for (int i = 1; i < 4; ++i) p.lineTo(corners[i]);
        p.closeSubPath();

        g.setColour(accent.withAlpha(0.6f));
        g.strokePath(p, juce::PathStrokeType(1.5f));
    }
}
