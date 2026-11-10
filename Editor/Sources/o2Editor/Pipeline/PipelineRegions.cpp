#include "o2Editor/stdafx.h"
#include "PipelineRegions.h"

#include "o2Editor/Pipeline/Nodes/PipelineNodesCommon.h"
#include "o2Editor/Pipeline/PipelineImageOps.h"
#include "o2Editor/Pipeline/PipelinePngMeta.h"
#include "o2Editor/Pipeline/PipelineUtils.h"
#include "o2Editor/Pipeline/UiDoc/UiDocDevice.h"
#include "o2Editor/Pipeline/UiDoc/UiDocJs.h"

namespace Editor
{
    namespace PipelineRegions
    {
        void NormalizeBox(float& x, float& y, float& w, float& h)
        {
            x = Math::Clamp(x, 0.0f, 1.0f);
            y = Math::Clamp(y, 0.0f, 1.0f);
            if (!(w > 0.0f)) w = 1.0f;
            if (!(h > 0.0f)) h = 1.0f;
            w = Math::Clamp(w, 0.001f, 1.0f - x);
            h = Math::Clamp(h, 0.001f, 1.0f - y);
        }

        static const Vector<String>& OwnKeys()
        {
            static Vector<String> keys = { "id", "name", "x", "y", "w", "h" };
            return keys;
        }

        static String RegionId(const DataValue& value, int index)
        {
            String id;
            if (auto member = value.FindMember("id"))
                id = String(member->IsString() ? member->GetString() : "");

            return id.IsEmpty() ? "r" + (String)index : id;
        }

        // Returns the stored object of the region with the id, null when there is none
        static const DataValue* FindElement(const PipelineNode& node, const String& id)
        {
            auto raw = node.GetConfigValue("regions");
            if (id.IsEmpty() || !raw || !raw->IsArray())
                return nullptr;

            for (int i = 0; i < raw->GetElementsCount(); i++)
            {
                auto& element = raw->GetElement(i);
                if (element.IsObject() && RegionId(element, i) == id)
                    return &element;
            }

            return nullptr;
        }

        static PipelineExtractRegion ReadOne(const DataValue& value, int index)
        {
            PipelineExtractRegion region;
            region.id = RegionId(value, index);
            auto own = value.FindMember("transparency");
            region.ownTransparency = own && own->IsObject();
            if (auto model = value.FindMember("model"); model && model->IsString())
                region.model = String(model->GetString()).Trimed(" \n\r\t");

            if (auto name = value.FindMember("name"))
                region.name = name->IsString() ? String(name->GetString()) : String();

            auto number = [&](const char* key, float def)
            {
                auto member = value.FindMember(key);
                return member ? PipelineUtils::ValueToNumber(*member, def) : def;
            };
            region.x = number("x", 0.0f);
            region.y = number("y", 0.0f);
            region.w = number("w", 1.0f);
            region.h = number("h", 1.0f);
            NormalizeBox(region.x, region.y, region.w, region.h);
            return region;
        }

        Vector<PipelineExtractRegion> Read(const PipelineNode& node)
        {
            Vector<PipelineExtractRegion> regions;
            if (auto raw = node.GetConfigValue("regions"))
            {
                if (raw->IsArray())
                {
                    for (int i = 0; i < raw->GetElementsCount(); i++)
                    {
                        auto& element = raw->GetElement(i);
                        if (element.IsObject())
                            regions.Add(ReadOne(element, i));
                    }
                }
            }

            if (!regions.IsEmpty())
                return regions;

            // A node from before regions existed: its prompt and region of interest are the one part
            PipelineExtractRegion single;
            single.id = node.outputs.IsEmpty() ? String("out") : node.outputs[0].id;
            single.name = node.GetConfigString("prompt", "");
            PipelineImageOps::CropRect roi;
            if (ReadNodeCrop(node, "roi", roi))
            {
                single.x = roi.x;
                single.y = roi.y;
                single.w = roi.w;
                single.h = roi.h;
                NormalizeBox(single.x, single.y, single.w, single.h);
            }

            regions.Add(single);
            return regions;
        }

