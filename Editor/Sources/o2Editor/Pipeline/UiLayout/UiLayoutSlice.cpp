#include "o2Editor/stdafx.h"
#include "UiLayoutSlice.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <set>

#include "o2Editor/Pipeline/PipelineImageOps.h"
#include "o2Editor/Pipeline/PipelineUpscale.h"
#include "o2Editor/Pipeline/UiLayout/UiLayoutJson.h"
#include "o2Editor/Pipeline/UiLayout/UiLayoutPrompts.h"

namespace Editor::UiLayout
{
    static const Vector<KeyColor>& Keys()
    {
        static Vector<KeyColor> keys = {
            { "pure green", "#00FF00", Rgb(0, 255, 0) },
            { "pure magenta", "#FF00FF", Rgb(255, 0, 255) },
            { "pure blue", "#0000FF", Rgb(0, 0, 255) },
        };
        return keys;
    }

    static void ChromaOf(double r, double g, double b, double& cb, double& cr)
    {
        cb = -0.168736*r - 0.331264*g + 0.5*b;
        cr = 0.5*r - 0.418688*g - 0.081312*b;
    }

    int Utf16Length(const String& text)
    {
        int n = 0;
        for (int i = 0; i < text.Length(); i++)
        {
            unsigned char c = (unsigned char)text[i];
            if ((c & 0xC0) == 0x80)
                continue;
            n += c >= 0xF0 ? 2 : 1;
        }
        return n;
    }

    KeyColor PickKey(const UiImage& img, const Box& b)
    {
        Box bb = IntBox(ClampBox(b, img.w, img.h));
        const KeyColor* best = &Keys()[0];
        double bestScore = -1;
        for (auto& k : Keys())
        {
            double kcb, kcr;
            ChromaOf(k.rgb.r, k.rgb.g, k.rgb.b, kcb, kcr);
            std::vector<double> d;
            for (int y = (int)bb.y; y < bb.y + bb.h; y += 2)
            {
                for (int x = (int)bb.x; x < bb.x + bb.w; x += 2)
                {
                    if (x >= img.w || y >= img.h)
                        continue;

                    const UInt8* p = img.At(x, y);
                    double cb, cr;
                    ChromaOf(p[0], p[1], p[2], cb, cr);
                    d.push_back(std::hypot(cb - kcb, cr - kcr));
                }
            }
            std::sort(d.begin(), d.end());
            double score = d.empty() ? 0.0 : d[(size_t)std::floor(d.size()*0.03)];
            if (score > bestScore)
            {
                bestScore = score;
                best = &k;
            }
        }
        return *best;
    }

    static bool IsAncestor(const UiDoc& doc, const String& id, const String& of)
    {
        auto find = [&](const String& eid) -> String
        {
            const UiEl* e = doc.Find(eid);
            return e ? e->parent : String();
        };

        String p = find(of);
        while (!p.IsEmpty())
        {
            if (p == id)
                return true;
            p = find(p);
        }
        return false;
    }

    static bool IsNumber(const String& text)
    {
        if (text.IsEmpty())
            return false;

        for (int i = 0; i < text.Length(); i++)
        {
            if (text[i] < '0' || text[i] > '9')
                return false;
        }
        return true;
    }

    static String LabelOf(const UiDoc& doc, const UiEl& o)
    {
        if (o.kind == "text")
        {
            if (Utf16Length(o.text) <= 3)
                return String("the ") + (IsNumber(o.text) ? "number" : "letter") + " \"" + o.text + "\"";
            return "the text \"" + o.text + "\"";
        }

        const String* describe = doc.looks.Find(o.look);
        if (describe)
            return "the " + *describe;
        return "the " + (o.look.IsEmpty() ? o.kind : o.look);
    }

