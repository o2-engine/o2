#include "o2Editor/stdafx.h"
#include "PipelineExtractCheck.h"

#include <algorithm>
#include <cmath>

#include "o2/Utils/FileSystem/FileSystem.h"
#include "o2Editor/Pipeline/Nodes/ImageExtractNode.h"
#include "o2Editor/Pipeline/Nodes/PipelineNodesCommon.h"
#include "o2Editor/Pipeline/PipelinePartFit.h"
#include "o2Editor/Pipeline/PipelinePngMeta.h"
#include "o2Editor/Pipeline/PipelineUtils.h"
#include "o2Editor/Pipeline/UiDoc/UiDocAiShared.h"
#include "o2Editor/Pipeline/UiDoc/UiDocJs.h"
#include "o2Editor/Pipeline/UiLayout/UiLayoutSlice.h"

namespace Editor::PipelineExtractCheck
{
    using UiLayout::JsRound;

    const char* const defaultModel = "gpt-6.1-sol";

    const Vector<int>& Limits()
    {
        static Vector<int> limits = { 1, 2, 3 };
        return limits;
    }

    bool Read(const PipelineNode& node, Config& out)
    {
        if (!PipelineGraph::IsRefCheckOn(node))
            return false;

        out = Config();
        auto max = node.GetConfigValue("refCheckMax");
        float n = max ? PipelineUtils::ValueToNumber(*max, NAN) : NAN;
        int limit = std::isfinite(n) ? (int)JsRound(n) : 0;
        out.limit = Limits().Contains(limit) ? limit : defaultLimit;
        auto model = node.GetConfigValue("refCheckModel");
        out.model = model ? UiDoc::JsTrim(PipelineUtils::ValueToString(*model)) : String();
        if (out.model.IsEmpty())
            out.model = defaultModel;
        return true;
    }

    String ShaKey(const Vector<String>& parts)
    {
        String payload;
        for (auto& part : parts)
            payload += part + "|";

        static const char* hex = "0123456789abcdef";
        String digest = PipelineUtils::Sha256(payload), out;
        for (int i = 0; i < 16; i++)
        {
            unsigned char c = (unsigned char)digest[i];
            out += hex[c >> 4];
            out += hex[c & 15];
        }
        return out;
    }

    Box RegionRect(int iw, int ih, const PipelineExtractRegion& region)
    {
        bool partial = region.x > 0.0f || region.y > 0.0f || region.w < 1.0f || region.h < 1.0f;
        if (!partial)
            return Box(0, 0, iw, ih);

        int left = std::clamp((int)std::lround(region.x*iw), 0, iw - 1);
        int top = std::clamp((int)std::lround(region.y*ih), 0, ih - 1);
        int w = std::max(1, std::min(iw - left, (int)std::lround(region.w*iw)));
        int h = std::max(1, std::min(ih - top, (int)std::lround(region.h*ih)));
        return Box(left, top, w, h);
    }

    Box ContextRect(int iw, int ih, const Box& r)
    {
        double m = std::max(16.0, JsRound(0.25*std::max(r.w, r.h)));
        double x0 = std::max(0.0, r.x - m), y0 = std::max(0.0, r.y - m);
        return Box(x0, y0, std::min((double)iw, r.x + r.w + m) - x0, std::min((double)ih, r.y + r.h + m) - y0);
    }

