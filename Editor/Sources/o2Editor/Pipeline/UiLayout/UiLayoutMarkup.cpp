#include "o2Editor/stdafx.h"
#include "UiLayoutMarkup.h"

#include "o2Editor/Pipeline/PipelineUtils.h"
#include "o2Editor/Pipeline/UiDoc/UiDocFonts.h"
#include "o2Editor/Pipeline/UiDoc/UiDocRaster.h"
#include "o2Editor/Pipeline/UiDoc/UiDocRender.h"
#include "o2Editor/Pipeline/UiLayout/UiLayoutCompare.h"
#include "o2Editor/Pipeline/UiLayout/UiLayoutGridFit.h"
#include "o2Editor/Pipeline/UiLayout/UiLayoutMeasure.h"
#include "o2Editor/Pipeline/UiLayout/UiLayoutOwnAsset.h"
#include "o2Editor/Pipeline/UiLayout/UiLayoutParse.h"
#include "o2Editor/Pipeline/UiLayout/UiLayoutPrompts.h"
#include "o2Editor/Pipeline/UiLayout/UiLayoutRunSteps.h"

namespace Editor::UiLayout
{
    namespace D = ::Editor::UiDoc;

    static String Joined(const Vector<String>& lines, const String& sep = "\n")
    {
        String out;
        for (int i = 0; i < lines.Count(); i++)
            out += (i ? sep : String()) + lines[i];
        return out;
    }

    static String Lower(const String& s)
    {
        String out = s;
        for (int i = 0; i < out.Length(); i++)
        {
            if (out[i] >= 'A' && out[i] <= 'Z')
                out[i] = (char)(out[i] - 'A' + 'a');
        }
        return out;
    }

    // Runs of _ and - as one space (the web's replace(/[_-]+/g, ' '))
    static String DashesToSpaces(const String& s)
    {
        String out;
        bool run = false;
        for (int i = 0; i < s.Length(); i++)
        {
            bool dash = s[i] == '_' || s[i] == '-';
            if (dash && !run)
                out += ' ';
            else if (!dash)
                out += s[i];
            run = dash;
        }
        return out;
    }

    const String& MarkupFlatRule()
    {
        static const String rule = Joined({
            "",
            "- A flat shape of ONE plain colour with no artwork \xE2\x80\x94 a translucent layer dimming the screen under a popup, a flat strip, a divider line \xE2\x80\x94 takes no look: give it \"color\": \"#RRGGBB\" and \"opacity\": 0.0-1.0 instead (a panel element).",
            "- An element tilted ON PURPOSE \xE2\x80\x94 a scattered letter tile, a sticker, a slanted badge or ribbon \xE2\x80\x94 keeps its box as the upright box around it as seen, and adds \"rotation\": its tilt in degrees, clockwise positive (-12 for a tile leaning left). Upright elements take no rotation.",
            "- The list is the drawing order, back to front: among elements of one parent, what lies UNDER comes before what covers it \xE2\x80\x94 the screen behind a popup, then the layer that dims it, then the popup; a frame before the header, the close button and the character that overlap its edge. This rule wins over \"top to bottom\" wherever things overlap.",
        });
        return rule;
    }

    String KitMarkupRules(const D::UiKit& kit, double layoutDpi, int sheetImage)
    {
        D::UiKit bare = kit;
        bare.textStyles.Clear();
        return Joined({
            "",
            "## The project's UI kit",
            "The screen will be rebuilt with the project's UI kit \xE2\x80\x94 image #" + (String)sheetImage + " is its sheet: every graphic with its name under it. The kit decides how things LOOK; the mockup decides WHAT is on the screen and WHERE. Mark the screen up in terms of the kit:",
            "- An element that a kit graphic can draw carries \"kit\": \"<graphic name>\" INSTEAD of \"look\". Choose by role and shape \xE2\x80\x94 the graphic made for that job (\"use:\" below), the closest-looking one of that role \xE2\x80\x94 even when its colours or details differ from the mockup: a counter capsule is drawn by the kit's bar or track, a window frame by the kit's panel, an avatar frame by the kit's slot or frame, a gear by the kit's settings icon. Sliced (9-slice) graphics stretch to any box; simple ones keep their proportions.",
            "- One kit graphic is ONE element: never split what a graphic shows whole (a pile of coins the kit has as one picture is ONE image element with the box around the whole pile), and add no elements for parts a graphic already contains (the rays inside a panel that has them, the rim of a button, the paper inside a framed panel).",
            "- A caption the kit has as a picture (a drawn \"VICTORY!\" header, a logo) is an image element with that graphic, not a text element.",
            "- \"look\" is only for artwork NO kit graphic can stand for; it will be cut out of the mockup, which is slow. Expect most elements to use the kit, and give a \"look\" only to the few pieces unique to this screen (special art, a banner, a character or an object the kit lacks). Before giving a \"look\", go through the sheet once more for a graphic of that role.",
            "- A kit name must be exactly one of the names below. The background may name a kit graphic too: \"background\": {..., \"kit\": \"<name>\"} when the kit has the backdrop of this screen.",
            D::KitSummary(bare, layoutDpi),
        });
    }

