#include "o2Editor/stdafx.h"
#include "PipelineRegions.h"

#include "o2Editor/Pipeline/Nodes/PipelineNodesCommon.h"
#include "o2Editor/Pipeline/PipelineImageOps.h"
#include "o2Editor/Pipeline/PipelineUtils.h"

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

        static PipelineExtractRegion ReadOne(const DataValue& value, int index)
        {
            PipelineExtractRegion region;
            if (auto id = value.FindMember("id"))
                region.id = String(id->IsString() ? id->GetString() : "");

            if (region.id.IsEmpty())
                region.id = "r" + (String)index;

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

            node.RemoveConfig("regions");
            auto& arr = node.config["regions"];
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
            }
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
            auto regions = Read(node);
            auto names = PortNames(regions);

            Vector<PipelinePort> ports;
            for (int i = 0; i < regions.Count(); i++)
                ports.Add(PipelinePort(regions[i].id, names[i], PipelinePortType::Image, false));

            node.outputs = ports;
        }

        const String autoSplitPrompt =
            "You are preparing this 2D game image to be rebuilt inside a game engine. It may be a game screen or UI mockup, a piece of concept art, or a SHEET of icons, sprites or items. "
            "List every separate visual element that must be cut out as its own sprite to reassemble the picture in the game: panels, frames, buttons, icons, characters, items, decorations, background pieces. "
            "If the image is a sheet, grid or set of icons, sprites or similar items, every single item is its own region: one box per item, all of them, even when they look alike. Never merge several items into one box, never list a row or the sheet itself. "
            "Skip plain text, numbers and labels - the engine renders those itself - and never make a region whose content is only text. Keep an element together with its own shadow, outline or glow. Do not list the whole image as one region. "
            "Answer with ONLY a JSON array, no commentary, no code fence: [{\"name\": \"<short English name of the element, 2-4 words>\", \"box_2d\": [ymin, xmin, ymax, xmax]}] "
            "box_2d is the bounding box as integers 0..1000 relative to the image height and width, in the order ymin, xmin, ymax, xmax, tight around the element. Up to 64 regions, in reading order: left to right, top to bottom.";

        Vector<PipelineExtractRegion> ParseAutoSplit(const String& answer)
        {
            Vector<PipelineExtractRegion> regions;

            String raw = answer.Trimed(" \n\r\t");
            int start = raw.Find("[");
            int end = raw.FindLast("]");
            if (start < 0 || end <= start)
                return regions;

            DataDocument doc;
            if (!doc.LoadFromData(raw.SubStr(start, end + 1)) || !doc.IsArray())
                return regions;

            const int maxRegions = 64;
            for (int i = 0; i < doc.GetElementsCount() && regions.Count() < maxRegions; i++)
            {
                auto& item = doc.GetElement(i);
                if (!item.IsObject())
                    continue;

                String name;
                if (auto n = item.FindMember("name"))
                    name = n->IsString() ? String(n->GetString()) : String();
                else if (auto label = item.FindMember("label"))
                    name = label->IsString() ? String(label->GetString()) : String();

                name = name.Trimed(" \n\r\t");
                if (name.IsEmpty())
                    continue;

                name = PipelineUtils::CutUtf8(name, 60);

                float x0 = 0, y0 = 0, x1 = 0, y1 = 0;
                auto readArray = [&](const DataValue& arr, bool yFirst)
                {
                    float v[4] = { 0, 0, 0, 0 };
                    for (int k = 0; k < 4; k++)
                        v[k] = PipelineUtils::ValueToNumber(arr.GetElement(k), 0.0f);

                    if (yFirst) { y0 = v[0]; x0 = v[1]; y1 = v[2]; x1 = v[3]; }
                    else { x0 = v[0]; y0 = v[1]; x1 = v[2]; y1 = v[3]; }
                };

                // Gemini answers in its native [ymin, xmin, ymax, xmax] on a 0..1000 grid whatever the field is called
                auto box2d = item.FindMember("box_2d");
                auto box = item.FindMember("box");
                if (box2d && box2d->IsArray() && box2d->GetElementsCount() == 4)
                    readArray(*box2d, true);
                else if (box && box->IsArray() && box->GetElementsCount() == 4)
                    readArray(*box, false);
                else if (item.FindMember("x") && item.FindMember("y") && item.FindMember("w") && item.FindMember("h"))
                {
                    x0 = PipelineUtils::ValueToNumber(*item.FindMember("x"), 0.0f);
                    y0 = PipelineUtils::ValueToNumber(*item.FindMember("y"), 0.0f);
                    x1 = x0 + PipelineUtils::ValueToNumber(*item.FindMember("w"), 0.0f);
                    y1 = y0 + PipelineUtils::ValueToNumber(*item.FindMember("h"), 0.0f);
                }
                else
                    continue;

                // Some models answer in pixels of a 1000-unit grid or in percent
                float largest = Math::Max(x1, y1);
                float scale = largest > 1.5f ? (largest > 100.0f ? 0.001f : 0.01f) : 1.0f;
                x0 *= scale; y0 *= scale; x1 *= scale; y1 *= scale;

                float rw = Math::Abs(x1 - x0), rh = Math::Abs(y1 - y0);
                if (rw < 0.005f || rh < 0.005f)
                    continue;

                // A little air around the element, a share of ITS size: on a sheet of small icons
                // a fixed margin would pull the neighbours in
                float pad = Math::Min(0.015f, 0.12f * Math::Min(rw, rh));
                PipelineExtractRegion region;
                region.id = PipelineNode::GenerateId();
                region.name = name;
                region.x = Math::Clamp(Math::Min(x0, x1) - pad, 0.0f, 1.0f);
                region.y = Math::Clamp(Math::Min(y0, y1) - pad, 0.0f, 1.0f);
                region.w = Math::Clamp(Math::Max(x0, x1) + pad, 0.0f, 1.0f) - region.x;
                region.h = Math::Clamp(Math::Max(y0, y1) + pad, 0.0f, 1.0f) - region.y;
                NormalizeBox(region.x, region.y, region.w, region.h);

                if (region.w > 0.98f && region.h > 0.98f)
                    continue; // the whole image is not a part

                regions.Add(region);
            }

            return regions;
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
    }
}
