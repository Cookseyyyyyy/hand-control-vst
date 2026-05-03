#include "OnnxHandTracker.h"

#include "ImagePreprocess.h"
#include "PalmAnchors.h"
#include "Roi.h"

#include <HandControlModels.h>

// Without ORT_API_MANUAL_INIT, onnxruntime_cxx_api.h initialises a global API
// pointer by calling OrtGetApiBase() during DLL load. That defeats Windows
// delay-loading: Ableton's scanner LoadLibrarys the VST3, the static init
// immediately tries to resolve onnxruntime.dll, and the plugin is rejected
// before our delay-load hook can resolve the runtime from the bundle.
//
// Manual init keeps plugin loading side-effect free. We call Ort::InitApi()
// explicitly in initialise(), after the DLL is actually needed.
#define ORT_API_MANUAL_INIT
#include <onnxruntime_cxx_api.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <string>
#include <vector>

namespace handcontrol::tracking
{
    #ifdef _WIN32
    extern "C" const OrtApiBase* handcontrolLoadOrtApiBaseFromPluginDir();
    #endif

    namespace
    {
        constexpr int kPalmInputSize     = 192;
        constexpr int kHandInputSize     = 224;

        // Match MediaPipe Hand Landmarker defaults exactly. v0.3 used a more
        // permissive hysteresis (0.55/0.4) that held tracking with low-confidence
        // landmarks and contributed to the "feels worse than v0.1" perception.
        constexpr float kPalmScoreThresh = 0.5f;
        constexpr float kPalmNmsIou      = 0.3f;
        constexpr float kHandConfThresh  = 0.5f;

        // Inter-frame tracking confidence: drop the cached ROI if the bbox of
        // current landmarks overlaps the previous frame's bbox by less than
        // this. Catches landmark drift before it compounds into a wrong ROI.
        constexpr float kTrackingIouThresh = 0.5f;

        // Periodic palm refresh: fall back to full re-detection every N frames
        // even when tracking looks healthy. This recovers cheaply if the slot
        // has subtly stuck on a hand-like region (e.g. a face).
        constexpr int   kPalmRefreshInterval = 10;

        constexpr int   kPalmLandmarks = 7;
        constexpr int   kHandLandmarks = 21;
        constexpr int   kPalmOutputDim = 4 + kPalmLandmarks * 2;   // 18

        struct PalmDetection
        {
            float score { 0.0f };
            float x1, y1, x2, y2;                              // original image pixels
            std::array<Point2D, kPalmLandmarks> landmarks {};
        };

        float sigmoid(float v) noexcept
        {
            return 1.0f / (1.0f + std::exp(-v));
        }

        float palmIou(const PalmDetection& a, const PalmDetection& b) noexcept
        {
            return bboxIou(a.x1, a.y1, a.x2, a.y2, b.x1, b.y1, b.x2, b.y2);
        }

        std::vector<PalmDetection> nms(std::vector<PalmDetection> in, float iouThresh)
        {
            std::sort(in.begin(), in.end(),
                [](const PalmDetection& a, const PalmDetection& b) { return a.score > b.score; });
            std::vector<PalmDetection> out;
            for (auto& d : in)
            {
                bool keep = true;
                for (const auto& k : out)
                {
                    if (palmIou(d, k) > iouThresh) { keep = false; break; }
                }
                if (keep) out.push_back(d);
            }
            return out;
        }

        using BBox = LandmarkBbox;
    }

    struct OnnxHandTracker::Impl
    {
        Ort::Env env { ORT_LOGGING_LEVEL_WARNING, "HandControl" };
        Ort::SessionOptions sessionOpts;
        std::unique_ptr<Ort::Session> palmSession;
        std::unique_ptr<Ort::Session> handSession;

        Ort::AllocatorWithDefaultOptions allocator;
        Ort::MemoryInfo memInfo { Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault) };

        std::string palmInputName;
        std::vector<std::string> palmOutputNames;
        int palmDeltaOutputIdx { 0 };
        int palmScoreOutputIdx { 1 };

        std::string handInputName;
        std::vector<std::string> handOutputNames;
        int handLandmarksIdx { 0 };
        int handConfIdx      { 1 };
        int handednessIdx    { 2 };

        std::vector<Anchor> anchors;
        std::vector<float> palmTensorBuf;
        std::vector<float> handTensorBuf;

