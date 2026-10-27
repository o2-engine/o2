#include "o2Editor/stdafx.h"
#include "UiLayoutRunSteps.h"

#include <algorithm>
#include <cmath>

#include "o2/Utils/Coroutines/Awaiters.h"
#include "o2/Utils/FileSystem/FileSystem.h"
#include "o2/Utils/Threading/ScopeLock.h"
#include "o2Editor/Pipeline/PipelineUtils.h"
#include "o2Editor/Pipeline/UiLayout/UiLayoutAgent.h"
#include "o2Editor/Pipeline/UiLayout/UiLayoutDraw.h"
#include "o2Editor/Pipeline/UiLayout/UiLayoutMeasure.h"
#include "o2Editor/Pipeline/UiLayout/UiLayoutParse.h"
#include "o2Editor/Pipeline/UiLayout/UiLayoutPrompts.h"

namespace Editor::UiLayout
{
    static const char* times = "\xC3\x97";

    static bool Cancelled(const RunState& st)
    {
        return st.ai && st.ai->exec && st.ai->exec->IsCancelled();
    }

    // --- sprites -----------------------------------------------------------------------------------------------

    static String SpriteProblem(const LookPlan& plan, const SpriteCut& cut)
    {
        if (cut.method != "ai")
            return "";

        const UiImage& img = cut.img;
        const Box& rb = plan.rep->box;
        if (img.w < rb.w*0.55 || img.h < rb.h*0.55)
        {
            return "matte too small (" + (String)img.w + times + (String)img.h + " for a " + (String)(int)JsRound(rb.w) + times +
                (String)(int)JsRound(rb.h) + " element)";
        }

        if (img.w > rb.w*1.6 + 8 || img.h > rb.h*1.6 + 8)
            return "matte spills over its surroundings";

        bool solid = plan.rep->kind == "panel" || plan.rep->kind == "input" || plan.rep->cell;
        if (solid)
        {
            int ix = (int)JsRound(img.w/6.0), iy = (int)JsRound(img.h/6.0);
            int n = 0, holes = 0;
            for (int y = iy; y < img.h - iy; y++)
            {
                for (int x = ix; x < img.w - ix; x++)
                {
                    n++;
                    if (img.At(x, y)[3] < 200)
                        holes++;
                }
            }
            if (n && (double)holes/n > 0.03)
                return "hole in the middle (" + (String)(int)JsRound((double)holes/n*100) + "% see-through)";
        }
        return "";
    }

    static UiImage SpriteOnDesign(const UiImage& design, const SpriteCut& cut)
    {
        const int pad = 6;
        Box b(cut.bounds.x - pad, cut.bounds.y - pad, cut.bounds.w + 2*pad, cut.bounds.h + 2*pad);
        int bw = (int)b.w, bh = (int)b.h;
        UiImage checker(bw, bh);
        for (int y = 0; y < bh; y++)
        {
            for (int x = 0; x < bw; x++)
            {
                UInt8 v = ((x >> 3) + (y >> 3)) % 2 ? 200 : 150;
                UInt8* p = checker.At(x, y);
                p[0] = p[1] = p[2] = v;
                p[3] = 255;
            }
        }
        checker.Blit(cut.img, pad, pad);
        UiImage crop = design.Crop((int)b.x, (int)b.y, bw, bh);
        int scale = Math::Max(1, Math::Min(4, (int)std::floor(220.0/Math::Max(bw, bh))));
        return Strip({ { "design", crop.Resize(bw*scale, bh*scale, true) }, { "sprite", checker.Resize(bw*scale, bh*scale, true) } });
    }

    namespace
    {
        struct CutJob: public RefCounterable
        {
            Ref<RunState>               st;
            UiDoc*                      doc = nullptr;
            Vector<LookPlan>*           plans = nullptr;
            std::vector<Ref<SpriteCut>> results;
            Atomic<int>                 next{ 0 };
            std::vector<Ref<AiCtx>>     ais;       // A model context per worker
            Mutex                       callMutex; // The run's log, cancel and retry callbacks, one worker at a time
        };