    SpriteView SpriteViewOf(const String& stored, const PipelineNode& node, const String& portId, int cropW, int cropH)
    {
        SpriteView view;
        view.png = stored;
        std::optional<PipelineExtract::Frame> frame;
        if (PipelineTransparency::UsesChromaPostStep(node, portId))
        {
            // Cut exactly as the executor's post-step cuts it
            auto raw = EnsureRgba(DecodeImageBytes(stored));
            if (raw)
            {
                auto cut = PipelineImageOps::ChromaKey(*raw, PipelineTransparency::Read(node, portId).chroma);
                int left, top, width, height;
                Vec2I size = raw->GetSize();
                if (PipelineImageOps::HasContent(*cut) && PipelineImageOps::ContentBox(*cut, PipelineImageOps::ContentMode::Alpha, left, top, width, height))
                {
                    view.png = EncodeBitmapPng(*PipelineImageOps::CropPixels(*cut, left, top, width, height));
                    frame = PipelineExtract::Frame{ size.x, size.y, left, top };
                }
                else
                {
                    // Nothing left once the key colour is cut: the judge sees that empty sprite, not the green render
                    view.png = EncodeBitmapPng(*cut);
                    frame = PipelineExtract::Frame{ size.x, size.y, 0, 0 };
                }
            }
        }
        else
        {
            PipelineExtract::Frame noted;
            if (PipelineExtract::ReadPngFrame(stored, noted))
                frame = noted;
        }

        UiImage::Decode(view.png, view.sprite);
        double w = std::max(1, view.sprite.w), h = std::max(1, view.sprite.h);

        // The crop is fitted inside the model's frame and centred there
        double s = frame ? PipelineRegions::PartScale(cropW, cropH, frame->w, frame->h) : 0.0;
        if (s <= 0.0)
            s = PipelinePngMeta::ReadScale(stored).value_or(1.0);
        if (s <= 0.0)
            s = 1.0;

        if (frame)
        {
            double ox = (frame->w - cropW*s)/2, oy = (frame->h - cropH*s)/2;
            view.nominal = Box((frame->left - ox)/s, (frame->top - oy)/s, w/s, h/s);
            view.from = "tracked";
            return view;
        }

        double nw = w/s, nh = h/s;
        view.nominal = Box((cropW - nw)/2, (cropH - nh)/2, nw, nh);
        view.from = "centred";
        return view;
    }

    // The registration works on a small copy: its sweep grows with the square of the size
    static constexpr int registerSide = 256;

    // Where the sprite, kept at its size, fits the crop best: the shift from at (both in the small copy's pixels). The score is
    // the mean colour distance under the sprite's opaque pixels, a pixel off the crop counting as a miss; a shift is taken only
    // when it is clearly better than staying, the nearest one among equals
    static void CoarseShift(const UiImage& drawn, int atX, int atY, const UiImage& crop, int& outDx, int& outDy)
    {
        outDx = outDy = 0;
        int opaque = 0;
        for (int i = 0; i < drawn.w*drawn.h; i++)
            opaque += drawn.data[(size_t)i*4 + 3] >= 128 ? 1 : 0;

        int step = std::max(1, (int)JsRound(std::sqrt(opaque/1200.0)));
        std::vector<int> pts;
        for (int y = 0; y < drawn.h; y += step)
        {
            for (int x = 0; x < drawn.w; x += step)
            {
                if (drawn.data[(size_t)(y*drawn.w + x)*4 + 3] >= 128)
                {
                    pts.push_back(x);
                    pts.push_back(y);
                }
            }
        }
        if (pts.size() < 40)
            return;

        auto score = [&](int dx, int dy)
        {
            double sum = 0;
            for (size_t i = 0; i < pts.size(); i += 2)
            {
                int cx = atX + dx + pts[i], cy = atY + dy + pts[i + 1];
                if (cx < 0 || cy < 0 || cx >= crop.w || cy >= crop.h)
                {
                    sum += 160;
                    continue;
                }
                const UInt8* a = drawn.At(pts[i], pts[i + 1]);
                const UInt8* b = crop.At(cx, cy);
                sum += std::abs(a[0] - b[0]) + std::abs(a[1] - b[1]) + std::abs(a[2] - b[2]);
            }
            return sum/(pts.size()/2.0)/3.0;
        };

        double stay = score(0, 0);
        int rx = (int)JsRound(0.3*crop.w), ry = (int)JsRound(0.3*crop.h);
        double far = std::hypot(rx, ry);
        if (far <= 0)
            far = 1;

        int bestX = 0, bestY = 0;
        double bs = stay;
        for (int dy = -ry; dy <= ry; dy++)
        {
            for (int dx = -rx; dx <= rx; dx++)
            {
                // A little for the distance: among equal fits (a flat surface) the nearest one
                double v = score(dx, dy) + 2*(std::hypot(dx, dy)/far);
                if (v < bs)
                {
                    bs = v;
                    bestX = dx;
                    bestY = dy;
                }
            }
        }

        if (bs < stay - 3 && bs < 0.85*stay)
        {
            outDx = bestX;
            outDy = bestY;
        }
    }

    static double Round2(double v) { return JsRound(v*100)/100; }
    static double Round3(double v) { return JsRound(v*1000)/1000; }

