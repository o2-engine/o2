#include "o2Editor/stdafx.h"
#include "UiDocAi.h"

#include <cmath>
#include <mutex>

#include "o2Editor/Pipeline/UiDoc/UiDocAiShared.h"
#include "o2Editor/Pipeline/UiDoc/UiDocRender.h"
#include "o2Editor/Pipeline/UiDoc/UiDocFp.h"

namespace Editor::UiDoc
{
    namespace
    {
        std::mutex   stubLock;
        UiAiTextStub textStub;
    }

    void SetUiAiTextStub(const UiAiTextStub& stub)
    {
        std::lock_guard<std::mutex> guard(stubLock);
        textStub = stub;
    }

    bool HasUiAiTextStub()
    {
        std::lock_guard<std::mutex> guard(stubLock);
        return (bool)textStub;
    }

    Coroutine<AiTextResult> UiAiText(Ref<PipelineExecContext> ctx, String model, String prompt, Vector<AiImageRef> images)
    {
        UiAiTextStub stub;
        {
            std::lock_guard<std::mutex> guard(stubLock);
            stub = textStub;
        }
        if (stub)
            co_return stub(model, prompt, images);
        co_return co_await AiRouter::GenerateText(ctx, model, prompt, images);
    }

    String PictureForModel(const String& png, int max /*= 1024*/)
    {
        UiImage img;
        if (!UiImage::Decode(png, img) || (img.w <= max && img.h <= max))
            return png;
        // fit: 'inside' at max x max
        double k = std::min((double)max/img.w, (double)max/img.h);
        int w = std::max(1, (int)JsRound(img.w*k)), h = std::max(1, (int)JsRound(img.h*k));
        return img.Resize(w, h).Png();
    }

    std::optional<double> PngDpiTag(const String& png)
    {
        // Chunks after the 8-byte signature: length, type, data, CRC; the tag is a tEXt "AssetsLine:dpi\0<number>"
        const std::string& s = png;
        if (s.size() < 8 || s.compare(1, 3, "PNG") != 0)
            return std::nullopt;
        size_t at = 8;
        while (at + 12 <= s.size())
        {
            size_t len = ((size_t)(unsigned char)s[at] << 24) | ((size_t)(unsigned char)s[at + 1] << 16) |
                ((size_t)(unsigned char)s[at + 2] << 8) | (size_t)(unsigned char)s[at + 3];
            std::string type = s.substr(at + 4, 4);
            if (at + 12 + len > s.size())
                break;
            if (type == "tEXt")
            {
                std::string data = s.substr(at + 8, len);
                size_t nul = data.find('\0');
                if (nul != std::string::npos && data.substr(0, nul) == "AssetsLine:dpi")
                    return CleanDpi(Json(String(data.substr(nul + 1))).ToJsNumber());
            }
            if (type == "IEND")
                break;
            at += 12 + len;
        }
        return std::nullopt;
    }

    // --- the layout agent ------------------------------------------------------------------------------------------

    // The long side of the layout screenshot the agent gets
    static const int SHOT_PX = 1024;

    // The layout as the agent sees it: drawn straight at 1024 px on the long side (with the kit when it has anything)
    static String RenderForAgent(const UiDoc& d, const UiKit* kit, bool skinned)
    {
        double k = std::min(1.0, SHOT_PX/std::max(d.width, d.height));
        UiRenderOptions o;
        o.mode = skinned ? "kit" : "lines";
        o.kit = kit;
        o.pixelLong = std::max(1, (int)JsRound(std::max(d.width, d.height)*k));
        return PictureForModel(RenderLayout(d, o).Png(), SHOT_PX);
    }