        // The workers share the run: they read the model without copying its references and call the run through their own
        // contexts
        Coroutine<void> CutWorker(CutJob* job, int worker)
        {
            auto& st = *job->st;
            const Ref<AiCtx>& ai = job->ais[worker];
            for (;;)
            {
                int index = job->next.FetchAdd(1);
                if (index >= job->plans->Count() || (ai->exec && ai->exec->IsCancelled()))
                    co_return;

                LookPlan* plan = &(*job->plans)[index];
                double t = NowMs();
                Ref<SpriteCut> cut;
                String error;
                Vector<String> attempts;
                for (int attempt = 0; attempt < 3 && !cut; attempt++)
                {
                    int seed = attempt ? (ai->seed >= 0 ? ai->seed : 1) + attempt*7919 : ai->seed;
                    CutResult res = co_await CutLook(ai, &st.design, job->doc, plan, st.cfg.sprites, seed);
                    if (!res.ok)
                    {
                        error = res.error;
                        attempts.Add("attempt " + (String)(attempt + 1) + " failed: " + error);
                        continue;
                    }

                    String problem = SpriteProblem(*plan, *res.cut);
                    if (!problem.IsEmpty() && attempt < 2 && res.cut->method == "ai")
                    {
                        attempts.Add("attempt " + (String)(attempt + 1) + ": " + problem);
                        continue;
                    }

                    if (!problem.IsEmpty())
                        attempts.Add("kept with: " + problem);
                    cut = res.cut;
                }

                if (!cut)
                {
                    Box b = ClampBox(plan->rep->box, st.design.w, st.design.h);
                    cut = mmake<SpriteCut>();
                    cut->key = plan->key;
                    cut->img = st.design.Crop((int)JsRound(b.x), (int)JsRound(b.y), (int)JsRound(b.w), (int)JsRound(b.h));
                    cut->bounds = Box(JsRound(b.x), JsRound(b.y), cut->img.w, cut->img.h);
                    cut->method = "local";
                    cut->note = "plain crop (" + error + ")";
                }

                job->results[index] = cut;

                Json images = Json::Array();
                for (auto& s : cut->steps)
                    images.Add(st.trace->Image(s.title, s.img));
                images.Add(st.trace->Image("sprite", cut->img));
                images.Add(st.trace->Image("sprite over the design", SpriteOnDesign(st.design, *cut)));

                String note = cut->note;
                bool kept = false;
                for (auto& a : attempts)
                {
                    note += String(" \xC2\xB7 ") + a;
                    kept = kept || a.StartsWith("kept");
                }

                Json data = Json::Object();
                data["key"] = plan->key;
                data["describe"] = plan->describe;
                data["rep"] = plan->rep->id;
                Json instances = Json::Array();
                for (auto& i : plan->instances)
                    instances.Add(i->id);
                data["instances"] = instances;
                Json erase = Json::Array();
                for (auto& e : plan->erase)
                    erase.Add(e.label);
                data["erase"] = erase;
                data["method"] = cut->method;
                if (cut->nine)
                {
                    Json nine = Json::Object();
                    nine["left"] = cut->nine->left;
                    nine["right"] = cut->nine->right;
                    nine["top"] = cut->nine->top;
                    nine["bottom"] = cut->nine->bottom;
                    data["nine"] = nine;
                }

                Json step = Json::Object();
                step["id"] = "sprite-" + plan->key;
                step["title"] = "Sprite " + plan->key + " (" + (String)cut->img.w + times + (String)cut->img.h + ", " +
                    (String)plan->instances.Count() + times + ")";
                step["kind"] = "sprite";
                step["ms"] = Since(t);
                step["note"] = note;
                step["status"] = kept || cut->note.StartsWith("plain") ? "warn" : "ok";
                step["images"] = images;
                step["data"] = data;
                st.trace->Step(step);
            }
        }
    }