    static Placement PlaceFine(const UiImage& sprite, const Box& nominal, const UiImage& crop)
    {
        Placement res;
        res.box = nominal;
        double k = std::min(1.0, (double)registerSide/std::max(crop.w, crop.h));
        UiImage small = k < 1 ? crop.Resize((int)JsRound(crop.w*k), (int)JsRound(crop.h*k)) : crop;
        double kx = (double)small.w/crop.w, ky = (double)small.h/crop.h;
        UiImage drawn = sprite.Resize(std::max(1, (int)JsRound(nominal.w*kx)), std::max(1, (int)JsRound(nominal.h*ky)));
        int atX = (int)JsRound(nominal.x*kx), atY = (int)JsRound(nominal.y*ky);
        int dx, dy;
        CoarseShift(drawn, atX, atY, small, dx, dy);
        atX += dx;
        atY += dy;

        UiImage render = UiImage::Blank(small.w, small.h);
        render.Blit(drawn, atX, atY);
        auto reg = UiLayout::Register(render, small, Box(atX, atY, drawn.w, drawn.h), {});
        bool still = reg.sx == 1 && reg.sy == 1 && reg.tx == 0 && reg.ty == 0;
        if (still && !dx && !dy)
            return res;

        res.box = still ? Box(nominal.x + dx/kx, nominal.y + dy/ky, nominal.w, nominal.h) :
            Box((atX*reg.sx + reg.tx)/kx, (atY*reg.sy + reg.ty)/ky, (drawn.w*reg.sx)/kx, (drawn.h*reg.sy)/ky);
        res.fit = true;
        res.scale = Round3((reg.sx + reg.sy)/2);
        res.dx = Round2(res.box.x + res.box.w/2 - (nominal.x + nominal.w/2));
        res.dy = Round2(res.box.y + res.box.h/2 - (nominal.y + nominal.h/2));
        return res;
    }

    Placement PlaceOnCrop(const UiImage& sprite, const Box& claimed, const UiImage& crop)
    {
        Placement res;
        res.box = claimed;
        if (crop.w < 12 || crop.h < 12 || claimed.w < 4 || claimed.h < 4 || sprite.IsEmpty())
            return res;

        // A part the model drew at clearly another size than the crop shows it starts from where it really lies: the steps
        // below only look a little around
        Box nominal = claimed;
        auto measured = PipelinePartFit::FitPartOnCrop(crop, sprite, sprite.w/claimed.w);
        if (measured)
            nominal = Box(measured->x, measured->y, measured->w, measured->h);

        Placement whole = PlaceFine(sprite, nominal, crop);
        if (!measured)
            return whole;

        whole.fit = true;
        whole.scale = Round3(whole.box.w/claimed.w);
        whole.dx = Round2(whole.box.x + whole.box.w/2 - (claimed.x + claimed.w/2));
        whole.dy = Round2(whole.box.y + whole.box.h/2 - (claimed.y + claimed.h/2));
        return whole;
    }

    Overlay OverlayOn(const UiImage& reference, const UiImage& sprite, const Box& at)
    {
        Overlay res;
        int w = std::max(1, (int)JsRound(at.w)), h = std::max(1, (int)JsRound(at.h));
        int x = (int)JsRound(at.x), y = (int)JsRound(at.y);
        UiImage laid = sprite.Resize(w, h);
        res.image = reference;
        res.image.Blit(laid, x, y);

        int n = 0, changed = 0;
        double sum = 0;
        for (int yy = 0; yy < laid.h; yy++)
        {
            int ry = y + yy;
            if (ry < 0 || ry >= reference.h)
                continue;
            for (int xx = 0; xx < laid.w; xx++)
            {
                int rx = x + xx;
                if (rx < 0 || rx >= reference.w || laid.At(xx, yy)[3] < 128)
                    continue;
                double d = UiLayout::ColorDist(UiLayout::RgbAt(reference, rx, ry), UiLayout::RgbAt(res.image, rx, ry))/441.673;
                sum += d;
                n++;
                if (d > 0.1)
                    changed++;
            }
        }

        auto r4 = [](double v) { return JsRound(v*10000)/10000; };
        res.pixelDiff = n ? r4(sum/n) : 0;
        res.pixelChanged = n ? r4((double)changed/n) : 0;
        return res;
    }

    double JudgeScale(int w, int h)
    {
        double side = std::max(1, std::max(w, h));
        return side < 384 ? 384/side : side > 1024 ? 1024/side : 1;
    }

