#include "o2Editor/stdafx.h"
#include "UiStyleCheck.h"

#include <cmath>

#include "o2/Utils/FileSystem/FileSystem.h"
#include "o2Editor/Pipeline/PipelineExtractCheck.h"
#include "o2Editor/Pipeline/PipelineUtils.h"
#include "o2Editor/Pipeline/UiDoc/UiDocAiShared.h"
#include "o2Editor/Pipeline/UiDoc/UiDocRender.h"
#include "o2Editor/Pipeline/UiDoc/UiStyleAi.h"

namespace Editor::UiDoc
{
    const char* const STYLE_CHECK_FILE = "style";

    String StyleCheckKey(const PipelineNode& node)
    {
        PipelineExtractCheck::Config check;
        if (!PipelineExtractCheck::Read(node, check))
            return "";
        return "style-check:v2:" + check.model + ":" + (String)check.limit;
    }

    String StyleBackdropOf(const String& png)
    {
        const int S = 24;
        UiLayout::UiImage img;
        if (!UiLayout::UiImage::Decode(png, img) || img.IsEmpty())
            return STYLE_MATTE;

        // The colours as they are, the alpha dropped
        for (size_t i = 3; i < img.data.size(); i += 4)
            img.data[i] = 255;
        UiLayout::UiImage small = img.Resize(S, S);
        double r = 0, g = 0, b = 0;
        int n = 0;
        for (int y = 0; y < S; y++)
        {
            for (int x = 0; x < S; x++)
            {
                if (x > 0 && y > 0 && x < S - 1 && y < S - 1)
                    continue;
                const UInt8* p = small.At(x, y);
                r += p[0];
                g += p[1];
                b += p[2];
                n++;
            }
        }
        return UiLayout::HexOf(UiLayout::Rgb(JsRound(r/n), JsRound(g/n), JsRound(b/n)));
    }

    UiLayout::UiImage RenderStyleSample(const TextStyle& style, const String& text, const String& background)
    {
        UiDoc doc = SampleDoc(style, text, background);
        UiKit kit;
        kit.name = "sample";
        kit.textStyles = { style };
        UiRenderOptions o;
        o.mode = "kit";
        o.kit = &kit;
        o.scale = doc.width > 1024 ? 1024.0/doc.width : 1.0;
        return RenderLayout(doc, o);
    }

    // A style as the judge is shown it: what it may change, nothing it may not
    static Json StyleForJudge(const TextStyle& style)
    {
        Json all = TextStyleToJson(style), out = Json::Object();
        for (const char* key : { "font", "weight", "size", "color", "fill", "effects", "letterSpacing", "uppercase" })
        {
            if (auto v = all.Get(key))
                out[key] = *v;
        }
        return out;
    }

    // A line of a list: the text without the dots, semicolons and white space it ends with
    static String TrimEndPunct(const String& text)
    {
        int end = text.Length();
        while (end > 0 && (text[end - 1] == '.' || text[end - 1] == ';' || text[end - 1] == ' ' || text[end - 1] == '\n' ||
                           text[end - 1] == '\t' || text[end - 1] == '\r'))
            end--;
        return text.SubStr(0, end);
    }