    Coroutine<bool> CutAll(Ref<RunState> st, UiDoc* doc, Vector<LookPlan>* plans, OrderedMap<Ref<SpriteCut>>* sprites)
    {
        auto job = mmake<CutJob>();
        job->st = st;
        job->doc = doc;
        job->plans = plans;
        job->results.resize(plans->Count());

        Vector<Coroutine<void>> workers;
        for (int i = 0; i < Math::Min(4, plans->Count()); i++)
        {
            job->ais.push_back(ParallelAiCtx(st->ai, &job->callMutex));
            workers.Add(CutWorker(job.Get(), i));
        }
        co_await WaitAll(workers);

        for (auto& ai : job->ais)
        {
            st->ai->textCalls.FetchAdd(ai->textCalls.Load());
            st->ai->imageCalls.FetchAdd(ai->imageCalls.Load());
            st->ai->replayed.FetchAdd(ai->replayed.Load());
        }

        for (int i = 0; i < plans->Count(); i++)
        {
            if (job->results[i])
                sprites->Set((*plans)[i].key, job->results[i]);
        }
        co_return !Cancelled(*st);
    }

    // --- background -------------------------------------------------------------------------------------------

    Coroutine<bool> MakeBackground(Ref<RunState> st)
    {
        if (st->cfg.background == "none")
        {
            st->background = UiBackdrop();
            co_return true;
        }

        double t = NowMs();
        const UiImage& design = st->design;
        Vector<Ref<UiEl>> tops;
        for (auto& e : st->doc.els)
        {
            if (e->parent.IsEmpty())
                tops.Add(e);
        }

        UiImage free(design.w, design.h);
        std::vector<Rgb> samples;
        for (int y = 0; y < design.h; y++)
        {
            for (int x = 0; x < design.w; x++)
            {
                bool covered = false;
                for (auto& e : tops)
                {
                    if (x >= e->box.x - 2 && x < e->box.x + e->box.w + 2 && y >= e->box.y - 2 && y < e->box.y + e->box.h + 2)
                    {
                        covered = true;
                        break;
                    }
                }
                if (covered)
                    continue;

                const UInt8* p = design.At(x, y);
                UInt8* d = free.At(x, y);
                d[0] = p[0]; d[1] = p[1]; d[2] = p[2]; d[3] = 255;
                if ((x + y) % 3 == 0)
                    samples.push_back(Rgb(p[0], p[1], p[2]));
            }
        }

        Rgb dom = DominantColor(free, Box(0, 0, design.w, design.h));
        std::vector<double> dists;
        for (auto& c : samples)
            dists.push_back(ColorDist(c, dom));
        std::sort(dists.begin(), dists.end());
        size_t at = (size_t)std::floor(dists.size()*0.9);
        double spread = at < dists.size() ? dists[at] : 0.0;

        String want = st->cfg.background;
        if (want == "auto")
            want = st->doc.background.kind == "image" || spread > 10 ? "image" : "color";

        if (want == "color")
        {
            String color = HexOf(dom);
            Json data = Json::Object();
            data["color"] = color;
            data["spread"] = JsRound(spread*10)/10;
            Json step = Json::Object();
            step["id"] = "background";
            step["title"] = "Background colour " + color;
            step["kind"] = "background";
            step["ms"] = Since(t);
            step["data"] = data;
            st->trace->Step(step);
            st->background.kind = "color";
            st->background.color = color;
            co_return true;
        }

        // A plate that still shows letters or small artwork of the elements gets one stricter ask; what the better plate
        // still keeps is filled from around it
        Json images = Json::Array();
        Json found = Json::Array();
        bool haveBest = false;
        UiImage best;
        Vector<Box> bestLeft;
        for (bool strict : { false, true })
        {
            AiAnswer answer = co_await AiImage(st->ai, Prompts::Background(st->doc.background.describe, strict), { st->designPng },
                                               strict ? "background-strict" : "background", "");
            UiImage decoded;
            if (!answer.ok || !UiImage::Decode(answer.data, decoded))
            {
                if (haveBest)
                    break;

                // No plate: the backdrop's own colour still stands behind the UI
                String color = HexOf(dom);
                Json data = Json::Object();
                data["color"] = color;
                data["spread"] = spread;
                Json step = Json::Object();
                step["id"] = "background";
                step["title"] = "Background colour " + color + " (no plate)";
                step["kind"] = "background";
                step["ms"] = Since(t);
                step["status"] = "warn";
                step["note"] = answer.ok ? String("the plate is not a decodable image") : answer.error;
                step["data"] = data;
                st->trace->Step(step);
                st->background.kind = "color";
                st->background.color = color;
                co_return true;
            }

            UiImage img = decoded.Resize(design.w, design.h).Flatten(255, 255, 255);
            Vector<Box> left = PlateLeftovers(img, design, st->doc, st->sprites);
            found.Add(Json((double)left.Count()));
            images.Add(st->trace->Image(strict ? "plate, second ask" : "background plate", img));
            if (left.Count() > 0)
            {
                UiImage marked = img;
                for (auto& b : left)
                    StrokeRect(marked, b, Rgb(230, 40, 40), 1);
                images.Add(st->trace->Image((String)left.Count() + " element(s) left on the plate", marked));
            }

            if (!haveBest || left.Count() < bestLeft.Count())
            {
                best = img;
                bestLeft = left;
                haveBest = true;
            }
            if (left.Count() < 2)
                break;
        }

        for (auto& b : bestLeft)
        {
            Box g = IntBox(ClampBox(Grow(b, 2), design.w, design.h));
            HarmonicFill(best, (int)g.x, (int)g.y, (int)(g.x + g.w - 1), (int)(g.y + g.h - 1), true);
        }
        if (bestLeft.Count() > 0)
            images.Add(st->trace->Image("leftovers filled", best));

        auto cut = mmake<SpriteCut>();
        cut->key = "background";
        cut->img = best;
        cut->bounds = Box(0, 0, design.w, design.h);
        cut->method = "ai";
        cut->note = "background plate";
        st->sprites.Set("background", cut);

        int filled = bestLeft.Count();
        Json data = Json::Object();
        data["spread"] = JsRound(spread*10)/10;
        data["leftovers"] = found;
        data["filled"] = (double)filled;
        Json step = Json::Object();
        step["id"] = "background";
        step["title"] = filled ? "Background plate (" + (String)filled + " leftover(s) filled)" : String("Background plate");
        step["kind"] = "background";
        step["ms"] = Since(t);
        if (filled)
            step["status"] = "warn";
        step["images"] = images;
        step["data"] = data;
        st->trace->Step(step);

        st->background.kind = "image";
        st->background.key = "background";
        co_return true;
    }