    String ScaledPng(const UiImage& img, double k)
    {
        return (k == 1 ? img : img.Resize((int)JsRound(img.w*k), (int)JsRound(img.h*k))).Png();
    }

    String SpriteOnChecker(const UiImage& sprite)
    {
        double side = std::max(1, std::max(sprite.w, sprite.h));
        double k = std::max(256.0, std::min(768.0, side))/side;
        UiImage s = k == 1 ? sprite : sprite.Resize((int)JsRound(sprite.w*k), (int)JsRound(sprite.h*k));
        int pad = std::max(12, (int)JsRound(0.08*std::max(s.w, s.h)));
        UiImage out = UiImage::Blank(s.w + 2*pad, s.h + 2*pad, 140, 140, 140, 255);
        const int cell = 16;
        for (int y = 0; y < out.h; y++)
        {
            for (int x = 0; x < out.w; x++)
            {
                if ((x/cell)%2 == (y/cell)%2)
                    continue;
                UInt8* p = out.At(x, y);
                p[0] = p[1] = p[2] = 166;
            }
        }
        out.Blit(s, pad, pad);
        return out.Png();
    }

    String JudgePrompt(const String& part)
    {
        Vector<String> lines = {
            "You are checking a sprite that an image model cut out of a 2D game picture. The sprite must be the SAME element as in the original \xE2\x80\x94 the original's own artwork, not a redraw of it.",
            "",
            "What was asked to be cut out: \"" + part + "\"",
            "",
            "Three images follow:",
            "1. REFERENCE \xE2\x80\x94 the original picture around the element: the element's region is the middle of the image, with some of the picture around it for context.",
            "2. OVERLAY \xE2\x80\x94 the same view with the cut-out sprite laid over the place it was cut from. Where the sprite matches the original, images 1 and 2 look the same there; a doubled or shifted outline, a different size, changed colours or shading, missing pieces (the original showing through where the element should be covered) and extra pieces show as differences between images 1 and 2.",
            "3. SPRITE \xE2\x80\x94 the cut-out sprite alone on a grey checkerboard (the checkerboard is transparency).",
            "",
            "Compare the element in the reference with the sprite: silhouette and shape, proportions, colours and shading, inner details; nothing of the element missing or cut off; nothing extra kept (pieces of the background or of neighbouring objects, a leftover backdrop colour, a halo or fringe); clean edges.",
            "These are NOT differences: anything the description asks to leave out (a label, text, numbers, an icon or another object drawn on the element) being removed with the element's own surface continued there; the background and the neighbouring objects of the reference being absent; a part that was hidden in the reference being plausibly completed; a slight overall change of sharpness.",
            "",
            "Answer with ONLY a JSON object, no commentary, no code fence:",
            "{\"verdict\": \"ok\" | \"minor\" | \"significant\", \"likeness\": <number 0..1>, \"differences\": [\"<one concrete difference>\"], \"instructions\": \"<what the image model must do differently when it cuts this element again>\"}",
            "- \"ok\": the same element, nothing that matters differs. \"minor\": small differences a player would not notice (a slight colour shift, a slightly soft edge). \"significant\": a wrong or changed shape, wrong proportions, changed colours or details, a part missing or cut off, something extra kept, dirty edges or leftover backdrop \xE2\x80\x94 a result that should be redone.",
            "- \"likeness\": 1 = the identical element, 0 = a different thing.",
            "- \"differences\": at most 6, each short and specific \xE2\x80\x94 what differs and where. Empty when there are none.",
            "- \"instructions\": one to three imperative sentences addressed to the image model, naming what to keep, restore or remove. Empty when the verdict is \"ok\".",
        };
        return UiDoc::Join(lines, "\n");
    }

    // String(v ?? '') with every run of white space as one space, trimmed and cut to max UTF-16 units
    static String Clean(const Json* v, int max)
    {
        String text = !v || v->IsNull() ? String() : v->IsString() ? v->Str() : v->ToJsString();
        return UiDoc::JsSlice(UiDoc::JsTrim(UiDoc::ReplaceSpaceRuns(text, " ")), 0, max);
    }