    String MarkupStylesPrompt(const Vector<MarkupLabel>& labels, const D::UiKit* kit, int w, int h)
    {
        Vector<String> lines = {
            "The image is a mockup of a game UI screen, " + (String)w + " x " + (String)h + " pixels. Its labels will be drawn by the engine with text styles. Describe the styles its lettering needs and say which style each label is drawn in.",
            "Labels drawn with the same lettering \xE2\x80\x94 font, weight, fill, outline, shadow \xE2\x80\x94 share ONE style whatever their size; a different colour, outline or font is another style. Usually a screen has 2\xE2\x80\x93" "6 styles.",
            "",
            "## Labels found (number. text \xE2\x80\x94 x, y, width, height in mockup pixels)",
        };
        for (int i = 0; i < labels.Count() && i < 80; i++)
        {
            String text = labels[i].text;
            String flat;
            bool space = false;
            for (int c = 0; c < text.Length(); c++)
            {
                char ch = text[c];
                bool ws = ch == ' ' || ch == '\n' || ch == '\r' || ch == '\t';
                if (ws) { if (!space) flat += ' '; space = true; }
                else { flat += ch; space = false; }
            }
            const Box& b = labels[i].box;
            lines.Add((String)(i + 1) + ". \"" + D::JsSlice(flat, 0, 40) + "\" \xE2\x80\x94 " + Json::FormatNumber(JsRound(b.x)) + "," +
                      Json::FormatNumber(JsRound(b.y)) + " " + Json::FormatNumber(JsRound(b.w)) + "x" + Json::FormatNumber(JsRound(b.h)));
        }
        lines.Add("");
        if (kit && !kit->textStyles.IsEmpty())
        {
            lines.Add("## Styles the UI kit already has \xE2\x80\x94 use them for the labels they fit (same kind of lettering, even when the colour is a little off); do not describe them again");
            for (auto& s : kit->textStyles)
            {
                String look = D::StyleLook(s);
                lines.Add("- " + s.name + ": " + s.role + ", " + s.font + " " + Json::FormatNumber(s.weight) + ", " + Json::FormatNumber(s.size) +
                          "px, " + s.color + (look.IsEmpty() ? String() : ", " + look) +
                          (s.description && !s.description->IsEmpty() ? " \xE2\x80\x94 " + *s.description : String()));
            }
            lines.Add("");
        }
        lines.Add("## Fonts to choose from (the closest one)");
        for (auto& f : D::UiFonts())
        {
            String weights;
            for (int i = 0; i < f.faces.Count(); i++)
                weights += (i ? ", " : "") + (String)f.faces[i].weight;
            lines.Add("- " + f.family + ": " + f.look + "; weights " + weights);
        }
        lines.Add("");
        lines.Add("For each NEW style, an object with: \"name\" (short snake_case, by what it is for: title, button_label, counter, caption\xE2\x80\xA6), \"role\" (title | heading | body | button | caption | number | label), \"font\", \"weight\", \"size\" (the usual size of labels in this style, in mockup pixels: about 1.4 \xC3\x97 the height of a capital letter), \"uppercase\" (true when the labels are all capitals), \"description\" (one short phrase: where it is used), and its look, read off the letters from the inside out \xE2\x80\x94 every layer the lettering has and none it has not:");
        lines.Add(D::StyleLookSpec("mockup pixels at that size"));
        lines.Add("Answer with JSON only: {\"styles\": [ {...}, ... ], \"labels\": {\"<label number>\": \"<style name: a new one or a kit style>\", ...}} \xE2\x80\x94 every label above.");
        return Joined(lines);
    }