    Vector<Box> PlateLeftovers(const UiImage& plate, const UiImage& design, const UiDoc& doc, const OrderedMap<Ref<SpriteCut>>& sprites)
    {
        std::vector<float> lp = Luma(plate), ld = Luma(design);
        double small = 0.06*design.w*design.h;
        Vector<Box> out;
        for (auto& e : doc.els)
        {
            bool text = e->kind == "text";
            if (!text && !(HasLook(*e) && !e->look.IsEmpty() && e->box.w*e->box.h < small))
                continue;

            Box b = IntBox(ClampBox(text && e->ink ? *e->ink : e->box, design.w, design.h));
            if (b.w < 3 || b.h < 3)
                continue;

            const SpriteCut* sp = nullptr;
            if (!text && !e->look.IsEmpty())
            {
                if (auto found = sprites.Find(e->look))
                    sp = found->Get();
            }
            auto on = [&](int x, int y)
            {
                if (!sp)
                    return true;
                int sx = Math::Min(sp->img.w - 1, (int)std::floor((x - b.x + 0.5)/b.w*sp->img.w));
                int sy = Math::Min(sp->img.h - 1, (int)std::floor((y - b.y + 0.5)/b.h*sp->img.h));
                return sp->img.At(sx, sy)[3] >= 128;
            };

            int x0 = (int)b.x, y0 = (int)b.y, x1 = (int)(b.x + b.w), y1 = (int)(b.y + b.h);
            int n = 0;
            double mp = 0, md = 0;
            for (int y = y0; y < y1; y++)
            {
                for (int x = x0; x < x1; x++)
                {
                    if (!on(x, y))
                        continue;
                    size_t i = (size_t)y*design.w + x;
                    mp += lp[i];
                    md += ld[i];
                    n++;
                }
            }
            if (n < 20)
                continue;

            mp /= n;
            md /= n;
            double cov = 0, vp = 0, vd = 0;
            for (int y = y0; y < y1; y++)
            {
                for (int x = x0; x < x1; x++)
                {
                    if (!on(x, y))
                        continue;
                    size_t i = (size_t)y*design.w + x;
                    double p = lp[i] - mp, d = ld[i] - md;
                    cov += p*d;
                    vp += p*p;
                    vd += d*d;
                }
            }
            if (std::sqrt(vp/n) < 12 || std::sqrt(vd/n) < 12)
                continue;
            if (cov/std::sqrt(vp*vd) > 0.6)
                out.Add(b);
        }
        return out;
    }