    bool ParseJudge(const String& text, JudgeAnswer& out, String* error)
    {
        auto fail = [&]() { if (error) *error = "The judge did not answer with a verdict"; return false; };

        String raw = UiDoc::JsTrim(text);
        int fence = raw.Find("```");
        if (fence >= 0)
        {
            int body = fence + 3;
            if (UiDoc::JsToLower(raw.SubStr(body, body + 4)) == "json")
                body += 4;
            int close = raw.Find("```", body);
            if (close >= 0)
                raw = UiDoc::JsTrim(raw.SubStr(body, close));
        }

        int start = raw.Find("{"), end = raw.FindLast("}");
        Json o;
        if (start < 0 || end <= start || !Json::Parse(raw.SubStr(start, end + 1), o) || !o.IsObject())
            return fail();

        double likeness = o.Get("likeness") ? o.Get("likeness")->ToJsNumber() : NAN;
        if (std::isfinite(likeness) && likeness > 1 && likeness <= 100)
            likeness /= 100;

        auto verdictValue = o.Get("verdict");
        String word = UiDoc::JsToLower(UiDoc::JsTrim(!verdictValue || verdictValue->IsNull() ? String() :
                                                    verdictValue->IsString() ? verdictValue->Str() : verdictValue->ToJsString()));
        auto starts = [&](std::initializer_list<const char*> prefixes)
        {
            for (auto p : prefixes)
            {
                if (word.StartsWith(p))
                    return true;
            }
            return false;
        };
        String verdict = starts({ "ok", "pass", "same", "good" }) ? "ok" : starts({ "minor", "small" }) ? "minor" :
            starts({ "significant", "major", "fail", "bad" }) ? "significant" : "";
        if (verdict.IsEmpty() && !std::isfinite(likeness))
            return fail();
        if (verdict.IsEmpty())
            verdict = likeness >= 0.9 ? "ok" : likeness >= 0.7 ? "minor" : "significant";
        if (!std::isfinite(likeness))
            likeness = verdict == "ok" ? 0.95 : verdict == "minor" ? 0.8 : 0.4;

        out = JudgeAnswer();
        out.verdict = verdict;
        out.likeness = std::max(0.0, std::min(1.0, JsRound(likeness*100)/100));
        if (auto differences = o.Get("differences"); differences && differences->IsArray())
        {
            for (auto& d : differences->Items())
            {
                String line = Clean(&d, 240);
                if (!line.IsEmpty() && out.differences.Count() < 6)
                    out.differences.Add(line);
            }
        }
        out.instructions = Clean(o.Get("instructions"), 600);
        return true;
    }

    // The text without the dots, semicolons and spaces it ends with
    static String TrimEndPunct(const String& text)
    {
        int end = text.Length();
        while (end > 0 && (text[end - 1] == '.' || text[end - 1] == ';' || text[end - 1] == ' ' || text[end - 1] == '\n' ||
                           text[end - 1] == '\t' || text[end - 1] == '\r'))
            end--;
        return text.SubStr(0, end);
    }

    String ClarificationsOf(const Vector<Judged>& attempts)
    {
        Vector<const Judged*> rejected;
        for (auto& a : attempts)
        {
            if (a.verdict == "significant")
                rejected.Add(&a);
        }
        if (rejected.IsEmpty())
            return "";

        Vector<String> asks;
        for (int i = rejected.Count() - 1; i >= 0; i--)
        {
            if (!rejected[i]->instructions.IsEmpty() && !asks.Contains(rejected[i]->instructions))
                asks.Add(rejected[i]->instructions);
        }

        Vector<String> lines = { "IMPORTANT \xE2\x80\x94 an earlier attempt at this extraction was compared with the original image and rejected. Cut the element again from the original and fix this:" };
        for (auto& ask : asks)
            lines.Add("- " + ask);

        auto& last = *rejected.Last();
        if (!last.differences.IsEmpty())
        {
            Vector<String> items;
            for (auto& d : last.differences)
                items.Add(TrimEndPunct(d));
            lines.Add("What was wrong in the rejected attempt: " + UiDoc::Join(items, "; ") + ".");
        }
        return UiDoc::JsSlice(UiDoc::Join(lines, "\n"), 0, 1400);
    }

    int BestAttempt(const Vector<Judged>& attempts)
    {
        const Judged* best = nullptr;
        auto rank = [](const Judged& a) { return a.verdict == "significant" ? 0 : 1; };
        for (auto& a : attempts)
        {
            if (a.verdict.IsEmpty())
                continue;
            if (!best)
            {
                best = &a;
                continue;
            }

            double la = a.likeness.value_or(0), lb = best->likeness.value_or(0);
            if (rank(a) > rank(*best) || (rank(a) == rank(*best) && (la > lb || (la == lb && a.n > best->n))))
                best = &a;
        }
        if (best)
            return best->n;
        return attempts.IsEmpty() ? 1 : attempts[0].n;
    }