    Vector<EraseItem> EraseList(const UiDoc& doc, const Ref<UiEl>& e)
    {
        Vector<EraseItem> out;
        std::set<std::string> desc;
        for (auto& d : doc.DescendantsOf(e->id))
            desc.insert(d->id);

        int at = doc.IndexOf(e);
        for (int i = 0; i < doc.els.Count(); i++)
        {
            auto& o = doc.els[i];
            if (o == e || o->kind == "group" || o->kind == "grid")
                continue;

            Box tb = o->kind == "text" && o->ink ? *o->ink : o->box;
            bool over = desc.count(o->id) > 0 ||
                (i > at && Inter(o->box, e->box) > 0.6*o->box.w*o->box.h && o->box.w*o->box.h < 0.7*e->box.w*e->box.h) ||
                (o->kind == "text" && !IsAncestor(doc, o->id, e->id) && Inter(tb, e->box) > 0.1*tb.w*tb.h);
            if (!over)
                continue;

            EraseItem item;
            item.label = LabelOf(doc, *o);
            item.box = !o->text.IsEmpty() && o->ink ? *o->ink : o->box;
            if (o->kind == "text" && !o->color.IsEmpty())
                item.color = o->color;
            out.Add(item);
        }
        return out;
    }

    static int EdgeTouch(const UiDoc& doc, const UiEl& e)
    {
        return (e.box.x < 2 ? 1 : 0) + (e.box.y < 2 ? 1 : 0) + (e.box.x + e.box.w > doc.w - 2 ? 1 : 0) + (e.box.y + e.box.h > doc.h - 2 ? 1 : 0);
    }

    Vector<LookPlan> PlanLooks(UiDoc& doc)
    {
        OrderedMap<Vector<Ref<UiEl>>> groups;
        for (auto& e : doc.els)
        {
            if (HasLook(*e))
                groups[e->look].Add(e);
        }

        Vector<LookPlan> plans;
        for (auto& group : groups)
        {
            String look = group.first;
            Vector<Vector<Ref<UiEl>>> clusters;
            for (auto& e : group.second)
            {
                Vector<Ref<UiEl>>* found = nullptr;
                for (auto& cl : clusters)
                {
                    auto& f = cl[0]->box;
                    if (std::fabs(f.w - e->box.w) <= Math::Max(3.0, f.w*0.12) && std::fabs(f.h - e->box.h) <= Math::Max(3.0, f.h*0.12))
                    {
                        found = &cl;
                        break;
                    }
                }

                if (found)
                    found->Add(e);
                else
                    clusters.Add({ e });
            }

            StableSortBy(clusters, [](const Vector<Ref<UiEl>>& a, const Vector<Ref<UiEl>>& b) { return a.Count() > b.Count(); });

            for (int i = 0; i < clusters.Count(); i++)
            {
                auto& cl = clusters[i];
                String key = i == 0 ? look : look + "_" + (String)(i + 1);
                for (auto& e : cl)
                    e->look = key;

                const String* lookDescribe = doc.looks.Find(look);
                if (i > 0)
                    doc.looks.Set(key, lookDescribe ? *lookDescribe : look);

                Vector<Vector<EraseItem>> erases;
                for (auto& e : cl)
                    erases.Add(EraseList(doc, e));

                // Cut from the instance with the least drawn over it, away from the edges, the largest
                int rep = 0;
                for (int k = 1; k < cl.Count(); k++)
                {
                    int dc = erases[k].Count() - erases[rep].Count();
                    int de = EdgeTouch(doc, *cl[k]) - EdgeTouch(doc, *cl[rep]);
                    double da = cl[k]->box.w*cl[k]->box.h - cl[rep]->box.w*cl[rep]->box.h;
                    if (dc < 0 || (dc == 0 && (de < 0 || (de == 0 && da > 0))))
                        rep = k;
                }

                LookPlan plan;
                plan.key = key;
                lookDescribe = doc.looks.Find(look);
                plan.describe = lookDescribe ? *lookDescribe : look;
                plan.rep = cl[rep];
                plan.instances = cl;
                plan.erase = erases[rep];
                plan.stretch = IsStretchy(*plan.rep);
                plans.Add(plan);
            }
        }
        return plans;
    }

    static Box CropBoxOf(const UiDoc& doc, const UiEl& e)
    {
        double px = JsRound(Math::Max(5.0, Math::Min(24.0, 0.18*e.box.w))), py = JsRound(Math::Max(5.0, Math::Min(24.0, 0.18*e.box.h)));
        return IntBox(ClampBox(Box(e.box.x - px, e.box.y - py, e.box.w + 2*px, e.box.h + 2*py), doc.w, doc.h));
    }