    String StyleJudgePrompt(const TextStyle& style, const String& text, const String& hint, bool matte,
                            const Vector<StyleAttemptBrief>& earlier /*= {}*/)
    {
        const char* dash = " \xE2\x80\x94 ";
        String cap = JsNum(JsRound(style.size*TEXT_METRICS.cap));
        Vector<String> fonts;
        for (auto& f : UiFonts())
        {
            Vector<String> weights;
            for (auto& face : f.faces)
                weights.Add((String)face.weight);
            fonts.Add("- " + f.family + dash + "weights " + Join(weights, "/") + dash + f.look + (f.cyrillic ? ", has Cyrillic" : ""));
        }

        Vector<String> lines = {
            "You are checking a text style that was read off lettering in a picture, so that labels like it can be drawn by an SVG renderer: one font out of a fixed list, a flat or gradient fill and a stack of layer effects.",
            "",
            "Two images follow:",
            String("1. ORIGINAL \xE2\x80\x94 the picture with the lettering.") +
                (hint.IsEmpty() ? String(" The lettering that was read is its most prominent one.") : " The lettering that was read: \"" + hint + "\".") +
                (matte ? " Its transparent areas are shown as flat " + String(STYLE_MATTE) + "." : String()),
            "2. RENDER \xE2\x80\x94 the words \"" + text + "\" drawn by the renderer in the style that was read, on a flat background.",
            "",
            "Compare the LOOK of the letters in the two images \xE2\x80\x94 against the height of a capital letter, since the two are at different sizes: the letter shapes and their weight (is another font of the list closer?), the fill (its colours, the direction of a gradient, bands and highlights), every outline (how many there are, their colours, how thick each is against the letter height), the solid depth under the letters, the shadows (direction, distance, softness), a glow, light and dark edges inside the letters, the letter spacing, the case.",
            "These are NOT differences: other words or another length of text; the size of the image; the background; hand-painted irregularities, textures, sparkles, perspective or an arched baseline, which the renderer cannot draw.",
            "The font is the closest one of a short list, so the letter shapes never match exactly: a difference of letter shapes alone is at most \"minor\". Name another font only when one of the list is clearly closer to the ORIGINAL than the current one \xE2\x80\x94 and never a font an earlier attempt already used.",
            "",
            "The style that drew the RENDER \xE2\x80\x94 sizes are px at the style's size of " + JsNum(style.size) + " px, where a capital letter is about " + cap + " px tall:",
            StyleForJudge(style).Dump(),
        };

        if (!earlier.IsEmpty())
        {
            lines.Add("");
            lines.Add("Earlier attempts at this style, already compared with the ORIGINAL (the RENDER above is the next one; do not undo what was corrected after them, and do not go back to their fonts):");
            for (auto& a : earlier)
            {
                Vector<String> differences;
                for (auto& d : a.differences)
                    differences.Add(TrimEndPunct(d));
                lines.Add("- attempt " + (String)a.n + ": " + a.style.font + " " + JsNum(a.style.weight) + dash +
                          (a.verdict.IsEmpty() ? String("not judged") : a.verdict) + (a.likeness ? ", likeness " + JsNum(*a.likeness) : String()) +
                          (differences.IsEmpty() ? String() : ": " + Join(differences, "; ")));
            }
        }

        lines.Add("");
        lines.Add("Fonts (family \xE2\x80\x94 weights \xE2\x80\x94 look):");
        lines.Add(Join(fonts, "\n"));
        lines.Add("");
        lines.Add("How the look of a style is written:");
        for (auto& line : StyleLookSpec("px at the style's size (" + JsNum(style.size) + " px)"))
            lines.Add(line);
        lines.Add("");
        lines.Add("Answer with ONLY a JSON object, no commentary, no code fence:");
        lines.Add("{\"verdict\": \"ok\" | \"minor\" | \"significant\", \"likeness\": <number 0..1>, \"differences\": [\"<one concrete difference>\"], \"style\": {<the fields to change so that the RENDER looks like the ORIGINAL>}}");
        lines.Add("- \"ok\": the same lettering, nothing that matters differs. \"minor\": differences a player would not notice at a glance \xE2\x80\x94 a shade a little lighter or warmer, an outline, a depth or a bevel a little thicker, thinner or softer, slightly different letter shapes. \"significant\": a look that reads differently at a glance \xE2\x80\x94 a missing or an extra outline, depth, shadow or glow; a colour of another hue; a thickness off by more than about a third; a gradient that is missing, reversed or of other colours; a font of another character (a script for block capitals, thin for heavy); wrong case.");
        lines.Add("- \"likeness\": 1 = the same lettering, 0 = nothing in common.");
        lines.Add("- \"differences\": at most 6, each short and specific \xE2\x80\x94 what differs and how (thicker, thinner, lighter, missing\xE2\x80\xA6). Empty when there are none.");
        lines.Add("- \"style\": any of \"font\", \"weight\", \"color\", \"fill\", \"effects\", \"letterSpacing\", \"uppercase\" \xE2\x80\x94 only what must change; \"effects\" is always the WHOLE corrected list (repeat the effects that stay). Never \"size\". Omit \"style\" when the verdict is \"ok\".");
        return Join(lines, "\n");
    }

    static String Pct(double opacity)
    {
        return JsNum(JsRound(opacity*100)) + "%";
    }