        void Write(PipelineNode& node, const Vector<PipelineExtractRegion>& regions)
        {
            if (!node.config.IsObject())
                node.config.SetObject();

            DataDocument arr;
            arr.SetArray();
            for (auto& region : regions)
            {
                auto& item = arr.AddElement();
                item.SetObject();
                item["id"] = region.id;
                item["name"] = region.name;
                item["x"] = region.x;
                item["y"] = region.y;
                item["w"] = region.w;
                item["h"] = region.h;
                if (auto previous = FindElement(node, region.id))
                {
                    for (auto it = previous->BeginMember(); it != previous->EndMember(); ++it)
                    {
                        if (!OwnKeys().Contains(String(it->name.GetString())))
                            item.AddMember(it->name.GetString()) = it->value;
                    }
                }
            }

            node.RemoveConfig("regions");
            node.config["regions"] = static_cast<const DataValue&>(arr);
        }

        Vector<String> PortNames(const Vector<PipelineExtractRegion>& regions)
        {
            Vector<String> names;
            for (int i = 0; i < regions.Count(); i++)
            {
                String base = PipelineUtils::CutUtf8(regions[i].name.Trimed(" \n\r\t"), 40);

                if (base.IsEmpty())
                    base = "part " + (String)(i + 1);

                String name = base;
                for (int k = 2; names.Contains(name); k++)
                    name = base + " " + (String)k;

                names.Add(name);
            }
            return names;
        }

        void SyncPorts(PipelineNode& node)
        {
            // Without a region list AssetsLine looks the output up by the schema name "out": the first port stays, with its links
            bool hasList = false;
            if (auto list = node.GetConfigValue("regions"); list && list->IsArray())
            {
                for (auto& item : *list)
                    hasList = hasList || item.IsObject();
            }

            if (!hasList)
            {
                PipelinePort port = node.outputs.IsEmpty() ? PipelinePort(PipelineNode::GenerateId(), "", PipelinePortType::Image, false)
                                                           : node.outputs[0];
                port.name = "out";
                port.portType = PipelinePortType::Image;
                node.outputs = { port };
                return;
            }

            auto regions = Read(node);
            auto names = PortNames(regions);

            Vector<PipelinePort> ports;
            for (int i = 0; i < regions.Count(); i++)
                ports.Add(PipelinePort(regions[i].id, names[i], PipelinePortType::Image, false));

            node.outputs = ports;
        }

        const char* autoSplitDefaultModel = "gpt-6.1-sol";
        const char* extractDefaultImageModel = "openai/gpt-5-image-mini";

        bool IsBackgroundPart(const String& name, bool transparent)
        {
            if (transparent)
                return false;

            String head = name.Trimed(" \n\r\t").ToLowerCase();
            for (const char* word : { "bg", "background" })
            {
                String w = word;
                if (head.StartsWith(w) && (head.Length() == w.Length() || head[w.Length()] == '_' ||
                                           !(isalnum((unsigned char)head[w.Length()]) || head[w.Length()] == '_')))
                {
                    return true;
                }
            }

            return false;
        }

