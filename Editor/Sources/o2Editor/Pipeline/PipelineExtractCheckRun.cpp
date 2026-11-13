#include "o2Editor/stdafx.h"
#include "PipelineExtractCheck.h"

#include <cmath>

#include "o2/Utils/FileSystem/FileSystem.h"
#include "o2Editor/Pipeline/Nodes/ImageExtractNode.h"
#include "o2Editor/Pipeline/Nodes/PipelineNodesCommon.h"
#include "o2Editor/Pipeline/PipelineUtils.h"
#include "o2Editor/Pipeline/UiDoc/UiDocAiShared.h"
#include "o2Editor/Pipeline/UiDoc/UiDocJs.h"

namespace Editor::PipelineExtractCheck
{
    using UiLayout::JsRound;

    namespace
    {
        // ------------------------------------------------------------------
        // One attempt of a check as its debug file keeps it (CheckAttempt)
        // ------------------------------------------------------------------
        struct Attempt
        {
            Judged    judged;
            String    sprite, overlay;  // Files beside the debug file
            int       spriteW = 0, spriteH = 0;
            Box       placed;           // Where the sprite was laid, source pixels
            String    from;
            Placement placement;
            double    pixelDiff = 0, pixelChanged = 0;
            String    clarifications;
            String    model, judge;
            double    drawMs = 0, judgeMs = 0;
            bool      replayedDraw = false, replayedJudge = false;
            String    error;

            Json ToJson() const
            {
                auto r1 = [](double v) { return JsRound(v*10)/10; };
                Json a = Json::Object();
                a["n"] = judged.n;
                a["sprite"] = sprite;
                a["overlay"] = overlay;
                Json size = Json::Object();
                size["w"] = spriteW;
                size["h"] = spriteH;
                a["spriteSize"] = size;
                Json place = Json::Object();
                place["x"] = r1(placed.x);
                place["y"] = r1(placed.y);
                place["w"] = r1(placed.w);
                place["h"] = r1(placed.h);
                place["from"] = from;
                if (placement.fit)
                {
                    Json fit = Json::Object();
                    fit["scale"] = placement.scale;
                    fit["dx"] = placement.dx;
                    fit["dy"] = placement.dy;
                    place["fit"] = fit;
                }
                a["placement"] = place;
                a["pixelDiff"] = pixelDiff;
                a["pixelChanged"] = pixelChanged;
                if (!judged.verdict.IsEmpty())
                    a["verdict"] = judged.verdict;
                if (judged.likeness)
                    a["likeness"] = *judged.likeness;
                a["differences"] = UiDoc::JsonStrings(judged.differences);
                a["instructions"] = judged.instructions;
                a["clarifications"] = clarifications;
                a["model"] = model;
                a["judge"] = judge;
                a["drawMs"] = JsRound(drawMs);
                a["judgeMs"] = JsRound(judgeMs);
                if (replayedDraw || replayedJudge)
                {
                    Json replayed = Json::Object();
                    if (replayedDraw)
                        replayed["draw"] = true;
                    if (replayedJudge)
                        replayed["judge"] = true;
                    a["replayed"] = replayed;
                }
                if (!error.IsEmpty())
                    a["error"] = error;
                return a;
            }
        };

        String FirstLine(const String& text, int max = 300)
        {
            int nl = text.Find("\n");
            return UiDoc::JsSlice(nl >= 0 ? text.SubStr(0, nl) : text, 0, max);
        }

        String Percent1(double v)
        {
            char buf[32];
            snprintf(buf, sizeof(buf), "%.1f", v*100);
            return buf;
        }
    }