    Coroutine<UiAgentResponse> AskUiAgent(Ref<PipelineExecContext> ctx, UiAgentRequest request)
    {
        UiAgentResponse res;
        String prompt = JsTrim(request.prompt);
        if (prompt.IsEmpty())
        {
            res.error = "empty request";
            co_return res;
        }

        const UiKit* kit = request.kit ? &*request.kit : nullptr;
        Vector<UiAgentReference> references;
        Vector<AiImageRef> refImages;
        // The first picture is usually the screen to rebuild: an empty layout starts at its size and density
        std::optional<UiScreen> refScreen;
        for (auto& in : request.references)
        {
            UiAgentReference ref;
            ref.name = in.name.IsEmpty() ? String(in.image ? "reference" : "notes") : in.name;
            if (in.image)
            {
                if (in.png.IsEmpty())
                    continue;
                ref.kind = "image";
                references.Add(ref);
                refImages.Add(AiImageRef{ "image/png", PictureForModel(in.png) });
                if (!refScreen)
                {
                    UiImage img;
                    if (UiImage::Decode(in.png, img))
                    {
                        auto dpi = in.dpi ? CleanDpi(*in.dpi) : PngDpiTag(in.png);
                        refScreen = UiScreen{ (double)img.w, (double)img.h, dpi ? *dpi : UI_BASE_DPI };
                    }
                }
            }
            else
            {
                ref.kind = "text";
                ref.text = in.text;
                references.Add(ref);
            }
        }

        UiDoc current = DocAt(request.config).doc;
        bool empty = current.root.children.empty();
        Vector<UiOp> start;
        if (empty && refScreen && (refScreen->w != current.width || refScreen->h != current.height || refScreen->dpi != DocDpi(current)))
            start.Add(MakeScreenOp("", refScreen->w, refScreen->h, refScreen->dpi));
        UiDoc doc = start.IsEmpty() ? current : ApplyOps(current, start).doc;
        bool skinned = kit && (!kit->elements.IsEmpty() || !kit->textStyles.IsEmpty());

        String shot = empty ? String() : RenderForAgent(doc, kit, skinned);
        // The kit as a picture: the agent matches graphics by what they look like, not only by their names
        String sheet = kit && !kit->elements.IsEmpty() ? PictureForModel(RenderKitSheet(*kit, 40, DocDpi(doc)).Png(), 1400) : String();

        String model = JsTrim(request.model);
        if (model.IsEmpty())
            model = JsTrim(JsStringOr(JsGet(request.config, "agentModel")));
        if (model.IsEmpty())
            model = UI_AGENT_DEFAULT_MODEL;

        UiAgentContext actx;
        actx.doc = doc;
        actx.kit = kit;
        actx.steps = StepsOf(request.config);
        actx.prompt = prompt;
        actx.references = references;
        actx.selected = request.selected;
        actx.withScreenshot = !shot.IsEmpty();
        actx.withKitSheet = !sheet.IsEmpty();
        String text = BuildAgentPrompt(actx);

        Vector<AiImageRef> images;
        if (!shot.IsEmpty())
            images.Add(AiImageRef{ "image/png", shot });
        if (!sheet.IsEmpty())
            images.Add(AiImageRef{ "image/png", sheet });
        images.Add(refImages);
        ctx->Log("uiLayout agent: model=" + model + " promptChars=" + (String)text.Length() + " images=" + (String)images.Count());

        AiTextResult answer = co_await UiAiText(ctx, model, text, images);
        res.calls++;
        if (!answer.ok)
        {
            res.error = answer.error;
            co_return res;
        }

        UiAgentAnswer parsed = ParseAgentAnswer(answer.text, doc);
        // A whole new document keeps the screen's density unless it names one
        for (auto& op : parsed.ops)
        {
            if (OpKind(op) == "doc" && op.Has("doc") && !JsTruthy(JsGet(op.Get("doc"), "dpi")) && doc.dpi)
                (*op.Get("doc"))["dpi"] = *doc.dpi;
        }
        int answered = parsed.ops.Count();
        Vector<UiOp> ops = start;
        ops.Add(parsed.ops);
        res.step = MakeAgentStep(prompt, parsed.reply, ops, request.nowMs ? request.nowMs : NowMs(), model);
        res.errors = parsed.errors;

        // The check: the screen it made, rendered with the kit. When graphics of the kit went unused while blocks stay
        // placeholders (or icons are drawn with characters), the agent gets one more turn to fix the kit usage
        const Json* review = JsGet(request.config, "agentReview");
        bool reviewOn = !(review && review->IsBool() && !review->Bool());
        if (kit && !sheet.IsEmpty() && answered && reviewOn)
        {
            UiDoc made = ApplyOps(current, ops).doc;
            UiKitReport report = KitReport(made, kit);
            if (!report.unused.IsEmpty() && (!report.missing.IsEmpty() || !report.glyphs.IsEmpty()))
            {
                String madeShot = RenderForAgent(made, kit, skinned);
                AiTextResult answer2 = co_await UiAiText(ctx, model, BuildReviewPrompt(made, *kit, prompt, report),
                                                         { AiImageRef{ "image/png", madeShot }, AiImageRef{ "image/png", sheet } });
                res.calls++;
                if (!answer2.ok)
                    ctx->Log("uiLayout agent: the check pass failed: " + answer2.error);
                else if (!answer2.text.IsEmpty())
                {
                    UiAgentAnswer fix = ParseAgentAnswer(answer2.text, made);
                    Vector<UiOp> all = ops;
                    all.Add(fix.ops);
                    Json list = Json::Array();
                    for (auto& op : all)
                        list.Add(op);
                    res.step.data["ops"] = list;
                    res.step.SetReview(fix.reply, fix.ops.Count());
                    res.errors.Add(fix.errors);
                }
            }
        }

        res.ok = true;
        co_return res;
    }