        String AutoSplitPrompt(const SplitFrame& frame)
        {
            bool pixels = frame.boxes == "pixels";
            String box = pixels ?
                "\"box\": [x0, y0, x1, y1] \xE2\x80\x94 integers in pixels of this " + (String)frame.w + " x " + (String)frame.h +
                    " image, x from its left edge, y from its top edge" :
                String("\"box_2d\": [ymin, xmin, ymax, xmax] \xE2\x80\x94 integers 0..1000 of the image height and width, in that order");
            String boxShape = pixels ? String("\"box\": [x0, y0, x1, y1]") : String("\"box_2d\": [ymin, xmin, ymax, xmax]");

            Vector<String> lines;
            lines.Add("You are cutting this 2D game image into the pieces of a UI KIT: separate sprites from which a game engine can rebuild the same picture. The image is a game screen or UI mockup, a piece of game art, or a SHEET of icons, sprites or items.");
            lines.Add("");
            lines.Add("How to split:");
            lines.Add("- Think back to front, layer by layer. Every piece is ONE layer of artwork, cut bare: whatever is drawn on top of it is either its own piece or text the engine renders.");
            lines.Add("- A panel, window, card, plate, frame, ribbon, slot, tile, tab or button is cut as its bare surface \xE2""\x80""\x94"" WITHOUT its label, numbers, icons, items, bars or other pieces on it. Its box is the whole surface, including the part hidden under its content.");
            lines.Add("- An icon, item, picture, character or decoration that sits on a plate, button, slot or panel is its own piece, separate from what it sits on.");
            lines.Add("- A progress, health or xp bar is two pieces: the empty track (without the fill and the text) and the fill alone.");
            lines.Add("- A screen with a drawn scene or texture behind the UI gets ONE background piece: its box covers the whole background (usually the whole image), described as the scene alone with every UI element removed. A flat single colour behind everything is not a piece.");
            lines.Add("- Text: plain labels, numbers, counters and button captions are NOT pieces \xE2""\x80""\x94"" the engine renders them; never make a piece whose content is only text. A title or logo drawn as decorated artwork (3D or ornate letters, a game logo) IS a piece, cut whole.");
            lines.Add("- Repeats: every distinct artwork ONCE. When several elements use the same artwork at the same size and differ only in the text or icon on top (identical tab buttons, a row of identical slots, the same plate behind two counters), list one piece on the instance that is least covered, named for the shared look. A different colour, shape or size is a different piece.");
            lines.Add("- A SHEET of icons, sprites or items: every item is its own piece, all of them \xE2""\x80""\x94"" similar but different drawings are different pieces. Never merge items, never list a row or the sheet itself.");
            lines.Add("- Keep a piece together with its own outline, shadow and glow. One drawing that belongs together (an icon with its rim, a character) stays whole \xE2""\x80""\x94"" do not cut it into sub-parts.");
            lines.Add("- Never list the whole image as one piece, except the background.");
            lines.Add("");
            lines.Add("For every piece:");
            lines.Add("- \"name\": its kit name in snake_case, starting with its kind, then its look: bg_ (background), panel_ (window, card, popup body, plate), frame_ (border around content), header_ (ribbon, banner, title plate), btn_ (button surface), tab_, slot_ (cell or tile), bar_<what> (bar track) and bar_<what>_fill, toggle_, slider_, handle_ (slider knob), input_ (text field), badge_ (counter bubble), divider_, icon_ (symbol: coin, gem, heart, star, gear, close X), image_ (character, item art, illustration, decoration, logo). Examples: btn_green, panel_purple, icon_coin, bar_xp_fill, image_logo.");
            lines.Add("- \"cut\": one or two sentences telling the person who cuts it out exactly what to keep and what to leave out: the piece (shape, colour, material) and, when anything sits on it, each thing to remove. Examples: \"the rounded glossy green button, without its PLAY label \xE2""\x80""\x94"" the surface left blank\"; \"the purple rounded panel with its darker top strip, without the level text, the bar, the buttons and the stars on it\"; \"the gold coin icon alone, without the dark plate behind it\"; \"the empty dark track of the xp bar, without the pink fill and the 650 / 1000 text\".");
            lines.Add(String("- ") + box + ", tight around the piece's own artwork including its outline and shadow.");
            lines.Add("");
            lines.Add("Answer with ONLY a JSON array, no commentary, no code fence:");
            lines.Add(String("[{\"name\": \"btn_green\", \"cut\": \"<what to keep and what to leave out>\", ") + boxShape + "}]");
            lines.Add("Up to 64 pieces: the background first, then in reading order, left to right, top to bottom.");

            String res;
            for (int i = 0; i < lines.Count(); i++)
                res += (i > 0 ? "\n" : "") + lines[i];

            return res;
        }