    UiImage OverlayKinds(const UiImage& design, const UiDoc& doc)
    {
        int scale = design.w < 700 ? 2 : 1;
        UiImage img = design.Resize(design.w*scale, design.h*scale);
        for (auto& e : doc.els)
        {
            Box b(e->box.x*scale, e->box.y*scale, e->box.w*scale, e->box.h*scale);
            StrokeRect(img, b, KindColor(e->kind), e->kind == "grid" || e->kind == "panel" ? 2 : 1);
        }
        return img;
    }


    // --- the sliced-asset folder -------------------------------------------------------------------------------

    String AssetFileName(const String& key, const String& kind, double area, double screen)
    {
        String k = key.ToLowerCase();
        auto has = [&](std::initializer_list<const char*> words)
        {
            for (auto w : words)
            {
                if (k.Contains(w))
                    return true;
            }
            return false;
        };

        String stem = SafeKey(key, "sprite");
        if (kind == "panel" || kind == "input")
        {
            if (!has({ "panel", "frame", "bg", "popup", "border", "slot", "bubble", "bar" }))
                stem = "panel_" + stem;
        }
        else if (kind == "button")
        {
            if (!has({ "button", "btn" }))
                stem = "button_" + stem;
        }
        else if (kind == "progress")
        {
            if (!has({ "bar" }))
                stem = "bar_" + stem;
        }
        else if (area < screen*0.1 && !has({ "icon", "character", "avatar", "portrait", "profile" }))
            stem = "icon_" + stem;

        return stem + ".png";
    }

    Vector<String> AssetRefs(const Json& structure)
    {
        Vector<String> out;
        Function<void(const Json&)> walk = [&](const Json& v)
        {
            if (v.IsArray())
            {
                for (auto& item : v.Items())
                    walk(item);
                return;
            }

            if (!v.IsObject())
                return;

            for (auto& member : v.Members())
            {
                if (member.first == "asset" && member.second.IsString())
                {
                    String asset = member.second.Str();
                    if (asset.StartsWith("./"))
                        asset = asset.SubStr(2);
                    else if (asset.StartsWith("/"))
                        asset = asset.SubStr(1);
                    if (!out.Contains(asset))
                        out.Add(asset);
                }
                else
                    walk(member.second);
            }
        };
        walk(structure);
        return out;
    }

    String SpritesNote(const Vector<AssetFile>& files, const UiBackdrop& background)
    {
        int nine = files.Count([](const AssetFile& f) { return f.nine.has_value(); });
        String bg = background.kind == "image"
            ? String("`bg_backdrop.png` is the screen's backdrop with every UI element removed.")
            : background.kind == "color"
                ? "The backdrop is a flat colour " + background.color + "; there is no sprite for it."
                : String("There is no backdrop sprite: leave the area behind the UI empty.");

        String note = "About the sliced assets: they were cut from this very design, one PNG per distinct piece of artwork, "
            "each at the size it appears, with the labels, numbers and other text drawn on it removed \xE2\x80\x94 the text "
            "belongs in text elements.";
        if (nine > 0)
        {
            note += " " + (String)nine + " stretchable sprite(s) carry Unity nine-slice borders in their .png.meta "
                "(has_meta_border in the inventory).";
        }
        return note + " " + bg;
    }

    String NineSliceMeta(const NineSlice& n)
    {
        return "fileFormatVersion: 2\nTextureImporter:\n  spriteBorder: {x: " + (String)n.left + ", y: " + (String)n.bottom +
            ", z: " + (String)n.right + ", w: " + (String)n.top + "}\n";
    }