    String PartFile(const String& portId)
    {
        String stem;
        for (int i = 0; i < portId.Length() && stem.Length() < 80; i++)
        {
            char c = portId[i];
            bool keep = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_' || c == '-';
            stem += keep ? c : '_';
        }
        return stem.IsEmpty() ? String("part") : stem;
    }

    String CheckFolder(const String& workDir)
    {
        if (workDir.IsEmpty())
            return "";
        return workDir.EndsWith("/") && workDir.Length() > 0 ? workDir + "check/" : workDir + "/check/";
    }

    void WriteDebug(const String& folder, const String& stem, const String& tag, const Json& debug, bool running)
    {
        if (folder.IsEmpty())
            return;

        Json body = debug;
        if (running)
            body["running"] = true;
        String text = body.Dump();
        if (!running)
            PipelineUtils::WriteFileBytes(folder + tag + ".json", text);
        PipelineUtils::WriteFileBytes(folder + stem + ".json", text);
    }

    // True when the name is "<8 hex digits>" followed by "-<anything>" or nothing, then ".png" or ".json"; key gets the digits
    static bool KeyedName(const String& rest, String& key)
    {
        if (rest.Length() < 9)
            return false;
        for (int i = 0; i < 8; i++)
        {
            char c = rest[i];
            if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f')))
                return false;
        }
        String tail = rest.SubStr(8);
        if (!(tail == ".json" || tail == ".png" || (tail.StartsWith("-") && (UiLayout::HasSuffix(tail, ".png") || UiLayout::HasSuffix(tail, ".json")))))
            return false;
        key = rest.SubStr(0, 8);
        return true;
    }

    void Prune(const String& folder, const String& stem, const Vector<String>& liveStems)
    {
        if (folder.IsEmpty() || !o2FileSystem.IsFolderExist(folder))
            return;

        Vector<String> names;
        for (auto& file : o2FileSystem.GetFolderInfo(folder).files)
            names.Add(o2FileSystem.GetPathWithoutDirectories(file.path));

        // The details of the latest three checks of this stem stay, by when they were made
        Vector<Pair<String, double>> checks;
        for (auto& name : names)
        {
            String key;
            if (name.StartsWith(stem + "-") && KeyedName(name.SubStr(stem.Length() + 1), key) && name == stem + "-" + key + ".json")
            {
                Json debug;
                Json::Parse(PipelineUtils::ReadFileBytes(folder + name), debug);
                auto at = debug.Get("at");
                checks.Add({ key, at && at->IsString() ? UiDoc::ParseIsoTime(at->Str()) : 0.0 });
            }
        }
        UiLayout::StableSortBy(checks, [](const Pair<String, double>& a, const Pair<String, double>& b) { return a.second > b.second; });
        Vector<String> old;
        for (int i = 3; i < checks.Count(); i++)
            old.Add(checks[i].first);

        for (auto& name : names)
        {
            String key;
            bool mine = name.StartsWith(stem + "-") && KeyedName(name.SubStr(stem.Length() + 1), key);
            if (mine && old.Contains(key))
            {
                o2FileSystem.FileDelete(folder + name);
                continue;
            }

            if (liveStems.IsEmpty())
                continue;

            bool live = liveStems.Any([&](const String& s)
            {
                String k;
                return name == s + ".json" || (name.StartsWith(s + "-") && KeyedName(name.SubStr(s.Length() + 1), k));
            });
            if (!live)
                o2FileSystem.FileDelete(folder + name);
        }
    }

    void Replay(const PipelineExecContext& ctx, const PipelineNode& node, const String& key)
    {
        String folder = CheckFolder(ctx.workDir);
        if (folder.IsEmpty())
            return;

        String stem = PartFile(PipelineRegions::RegionOfPort(node, ctx.outputPortId).id);
        String current = folder + stem + ".json";
        Json now;
        if (o2FileSystem.IsFileExist(current) && Json::Parse(PipelineUtils::ReadFileBytes(current), now) && now.Get("key") &&
            now.Get("key")->Str() == key)
            return;

        String kept = folder + stem + "-" + key.SubStr(0, 8) + ".json";
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
}