        // Collapses whitespace runs into one space and trims, as JS replace(/\s+/g, ' ').trim()
        static String Squash(const String& text)
        {
            String res;
            bool space = false;
            for (auto c : text.Trimed(" \n\r\t\f\v"))
            {
                if (c == ' ' || c == '\n' || c == '\r' || c == '\t' || c == '\f' || c == '\v')
                    space = true;
                else
                {
                    if (space && !res.IsEmpty())
                        res += ' ';
                    space = false;
                    res += c;
                }
            }
            return res;
        }

        Vector<PipelineExtractRegion> ParseAutoSplit(const String& answer, const SplitFrame* frame /*= nullptr*/)
        {
            Vector<PipelineExtractRegion> found;

            String raw = answer.Trimed(" \n\r\t");
            int fence = raw.Find("```");
            if (fence >= 0)
            {
                int bodyStart = raw.Find("\n", fence);
                int close = bodyStart >= 0 ? raw.Find("```", bodyStart) : -1;
                if (bodyStart >= 0 && close > bodyStart)
                    raw = raw.SubStr(bodyStart + 1, close).Trimed(" \n\r\t");
            }

            UiLayout::Json parsed;
            if (!UiLayout::Json::Parse(raw, parsed))
            {
                int start = raw.Find("[");
                int end = raw.FindLast("]");
                if (start < 0 || end <= start || !UiLayout::Json::Parse(raw.SubStr(start, end + 1), parsed))
                    return found;
            }

            const UiLayout::Json* items = parsed.IsArray() ? &parsed : parsed.Get("regions");
            if (!items || !items->IsArray())
                return found;

            bool pixelFrame = frame && frame->boxes == "pixels" && frame->w > 0 && frame->h > 0;
            for (auto& it : items->Items())
            {
                if (!it.IsObject())
                    continue;

                auto text = [&](const char* a, const char* b)
                {
                    auto v = it.Get(a);
                    if (!v || v->IsNull()) v = it.Get(b);
                    return v && !v->IsNull() ? v->ToJsString() : String();
                };
                String label = UiDoc::JsSlice(Squash(text("name", "label")), 0, 48);
                String cut = UiDoc::JsSlice(Squash(text("cut", "description")), 0, 240);
                String name = !cut.IsEmpty() && cut.ToLowerCase() != label.ToLowerCase() ? label + " - " + cut : label;

                double x0, y0, x1, y1;
                String unit = "grid";
                auto four = [&](const UiLayout::Json* v, double* out)
                {
                    if (!v || !v->IsArray() || v->Count() != 4)
                        return false;
                    for (int k = 0; k < 4; k++)
                        out[k] = v->At(k)->ToJsNumber();
                    return true;
                };
                double v[4];
                if (four(it.Get("box_2d"), v)) { y0 = v[0]; x0 = v[1]; y1 = v[2]; x1 = v[3]; }
                else if (four(it.Get("box"), v)) { x0 = v[0]; y0 = v[1]; x1 = v[2]; y1 = v[3]; unit = pixelFrame ? "pixels" : "guess"; }
                else if (it.Get("x") && it.Get("x")->IsNumber() && it.Get("y") && it.Get("y")->IsNumber() &&
                         it.Get("w") && it.Get("w")->IsNumber() && it.Get("h") && it.Get("h")->IsNumber())
                {
                    x0 = it.Get("x")->Num(); y0 = it.Get("y")->Num();
                    x1 = x0 + it.Get("w")->Num(); y1 = y0 + it.Get("h")->Num();
                    unit = pixelFrame ? "pixels" : "guess";
                }
                else
                    continue;

                if (!std::isfinite(x0) || !std::isfinite(y0) || !std::isfinite(x1) || !std::isfinite(y1))
                    continue;

                if (unit == "pixels")
                {
                    x0 /= frame->w; x1 /= frame->w; y0 /= frame->h; y1 /= frame->h;
                }
                else
                {
                    // The grid is 0..1000; a guessed box may be a fraction, percent or the grid
                    double m = Math::Max(x1, y1);
                    double scale = unit == "grid" ? (m > 1.5 ? 0.001 : 1.0) : (m > 1.5 ? (m > 100.0 ? 0.001 : 0.01) : 1.0);
                    x0 *= scale; y0 *= scale; x1 *= scale; y1 *= scale;
                }

                auto clamp = [](double c) { return Math::Clamp(c, 0.0, 1.0); };
                double rw = Math::Abs(x1 - x0), rh = Math::Abs(y1 - y0);
                if (rw < 0.005 || rh < 0.005 || label.IsEmpty())
                    continue;

                PipelineExtractRegion region;
                region.id = PipelineNode::GenerateId();
                region.name = name;

                String lower = label.ToLowerCase();
                bool background = IsBackgroundPart(lower, false);
                if (background)
                {
                    // The plate behind the screen: nearly the whole picture is the whole picture
                    bool full = rw > 0.9 && rh > 0.9;
                    region.background = true;
                    region.x = full ? 0.0f : (float)clamp(Math::Min(x0, x1));
                    region.y = full ? 0.0f : (float)clamp(Math::Min(y0, y1));
                    region.w = full ? 1.0f : (float)Math::Min(rw, 1.0);
                    region.h = full ? 1.0f : (float)Math::Min(rh, 1.0);
                    found.Add(region);
                    continue;
                }

                // A little air around the element, a share of ITS size: on a sheet of small icons a fixed margin would pull
                // the neighbours in
                double pad = Math::Min(0.015, 0.12 * Math::Min(rw, rh));
                double bx0 = clamp(Math::Min(x0, x1) - pad), by0 = clamp(Math::Min(y0, y1) - pad);
                double bx1 = clamp(Math::Max(x0, x1) + pad), by1 = clamp(Math::Max(y0, y1) + pad);
                double w = bx1 - bx0, h = by1 - by0;
                if (w > 0.98 && h > 0.98)
                    continue; // the whole image is not a part

                region.x = (float)bx0; region.y = (float)by0; region.w = (float)w; region.h = (float)h;
                found.Add(region);
            }

            // The background first (it is the layer behind the others), at most one
            Vector<PipelineExtractRegion> regions;
            for (auto& r : found)
            {
                if (r.background) { regions.Add(r); break; }
            }
            for (auto& r : found)
            {
                if (!r.background && regions.Count() < 64)
                    regions.Add(r);
            }

            return regions;
        }