    MarkupStyles ParseMarkupStyles(const String& answer, const D::UiKit* kit, int labels)
    {
        MarkupStyles res;
        res.byLabel.Resize(labels);
        Json raw;
        if (!JsonOf(answer, raw))
            return res;

        Map<String, String> kitNames;
        Map<String, bool> taken;
        if (kit)
        {
            for (auto& s : kit->textStyles)
            {
                kitNames[Lower(s.name)] = s.name;
                taken[Lower(s.name)] = true;
            }
        }
        Map<String, String> renamed;
        const Json* list = raw.IsObject() ? raw.Get("styles") : nullptr;
        if (list && list->IsArray())
        {
            for (int i = 0; i < list->Count() && i < 12; i++)
            {
                D::TextStyle st = D::NormalizeTextStyle(*list->At(i), "style_" + (String)(res.styles.Count() + 1));
                String name = st.name;
                for (int n = 2; taken.ContainsKey(Lower(name)); n++)
                    name = st.name + "_" + (String)n;
                taken[Lower(name)] = true;
                renamed[Lower(st.name)] = name;
                st.name = name;
                res.styles.Add(st);
            }
        }

        const Json* map = raw.IsObject() ? raw.Get("labels") : nullptr;
        if (map && map->IsObject())
        {
            for (auto& m : map->Members())
            {
                int i = (int)JsRound(Json(m.first).ToJsNumber()) - 1;
                String want = Lower(JsTrim(m.second.IsNull() ? String() : m.second.ToJsString()));
                if (!(i >= 0 && i < labels) || want.IsEmpty())
                    continue;
                // A new style of the answer first: a kit style of the same name was renamed around
                String hit;
                if (renamed.TryGetValue(want, hit) || kitNames.TryGetValue(want, hit))
                    res.byLabel[i] = hit;
                else
                    res.byLabel[i] = String();
            }
        }
        return res;
    }

    const D::UiKitElement* KitByName(const D::UiKit* kit, const String& name)
    {
        if (!kit || name.IsEmpty())
            return nullptr;
        if (auto hit = D::KitElement(kit, name))
            return hit;
        String n = SafeKey(name, "");
        if (n.IsEmpty())
            return nullptr;
        const D::UiKitElement* one = nullptr;
        int count = 0;
        for (auto& e : kit->elements)
        {
            String k = SafeKey(e.name, "");
            if (k == n || HasSuffix(k, "_" + n) || HasSuffix(n, "_" + k))
            {
                one = &e;
                count++;
            }
        }
        return count == 1 ? one : nullptr;
    }

    double InkFontSize(const String& text, const String& weight, double inkH)
    {
        // The web renders the text at 100 px in Roboto of the weight and measures its ink
        String file = D::FaceOf("Roboto", weight == "bold" ? 700 : weight == "medium" ? 500 : 400).file;
        D::UiShapedLine line = D::ShapeUiText(file, text.IsEmpty() ? String("A") : text, 100, 0);
        double probe = 0;
        if (!line.glyphs.empty())
        {
            D::UiMask mask;
            int w = (int)std::ceil(line.width) + 40, h = 200;
            mask.Reset(w, h);
            D::DrawUiText(line, 20, 140, 100, 0, mask);
            int top = -1, bottom = -1;
            for (int y = 0; y < h; y++)
            {
                for (int x = 0; x < w; x++)
                {
                    if (mask.a[y*w + x] > 0.02f)
                    {
                        if (top < 0)
                            top = y;
                        bottom = y;
                        break;
                    }
                }
            }
            probe = top >= 0 ? bottom - top + 1 : 0;
        }
        if (probe <= 0)
            return FontSizeForInk(inkH);
        return std::max(4.0, JsRound(((inkH*100)/std::max(1.0, probe))*4)/4);
    }

