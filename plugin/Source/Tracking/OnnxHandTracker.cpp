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
        constexpr float kPalmScoreThresh = 0.4f;   // lowered: palm rarely runs now
        constexpr float kPalmNmsIou      = 0.3f;
        constexpr float kHandEnterThresh = 0.55f;  // confidence needed to fill a slot
        constexpr float kHandExitThresh  = 0.4f;   // confidence below which a slot is dropped
        constexpr float kSlotOverlapIou  = 0.5f;   // skip new detection if it overlaps an existing slot
        constexpr int   kMaxHands        = 2;
        constexpr int   kPalmLandmarks   = 7;
        constexpr int   kHandLandmarks   = 21;
        constexpr int   kPalmOutputDim   = 4 + kPalmLandmarks * 2;   // 18

        // Re-run the palm detector occasionally even when slots are full, so we can
        // pick up a new hand entering the frame.
        constexpr int   kPalmRefreshInterval = 30;  // frames

        struct PalmDetection
        {
            float score { 0.0f };
            float x1, y1, x2, y2;                              // in original image pixels
            std::array<Point2D, kPalmLandmarks> landmarks {};  // in original image pixels
        };

        float sigmoid(float v) noexcept
        {
            return 1.0f / (1.0f + std::exp(-v));
        }

        float iou(const PalmDetection& a, const PalmDetection& b) noexcept
        {
            const float ix1 = std::max(a.x1, b.x1);
            const float iy1 = std::max(a.y1, b.y1);
            const float ix2 = std::min(a.x2, b.x2);
            const float iy2 = std::min(a.y2, b.y2);
            const float iw  = std::max(0.0f, ix2 - ix1);
            const float ih  = std::max(0.0f, iy2 - iy1);
            const float inter = iw * ih;
            const float aA = std::max(0.0f, a.x2 - a.x1) * std::max(0.0f, a.y2 - a.y1);
            const float aB = std::max(0.0f, b.x2 - b.x1) * std::max(0.0f, b.y2 - b.y1);
            const float un = aA + aB - inter;
            return un > 0.0f ? inter / un : 0.0f;
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
                    if (iou(d, k) > iouThresh) { keep = false; break; }
                }
                if (keep) out.push_back(d);
            }
            return out;
        }

        // Axis-aligned IoU between two ROIs, used for "is this new palm detection
        // already covered by a tracked slot".
        float roiIou(const RoiTransform& a, const RoiTransform& b) noexcept
        {
            const float aHalf = a.size * 0.5f;
            const float bHalf = b.size * 0.5f;
            const float ix1 = std::max(a.centerX - aHalf, b.centerX - bHalf);
            const float iy1 = std::max(a.centerY - aHalf, b.centerY - bHalf);
            const float ix2 = std::min(a.centerX + aHalf, b.centerX + bHalf);
            const float iy2 = std::min(a.centerY + aHalf, b.centerY + bHalf);
            const float iw = std::max(0.0f, ix2 - ix1);
            const float ih = std::max(0.0f, iy2 - iy1);
            const float inter = iw * ih;
            const float un = a.size * a.size + b.size * b.size - inter;
            return un > 0.0f ? inter / un : 0.0f;
        }
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

        // Per-slot tracking state. When a slot is active we keep its ROI
        // and reuse it next frame, skipping the palm detector entirely.
        struct TrackingSlot
        {
            bool   active        { false };
            RoiTransform roi     {};
            float  lastConfidence { 0.0f };
            Handedness handedness { Handedness::unknown };
            std::array<Point2D, kHandLandmarks> lastLandmarks {};
        };
        std::array<TrackingSlot, kMaxHands> slots {};

        int framesSincePalmRefresh { kPalmRefreshInterval };  // run on first frame

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

            // Combined enlarge / shift approximating MediaPipe's two-pass transform
            // for the *first* ROI from a palm detection. After the first frame we
            // switch to roiFromLandmarks, which is much tighter.
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
            std::array<Point2D, kHandLandmarks> landmarks {};
        };

        // Returns landmarks in *pixel* coords of the source frame. Normalisation
        // to [0,1] for downstream consumers happens later in process().
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
                const float srcX = roi.centerX + (nx * cosR - ny * sinR);
                const float srcY = roi.centerY + (nx * sinR + ny * cosR);

                // Pixel coords - we will normalise when emitting the public result.
                out.landmarks[static_cast<size_t>(i)].x = srcX;
                out.landmarks[static_cast<size_t>(i)].y = srcY;
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
        for (auto& s : impl->slots) s.active = false;
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

        // ---- Step 1: refresh tracked slots from their cached ROIs ---------------
        for (auto& slot : impl->slots)
        {
            if (! slot.active) continue;

            auto lr = impl->runHandLandmarks(frame, slot.roi);
            if (! lr.ok || lr.confidence <= kHandExitThresh)
            {
                slot.active = false;
                slot.lastConfidence = lr.ok ? lr.confidence : 0.0f;
                continue;
            }

            slot.lastConfidence = lr.confidence;
            slot.handedness     = lr.handedness;
            slot.lastLandmarks  = lr.landmarks;
            slot.roi            = roiFromLandmarks(lr.landmarks);
        }

        // ---- Step 2: run palm detector if any slot is empty (or for refresh) ---
        const bool anySlotEmpty = std::any_of(impl->slots.begin(), impl->slots.end(),
                                              [](const auto& s) { return ! s.active; });
        const bool periodicRefresh = (++impl->framesSincePalmRefresh) >= kPalmRefreshInterval;

        if (anySlotEmpty || periodicRefresh)
        {
            float bestPalmScore = 0.0f;
            auto detections = impl->runPalmDetector(frame, bestPalmScore);
            impl->framesSincePalmRefresh = 0;
            result.diagnostics.lastPalmScore   = bestPalmScore;
            result.diagnostics.palmRanThisFrame = true;

            for (auto& det : detections)
            {
                RoiTransform candidateRoi = impl->roiFromPalm(det);

                // Skip if this detection overlaps an already-tracked slot.
                bool overlapsExisting = false;
                for (const auto& s : impl->slots)
                {
                    if (! s.active) continue;
                    if (roiIou(s.roi, candidateRoi) > kSlotOverlapIou)
                    {
                        overlapsExisting = true;
                        break;
                    }
                }
                if (overlapsExisting) continue;

                // Find first empty slot.
                int target = -1;
                for (int i = 0; i < kMaxHands; ++i)
                {
                    if (! impl->slots[static_cast<size_t>(i)].active)
                    {
                        target = i;
                        break;
                    }
                }
                if (target < 0) break;

                // Run landmark model on the new ROI to verify before committing.
                auto lr = impl->runHandLandmarks(frame, candidateRoi);
                if (! lr.ok || lr.confidence < kHandEnterThresh) continue;

                auto& slot = impl->slots[static_cast<size_t>(target)];
                slot.active         = true;
                slot.lastConfidence = lr.confidence;
                slot.handedness     = lr.handedness;
                slot.lastLandmarks  = lr.landmarks;
                slot.roi            = roiFromLandmarks(lr.landmarks);
            }
        }

        // ---- Step 3: emit the result -------------------------------------------
        const float frameW = static_cast<float>(frame.getWidth());
        const float frameH = static_cast<float>(frame.getHeight());

        for (int i = 0; i < kMaxHands; ++i)
        {
            const auto& slot = impl->slots[static_cast<size_t>(i)];
            auto& outHand = result.hands[static_cast<size_t>(i)];
            outHand.present     = slot.active;
            outHand.handedness  = slot.handedness;
            outHand.confidence  = slot.lastConfidence;

            // Normalise pixel-space landmarks to [0,1] for downstream consumers.
            for (int j = 0; j < kHandLandmarks; ++j)
            {
                outHand.landmarks[static_cast<size_t>(j)].x =
                    slot.lastLandmarks[static_cast<size_t>(j)].x / frameW;
                outHand.landmarks[static_cast<size_t>(j)].y =
                    slot.lastLandmarks[static_cast<size_t>(j)].y / frameH;
            }

            // ROIs stay in pixel coords; the preview converts to its display
            // rectangle when drawing.
            result.diagnostics.activeRois[static_cast<size_t>(i)] = slot.roi;
            result.diagnostics.roiActive[static_cast<size_t>(i)]  = slot.active;
        }

        return result;
    }
}
