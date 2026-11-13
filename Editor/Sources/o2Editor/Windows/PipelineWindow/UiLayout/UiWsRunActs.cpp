#include "o2Editor/stdafx.h"
#include "UiWsRunActs.h"

#include <cmath>
#include <map>

#include "o2/Utils/System/Time/Time.h"
#include "o2Editor/Pipeline/PipelineUtils.h"
#include "o2Editor/Pipeline/UiLayout/UiLayoutPixels.h"

namespace Editor
{
    namespace UiWsRunActs
    {
        using namespace UiWsDraw;
        using UiDoc::Json;

        static String Num(const Json* v)
        {
            return v ? v->ToJsString() : String("0");
        }

        static String Pct(const Json* v)
        {
            return (String)(int)UiDoc::JsRound((v ? v->Num() : 0)*100);
        }

        static String Str(const Json& o, const char* key)
        {
            const Json* v = o.Get(key);
            return v && !v->IsNull() ? v->ToJsString() : String();
        }

        String Took(double ms)
        {
            if (!(ms >= 500))
                return String();
            int s = (int)UiDoc::JsRound(ms/1000);
            if (s < 90)
                return (String)s + " s";
            int m = (int)UiDoc::JsRound(s/60.0);
            return m < 90 ? (String)m + " min" : (String)(m/60) + " h " + (String)(m%60) + " min";
        }

        // What the skill's agent did on a step, from the step's own title ("Turn 6: measure 60 270 650 770")
        static String SkillWhat(const Json& a)
        {
            const Json* data = a.Get("data");
            String label = data && data->Get("label") ? data->Get("label")->ToJsString() : Str(a, "title");
            if (label.StartsWith("Turn "))
            {
                int colon = label.Find(":");
                if (colon > 0)
                    return UiDoc::JsTrimStart(label.SubStr(colon + 1));
            }
            return label;
        }

        static String SkillActionWords(const String& action)
        {
            static const Map<String, String> words = {
                { "prepare", "Preparation: grid, sprite sheets" }, { "draft", "Draft checked: diff with the reference" },
                { "view", "Looks at" }, { "measure", "Measures on the grid" }, { "write", "Writes the layout" },
                { "patch", "Fixes the layout" }, { "edit", "Edits a review" }, { "check", "Check: diff with the reference" },
                { "target", "Compares elements" }, { "preview", "Preview of a state" }, { "finalize", "Final validation" },
                { "backfill_anchors", "Fills in anchors" }, { "stop", "Stops" }, { "end", "The layout pass ended" },
            };
            String out;
            words.TryGetValue(action, out);
            return out;
        }

        String ActTitle(const Json& a)
        {
            const Json* data = a.Get("data");
            String id = Str(a, "id"), kind = Str(a, "kind");
            if (data && data->Get("skill"))
            {
                String step = data->Get("step") ? data->Get("step")->ToJsString() : String();
                String action = data->Get("action") ? data->Get("action")->ToJsString() :
                    step == "check" ? String("draft") : step == "prepare" ? String("prepare") : step == "agent-done" ? String("end") : String();
                String named = SkillActionWords(action);
                if (named.IsEmpty())
                    named = SkillWhat(a);
                if (step.StartsWith("turn-"))
                    return "Turn " + step.SubStr(5) + " \xC2\xB7 " + named;
                return named;
            }
            if (kind == "turn")
                return "Turn " + (id.StartsWith("turn-") ? id.SubStr(5) : id);
            if (kind == "analysis") return "Reading the reference";
            if (kind == "styles") return "Text styles";
            if (kind == "assets") return "Graphics the kit lacks";
            if (kind == "draft") return "First layout";
            if (kind == "check") return "Result against the reference";
            return Str(a, "title");
        }

        String ActDetail(const Json& a)
        {
            if (Str(a, "status") == "run")
                return String();
            static const Json empty = Json::Object();
            const Json* dp = a.Get("data");
            const Json& d = dp && dp->IsObject() ? *dp : empty;
            String kind = Str(a, "kind");
            Vector<String> parts;
            if (d.Get("skill"))
            {
                String what = SkillWhat(a);
                for (auto verb : { "view", "measure", "check", "target", "write", "patch", "edit", "finalize", "preview" })
                {
                    String v = verb;
                    if (what.StartsWith(v) && (what.Length() == v.Length() || what[v.Length()] == ' '))
                    {
                        what = UiDoc::JsTrimStart(what.SubStr(v.Length()));
                        break;
                    }
                }
                if (d.Get("score"))
                    parts.Add("likeness " + Pct(d.Get("score")) + "%");
                if (!what.IsEmpty())
                    parts.Add(what);
                return UiDoc::Join(parts, " \xC2\xB7 ");
            }
            if (kind == "analysis" && d.Get("elements"))
                return Num(d.Get("elements")) + " elements \xC2\xB7 " + Num(d.Get("kit")) + " drawn by the kit \xC2\xB7 " + Num(d.Get("looks")) + " to cut out";
            if (kind == "styles" && d.Get("labels"))
                return Num(d.Get("made")) + " new for " + Num(d.Get("labels")) + " labels";
            if (kind == "assets" && d.Get("made"))
                return Num(d.Get("made")) + " cut from the reference";
            if (kind == "draft" && d.Get("blocks"))
                return Num(d.Get("blocks")) + " blocks";
            if (kind == "check" && d.Get("score"))
                return "likeness " + Pct(d.Get("score")) + "%";
            if (kind == "turn")
            {
                if (d.Get("markup"))
                    parts.Add("rebuilding from the reference");
                if (d.Get("score"))
                    parts.Add("likeness " + Pct(d.Get("score")) + "%");
                if (d.Get("edits") && !d.Get("markup"))
                    parts.Add("edits: " + Num(d.Get("edits")));
                if (d.Get("made") && d.Get("made")->Num())
                    parts.Add("drawn: " + Num(d.Get("made")));
                if (d.Get("styles") && d.Get("styles")->Num())
                    parts.Add("styles: " + Num(d.Get("styles")));
                if (d.Get("done"))
                    parts.Add("done");
                return UiDoc::Join(parts, " \xC2\xB7 ");
            }
            return String();
        }

