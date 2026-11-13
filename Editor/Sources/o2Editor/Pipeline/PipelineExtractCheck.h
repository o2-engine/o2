#pragma once

#include <optional>

#include "o2/Utils/Coroutines/Coroutines.h"
#include "o2Editor/Pipeline/PipelineNodeType.h"
#include "o2Editor/Pipeline/PipelineRegions.h"
#include "o2Editor/Pipeline/UiLayout/UiLayoutJson.h"
#include "o2Editor/Pipeline/UiLayout/UiLayoutPixels.h"

using namespace o2;

namespace Editor
{
    // ---------------------------------------------------------------------------------------------------------------
    // The AI extract's check against the reference (port of AssetsLine shared/extractCheck.ts and
    // backend/src/nodes/extractCheck.ts). After a part is cut, the sprite is laid over the place it was cut from (where
    // the generation drew it in its frame, registered onto the crop) and a vision model compares the overlay with the
    // original; a significant difference draws the part again with the judge's notes added to its prompt, a limited
    // number of times, and the best attempt by the judge's likeness is kept. It runs as the node's second step
    // (IPipelineNodeImpl::Refine), cached apart from the generation. Every model answer is memoised by what was asked;
    // the details land in the node's work folder (check/<part>.json and its pictures), which the card's popup reads.
    // The switch, the limit and the judge (refCheck, refCheckMax, refCheckModel) are shared with the AI text style's check
    // ---------------------------------------------------------------------------------------------------------------
    namespace PipelineExtractCheck
    {
        using UiLayout::Box;
        using UiLayout::Json;
        using UiLayout::UiImage;

        extern const char* const defaultModel; // The vision model that judges when the node names none
        constexpr int defaultLimit = 2;        // Regenerations at most when the node names no valid limit

        // The limits a node may choose: 1, 2, 3
        const Vector<int>& Limits();

        // -------------------------
        // The check's settings
        // -------------------------
        struct Config
        {
            String model;              // The vision model that judges
            int    limit = defaultLimit; // Regenerations (or corrections) at most
        };

        // The check's settings of a node; false while it is off
        bool Read(const PipelineNode& node, Config& out);

        // The region in source pixels, exactly as the generation's crop cuts it
        Box RegionRect(int iw, int ih, const PipelineExtractRegion& region);

        // The region with some of the picture around it: a quarter of its larger side, at least 16 px, inside the image
        Box ContextRect(int iw, int ih, const Box& r);

        // ---------------------------------------------------------------------
        // The sprite of a stored generation and where the generation put it
        // ---------------------------------------------------------------------
        struct SpriteView
        {
            UiImage sprite;  // The cut sprite, trimmed to its content: what the part's result is
            String  png;     // Its PNG bytes
            Box     nominal; // Where it sits on the crop the model was given, crop pixels, before registration
            String  from;    // tracked: from where the model drew it in its frame; centred: the frame was not kept
        };

        // The sprite of a stored part and its place on its crop (cropW x cropH). A chroma part is stored as the raw render
        // and cut here as the executor's post-step cuts it; any other part is stored trimmed with its frame in the PNG
        // (PipelineExtract::ReadPngFrame); a result older than that note is taken as centred
        SpriteView SpriteViewOf(const String& stored, const PipelineNode& node, const String& portId, int cropW, int cropH);

        // ------------------------------------------------------------
        // Where a sprite was laid on its crop and the correction made
        // ------------------------------------------------------------
        struct Placement
        {
            Box    box;          // Crop pixels
            bool   fit = false;  // The registration moved or scaled it
            double scale = 1, dx = 0, dy = 0;
        };

        // Lays the sprite on its crop at claimed and moves it where it fits best: the part's measured size first (a part the
        // model drew clearly larger or smaller), a shift at that size, then the registration (a uniform scale and a fine shift)
        Placement PlaceOnCrop(const UiImage& sprite, const Box& claimed, const UiImage& crop);

        // -------------------------------------------------------------------
        // The reference with the sprite laid over it and how that changed it
        // -------------------------------------------------------------------
        struct Overlay
        {
            UiImage image;
            double  pixelDiff = 0;    // 0..1: mean colour distance under the sprite's opaque pixels
            double  pixelChanged = 0; // Share of those pixels that moved visibly (over a tenth of the range)
        };

        // Lays the sprite over the reference at the box (reference pixels)
        Overlay OverlayOn(const UiImage& reference, const UiImage& sprite, const Box& at);

        // The sprite alone on a grey checkerboard with a margin, as the judge sees it
        String SpriteOnChecker(const UiImage& sprite);

        // The judge's prompt for a part
        String JudgePrompt(const String& part);

        // ------------------------
        // What the judge answered
        // ------------------------
        struct JudgeAnswer
        {
            String         verdict;    // ok, minor or significant
            double         likeness = 0;
            Vector<String> differences;
            String         instructions;
        };

        // Reads the judge's answer whatever wraps it; false (with the reason) when it holds no verdict
        bool ParseJudge(const String& text, JudgeAnswer& out, String* error = nullptr);

        // ----------------------------------------------------
        // One attempt as the choice of the kept one needs it
        // ----------------------------------------------------
        struct Judged
        {
            int                   n = 1;
            String                verdict; // empty = not judged
            std::optional<double> likeness;
            Vector<String>        differences;
            String                instructions;
        };

        // What a regeneration's prompt gets after its own instructions: what the judge asked for, the latest first, and what
        // was wrong in the last rejected attempt; empty when nothing was rejected
        String ClarificationsOf(const Vector<Judged>& attempts);

        // The attempt to keep: accepted ones first, then the judge's likeness, then the later one
        int BestAttempt(const Vector<Judged>& attempts);

        // The file name stem of a part inside the node's check folder
        String PartFile(const String& portId);

        // The check folder of a node's work folder
        String CheckFolder(const String& workDir);

        // 32 hex digits of sha256 over the parts, each followed by "|"
        String ShaKey(const Vector<String>& parts);

        // ---------------------
        // What a check ended in
        // ---------------------
        struct Outcome
        {
            String data;         // The kept attempt's stored bytes
            bool   final = true; // False when a model call failed: nothing is stored for the check, the next run tries again
        };

        // Checks one part (ctx carries its port, the work folder, the memo and the phase callback): source is the picture
        // the part was cut from, first its stored generation, key the check's cache key
        Coroutine<Outcome> Run(Ref<PipelineExecContext> ctx, Ref<PipelineNode> node, String source, String first, String key);

        // The stored result of a check was read back: the part's debug file becomes that check's, when its details are kept
        void Replay(const PipelineExecContext& ctx, const PipelineNode& node, const String& key);

        // Writes a debug file of a check: the details of the key (when finished) and the part's current file
        void WriteDebug(const String& folder, const String& stem, const String& tag, const Json& debug, bool running);

        // Keeps the details of the latest checks of a stem (three) and drops the files of stems no longer live (empty =
        // keep every stem)
        void Prune(const String& folder, const String& stem, const Vector<String>& liveStems);

        // The picture scaled by k (lanczos), as PNG
        String ScaledPng(const UiImage& img, double k);

        // What the judge is sent: small pictures enlarged to 384, large ones reduced to 1024 (on the longer side)
        double JudgeScale(int w, int h);
    }
}