    static Ref<SpriteCut> Finish(const LookPlan& plan, const Box& cb, UiImage img, const String& method, const Vector<SpriteStep>& steps,
                                 const String& note)
    {
        int n = img.w*img.h;
        std::vector<UInt8> m((size_t)n);
        for (int i = 0; i < n; i++)
            m[i] = img.data[(size_t)i*4 + 3] > 20 ? 1 : 0;

        std::vector<int> labels;
        std::vector<Component> comps;
        Components(m, img.w, img.h, labels, comps);

        Box inner(plan.rep->box.x - cb.x, plan.rep->box.y - cb.y, plan.rep->box.w, plan.rep->box.h);
        std::set<int> keep;
        for (auto& c : comps)
        {
            if (Inter(c.box, inner) > 0 && (c.area > 12 || c.area > 0.02*inner.w*inner.h))
                keep.insert(c.label);
        }

        for (int i = 0; i < n; i++)
        {
            if (m[i] && !keep.count(labels[i]))
                img.data[(size_t)i*4 + 3] = 0;
        }

        Box b;
        if (!AlphaBounds(img, 8, b))
            b = Box(0, 0, img.w, img.h);

        auto cut = mmake<SpriteCut>();
        cut->key = plan.key;
        cut->img = img.Crop((int)b.x, (int)b.y, (int)b.w, (int)b.h);
        cut->bounds = Box(cb.x + b.x, cb.y + b.y, b.w, b.h);
        cut->method = method;
        cut->steps = steps;
        cut->note = note;
        return cut;
    }

    static String HexOfRounded(const Rgb& c)
    {
        return HexOf(Rgb(JsRound(c.r), JsRound(c.g), JsRound(c.b)));
    }

    static Ref<SpriteCut> LocalCut(const UiDoc& doc, const LookPlan& plan, const Box& cb, const UiImage& crop)
    {
        std::set<std::string> family = { plan.rep->id.Data() };
        doc.CollectDescendantIds(plan.rep->id, family);
        for (String anc = plan.rep->parent; !anc.IsEmpty();)
        {
            family.insert(anc.Data());
            const UiEl* e = doc.Find(anc);
            anc = e ? e->parent : String();
        }

        for (auto& o : doc.els)
        {
            if (family.count(o->id) || o->kind == "group" || o->kind == "grid" || (o->cell && o->cell->of == plan.rep->parent))
                continue;

            double i = Inter(o->box, cb);
            if (i > 4 && i < o->box.w*o->box.h*0.95)
                return nullptr;
        }

        std::vector<Rgb> ring;
        for (int x = 0; x < crop.w; x++)
        {
            for (int y : { 0, crop.h - 1 })
                ring.push_back(RgbAt(crop, x, y));
        }
        for (int y = 0; y < crop.h; y++)
        {
            for (int x : { 0, crop.w - 1 })
                ring.push_back(RgbAt(crop, x, y));
        }

        Rgb bg;
        for (int c = 0; c < 3; c++)
        {
            std::vector<double> v;
            for (auto& p : ring)
                v.push_back(p[c]);
            bg[c] = Median(v);
        }

        std::vector<double> ringD;
        for (auto& p : ring)
            ringD.push_back(ColorDist(p, bg));
        std::sort(ringD.begin(), ringD.end());
        double spread = ringD[(size_t)std::floor(ringD.size()*0.9)];
        if (spread > 14)
            return nullptr;

        int n = crop.w*crop.h;
        std::vector<float> dist((size_t)n);
        for (int i = 0; i < n; i++)
            dist[i] = (float)ColorDist(Rgb(crop.data[(size_t)i*4], crop.data[(size_t)i*4 + 1], crop.data[(size_t)i*4 + 2]), bg);

        Box inner(plan.rep->box.x - cb.x, plan.rep->box.y - cb.y, plan.rep->box.w, plan.rep->box.h);
        std::vector<double> innerD;
        for (int y = Math::Max(0, (int)JsRound(inner.y + inner.h*0.25)); y < Math::Min((double)crop.h, inner.y + inner.h*0.75); y++)
        {
            for (int x = Math::Max(0, (int)JsRound(inner.x + inner.w*0.25)); x < Math::Min((double)crop.w, inner.x + inner.w*0.75); x++)
                innerD.push_back(dist[(size_t)y*crop.w + x]);
        }
        if (Median(innerD) < 45)
            return nullptr;

        double tLow = spread + 4, tHigh = Math::Max(tLow + 20, 40.0);
        std::vector<UInt8> bgMask((size_t)n, 0);
        std::vector<int> stack;
        for (int x = 0; x < crop.w; x++)
        {
            stack.push_back(x);
            stack.push_back((crop.h - 1)*crop.w + x);
        }
        for (int y = 0; y < crop.h; y++)
        {
            stack.push_back(y*crop.w);
            stack.push_back(y*crop.w + crop.w - 1);
        }
        while (!stack.empty())
        {
            int p = stack.back();
            stack.pop_back();
            if (bgMask[p] || dist[p] >= tHigh)
                continue;

            bgMask[p] = 1;
            int x = p % crop.w, y = p / crop.w;
            if (x > 0) stack.push_back(p - 1);
            if (x < crop.w - 1) stack.push_back(p + 1);
            if (y > 0) stack.push_back(p - crop.w);
            if (y < crop.h - 1) stack.push_back(p + crop.w);
        }

        std::vector<UInt8> fg((size_t)n);
        for (int i = 0; i < n; i++)
            fg[i] = bgMask[i] ? 0 : 1;

        std::vector<int> labels;
        std::vector<Component> comps;
        Components(fg, crop.w, crop.h, labels, comps);
        Box core = Grow(inner, -inner.w*0.2, -inner.h*0.2);
        std::set<int> keep;
        for (auto& c : comps)
        {
            if (Inter(c.box, core) > 0)
                keep.insert(c.label);
        }

        UiImage out(crop.w, crop.h);
        for (int i = 0; i < n; i++)
        {
            double a;
            if (!bgMask[i])
                a = keep.count(labels[i]) ? 1 : 0;
            else
                a = Math::Max(0.0, Math::Min(1.0, (dist[i] - tLow)/(tHigh - tLow)));

            if (a <= 0.01)
                continue;

            size_t o = (size_t)i*4;
            for (int c = 0; c < 3; c++)
                out.data[o + c] = Clamp8(Math::Max(0.0, Math::Min(255.0, (crop.data[o + c] - (1 - a)*bg[c])/a)));
            out.data[o + 3] = Clamp8(a*255);
        }

        return Finish(plan, cb, out, "local", {}, "keyed out a flat backdrop " + HexOfRounded(bg));
    }