    Vector<AssetFile> WriteAssets(const String& dir, const Vector<LookPlan>& plans, const OrderedMap<Ref<SpriteCut>>& sprites,
                                  double screen)
    {
        Vector<AssetFile> files;
        Vector<String> taken;
        auto unique = [&](const String& name)
        {
            String n = name;
            for (int i = 2; taken.Contains(n.ToLowerCase()); i++)
                n = name.SubStr(0, name.Length() - 4) + "_" + (String)i + ".png";
            taken.Add(n.ToLowerCase());
            return n;
        };

        for (auto& plan : plans)
        {
            const Ref<SpriteCut>* found = sprites.Find(plan.key);
            if (!found || !*found)
                continue;

            const SpriteCut& cut = **found;
            String file = unique(AssetFileName(plan.key, plan.rep->kind, (double)cut.img.w*cut.img.h, screen));
            PipelineUtils::WriteFileBytes(dir + "/" + file, cut.img.Png());
            if (cut.nine && (cut.nine->left || cut.nine->top || cut.nine->right || cut.nine->bottom))
                PipelineUtils::WriteFileBytes(dir + "/" + file + ".meta", NineSliceMeta(*cut.nine));
            files.Add({ file, plan.key, cut.nine });
        }

        if (const Ref<SpriteCut>* plate = sprites.Find("background"))
        {
            String file = unique("bg_backdrop.png");
            PipelineUtils::WriteFileBytes(dir + "/" + file, (*plate)->img.Png());
            files.Add({ file, "background", std::nullopt });
        }
        return files;
    }

    static String FontsFolder()
    {
        String editor = String(GetEditorAssetsPath()) + "ui/pipeline/fonts/";
        if (o2FileSystem.IsFileExist(editor + "Roboto-Regular.ttf"))
            return editor;

        String found = FindRepoPath("o2/Editor/Assets/ui/pipeline/fonts");
        if (found.IsEmpty())
            found = FindRepoPath("Editor/Assets/ui/pipeline/fonts");
        return found.IsEmpty() ? editor : found + "/";
    }

    static const char* fontFiles[] = { "Roboto-Regular.ttf", "Roboto-Medium.ttf", "Roboto-Bold.ttf" };

    // --- the run -----------------------------------------------------------------------------------------------

    static ImageToUiResult Fail(const String& error)
    {
        ImageToUiResult res;
        res.error = error;
        return res;
    }

    static Json NineJson(const std::optional<NineSlice>& nine)
    {
        if (!nine)
            return Json();

        Json json = Json::Object();
        json["left"] = nine->left;
        json["top"] = nine->top;
        json["right"] = nine->right;
        json["bottom"] = nine->bottom;
        return json;
    }