    void TrustText(UiEl& e)
    {
        if (!e.rough || e.kind != "text")
            return;
        const Box& r = *e.rough;
        if (e.measured == "ink" && e.ink)
        {
            const Box& ink = *e.ink;
            if (ink.h >= r.h*0.6 && ink.h <= r.h*1.2 && ink.w >= r.w*0.6 && ink.w <= r.w*1.3)
                return;
        }
        Vector<String> lines = D::JsSplit(e.text, "\n");
        int count = std::max(1, lines.Count());
        Box glyphs(r.x + r.w*0.02, r.y + r.h*0.06, r.w*0.96, r.h*0.88);
        String first = lines.IsEmpty() || lines[0].IsEmpty() ? String("A") : lines[0];
        e.fontSize = InkFontSize(first, e.weight.IsEmpty() ? String("regular") : e.weight, glyphs.h/count);
        e.ink = glyphs;
        if (count > 1)
            e.box = Box(JsRound(r.x - *e.fontSize*0.3), JsRound(r.y), JsRound(r.w + *e.fontSize*0.6), JsRound(r.h));
        else
            e.box = TextBoxFor(glyphs, *e.fontSize, e.align.IsEmpty() ? String("center") : e.align);
        e.measured = "analysis";
    }

    // The node type word of an own graphic cut for a look
    static String TypeOfCut(const UiEl* el, const String& stem)
    {
        if (el && el->kind == "button") return "button";
        if (el && el->kind == "panel") return "panel";
        if (el && el->kind == "input") return "input";
        if (el && el->kind == "progress") return "progressBar";
        String g = D::GuessElementType(stem);
        return g == "other" ? String("image") : g;
    }