        // Single-hand tracking state. v0.3's two-slot setup was fundamentally
        // flawed: there was no clean way to disambiguate "same hand, two
        // detections" from "two real hands" without much heavier landmark
        // matching. Single-hand removes the whole class of problem.
        struct Slot
        {
            bool   active { false };
            RoiTransform roi {};
            float  lastConfidence { 0.0f };
            Handedness handedness { Handedness::unknown };
            std::array<Point2D, kHandLandmarks> lastLandmarks {};
            BBox   lastBbox {};
        };
        Slot slot {};

        int framesSincePalmRefresh { kPalmRefreshInterval };

        Impl()
        {
            sessionOpts.SetIntraOpNumThreads(1);
            sessionOpts.SetInterOpNumThreads(1);
            sessionOpts.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);
        }

        std::optional<std::string> loadModels()
        {
            int palmSize = 0;
            const char* palmBytes = handcontrol::models::getNamedResource(
                "palm_detection_mediapipe_2023feb_onnx", palmSize);
            int handSize = 0;
            const char* handBytes = handcontrol::models::getNamedResource(
                "handpose_estimation_mediapipe_2023feb_onnx", handSize);

            if (palmBytes == nullptr || palmSize <= 0)
                return std::string("Embedded palm detection model missing from plugin binary.");
            if (handBytes == nullptr || handSize <= 0)
                return std::string("Embedded hand landmark model missing from plugin binary.");

            try
            {
                palmSession = std::make_unique<Ort::Session>(
                    env, palmBytes, static_cast<size_t>(palmSize), sessionOpts);
                handSession = std::make_unique<Ort::Session>(
                    env, handBytes, static_cast<size_t>(handSize), sessionOpts);
            }
            catch (const Ort::Exception& e)
            {
                return std::string("ONNX Runtime failed to load models: ") + e.what();
            }

            return std::nullopt;
        }

        void queryModelIO()
        {
            {
                auto in0 = palmSession->GetInputNameAllocated(0, allocator);
                palmInputName = in0.get();

                palmOutputNames.clear();
                const size_t n = palmSession->GetOutputCount();
                palmOutputNames.reserve(n);
                for (size_t i = 0; i < n; ++i)
                {
                    auto name = palmSession->GetOutputNameAllocated(i, allocator);
                    palmOutputNames.emplace_back(name.get());

                    auto info = palmSession->GetOutputTypeInfo(i).GetTensorTypeAndShapeInfo();
                    auto shape = info.GetShape();
                    if (! shape.empty() && shape.back() == kPalmOutputDim)
                        palmDeltaOutputIdx = static_cast<int>(i);
                    else if (! shape.empty() && shape.back() == 1)
                        palmScoreOutputIdx = static_cast<int>(i);
                }
            }
            {
                auto in0 = handSession->GetInputNameAllocated(0, allocator);
                handInputName = in0.get();

                handOutputNames.clear();
                const size_t n = handSession->GetOutputCount();
                handOutputNames.reserve(n);

                int seenLm = 0;
                int seenScalar = 0;
                for (size_t i = 0; i < n; ++i)
                {
                    auto name = handSession->GetOutputNameAllocated(i, allocator);
                    handOutputNames.emplace_back(name.get());

                    auto info = handSession->GetOutputTypeInfo(i).GetTensorTypeAndShapeInfo();
                    auto shape = info.GetShape();
                    int64_t totalElems = 1;
                    for (auto d : shape) if (d > 0) totalElems *= d;

                    if (totalElems == kHandLandmarks * 3)
                    {
                        if (seenLm == 0) handLandmarksIdx = static_cast<int>(i);
                        ++seenLm;
                    }
                    else if (totalElems == 1)
                    {
                        if (seenScalar == 0)      handConfIdx   = static_cast<int>(i);
                        else if (seenScalar == 1) handednessIdx = static_cast<int>(i);
                        ++seenScalar;
                    }
                }
            }
        }

        std::vector<PalmDetection> runPalmDetector(const juce::Image& frame, float& outBestScore)
        {
            outBestScore = 0.0f;

            int padX = 0, padY = 0;
            letterboxToTensor(frame, kPalmInputSize, palmTensorBuf, padX, padY);

            std::array<int64_t, 4> shape { 1, kPalmInputSize, kPalmInputSize, 3 };
            Ort::Value input = Ort::Value::CreateTensor<float>(
                memInfo,
                palmTensorBuf.data(),
                palmTensorBuf.size(),
                shape.data(), shape.size());

            std::array<const char*, 1> inputNames { palmInputName.c_str() };
            std::vector<const char*> outputNames;
            outputNames.reserve(palmOutputNames.size());
            for (auto& s : palmOutputNames) outputNames.push_back(s.c_str());

            std::vector<Ort::Value> outputs;
            try
            {
                outputs = palmSession->Run(Ort::RunOptions {},
                                           inputNames.data(), &input, 1,
                                           outputNames.data(), outputNames.size());
            }
            catch (const Ort::Exception& e)
            {
                juce::Logger::writeToLog(juce::String("Palm detector run failed: ") + e.what());
                return {};
            }

            const float* deltas = outputs[static_cast<size_t>(palmDeltaOutputIdx)].GetTensorData<float>();
            const float* scores = outputs[static_cast<size_t>(palmScoreOutputIdx)].GetTensorData<float>();

            const int imgW = frame.getWidth();
            const int imgH = frame.getHeight();
            const int scale = std::max(imgW, imgH);

            std::vector<PalmDetection> raw;
            raw.reserve(64);
            for (int i = 0; i < kNumPalmAnchors; ++i)
            {
                const float s = sigmoid(scores[i]);
                if (s > outBestScore) outBestScore = s;
                if (s < kPalmScoreThresh) continue;

                const float* d = deltas + i * kPalmOutputDim;
                const float ancX = anchors[static_cast<size_t>(i)].cx;
                const float ancY = anchors[static_cast<size_t>(i)].cy;

                const float cx = d[0] / static_cast<float>(kPalmInputSize);
                const float cy = d[1] / static_cast<float>(kPalmInputSize);
                const float w  = d[2] / static_cast<float>(kPalmInputSize);
                const float h  = d[3] / static_cast<float>(kPalmInputSize);

                PalmDetection det;
                det.score = s;
                det.x1 = (cx - w * 0.5f + ancX) * scale - static_cast<float>(padX);
                det.y1 = (cy - h * 0.5f + ancY) * scale - static_cast<float>(padY);
                det.x2 = (cx + w * 0.5f + ancX) * scale - static_cast<float>(padX);
                det.y2 = (cy + h * 0.5f + ancY) * scale - static_cast<float>(padY);

                for (int lm = 0; lm < kPalmLandmarks; ++lm)
                {
                    const float lx = d[4 + lm * 2 + 0] / static_cast<float>(kPalmInputSize);
                    const float ly = d[4 + lm * 2 + 1] / static_cast<float>(kPalmInputSize);
                    det.landmarks[static_cast<size_t>(lm)].x = (lx + ancX) * scale - static_cast<float>(padX);
                    det.landmarks[static_cast<size_t>(lm)].y = (ly + ancY) * scale - static_cast<float>(padY);
                }
                raw.push_back(det);
            }
            return nms(std::move(raw), kPalmNmsIou);
        }

        RoiTransform roiFromPalm(const PalmDetection& det) const
        {
            RoiTransform roi;
            const auto& p1 = det.landmarks[0];
            const auto& p2 = det.landmarks[2];
            const float dx = p2.x - p1.x;
            const float dy = p2.y - p1.y;
            roi.rotationRad = std::atan2(dx, -dy);

            const float bboxW = std::max(1.0f, det.x2 - det.x1);
            const float bboxH = std::max(1.0f, det.y2 - det.y1);
            const float bboxCx = 0.5f * (det.x1 + det.x2);
            const float bboxCy = 0.5f * (det.y1 + det.y2);

            // Initial ROI from a fresh palm detection: 2.6x scale, slight shift
            // toward the fingers along the hand's axis. After the first frame
            // we switch to the much-tighter landmark-derived ROI.
            const float side = std::max(bboxW, bboxH) * 2.6f;
            const float shiftLocal = -0.2f * side;

            roi.size    = side;
            roi.centerX = bboxCx + std::sin(roi.rotationRad) * shiftLocal;
            roi.centerY = bboxCy - std::cos(roi.rotationRad) * shiftLocal;
            return roi;
        }

        struct LandmarkResult
        {
            bool ok { false };
            float confidence { 0.0f };
            Handedness handedness { Handedness::unknown };
            std::array<Point2D, kHandLandmarks> landmarks {};  // pixel coords in source frame
        };

        LandmarkResult runHandLandmarks(const juce::Image& frame, const RoiTransform& roi)
        {
            LandmarkResult out;
            cropRotateResizeToTensor(frame, roi, kHandInputSize, handTensorBuf);

            std::array<int64_t, 4> shape { 1, kHandInputSize, kHandInputSize, 3 };
            Ort::Value input = Ort::Value::CreateTensor<float>(
                memInfo,
                handTensorBuf.data(),
                handTensorBuf.size(),
                shape.data(), shape.size());

            std::array<const char*, 1> inputNames { handInputName.c_str() };
            std::vector<const char*> outputNames;
            outputNames.reserve(handOutputNames.size());
            for (auto& s : handOutputNames) outputNames.push_back(s.c_str());

            std::vector<Ort::Value> outputs;
            try
            {
                outputs = handSession->Run(Ort::RunOptions {},
                                           inputNames.data(), &input, 1,
                                           outputNames.data(), outputNames.size());
            }
            catch (const Ort::Exception& e)
            {
                juce::Logger::writeToLog(juce::String("Hand landmark run failed: ") + e.what());
                return out;
            }

            const float* lmRaw   = outputs[static_cast<size_t>(handLandmarksIdx)].GetTensorData<float>();
            const float  confRaw = *outputs[static_cast<size_t>(handConfIdx)].GetTensorData<float>();
            const float  handRaw = *outputs[static_cast<size_t>(handednessIdx)].GetTensorData<float>();

            const float conf = (confRaw >= 0.0f && confRaw <= 1.0f) ? confRaw : sigmoid(confRaw);
            out.confidence = conf;
            out.handedness = (handRaw > 0.5f) ? Handedness::right : Handedness::left;

            const float handSide = static_cast<float>(kHandInputSize);
            const float cosR = std::cos(roi.rotationRad);
            const float sinR = std::sin(roi.rotationRad);

            for (int i = 0; i < kHandLandmarks; ++i)
            {
                const float lx = lmRaw[i * 3 + 0];
                const float ly = lmRaw[i * 3 + 1];
                const float nx = (lx / handSide - 0.5f) * roi.size;
                const float ny = (ly / handSide - 0.5f) * roi.size;
                out.landmarks[static_cast<size_t>(i)].x = roi.centerX + (nx * cosR - ny * sinR);
                out.landmarks[static_cast<size_t>(i)].y = roi.centerY + (nx * sinR + ny * cosR);
            }
            out.ok = true;
            return out;
        }
    };

    OnnxHandTracker::OnnxHandTracker() = default;
    OnnxHandTracker::~OnnxHandTracker() = default;

    std::optional<std::string> OnnxHandTracker::initialise()
    {
        #ifdef _WIN32
        const auto* apiBase = handcontrolLoadOrtApiBaseFromPluginDir();
        if (apiBase == nullptr)
            return std::string("Failed to load bundled onnxruntime.dll from the plugin directory.");
        Ort::InitApi(apiBase->GetApi(ORT_API_VERSION));
        #else
        Ort::InitApi();
        #endif

        if (impl == nullptr)
            impl = std::make_unique<Impl>();

        if (auto err = impl->loadModels())
            return err;
        impl->queryModelIO();
        impl->anchors = buildPalmAnchors();
        impl->slot.active = false;
        impl->framesSincePalmRefresh = kPalmRefreshInterval;
        return std::nullopt;
    }

    TrackingResult OnnxHandTracker::process(const juce::Image& frame, double timestampSeconds)
    {
        TrackingResult result;
        result.timestampSeconds = timestampSeconds;

        if (! frame.isValid() || impl == nullptr
            || impl->palmSession == nullptr || impl->handSession == nullptr)
            return result;

        auto& slot = impl->slot;

        bool ranLandmarkOnCachedRoi = false;
        bool ranPalm = false;

        // ---- Step 1: cached-ROI tracking, if active ---------------------------
        if (slot.active)
        {
            ranLandmarkOnCachedRoi = true;
            auto lr = impl->runHandLandmarks(frame, slot.roi);

            // Drop the slot if confidence is too low...
            bool dropSlot = (! lr.ok) || (lr.confidence < kHandConfThresh);

            // ...or if the new landmarks have drifted too far from the previous
            // frame's bbox (MediaPipe's min_tracking_confidence equivalent).
            if (! dropSlot)
            {
                const auto newBbox = computeLandmarkBbox(lr.landmarks);
                if (slot.lastBbox.valid && newBbox.valid)
                {
                    const float iou = bboxIou(slot.lastBbox.x1, slot.lastBbox.y1,
                                              slot.lastBbox.x2, slot.lastBbox.y2,
                                              newBbox.x1, newBbox.y1,
                                              newBbox.x2, newBbox.y2);
                    if (iou < kTrackingIouThresh) dropSlot = true;
                }
            }

            if (dropSlot)
            {
                slot.active = false;
                slot.lastConfidence = lr.ok ? lr.confidence : 0.0f;
            }
            else
            {
                slot.lastConfidence = lr.confidence;
                slot.handedness     = lr.handedness;
                slot.lastLandmarks  = lr.landmarks;
                slot.roi            = roiFromLandmarks(lr.landmarks);
                slot.lastBbox       = computeLandmarkBbox(lr.landmarks);
            }
        }

        // ---- Step 2: palm detector if no active slot, or for periodic refresh -
        const bool needsPalm = (! slot.active)
                             || (++impl->framesSincePalmRefresh >= kPalmRefreshInterval);

        if (needsPalm)
        {
            ranPalm = true;
            float bestPalmScore = 0.0f;
            auto detections = impl->runPalmDetector(frame, bestPalmScore);
            impl->framesSincePalmRefresh = 0;
            result.diagnostics.lastPalmScore   = bestPalmScore;
            result.diagnostics.palmRanThisFrame = true;

            if (! detections.empty())
            {
                // Single-hand: just take the highest-scoring detection
                // (NMS already deduplicated within-frame palm overlaps).
                const auto& best = detections[0];

                if (slot.active)
                {
                    // Periodic refresh: only replace the cached track if the
                    // new detection significantly disagrees with the cached
                    // ROI's current centre - protects against the detector
                    // briefly latching onto a face or noise.
                    const float cx = 0.5f * (best.x1 + best.x2);
                    const float cy = 0.5f * (best.y1 + best.y2);
                    const float dx = cx - slot.roi.centerX;
                    const float dy = cy - slot.roi.centerY;
                    const float dist2 = dx * dx + dy * dy;
                    const float roiHalf = slot.roi.size * 0.5f;
                    if (dist2 > roiHalf * roiHalf)
                    {
                        // The detector is finding something far from where we
                        // were tracking - prefer the fresh detection.
                        const RoiTransform candidateRoi = impl->roiFromPalm(best);
                        const auto lr = impl->runHandLandmarks(frame, candidateRoi);
                        if (lr.ok && lr.confidence >= kHandConfThresh)
                        {
                            slot.active         = true;
                            slot.lastConfidence = lr.confidence;
                            slot.handedness     = lr.handedness;
                            slot.lastLandmarks  = lr.landmarks;
                            slot.roi            = roiFromLandmarks(lr.landmarks);
                            slot.lastBbox       = computeLandmarkBbox(lr.landmarks);
                        }
                    }
                }
                else
                {
                    // Empty slot - run landmark on the candidate to verify
                    // before committing.
                    const RoiTransform candidateRoi = impl->roiFromPalm(best);
                    const auto lr = impl->runHandLandmarks(frame, candidateRoi);
                    if (lr.ok && lr.confidence >= kHandConfThresh)
                    {
                        slot.active         = true;
                        slot.lastConfidence = lr.confidence;
                        slot.handedness     = lr.handedness;
                        slot.lastLandmarks  = lr.landmarks;
                        slot.roi            = roiFromLandmarks(lr.landmarks);
                        slot.lastBbox       = computeLandmarkBbox(lr.landmarks);
                    }
                }
            }
        }

        // ---- Step 3: emit the result -------------------------------------------
        const float frameW = static_cast<float>(frame.getWidth());
        const float frameH = static_cast<float>(frame.getHeight());

        auto& outHand = result.hands[0];
        outHand.present     = slot.active;
        outHand.handedness  = slot.handedness;
        outHand.confidence  = slot.lastConfidence;
        for (int j = 0; j < kHandLandmarks; ++j)
        {
            outHand.landmarks[static_cast<size_t>(j)].x =
                slot.lastLandmarks[static_cast<size_t>(j)].x / frameW;
            outHand.landmarks[static_cast<size_t>(j)].y =
                slot.lastLandmarks[static_cast<size_t>(j)].y / frameH;
        }

        result.diagnostics.activeRois[0] = slot.roi;
        result.diagnostics.roiActive[0]  = slot.active;

        // Surface "what path did we take this frame" so the status bar can
        // show it. Useful when investigating tracking issues.
        juce::ignoreUnused(ranLandmarkOnCachedRoi, ranPalm);

        return result;
    }
}