    static String FxLine(const TextEffect& e)
    {
        String off = e.off ? " (off)" : "";
        if (e.type == "stroke")
            return "stroke " + JsNum(e.width) + "px " + e.color + (e.opacity < 1 ? " " + Pct(e.opacity) : String()) + off;
        if (e.type == "extrude")
            return "extrude " + JsNum(e.x) + "," + JsNum(e.y) + " " + e.color + off;
        if (e.type == "shadow")
            return "shadow " + JsNum(e.x) + "," + JsNum(e.y) + " blur " + JsNum(e.blur) + (e.spread ? " spread " + JsNum(e.spread) : String()) +
                " " + e.color + " " + Pct(e.opacity) + off;
        if (e.type == "glow")
            return "glow " + JsNum(e.size) + "px " + e.color + " " + Pct(e.opacity) + off;
        if (e.type == "innerShadow")
            return "inner " + JsNum(e.x) + "," + JsNum(e.y) + " blur " + JsNum(e.blur) + " " + e.color + " " + Pct(e.opacity) + off;
        return "bevel " + JsNum(e.depth) + "px " + e.highlight + "/" + e.shade + " " + Pct(e.opacity) + off;
    }

    static String FillLine(const TextStyle& s)
    {
        if (!s.fill)
            return s.color;

        Vector<String> stops;
        for (auto& stop : s.fill->stops)
            stops.Add(stop.color + "@" + JsNum(stop.at));
        return "gradient " + Join(stops, " ") + " " + JsNum(s.fill->angle) + "\xC2\xB0";
    }

    Vector<String> StyleChanges(const TextStyle& a, const TextStyle& b)
    {
        Vector<String> out;
        if (a.font != b.font || a.weight != b.weight)
            out.Add("font: " + a.font + " " + JsNum(a.weight) + " \xE2\x86\x92 " + b.font + " " + JsNum(b.weight));
        if (FillLine(a) != FillLine(b))
            out.Add("fill: " + FillLine(a) + " \xE2\x86\x92 " + FillLine(b));

        Vector<String> fa, fb;
        for (auto& e : a.effects)
            fa.Add(FxLine(e));
        for (auto& e : b.effects)
            fb.Add(FxLine(e));
        if (Join(fa, "|") != Join(fb, "|"))
        {
            for (auto& line : fa)
            {
                if (!fb.Contains(line))
                    out.Add("\xE2\x88\x92 " + line);
            }
            for (auto& line : fb)
            {
                if (!fa.Contains(line))
                    out.Add("+ " + line);
            }
        }
        if (a.letterSpacing != b.letterSpacing)
            out.Add("letterSpacing: " + JsNum(a.letterSpacing) + " \xE2\x86\x92 " + JsNum(b.letterSpacing));
        if (a.uppercase != b.uppercase)
            out.Add(String("uppercase: ") + (a.uppercase ? "true" : "false") + " \xE2\x86\x92 " + (b.uppercase ? "true" : "false"));
        return out;
    }

    TextStyle ApplyJudgeFix(const TextStyle& style, const Json* fix)
    {
        if (!fix || !fix->IsObject())
            return style;

        Json raw = Json::Object();
        for (const char* key : { "font", "weight", "color", "fill", "gradient", "effects", "stroke", "shadow", "letterSpacing", "uppercase" })
        {
            if (auto v = fix->Get(key))
                raw[key] = *v;
        }
        return raw.Count() ? MergeTextStyle(style, raw) : style;
    }

    void ReplayStyleCheck(const PipelineExecContext& ctx, const String& key)
    {
        String folder = PipelineExtractCheck::CheckFolder(ctx.workDir);
        if (folder.IsEmpty())
            return;

        String current = folder + STYLE_CHECK_FILE + ".json";
        Json now;
        if (o2FileSystem.IsFileExist(current) && Json::Parse(PipelineUtils::ReadFileBytes(current), now) && now.Get("key") &&
            now.Get("key")->Str() == key)
            return;

        o2FileSystem.FolderCreate(folder, true);
        String kept = folder + STYLE_CHECK_FILE + "-" + key.SubStr(0, 8) + ".json";
        if (o2FileSystem.IsFileExist(kept))
        {
            PipelineUtils::WriteFileBytes(current, PipelineUtils::ReadFileBytes(kept));
            return;
        }

        Json missing = Json::Object();
        missing["version"] = 1;
        missing["key"] = key;
        missing["missing"] = true;
        PipelineUtils::WriteFileBytes(current, missing.Dump());
    }

    namespace
    {
        // ------------------------------------------------------------------
        // One attempt of a check as its debug file keeps it (StyleCheckAttempt)
        // ------------------------------------------------------------------
        struct StyleAttempt
        {
            StyleAttemptBrief brief;
            String            render;
            Vector<String>    changes;
            String            judge;
            double            judgeMs = 0;
            bool              replayed = false;
            String            error;