    Coroutine<Outcome> Run(Ref<PipelineExecContext> ctx, Ref<PipelineNode> node, String source, String first, String key)
    {
        Outcome res;
        res.data = first;
        Config check;
        if (!Read(*node, check))
            co_return res;

        double started = (double)UiDoc::NowMs();
        auto region = PipelineRegions::RegionOfPort(*node, ctx->outputPortId);
        String part = UiDoc::JsTrim(region.name);
        String label = "imageExtract" + (ctx->outputPort.IsEmpty() ? String() : " . " + ctx->outputPort) + ": check";
        String imageModel = PipelineRegions::PartModel(*node, region.id);
        auto tr = PipelineTransparency::Read(*node, region.id);

        String folder = CheckFolder(ctx->workDir);
        String stem = PartFile(region.id);
        String tag = stem + "-" + key.SubStr(0, 8);
        if (!folder.IsEmpty())
            o2FileSystem.FolderCreate(folder, true);
        auto put = [&](const String& name, const String& data)
        {
            if (!folder.IsEmpty())
                PipelineUtils::WriteFileBytes(folder + name, data);
            return name;
        };

        Vector<String> liveStems;
        for (auto& r : PipelineRegions::Read(*node))
            liveStems.Add(PartFile(r.id));

        Vector<Attempt> attempts;
        Vector<Judged> judged;
        Vector<String> stored = { first };
        int kept = 1, judgeCalls = 0, drawCalls = 0;
        String reference, skipped;
        Box referenceSize, regionInReference;

        auto debugJson = [&](const String& stop, const String& note)
        {
            Json d = Json::Object();
            d["version"] = 1;
            d["key"] = key;
            Json p = Json::Object();
            p["id"] = region.id;
            p["name"] = part;
            d["part"] = p;
            d["at"] = UiDoc::IsoTime(started);
            d["ms"] = JsRound((double)UiDoc::NowMs() - started);
            d["limit"] = check.limit;
            d["judge"] = check.model;
            d["reference"] = reference;
            Json size = Json::Object();
            size["w"] = referenceSize.w;
            size["h"] = referenceSize.h;
            d["referenceSize"] = size;
            Json inRef = Json::Object();
            inRef["x"] = regionInReference.x;
            inRef["y"] = regionInReference.y;
            inRef["w"] = regionInReference.w;
            inRef["h"] = regionInReference.h;
            d["regionInReference"] = inRef;
            Json list = Json::Array();
            for (auto& a : attempts)
                list.Add(a.ToJson());
            d["attempts"] = list;
            d["kept"] = kept;
            d["stop"] = stop;
            if (!skipped.IsEmpty())
                d["skipped"] = skipped;
            d["note"] = note;
            Json calls = Json::Object();
            calls["judge"] = judgeCalls;
            calls["draw"] = drawCalls;
            d["calls"] = calls;
            return d;
        };
        auto keptData = [&]() { return kept >= 1 && kept <= stored.Count() ? stored[kept - 1] : first; };
        auto finish = [&](const String& stop, const String& note, const String& data)
        {
            WriteDebug(folder, stem, tag, debugJson(stop, note), false);
            Prune(folder, stem, liveStems);
            char secs[32];
            snprintf(secs, sizeof(secs), "%.1f", ((double)UiDoc::NowMs() - started)/1000.0);
            ctx->Log(label + " " + stop + (note.IsEmpty() ? String() : " \xE2\x80\x94 " + note) + " (" + (String)judgeCalls +
                     " judge call(s), " + (String)drawCalls + " regeneration(s), " + secs + " s)");
            Outcome out;
            out.data = data;
            out.final = stop != "failed";
            return out;
        };

        // Only a cut-out can be laid over what it was cut from
        if (!tr.transparent)
        {
            bool plate = PipelineRegions::IsBackgroundPart(part, false);
            skipped = plate ? "plate" : "opaque";
            co_return finish("skipped", plate ? "a background plate is not compared: everything on it is removed on purpose" :
                             "an opaque part has no cut-out to lay over the reference", first);
        }

        UiImage src;
        auto sourceBitmap = DecodeImageBytes(source);
        if (!UiImage::Decode(source, src) || src.IsEmpty() || !sourceBitmap)
            co_return finish("failed", "the source picture could not be read", first);

        Box rr = RegionRect(src.w, src.h, region);
        Box cr = ContextRect(src.w, src.h, rr);
        auto roi = PipelineExtract::RoiOf(*sourceBitmap, *node, region.id);
        String roiPng = EncodeBitmapPng(*roi);
        UiImage crop = src.Crop((int)rr.x, (int)rr.y, (int)rr.w, (int)rr.h);
        UiImage referenceImg = src.Crop((int)cr.x, (int)cr.y, (int)cr.w, (int)cr.h);
        double k = JudgeScale(referenceImg.w, referenceImg.h);
        String referencePng = ScaledPng(referenceImg, k);
        reference = put(tag + "-ref.png", referencePng);
        referenceSize = Box(0, 0, JsRound(referenceImg.w*k), JsRound(referenceImg.h*k));
        regionInReference = Box(JsRound((rr.x - cr.x)*k), JsRound((rr.y - cr.y)*k), JsRound(rr.w*k), JsRound(rr.h*k));
        String prompt = JudgePrompt(part);

        String clarifications;
        double drawMs = 0;
        bool replayedDraw = false;
        for (int n = 1; ; n++)
        {
            // Lay the attempt over the reference and ask the judge
            ctx->Phase("check");
            Attempt a;
            a.judged.n = n;
            SpriteView view = SpriteViewOf(stored[n - 1], *node, region.id, (int)rr.w, (int)rr.h);
            a.placement = PlaceOnCrop(view.sprite, view.nominal, crop);
            Box inReference(a.placement.box.x + rr.x - cr.x, a.placement.box.y + rr.y - cr.y, a.placement.box.w, a.placement.box.h);
            Overlay overlay = OverlayOn(referenceImg, view.sprite, inReference);
            String overlayPng = ScaledPng(overlay.image, k);
            // The popup shows the sprite small: a copy of at most 512 px is kept, not the model's full render
            double spriteK = std::min(1.0, 512.0/std::max(1, std::max(view.sprite.w, view.sprite.h)));
            a.sprite = put(tag + "-a" + (String)n + "-sprite.png", ScaledPng(view.sprite, spriteK));
            a.overlay = put(tag + "-a" + (String)n + "-overlay.png", overlayPng);
            a.spriteW = view.sprite.w;
            a.spriteH = view.sprite.h;
            a.placed = Box(a.placement.box.x + rr.x, a.placement.box.y + rr.y, a.placement.box.w, a.placement.box.h);
            a.from = view.from;
            a.pixelDiff = overlay.pixelDiff;
            a.pixelChanged = overlay.pixelChanged;
            a.clarifications = clarifications;
            a.model = imageModel;
            a.judge = check.model;
            a.drawMs = drawMs;
            a.replayedDraw = replayedDraw;

            bool empty = true;
            for (size_t i = 3; i < view.sprite.data.size() && empty; i += 4)
                empty = view.sprite.data[i] == 0;
            if (empty)
                a.error = "the sprite is empty";
            else
            {
                String alone = SpriteOnChecker(view.sprite);
                String memoKey = ShaKey({ "extract-check-judge:v1", check.model, prompt, referencePng, overlayPng, alone });
                double t0 = (double)UiDoc::NowMs();
                String text;
                // A fresh run (↻) asks again rather than replaying what was answered before
                if (!ctx->fresh && ctx->MemoGet(memoKey, ".txt", text))
                    a.replayedJudge = true;
                else
                {
                    AiTextResult answer = co_await UiDoc::UiAiText(ctx, check.model, prompt, {
                        AiImageRef{ "image/png", referencePng }, AiImageRef{ "image/png", overlayPng }, AiImageRef{ "image/png", alone } });
                    if (!answer.ok)
                        a.error = FirstLine(answer.error.IsEmpty() ? String("The judge did not answer") : answer.error);
                    else
                    {
                        text = answer.text;
                        judgeCalls++;
                    }
                }

                if (a.error.IsEmpty())
                {
                    JudgeAnswer verdict;
                    String why;
                    if (ParseJudge(text, verdict, &why))
                    {
                        // Only an answer that reads as a verdict is worth remembering
                        if (!a.replayedJudge)
                            ctx->MemoPut(memoKey, ".txt", text);
                        a.judged.verdict = verdict.verdict;
                        a.judged.likeness = verdict.likeness;
                        a.judged.differences = verdict.differences;
                        a.judged.instructions = verdict.instructions;
                    }
                    else
                        a.error = why;
                }
                a.judgeMs = (double)UiDoc::NowMs() - t0;
                ctx->Log(label + " #" + (String)n + ": " + (a.judged.verdict.IsEmpty() ? "failed (" + a.error + ")" : a.judged.verdict) +
                         (a.judged.likeness ? ", likeness " + UiDoc::JsNum(*a.judged.likeness) : String()) + ", pixels " +
                         Percent1(a.pixelDiff) + "%" + (a.judged.differences.IsEmpty() ? String() :
                                                        " \xE2\x80\x94 " + UiDoc::Join(a.judged.differences, "; ")));
            }

            attempts.Add(a);
            judged.Add(a.judged);
            kept = BestAttempt(judged);
            if (!a.error.IsEmpty())
            {
                if (n == 1 && a.error == "the sprite is empty")
                {
                    skipped = "empty";
                    co_return finish("skipped", "the sprite is empty: nothing to compare", first);
                }
                co_return finish("failed", "attempt " + (String)n + " could not be judged: " + a.error, keptData());
            }

            if (a.judged.verdict != "significant")
                co_return finish(a.judged.verdict, kept == n ? String() : "attempt " + (String)kept + " was closer to the reference", keptData());

            if (n > check.limit)
                co_return finish("limit", "still different after " + (String)check.limit + " regeneration(s); attempt " + (String)kept +
                                 " is the closest", keptData());

            WriteDebug(folder, stem, tag, debugJson("ok", ""), true);

            // Regenerate with what the judge asked for
            clarifications = ClarificationsOf(judged);
            String transparency = String(tr.transparent ? "true" : "false") + "," + tr.mode + "," + tr.colorHex + "," +
                (String)tr.chroma.tolerance + "," + (String)tr.chroma.softness + "," + (String)tr.chroma.spill;
            String memoKey = ShaKey({ "extract-check-draw:v1", imageModel, transparency, ctx->seed < 0 ? String() : (String)ctx->seed,
                                      part, clarifications, roiPng });
            double t0 = (double)UiDoc::NowMs();
            String next;
            // A fresh run (↻) asks again rather than replaying what was answered before
            replayedDraw = !ctx->fresh && ctx->MemoGet(memoKey, ".png", next);
            if (!replayedDraw)
            {
                ctx->Phase("redo");
                PipelineRunResult drawn = co_await PipelineExtract::GeneratePart(ctx, node, region.id, roi, part, clarifications);
                if (ctx->IsCancelled())
                    co_return finish("failed", "the run was cancelled", keptData());
                if (!drawn.ok || drawn.outputs.empty() || !drawn.outputs.begin()->second.IsValid())
                    co_return finish("failed", "regeneration " + (String)n + " failed: " +
                                     FirstLine(drawn.ok ? String("the model drew nothing") : drawn.error), keptData());

                next = drawn.outputs.begin()->second.data;
                drawCalls++;
                ctx->MemoPut(memoKey, ".png", next);
            }
            stored.Add(next);
            drawMs = (double)UiDoc::NowMs() - t0;
        }
    }
}