        double PartScale(int cropW, int cropH, int frameW, int frameH)
        {
            if (cropW <= 0 || cropH <= 0 || frameW <= 0 || frameH <= 0)
                return 0.0;

            double s = Math::Min((double)frameW / cropW, (double)frameH / cropH);
            return std::isfinite(s) && s > 0.0 ? UiDoc::JsRound(s * 10000.0) / 10000.0 : 0.0;
        }

        double ExtractSourceDpi(const PipelineNode& node, const String& sourcePng)
        {
            if (auto value = node.GetConfigValue("sourceDpi"))
            {
                UiLayout::Json json = UiDoc::FromDataValue(*value);
                if (auto dpi = UiDoc::CleanDpi(&json))
                    return *dpi;
            }

            if (auto tagged = PipelinePngMeta::ReadDpi(sourcePng))
                return *tagged;

            return UiDoc::UI_BASE_DPI;
        }

        String TagExtractPart(const PipelineNode& node, const String& portId, const String& sourcePng, const String& storedPng,
                              const String& outPng, bool raw)
        {
            double scale = PipelinePngMeta::ReadScale(storedPng).value_or(0.0);
            if (scale <= 0.0 && raw && !sourcePng.IsEmpty())
            {
                int sw = 0, sh = 0, fw = 0, fh = 0;
                if (PipelinePngMeta::Size(storedPng, fw, fh) && PipelinePngMeta::Size(sourcePng, sw, sh))
                {
                    auto region = RegionOfPort(node, portId);
                    PipelineImageOps::CropRect crop{ region.x, region.y, region.w, region.h };
                    Vec2I cropped = PipelineImageOps::CropSize(Vec2I(sw, sh), crop);
                    scale = PartScale(cropped.x, cropped.y, fw, fh);
                }
            }

            if (scale <= 0.0)
                return outPng;

            return PipelinePngMeta::WithDpi(outPng, ExtractSourceDpi(node, sourcePng) * scale, scale);
        }