        String RunNow(const Json& acts, double startedMs, double nowMs)
        {
            const Json* cur = nullptr;
            for (int i = acts.Count() - 1; i >= 0 && !cur; i--)
            {
                if (Str(*acts.At(i), "status") == "run")
                    cur = acts.At(i);
            }
            if (!cur && acts.Count())
                cur = acts.At(acts.Count() - 1);
            Vector<String> parts;
            parts.Add(cur ? ActTitle(*cur) + "\xE2\x80\xA6" : String("Starting\xE2\x80\xA6"));
            String since = startedMs > 0 ? Took(nowMs - startedMs) : String();
            if (!since.IsEmpty())
                parts.Add(since);
            return UiDoc::Join(parts, " \xC2\xB7 ");
        }

        String RunSummary(const Json& r)
        {
            Vector<String> parts;
            if (r.Get("turns") && r.Get("turns")->Num())
                parts.Add("turns: " + Num(r.Get("turns")));
            if (const Json* skill = r.Get("skill"))
                parts.Add("checks against the design: " + Num(skill->Get("checks")) + " \xC2\xB7 elements compared: " + Num(skill->Get("targets")));
            if (const Json* score = r.Get("score"))
            {
                const Json* from = r.Get("scoreFrom");
                if (from && std::abs(from->Num() - score->Num()) >= 0.005)
                    parts.Add("likeness " + Pct(from) + "% \xE2\x86\x92 " + Pct(score) + "%");
                else
                    parts.Add("likeness " + Pct(score) + "%");
            }
            if (r.Get("built") && r.Get("reused"))
                parts.Add("blocks drawn by the kit: " + Num(r.Get("reused")));
            if (r.Get("made") && r.Get("made")->Num())
                parts.Add("graphics made: " + Num(r.Get("made")));
            String stopped = Str(r, "stopped");
            if (stopped == "budget")
                parts.Add("the turns ran out");
            else if (stopped == "cancelled")
                parts.Add("stopped");
            String took = Took(r.Get("ms") ? r.Get("ms")->Num() : 0);
            if (!took.IsEmpty())
                parts.Add(took);
            return UiDoc::Join(parts, " \xC2\xB7 ");
        }

        String PictureTitle(const String& title)
        {
            static const Map<String, String> words = {
                { "elements found", "the elements found on the reference" }, { "kit sheet", "the kit sheet the agent sees" },
                { "cut from the reference", "cut out of the reference" }, { "the screen before this turn", "the screen before this turn" },
                { "where it differs: reference | now", "where it differs: the reference | the screen" }, { "the screen", "the screen" },
                { "what still differs: reference | now", "what still differs: the reference | the screen" },
                { "pixel difference", "pixel difference with the reference (yellow: a little, red: much)" },
                { "comparison.png", "reference | reconstruction on the measuring grid" }, { "risk_review.png", "risky elements: reference against reconstruction" },
                { "reconstruction.png", "the reconstruction" }, { "all_elements.png", "every element outlined" },
                { "design_grid.png", "the reference under the measuring grid" },
            };
            String out;
            return words.TryGetValue(title, out) ? out : title;
        }

        // A run's pictures, decoded once
        struct CachedPicture
        {
            TextureRef texture;
            Vec2I      size;
            bool       failed = false;
        };

        bool RunPicture(const String& path, TextureRef& texture, Vec2I& size)
        {
            static std::map<std::string, CachedPicture> cache;
            std::string key = (const char*)path.Data();
            auto it = cache.find(key);
            if (it == cache.end())
            {
                CachedPicture entry;
                UiLayout::UiImage image;
                if (UiLayout::UiImage::Decode(PipelineUtils::ReadFileBytes(path), image) && image.w > 0)
                {
                    auto bitmap = image.ToBitmap();
                    entry.texture = TextureRef(*bitmap);
                    entry.size = Vec2I(image.w, image.h);
                }
                else
                    entry.failed = true;
                if (cache.size() > 96)
                    cache.clear();
                it = cache.emplace(key, entry).first;
            }
            if (it->second.failed)
                return false;
            texture = it->second.texture;
            size = it->second.size;
            return true;
        }