    Coroutine<ImageToUiResult> RunImageToUi(Ref<AiCtx> ai, String designPng, ImageToUiConfig cfg, Ref<Trace> trace, String workDir)
    {
        double t0 = NowMs();
        auto st = mmake<RunState>();
        st->ai = ai;
        st->trace = trace;
        st->cfg = cfg;

        if (workDir.IsEmpty())
            co_return Fail("imageToUi: the node needs its work folder");

        UiImage decoded;
        if (!UiImage::Decode(designPng, decoded))
            co_return Fail("imageToUi: the input is not a decodable image");

        st->design = decoded.Flatten(255, 255, 255);
        st->designPng = st->design.Png();
        int W = st->design.w, H = st->design.h;

        {
            Json images = Json::Array();
            images.Add(trace->Image("design", st->design, "design.png"));
            Json data = Json::Object();
            data["width"] = W;
            data["height"] = H;
            Json step = Json::Object();
            step["id"] = "input";
            step["title"] = "Design";
            step["kind"] = "input";
            step["images"] = images;
            step["data"] = data;
            trace->Step(step);
        }

        // 1. What artwork the screen is made of
        double t = NowMs();
        AiAnswer answer = co_await AiText(ai, Prompts::Analyze(W, H, cfg.hint), { st->designPng }, "analysis");
        if (!answer.ok)
            co_return Fail(answer.error);

        String parseError;
        if (!ParseAnalysis(answer.data, W, H, st->doc, &parseError) || st->doc.els.IsEmpty())
            co_return Fail(parseError.IsEmpty() ? String("imageToUi: the model found no UI elements in the image") : "imageToUi: " + parseError);

        UiDoc& doc = st->doc;
        {
            Json looks = Json::Object();
            for (auto& look : doc.looks)
                looks[look.first] = look.second;
            Json background = Json::Object();
            background["kind"] = doc.background.kind;
            background["color"] = doc.background.color;
            background["describe"] = doc.background.describe;
            Json data = Json::Object();
            data["elements"] = doc.els.Count();
            data["looks"] = looks;
            data["background"] = background;
            Json images = Json::Array();
            images.Add(trace->Image("analysis boxes", OverlayKinds(st->design, doc)));
            Json step = Json::Object();
            step["id"] = "analysis";
            step["title"] = "Analysis: " + (String)doc.els.Count() + " elements, " + (String)doc.looks.Count() + " looks";
            step["kind"] = "analysis";
            step["ms"] = Since(t);
            step["text"] = answer.data;
            step["images"] = images;
            step["data"] = data;
            trace->Step(step);
        }

        // The labels a sprite must lose, where their ink really is
        for (auto& e : doc.els)
        {
            if (e->kind == "text" && !e->slot)
                MeasureText(st->design, *e);
        }
        for (auto& cell : doc.els)
        {
            if (!cell->cell)
                continue;

            Vector<Ref<UiEl>> texts;
            for (auto& c : doc.ChildrenOf(cell->id))
            {
                if (c->kind == "text")
                    texts.Add(c);
            }
            MeasureCellTexts(st->design, *cell, texts);
        }

        // 2. The sliced sprites: one per look, and the backdrop
        Vector<LookPlan> plans = PlanLooks(doc);
        if (!co_await CutAll(st, &doc, &plans, &st->sprites))
            co_return Fail("Cancelled");

        co_await MakeBackground(st);
        if (Cancelled(*st))
            co_return Fail("Cancelled");

        // 3. The skill's inputs, laid out as it expects them: the design, a flat folder of sliced PNGs (Unity nine-slice borders
        // in .png.meta), the fonts, a fresh task folder
        o2FileSystem.FolderCreate(workDir, true);
        String work = AbsolutePath(workDir);
        while (HasSuffix(work, "/"))
            work = work.SubStr(0, work.Length() - 1);
        String inputDir = work + "/input", assetsDir = work + "/sprites", fontsDir = work + "/fonts", taskDir = work + "/task",
            scratchDir = work + "/agent-view";
        for (auto& d : { inputDir, assetsDir, fontsDir, taskDir, scratchDir })
        {
            if (o2FileSystem.IsFolderExist(d))
                o2FileSystem.FolderRemove(d, true);
        }
        for (auto& d : { inputDir, assetsDir, fontsDir })
            o2FileSystem.FolderCreate(d, true);

        String designFile = inputDir + "/design.png";
        PipelineUtils::WriteFileBytes(designFile, st->designPng);
        Vector<AssetFile> files = WriteAssets(assetsDir, plans, st->sprites, (double)W*H);

        // The skill looks a fontFamily up next to the task, asset and design folders and their parents: fonts/Roboto-Bold.ttf
        // resolves from <work>, a bare Roboto-Bold.ttf (models write that too) from the design's folder
        String fonts = FontsFolder();
        for (auto font : fontFiles)
        {
            o2FileSystem.FileCopy(fonts + font, fontsDir + "/" + font);
            o2FileSystem.FileCopy(fonts + font, inputDir + "/" + font);
        }
        o2FileSystem.FileCopy(fonts + "LICENSE-Roboto.txt", fontsDir + "/LICENSE-Roboto.txt");

        {
            Json list = Json::Array();
            for (auto& f : files)
            {
                Json item = Json::Object();
                item["file"] = f.file;
                item["look"] = f.look;
                item["nineSlice"] = NineJson(f.nine);
                list.Add(item);
            }
            Json background = Json::Object();
            background["kind"] = st->background.kind;
            if (st->background.kind == "color")
                background["color"] = st->background.color;
            if (st->background.kind == "image")
                background["key"] = st->background.key;
            Json data = Json::Object();
            data["files"] = list;
            data["background"] = background;
            Json step = Json::Object();
            step["id"] = "assets";
            step["title"] = "Sliced assets: " + (String)files.Count() + " sprites";
            step["kind"] = "measure";
            step["data"] = data;
            trace->Step(step);
        }

        // 4. The skill, with the model as its agent; it appends to the trace itself
        t = NowMs();
        trace->Flush();
        AgentRun run;
        run.python = cfg.python;
        run.workDir = work;
        run.design = designFile;
        run.assets = assetsDir;
        run.task = taskDir;
        run.scratch = scratchDir;
        run.traceDir = trace->GetDir().IsEmpty() ? String() : AbsolutePath(trace->GetDir());
        run.maxTurns = cfg.maxTurns;
        run.name = cfg.name;
        run.hint = cfg.hint;
        run.note = SpritesNote(files, st->background);
        for (auto font : fontFiles)
            run.fonts.Add(String("fonts/") + font);

        AgentResult agent = co_await RunAgent(ai, run);
        trace->Reload();
        if (!agent.failure.IsEmpty())
            co_return Fail(agent.failure);

        if (!agent.ok)
        {
            String why = agent.error;
            const Json* status = agent.summary.Get("status");
            if (why.IsEmpty() && status && status->IsString())
                why = status->Str();
            co_return Fail("imageToUi: the agent ended without a checked layout (" + (why.IsEmpty() ? String("no check passed") : why) + ")");
        }

        // 5. Outputs: the reconstruction, ui_structure.json as the skill left it, and every sprite the layout names
        ImageToUiResult res;
        res.layout = PipelineUtils::ReadFileBytes(taskDir + "/ui_structure.json");
        res.preview = PipelineUtils::ReadFileBytes(taskDir + "/reconstruction.png");
        Json structure;
        if (res.layout.IsEmpty() || !Json::Parse(res.layout, structure))
            co_return Fail("imageToUi: the agent left no readable ui_structure.json");
        if (res.preview.IsEmpty())
            co_return Fail("imageToUi: the agent left no reconstruction.png");

        Vector<String> used = AssetRefs(structure);
        for (auto& f : files)
        {
            if (!used.Contains(f.file))
                continue;
            res.sprites.Add({ f.file.SubStr(0, f.file.Length() - 4), PipelineUtils::ReadFileBytes(assetsDir + "/" + f.file), f.nine });
        }

        Json summary = agent.summary.IsObject() ? agent.summary : Json::Object();
        summary["sprites"] = res.sprites.Count();
        summary["textCalls"] = ai->textCalls.Load();
        summary["imageCalls"] = ai->imageCalls.Load();
        summary["replayed"] = ai->replayed.Load();
        summary["ms"] = Since(t0);
        res.complete = summary.Get("complete") && summary.Get("complete")->Bool();
        if (const Json* blockers = summary.Get("blockers"))
        {
            for (auto& b : blockers->Items())
                res.blockers.Add(b.ToJsString());
        }
        int checks = summary.Get("check_count") ? (int)summary.Get("check_count")->Num() : 0;
        int turns = summary.Get("turns") ? (int)summary.Get("turns")->Num() : 0;

        trace->SetSummary(summary);
        {
            Json step = Json::Object();
            step["id"] = "output";
            step["kind"] = "output";
            step["ms"] = Since(t);
            step["status"] = res.complete ? "ok" : "warn";
            step["title"] = res.complete
                ? "Done: the skill completed (" + (String)checks + " checks, " + (String)res.sprites.Count() + " sprites)"
                : "Done, NOT completed: " + (String)res.blockers.Count() + " blocker(s) after " + (String)turns + " turns";
            step["data"] = summary;
            step["text"] = res.layout;
            trace->Step(step);
        }
        trace->Save();

        res.ok = true;
        res.summary = summary;
        co_return res;
    }
}