            Json ToJson() const
            {
                Json a = Json::Object();
                a["n"] = brief.n;
                a["render"] = render;
                Json style = TextStyleToJson(brief.style);
                style.Remove("extracted");
                a["style"] = style;
                if (!brief.verdict.IsEmpty())
                    a["verdict"] = brief.verdict;
                if (brief.likeness)
                    a["likeness"] = *brief.likeness;
                a["differences"] = JsonStrings(brief.differences);
                a["changes"] = JsonStrings(changes);
                a["judge"] = judge;
                a["judgeMs"] = JsRound(judgeMs);
                if (replayed)
                    a["replayed"] = true;
                if (!error.IsEmpty())
                    a["error"] = error;
                return a;
            }
        };
    }

    Coroutine<StyleCheckOutcome> RunStyleCheck(Ref<PipelineExecContext> ctx, Ref<PipelineNode> node, String source, String first,
                                               String key)
    {
        StyleCheckOutcome res;
        res.data = first;
        PipelineExtractCheck::Config check;
        Json read;
        if (!PipelineExtractCheck::Read(*node, check) || !Json::Parse(first, read) || !read.IsObject() || !JsTruthy(read.Get("font")))
            co_return res;

        double started = (double)NowMs();
        String label = "textStyleExtract: check";
        String hint = JsTrim(node->GetConfigString("hint", ""));
        const Json* extracted = read.Get("extracted");
        String text = JsTrim(extracted && extracted->IsObject() ? JsStringOr(extracted->Get("text")) : String());
        if (text.IsEmpty())
            text = "Sample 123";

        String folder = PipelineExtractCheck::CheckFolder(ctx->workDir);
        String tag = String(STYLE_CHECK_FILE) + "-" + key.SubStr(0, 8);
        if (!folder.IsEmpty())
            o2FileSystem.FolderCreate(folder, true);
        auto put = [&](const String& name, const String& data)
        {
            if (!folder.IsEmpty())
                PipelineUtils::WriteFileBytes(folder + name, data);
            return name;
        };

        Vector<StyleAttempt> attempts;
        Vector<PipelineExtractCheck::Judged> judged;
        Vector<TextStyle> styles = { NormalizeTextStyle(read, JsStringOr(read.Get("name"), "body")) };
        int kept = 1, judgeCalls = 0;
        String reference;

        auto debugJson = [&](const String& stop, const String& note)
        {
            Json d = Json::Object();
            d["version"] = 1;
            d["key"] = key;
            d["at"] = IsoTime(started);
            d["ms"] = JsRound((double)NowMs() - started);
            d["limit"] = check.limit;
            d["judge"] = check.model;
            d["reference"] = reference;
            d["text"] = text;
            Json list = Json::Array();
            for (auto& a : attempts)
                list.Add(a.ToJson());
            d["attempts"] = list;
            d["kept"] = kept;
            d["stop"] = stop;
            d["note"] = note;
            Json calls = Json::Object();
            calls["judge"] = judgeCalls;
            d["calls"] = calls;
            return d;
        };

        // The kept attempt's look in the style as it was read: its name, role and what the model called it stay
        auto result = [&]()
        {
            const TextStyle& best = kept >= 1 && kept <= styles.Count() ? styles[kept - 1] : styles[0];
            Json out = read, look = TextStyleToJson(best);
            for (auto& member : look.Members())
                out[member.first] = member.second;
            if (auto v = read.Get("name")) out["name"] = *v; else out.Remove("name");
            if (auto v = read.Get("role")) out["role"] = *v; else out.Remove("role");
            if (JsTruthy(read.Get("description")))
                out["description"] = *read.Get("description");
            if (extracted)
                out["extracted"] = *extracted;
            else
                out.Remove("extracted");
            return out.Dump();
        };

        auto finish = [&](const String& stop, const String& note)
        {
            PipelineExtractCheck::WriteDebug(folder, STYLE_CHECK_FILE, tag, debugJson(stop, note), false);
            PipelineExtractCheck::Prune(folder, STYLE_CHECK_FILE, {});
            char secs[32];
            snprintf(secs, sizeof(secs), "%.1f", ((double)NowMs() - started)/1000.0);
            ctx->Log(label + " " + stop + (note.IsEmpty() ? String() : " \xE2\x80\x94 " + note) + " (" + (String)judgeCalls +
                     " judge call(s), " + secs + " s)");
            StyleCheckOutcome out;
            out.data = attempts.IsEmpty() ? first : result();
            out.final = stop != "failed";
            return out;
        };

        std::optional<PipelineImageOps::CropRect> frame;
        PipelineImageOps::CropRect crop;
        if (PipelineImageOps::ParseCrop(node->GetConfigValue("srcCrop"), crop))
            frame = crop;
        UiStylePicture shown;
        if (!StyleSourcePicture(source, frame ? &*frame : nullptr, shown))
            co_return finish("failed", "the picture could not be read");

        String referencePng = shown.png;
        reference = put(tag + "-ref.png", referencePng);
        String backdrop = shown.matte ? String(STYLE_MATTE) : StyleBackdropOf(referencePng);

        for (int n = 1; ; n++)
        {
            ctx->Phase("check");
            StyleAttempt a;
            a.brief.n = n;
            a.brief.style = styles[n - 1];
            a.judge = check.model;
            Json fix;
            double t0 = (double)NowMs();

            String renderPng = RenderStyleSample(a.brief.style, text, backdrop).Png();
            a.render = put(tag + "-a" + (String)n + ".png", renderPng);
            Vector<StyleAttemptBrief> earlier;
            for (auto& e : attempts)
                earlier.Add(e.brief);
            String prompt = StyleJudgePrompt(a.brief.style, text, hint, shown.matte, earlier);
            String memoKey = PipelineExtractCheck::ShaKey({ "style-check-judge:v1", check.model, prompt, referencePng, renderPng });
            String answer;
            // A fresh run (↻) asks again rather than replaying what was answered before
            if (!ctx->fresh && ctx->MemoGet(memoKey, ".txt", answer))
                a.replayed = true;
            else
            {
                AiTextResult reply = co_await UiAiText(ctx, check.model, prompt, { AiImageRef{ "image/png", referencePng },
                                                                                  AiImageRef{ "image/png", renderPng } });
                if (!reply.ok)
                    a.error = JsSlice(reply.error.IsEmpty() ? String("The judge did not answer") : reply.error.Split("\n")[0], 0, 300);
                else
                {
                    answer = reply.text;
                    judgeCalls++;
                }
            }

            if (a.error.IsEmpty())
            {
                PipelineExtractCheck::JudgeAnswer verdict;
                String why;
                if (PipelineExtractCheck::ParseJudge(answer, verdict, &why))
                {
                    // Only an answer that reads as a verdict is worth remembering
                    if (!a.replayed)
                        ctx->MemoPut(memoKey, ".txt", answer);
                    a.brief.verdict = verdict.verdict;
                    a.brief.likeness = verdict.likeness;
                    a.brief.differences = verdict.differences;
                    Json object;
                    if (AnswerObject(answer, object) && object.Get("style"))
                        fix = *object.Get("style");
                }
                else
                    a.error = why;
            }
            a.judgeMs = (double)NowMs() - t0;

            PipelineExtractCheck::Judged j;
            j.n = n;
            j.verdict = a.brief.verdict;
            j.likeness = a.brief.likeness;
            j.differences = a.brief.differences;
            judged.Add(j);
            kept = PipelineExtractCheck::BestAttempt(judged);
            ctx->Log(label + " #" + (String)n + ": " + (a.brief.verdict.IsEmpty() ? "failed (" + a.error + ")" : a.brief.verdict) +
                     (a.brief.likeness ? ", likeness " + JsNum(*a.brief.likeness) : String()) +
                     (a.brief.differences.IsEmpty() ? String() : " \xE2\x80\x94 " + Join(a.brief.differences, "; ")));

            if (!a.error.IsEmpty())
            {
                attempts.Add(a);
                co_return finish("failed", "attempt " + (String)n + " could not be judged: " + a.error);
            }
            if (a.brief.verdict != "significant")
            {
                attempts.Add(a);
                co_return finish(a.brief.verdict, kept == n ? String() : "attempt " + (String)kept + " was closer to the picture");
            }
            if (n > check.limit)
            {
                attempts.Add(a);
                co_return finish("limit", "still different after " + (String)check.limit + " correction(s); attempt " + (String)kept +
                                 " is the closest");
            }

            TextStyle next = ApplyJudgeFix(a.brief.style, fix.IsNull() ? nullptr : &fix);
            a.changes = StyleChanges(a.brief.style, next);
            attempts.Add(a);
            if (a.changes.IsEmpty())
                co_return finish("limit", "the judge named differences but no change to the style; attempt " + (String)kept + " is kept");

            styles.Add(next);
            PipelineExtractCheck::WriteDebug(folder, STYLE_CHECK_FILE, tag, debugJson("ok", ""), true);
        }
    }
}