        float Lay(const Frame& f, float y, const Json& acts, const String& dir, const String& key, const Vector<String>& open, bool live)
        {
            auto& t = T();
            float y0 = y;
            float x = f.x, w = f.w;
            auto visible = [&](float top, float h) { return f.draw && top + h > f.top && top < f.bottom; };
            const float rowH = 22;
            if (!acts.Count())
                return 0.0f;

            float listTop = y;
            y += 4;
            for (auto& a : acts.Items())
            {
                String id = Str(a, "id"), status = Str(a, "status");
                String hot = "act:" + key + ":" + id;
                const Json* images = a.Get("images");
                const Json* data = a.Get("data");
                String note = Str(a, "note");
                String names = data && data->Get("names") ? data->Get("names")->ToJsString() : String();
                bool more = !note.IsEmpty() || (images && images->Count()) || !names.IsEmpty();
                bool unfolded = more && open.Contains(hot);

                if (visible(y, rowH))
                {
                    RectF row = f.box.R(x + 4, y, w - 8, rowH);
                    if (more && f.isHover && f.isHover(hot))
                        Fill(row, 5.0f, t.active);
                    Color4 dot = status == "run" ? A(t.accent, 0.55f + 0.45f*std::sin(o2Time.GetApplicationTime()*5.2f)) :
                        status == "warn" ? t.yellow : status == "error" ? t.red : t.green;
                    Circle(f.box.P(x + 13, y + rowH*0.5f), 3.5f, dot);
                    String title = ActTitle(a);
                    String detail = ActDetail(a);
                    String took = status == "run" ? String() : Took(a.Get("ms") ? a.Get("ms")->Num() : 0);
                    float tookW = took.IsEmpty() ? 0.0f : TextWidth(took, 10.0f, 400, true) + 6;
                    float chevW = more ? 14.0f : 0.0f;
                    float titleW = Math::Min(TextWidth(title, 11.5f, 500) + 2, w - 24 - tookW - chevW - 8);
                    float tx = x + 22;
                    Text(title, f.box.R(tx, y, titleW, rowH), TextStyle(11.5f, 500, t.fg));
                    float detailW = w - 8 - (tx - x) - titleW - 8 - tookW - chevW;
                    if (!detail.IsEmpty() && detailW > 20)
                        Text(detail, f.box.R(tx + titleW + 8, y, detailW, rowH), TextStyle(10.5f, 400, t.fg3));
                    if (!took.IsEmpty())
                        Text(took, f.box.R(x + w - 8 - chevW - tookW, y, tookW, rowH), TextStyle(10.0f, 400, t.fg3, HorAlign::Right, true));
                    if (more)
                        Icon(unfolded ? "uiws_chevron_down" : "uiws_chevron_right", f.box.P(x + w - 8 - 6, y + rowH*0.5f), 9.0f, t.fg3);
                }
                if (more && f.draw && f.addHot && visible(y, rowH))
                    f.addHot(f.box.R(x + 4, y, w - 8, rowH), hot);
                y += rowH;

                if (!unfolded)
                    continue;

                float bx = x + 22, bw = w - 22 - 10;
                if (!note.IsEmpty())
                {
                    TextStyle style(11.0f, 400, t.fgDim);
                    style.wrap = true;
                    style.ver = VerAlign::Top;
                    style.lineHeight = 15.5f;
                    float h = TextHeight(note, bw, style);
                    if (visible(y, h + 4))
                        Text(note, f.box.R(bx, y + 2, bw, h + 2), style);
                    y += h + 6;
                }
                if (!names.IsEmpty())
                {
                    TextStyle style(10.5f, 400, t.fg3);
                    style.wrap = true;
                    style.ver = VerAlign::Top;
                    float h = TextHeight(names, bw, style);
                    if (visible(y, h + 2))
                        Text(names, f.box.R(bx, y, bw, h + 2), style);
                    y += h + 4;
                }
                for (int i = 0; images && i < images->Count(); i++)
                {
                    const Json& im = *images->At(i);
                    TextureRef texture;
                    Vec2I size;
                    // A run's pictures live in the node's work folder for a while; an old step's may be gone
                    if (!RunPicture(dir + "/" + Str(im, "file"), texture, size))
                        continue;
                    float k = Math::Min(bw/Math::Max(1, size.x), 260.0f/Math::Max(1, size.y));
                    float pw = size.x*k, ph = size.y*k;
                    if (visible(y + 2, ph))
                        Picture(texture, size, f.box.R(bx, y + 2, pw, ph));
                    y += ph + 4;
                    String caption = PictureTitle(Str(im, "title"));
                    if (visible(y, 15))
                        Text(caption, f.box.R(bx, y, bw, 15), TextStyle(10.0f, 400, t.fg3));
                    y += 17;
                }
                y += 4;
            }
            y += 4;
            if (f.draw && visible(listTop, y - listTop))
                Border(f.box.R(x, listTop, w, y - listTop), 7.0f, live ? A(t.accent, 0.35f) : t.line);
            return y - y0;
        }
    }
}