    NineSlice NineOf(const UiImage& img)
    {
        auto diag = [&](int sx, int sy, int dxs, int dys)
        {
            for (int d = 0; d < Math::Min(img.w, img.h)/2.0; d++)
            {
                int x = sx + dxs*d, y = sy + dys*d;
                if (img.At(x, y)[3] > 128)
                    return d;
            }
            return 0;
        };

        int r = Math::Max(Math::Max(diag(0, 0, 1, 1), diag(img.w - 1, 0, -1, 1)), Math::Max(diag(0, img.h - 1, 1, -1), diag(img.w - 1, img.h - 1, -1, -1)));
        int m = (int)JsRound(r*3.4 + 4);
        int mx = Math::Max(1, Math::Min(m, (int)std::floor(img.w*0.4))), my = Math::Max(1, Math::Min(m, (int)std::floor(img.h*0.4)));
        return NineSlice{ mx, my, mx, my };
    }

    static String Fixed(double v, int digits)
    {
        char buffer[64];
        snprintf(buffer, sizeof(buffer), "%.*f", digits, v);
        return buffer;
    }

    static Coroutine<CutResult> AiCut(const Ref<AiCtx>& ai, const UiImage* design, const UiDoc* doc, const LookPlan* plan, Box cb, UiImage crop, int seed)
    {
        CutResult res;
        Vector<SpriteStep> steps;
        steps.Add({ "crop", crop });

        auto aspect = PipelineUpscale::ClosestAspect((float)crop.w, (float)crop.h);
        int colon = aspect.label.Find(":");
        double ratio = colon > 0 ? (double)(int)aspect.label.SubStr(0, colon)/(double)(int)aspect.label.SubStr(colon + 1) : 1.0;
        int fw = crop.w, fh = crop.h;
        if ((double)crop.w/crop.h < ratio)
            fw = (int)JsRound(crop.h*ratio);
        else
            fh = (int)JsRound(crop.w/ratio);

        int offX = (int)std::floor((fw - crop.w)/2.0), offY = (int)std::floor((fh - crop.h)/2.0);
        double scale = Math::Max(1.0, 768.0/Math::Max(fw, fh));
        UiImage framed(fw, fh);
        for (int y = 0; y < fh; y++)
        {
            int sy = Math::Max(0, Math::Min(crop.h - 1, y - offY));
            for (int x = 0; x < fw; x++)
            {
                int sx = Math::Max(0, Math::Min(crop.w - 1, x - offX));
                const UInt8* s = crop.At(sx, sy);
                UInt8* d = framed.At(x, y);
                d[0] = s[0]; d[1] = s[1]; d[2] = s[2]; d[3] = s[3];
            }
        }

        UiImage input = framed.Resize((int)JsRound(fw*scale), (int)JsRound(fh*scale));
        steps.Add({ "sent to the model", input });

        KeyColor key = PickKey(*design, plan->rep->box);
        Vector<String> eraseLabels;
        for (auto& e : plan->erase)
            eraseLabels.Add(e.label);

        String prompt = Prompts::Cut(plan->describe, eraseLabels, key.name, key.hex);
        AiAnswer answer = co_await AiImage(ai, prompt, { input.Png() }, "cut " + plan->key, aspect.label, seed);
        if (!answer.ok)
        {
            res.error = answer.error;
            co_return res;
        }

        UiImage rendered;
        if (!UiImage::Decode(answer.data, rendered))
        {
            res.error = "the model answered with an undecodable image";
            co_return res;
        }
        steps.Add({ "model render", rendered });

        PipelineImageOps::ChromaOptions chroma;
        chroma.color = Color4((int)key.rgb.r, (int)key.rgb.g, (int)key.rgb.b, 255);
        chroma.tolerance = 26;
        chroma.softness = 16;
        chroma.spill = 70;
        UiImage keyedFull = UiImage::FromBitmap(*PipelineImageOps::ChromaKey(*rendered.ToBitmap(), chroma)).Resize(fw, fh);
        UiImage keyed = keyedFull.Crop(offX, offY, crop.w, crop.h);

        Vector<Box> eraseLocal;
        for (auto& e : plan->erase)
            eraseLocal.Add(Box(e.box.x - cb.x, e.box.y - cb.y, e.box.w, e.box.h));

        Box roughLocal(plan->rep->box.x - cb.x, plan->rep->box.y - cb.y, plan->rep->box.w, plan->rep->box.h);
        Registration reg = Register(keyed, crop, roughLocal, eraseLocal);
        UiImage warped = Warp(keyed, reg, crop.w, crop.h);
        Despill(warped, key.rgb);
        steps.Add({ "registered (scale " + Fixed(reg.sx, 3) + "\xC3\x97" + Fixed(reg.sy, 3) + ", shift " + Fixed(reg.tx, 1) + "," + Fixed(reg.ty, 1) + ")", warped });

        // A panel takes the render's colours everywhere: design pixels around render pixels would show the seam
        UiImage out = Composite(crop, warped, eraseLocal, plan->stretch);
        Vector<String> cleaned = CleanLeftovers(out, crop, plan->erase, eraseLocal);
        if (!cleaned.IsEmpty())
        {
            String joined;
            for (int i = 0; i < cleaned.Count(); i++)
                joined += (i ? ", " : "") + cleaned[i];
            steps.Add({ "label leftovers filled locally: " + joined, out });
        }

        String note = plan->erase.IsEmpty() ? String("model matte") : "model matte, " + (String)plan->erase.Count() + " overlay(s) restored";
        res.cut = Finish(*plan, cb, out, "ai", steps, note);
        res.ok = true;
        co_return res;
    }

    Coroutine<CutResult> CutLook(const Ref<AiCtx>& ai, const UiImage* design, const UiDoc* doc, const LookPlan* plan, String mode, int seed)
    {
        Box cb = CropBoxOf(*doc, *plan->rep);
        UiImage crop = design->Crop((int)cb.x, (int)cb.y, (int)cb.w, (int)cb.h);

        CutResult res;
        if (mode == "auto" && plan->erase.IsEmpty())
            res.cut = LocalCut(*doc, *plan, cb, crop);

        if (!res.cut)
        {
            res = co_await AiCut(ai, design, doc, plan, cb, crop, seed);
            if (!res.ok)
                co_return res;
        }

        if (plan->stretch)
            res.cut->nine = NineOf(res.cut->img);

        res.ok = true;
        co_return res;
    }
}