    void StoreAgentStep(DataValue& config, const UiStep& step)
    {
        Vector<UiStep> steps = StepsOf(config);
        steps.Add(step);
        SetConfigJson(config, "steps", StepsToJson(steps));
        SetConfigJson(config, "uiRedo", Json::Array());
        SetConfigJson(config, "uiDraft", Json(String()));
        SetConfigJson(config, "uiPoints", Json::Array());
    }

    void CommitUserAction(DataValue& config, const Vector<UiOp>& ops, const String& label, Int64 nowMs, bool fold /*= true*/)
    {
        if (ops.IsEmpty())
            return;
        SetConfigJson(config, "steps", StepsToJson(PushUserAction(StepsOf(config), ops, label, nowMs, fold)));
        SetConfigJson(config, "uiRedo", Json::Array());
    }

    bool UndoInConfig(DataValue& config)
    {
        UiUndoResult r;
        if (!UndoAction(StepsOf(config), RedoOf(config), r))
            return false;
        WriteSteps(config, r.steps, r.redo);
        return true;
    }

    bool RedoInConfig(DataValue& config)
    {
        UiUndoResult r;
        if (!RedoAction(StepsOf(config), RedoOf(config), r))
            return false;
        WriteSteps(config, r.steps, r.redo);
        return true;
    }

    // --- the Text style node ------------------------------------------------------------------------------------------

    Coroutine<UiStyleResponse> GenerateUiTextStyle(Ref<PipelineExecContext> ctx, UiStyleRequest request)
    {
        UiStyleResponse res;
        String prompt = JsTrim(request.prompt);
        if (prompt.IsEmpty())
        {
            res.error = "empty request";
            co_return res;
        }

        Vector<AiImageRef> images;
        for (auto& png : request.references)
        {
            if (!png.IsEmpty())
                images.Add(AiImageRef{ "image/png", PictureForModel(png, 1024) });
        }

        TextStyle current = TextStyleFromConfig(request.config);
        String model = JsTrim(request.model);
        if (model.IsEmpty())
            model = JsTrim(JsStringOr(JsGet(request.config, "styleModel")));
        if (model.IsEmpty())
            model = TEXT_STYLE_AI_DEFAULT_MODEL;
        res.model = model;

        AiTextResult answer = co_await UiAiText(ctx, model, BuildStylePrompt(current, prompt, images.Count()), images);
        if (!answer.ok)
        {
            res.error = answer.error;
            co_return res;
        }
        if (!ParseStyleAnswer(answer.text, current, res.style))
        {
            res.error = "The model did not answer with a style";
            co_return res;
        }
        res.ok = true;
        co_return res;
    }

    void StoreTextStyle(DataValue& config, const TextStyle& style)
    {
        Json fields = TextStyleToJson(style);
        fields.Remove("name");
        if (!fields.Has("description"))
            fields["description"] = Json();
        for (auto& m : fields.Members())
            SetConfigJson(config, m.first.Data(), m.second);
    }
}