        PipelineExtractRegion RegionOfPort(const PipelineNode& node, const String& portId)
        {
            auto regions = Read(node);
            for (auto& region : regions)
            {
                if (region.id == portId)
                    return region;
            }

            return regions.IsEmpty() ? PipelineExtractRegion() : regions[0];
        }

        DataValue* FindPart(PipelineNode& node, const String& portId)
        {
            return const_cast<DataValue*>(FindElement(node, portId));
        }

        const DataValue* FindTransparency(const PipelineNode& node, const String& portId)
        {
            auto element = FindElement(node, portId);
            auto own = element ? element->FindMember("transparency") : nullptr;
            return own && own->IsObject() ? own : nullptr;
        }

        DataValue* FindTransparency(PipelineNode& node, const String& portId)
        {
            return const_cast<DataValue*>(FindTransparency(static_cast<const PipelineNode&>(node), portId));
        }

        bool SetOwnTransparency(PipelineNode& node, const String& portId, bool own)
        {
            // A node from before regions existed gets its list written first
            if (own && !FindElement(node, portId) && RegionOfPort(node, portId).id == portId)
                Write(node, Read(node));

            auto element = const_cast<DataValue*>(FindElement(node, portId));
            if (!element || (FindTransparency(node, portId) != nullptr) == own)
                return false;

            if (own)
            {
                DataDocument settings;
                PipelineTransparency::WriteSettings(node, portId, settings);
                (*element)["transparency"] = static_cast<const DataValue&>(settings);
            }
            else
                element->RemoveMember("transparency");

            return true;
        }

        String OwnModel(const PipelineNode& node, const String& portId)
        {
            auto element = FindElement(node, portId);
            auto model = element ? element->FindMember("model") : nullptr;
            return model && model->IsString() ? String(model->GetString()).Trimed(" \n\r\t") : String();
        }

        String PartModel(const PipelineNode& node, const String& portId)
        {
            String own = node.nodeType == "imageExtract" ? OwnModel(node, portId) : String();
            return own.IsEmpty() ? node.GetConfigString("model", GeminiProvider::defaultImageModel) : own;
        }

        bool SetOwnPart(PipelineNode& node, const String& portId, bool own)
        {
            // A node from before regions existed gets its list written first
            if (own && !FindElement(node, portId) && RegionOfPort(node, portId).id == portId)
                Write(node, Read(node));

            auto element = const_cast<DataValue*>(FindElement(node, portId));
            if (!element)
                return false;

            bool changed = false;
            if (own)
            {
                if (OwnModel(node, portId).IsEmpty())
                {
                    (*element)["model"] = PartModel(node, portId);
                    changed = true;
                }
            }
            else if (element->FindMember("model"))
            {
                element->RemoveMember("model");
                changed = true;
            }

            changed = SetOwnTransparency(node, portId, own) || changed;
            return changed;
        }

        bool SetPartModel(PipelineNode& node, const String& portId, const String& model)
        {
            auto element = const_cast<DataValue*>(FindElement(node, portId));
            String clean = model.Trimed(" \n\r\t");
            if (!element || clean.IsEmpty() || (OwnModel(node, portId).IsEmpty() && !FindTransparency(node, portId)))
                return false;

            (*element)["model"] = clean;
            return true;
        }
    }
}