    Coroutine<MarkupResult> MarkupReference(MarkupInput m)
    {
        MarkupResult res;
        auto ai = m.ai;
        auto& log = m.log;
        auto cancelled = [&]() { return ai->exec && ai->exec->IsCancelled(); };

        UiImage decoded;
        if (!UiImage::Decode(m.design, decoded))
        {
            res.error = "the reference picture cannot be read";
            co_return res;
        }
        UiImage design = decoded.Flatten(255, 255, 255);
        res.designFlatPng = design.Png();
        int W = design.w, H = design.h;
        auto trace = mmake<Trace>(m.workDir + "trace", W, H);
        trace->Reset();
        {
            Json step = Json::Object();
            step["id"] = "input";
            step["title"] = "Reference";
            step["kind"] = "input";
            Json images = Json::Array();
            images.Add(trace->Image("design", design, "design.png"));
            step["images"] = images;
            Json data = Json::Object();
            data["width"] = W;
            data["height"] = H;
            data["dpi"] = m.designDpi;
            step["data"] = data;
            trace->Step(step);
        }
        const D::UiKit* kit = m.kit && (!m.kit->elements.IsEmpty() || !m.kit->textStyles.IsEmpty()) ? m.kit : nullptr;

        // 1. The screen as elements, in terms of the kit
        int act = log->Begin("analysis", "analysis", "Reading the reference");
        AnalysisFrame frame = AnalysisFrameOf(ai->textModel, W, H);
        String sent = frame.scale < 1 ? design.Resize(frame.w, frame.h).Png() : res.designFlatPng;
        String sheet = kit && !kit->elements.IsEmpty() ? D::RenderKitSheet(*kit, 60, m.designDpi).Png() : String();
        String prompt = Prompts::Analyze(frame.w, frame.h, m.hint, frame.boxes) + MarkupFlatRule() +
            (kit && !kit->elements.IsEmpty() && !sheet.IsEmpty() ? KitMarkupRules(*kit, m.designDpi, 2) : String());
        Vector<String> images = { sent };
        if (!sheet.IsEmpty())
            images.Add(sheet);

        UiDoc an;
        bool read = false;
        String answer, bgKitName;
        // An answer that cannot be read (cut short, not JSON, no elements) is asked for once more, fresh
        for (int attempt = 0; attempt < 2 && !read; attempt++)
        {
            bool wasFresh = ai->fresh;
            if (attempt)
                ai->fresh = true;
            AiAnswer got = co_await AiText(ai, prompt, images, "markup");
            ai->fresh = wasFresh;
            if (cancelled())
            {
                res.error = "Cancelled";
                co_return res;
            }
            String why = got.ok ? String() : got.error;
            if (got.ok)
            {
                answer = got.data;
                UiDoc parsed;
                String error;
                if (ParseAnalysis(answer, W, H, parsed, &error, &frame) && !parsed.els.IsEmpty())
                {
                    an = parsed;
                    read = true;
                    Json raw;
                    if (JsonOf(answer, raw) && raw.IsObject())
                    {
                        const Json* bg = raw.Get("background");
                        const Json* k = bg ? bg->Get("kit") : nullptr;
                        if (k && !k->IsNull())
                            bgKitName = k->ToJsString();
                    }
                    break;
                }
                why = error.IsEmpty() ? String("no elements") : error;
            }
            if (attempt)
            {
                Json patch = Json::Object();
                patch["status"] = "error";
                patch["note"] = D::JsSlice(why, 0, 300);
                log->End(act, patch);
                res.error = answer.IsEmpty() ? why : String("The model's reading of the reference picture could not be understood");
                co_return res;
            }
        }
        const D::UiKitElement* bgKit = KitByName(kit, bgKitName);

        // Kit names as the kit spells them; one the kit does not have becomes a look to cut
        int unknownKit = 0;
        for (auto& e : an.els)
        {
            if (e->kit.IsEmpty())
                continue;
            if (auto el = KitByName(kit, e->kit))
            {
                e->kit = el->name;
                e->look = String();
                continue;
            }
            unknownKit++;
            if (e->look.IsEmpty() && e->kind != "text" && e->kind != "group" && e->kind != "grid")
            {
                e->look = SafeKey(e->kit, "look");
                if (!an.looks.Has(e->look))
                    an.looks.Set(e->look, DashesToSpaces(e->kit));
            }
            e->kit = String();
        }
        FitGrids(an, design);
        for (auto& e : an.els)
        {
            if (e->kind == "text" && !e->slot)
            {
                MeasureText(design, *e);
                TrustText(*e);
            }
        }
        for (auto& cell : an.els)
        {
            if (!cell->cell)
                continue;
            Vector<Ref<UiEl>> texts;
            for (auto& c : an.ChildrenOf(cell->id))
            {
                if (c->kind == "text")
                    texts.Add(c);
            }
            MeasureCellTexts(design, *cell, texts);
        }
        Vector<LookPlan> plans = PlanLooks(an);
        int withKit = 0, textCount = 0;
        for (auto& e : an.els)
        {
            withKit += e->kit.IsEmpty() ? 0 : 1;
            textCount += e->kind == "text" ? 1 : 0;
        }
        log->Picture(act, "elements found", OverlayKinds(design, an));
        if (!sheet.IsEmpty())
        {
            UiImage sheetImg;
            if (UiImage::Decode(sheet, sheetImg))
                log->Picture(act, "kit sheet", sheetImg);
        }
        {
            Json step = Json::Object();
            step["id"] = "analysis";
            step["title"] = "Analysis: " + (String)an.els.Count() + " elements";
            step["kind"] = "analysis";
            step["text"] = answer;
            step["images"] = Json::Array();
            trace->Step(step);
        }
        {
            Json patch = Json::Object();
            patch["status"] = unknownKit ? "warn" : "ok";
            Json data = Json::Object();
            data["elements"] = an.els.Count();
            data["kit"] = withKit;
            data["looks"] = plans.Count();
            data["texts"] = textCount;
            patch["data"] = data;
            if (unknownKit)
                patch["note"] = (String)unknownKit + " element(s) named a graphic the kit does not have";
            log->End(act, patch);
        }
        if (cancelled())
        {
            res.error = "Cancelled";
            co_return res;
        }

        // 2. The lettering as text styles (the kit's are reused), each label with its style
        Vector<Ref<UiEl>> labels;
        for (auto& e : an.els)
        {
            if (e->kind == "text" && !e->text.IsEmpty())
                labels.Add(e);
        }
        Vector<D::TextStyle> styles;
        Map<String, String> styleOf;
        if (!labels.IsEmpty())
        {
            act = log->Begin("styles", "styles", "Text styles");
            Vector<MarkupLabel> found;
            for (auto& e : labels)
                found.Add({ e->text, e->ink ? *e->ink : e->box });
            AiAnswer got = co_await AiText(ai, MarkupStylesPrompt(found, kit, W, H), { sent }, "text styles");
            if (got.ok)
            {
                MarkupStyles parsed = ParseMarkupStyles(got.data, kit, labels.Count());
                styles = parsed.styles;
                for (int i = 0; i < labels.Count(); i++)
                {
                    if (!parsed.byLabel[i].IsEmpty())
                        styleOf[labels[i]->id] = parsed.byLabel[i];
                }
                Json step = Json::Object();
                step["id"] = "styles";
                step["title"] = "Text styles: " + (String)styles.Count() + " new";
                step["kind"] = "analysis";
                step["text"] = got.data;
                step["images"] = Json::Array();
                trace->Step(step);
                Json patch = Json::Object();
                Json data = Json::Object();
                data["made"] = styles.Count();
                data["kit"] = kit ? kit->textStyles.Count() : 0;
                data["labels"] = labels.Count();
                String names;
                for (auto& s : styles)
                    names += (names.IsEmpty() ? "" : ", ") + s.name;
                data["names"] = names;
                patch["data"] = data;
                log->End(act, patch);
            }
            else
            {
                Json patch = Json::Object();
                patch["status"] = "warn";
                patch["note"] = D::JsSlice(got.error, 0, 300);
                log->End(act, patch);
            }
            if (cancelled())
            {
                res.error = "Cancelled";
                co_return res;
            }
        }

        // 3. What no kit graphic stands for is cut out of the picture - the node's own graphics
        Vector<D::UiKitElement> ownEls;
        Map<String, String> assetOf;
        String backgroundKit = bgKit ? bgKit->name : String();
        String backgroundColor = bgKit ? String() : an.background.color;
        if (m.canDraw && (!plans.IsEmpty() || (!bgKit && an.background.kind == "image")))
        {
            act = log->Begin("assets", "assets", "Graphics the kit lacks");
            auto st = mmake<RunState>();
            st->ai = ai;
            st->trace = trace;
            st->cfg.sprites = "auto";
            st->cfg.background = bgKit ? "none" : "auto";
            st->cfg.name = m.name;
            st->cfg.hint = m.hint;
            st->cfg.maxTurns = 0;
            st->design = design;
            st->designPng = res.designFlatPng;
            st->doc = an.Clone();
            OrderedMap<Ref<SpriteCut>> sprites;
            co_await CutAll(st, &st->doc, &plans, &sprites);
            if (cancelled())
            {
                res.error = "Cancelled";
                co_return res;
            }
            st->sprites = sprites;
            if (!bgKit)
                co_await MakeBackground(st);
            if (cancelled())
            {
                res.error = "Cancelled";
                co_return res;
            }

            Vector<String> taken;
            if (kit)
            {
                for (auto& e : kit->elements)
                    taken.Add(Lower(e.name));
            }
            auto freeName = [&](const String& stem)
            {
                String n = stem;
                for (int i = 2; taken.Contains(Lower(n)); i++)
                    n = stem + "_" + (String)i;
                taken.Add(Lower(n));
                return n;
            };
            Vector<UiImage> thumbs;
            for (auto& plan : plans)
            {
                auto cut = sprites.Find(plan.key);
                if (!cut || !*cut)
                    continue;
                String file = AssetFileName(plan.key, plan.rep ? plan.rep->kind : String(), (*cut)->img.w*(double)(*cut)->img.h, (double)W*H);
                String stem = freeName(HasSuffix(Lower(file), ".png") ? file.SubStr(0, file.Length() - 4) : file);
                OwnAssetInput in;
                in.png = (*cut)->img.Png();
                in.name = stem;
                in.type = TypeOfCut(plan.rep.Get(), stem);
                in.assetType = (*cut)->nine ? "sliced" : "simple";
                if ((*cut)->nine)
                    in.nineSlice = D::UiNine{ (double)(*cut)->nine->left, (double)(*cut)->nine->top, (double)(*cut)->nine->right, (double)(*cut)->nine->bottom };
                in.asIs = true;
                in.dpi = m.designDpi;
                in.from = "cut";
                in.description = plan.describe;
                D::UiOwnAsset asset;
                String error;
                if (!StoreOwnAsset(in, asset, error))
                    continue;
                res.assets.Add(asset);
                ownEls.Add(D::OwnElement(asset, PipelineUtils::Base64Encode(in.png)));
                assetOf[plan.key] = asset.name;
                thumbs.Add((*cut)->img);
            }
            if (st->background.kind == "image")
            {
                if (auto plate = sprites.Find("background"))
                {
                    OwnAssetInput in;
                    in.png = (*plate)->img.Png();
                    in.name = freeName("bg_backdrop");
                    in.type = "background";
                    in.assetType = "simple";
                    in.asIs = true;
                    in.dpi = m.designDpi;
                    in.from = "cut";
                    in.description = an.background.describe.IsEmpty() ? String("the backdrop of the screen") : an.background.describe;
                    D::UiOwnAsset asset;
                    String error;
                    if (StoreOwnAsset(in, asset, error))
                    {
                        res.assets.Add(asset);
                        ownEls.Add(D::OwnElement(asset, PipelineUtils::Base64Encode(in.png)));
                        backgroundKit = asset.name;
                        backgroundColor = String();
                    }
                }
            }
            else if (st->background.kind == "color")
            {
                backgroundKit = String();
                backgroundColor = st->background.color;
            }
            if (!thumbs.IsEmpty())
                log->Picture(act, "cut from the reference", ContactStrip(thumbs));
            Json patch = Json::Object();
            Json data = Json::Object();
            data["made"] = res.assets.Count();
            String names;
            for (auto& a : res.assets)
                names += (names.IsEmpty() ? "" : ", ") + a.name;
            data["names"] = names;
            patch["data"] = data;
            log->End(act, patch);
        }

        // 4. The elements as the layout's blocks
        act = log->Begin("draft", "draft", "First layout");
        D::UiKit own;
        own.name = "own";
        own.elements = ownEls;
        own.textStyles = styles;
        Vector<const D::UiKit*> parts = { kit };
        if (!ownEls.IsEmpty() || !styles.IsEmpty())
            parts.Add(&own);
        std::optional<D::UiKit> kitNow = D::MergeKits(parts);
        DraftOptions o;
        o.kit = kitNow ? &*kitNow : nullptr;
        o.assetOf = assetOf;
        o.styleOf = styleOf;
        if (kit)
            o.styles = kit->textStyles;
        o.styles.Add(styles);
        o.dpi = m.designDpi;
        o.name = m.name;
        o.backgroundKit = backgroundKit;
        o.backgroundColor = backgroundColor;
        res.draft = DraftFromAnalysis(an, o);

        // Only the styles the layout uses stay with the step
        Vector<String> used;
        D::Walk(res.draft.doc.root, [&](const D::UiNode& n, const D::UiNode*, int)
        {
            if (n.style)
                used.Add(Lower(*n.style));
        });
        Vector<D::TextStyle> all = styles;
        all.Add(res.draft.styles);
        for (auto& s : all)
        {
            if (used.Contains(Lower(s.name)))
                res.styles.Add(s);
        }
        for (auto& e : an.els)
        {
            if (!e->kit.IsEmpty())
                res.fromKit++;
            else if (!e->look.IsEmpty() && assetOf.ContainsKey(e->look))
                res.cut++;
            else if (!e->look.IsEmpty())
                res.bare++;
        }
        {
            Json patch = Json::Object();
            Json data = Json::Object();
            data["blocks"] = res.draft.blocks;
            data["kit"] = res.fromKit;
            data["made"] = res.cut;
            data["bare"] = res.bare;
            patch["data"] = data;
            log->End(act, patch);
        }
        trace->Save();

        D::UiKit kept;
        kept.name = "own";
        kept.elements = ownEls;
        kept.textStyles = res.styles;
        Vector<const D::UiKit*> finals = { kit };
        if (!ownEls.IsEmpty() || !res.styles.IsEmpty())
            finals.Add(&kept);
        res.kit = D::MergeKits(finals);
        res.ok = true;
        co_return res;
    }
}
