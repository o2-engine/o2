#include "o2Editor/stdafx.h"
#include "PipelineNodesCommon.h"

#include "o2Editor/Pipeline/PipelineEditRegion.h"
#include "o2Editor/Pipeline/PipelineRegions.h"
#include "o2Editor/Pipeline/PipelineUtils.h"

namespace Editor
{
    namespace PipelineTransparency
    {
        const String nativeBackdrop = "full transparency (alpha 0, no colour at all)";

        const String nativeEditSuffix =
            "Then isolate the main subject of the edited result and remove the original background entirely, leaving it fully transparent. "
            "Keep the subject fully opaque with crisp edges. Do not add shadows, gradients, reflections, or vignettes.";

        const String whiteBgInstruction =
            "Place the described subject on a solid, uniform, pure white (#FFFFFF) background that fills the whole frame. "
            "Keep the subject fully opaque with crisp edges. Do not add shadows, gradients, reflections, vignettes, or any non-white background.";

        const String blackBgInstruction =
            "You are given an image of a subject on a white background. Reproduce it EXACTLY - identical subject, pose, position, scale, "
            "colors, lighting and details - changing ONLY the background to a solid, uniform, pure black (#000000) that fills the whole frame. "
            "Do not move, resize, recolor, or alter the subject in any way. Do not add shadows or gradients. Output only the resulting image.";

        // True for YYYY-MM-DD
        static bool IsDateSuffix(const String& text)
        {
            if (text.Length() != 10)
                return false;

            for (int i = 0; i < 10; i++)
            {
                bool dash = i == 4 || i == 7;
                if (dash ? text[i] != '-' : !(text[i] >= '0' && text[i] <= '9'))
                    return false;
            }
            return true;
        }

        bool SupportsNativeTransparency(const String& modelIn)
        {
            String model = modelIn.Trimed(" \n\r\t");
            if (model.StartsWith("models/"))
                model = model.SubStr(7);

            model = model.ToLowerCase();
            if (model.StartsWith("gpt-image"))
                return true;

            // OpenRouter takes only an automatic or opaque background for this one and its dated snapshots
            static const String opaqueOnly = "openai/gpt-image-2";
            if (model == opaqueOnly || (model.StartsWith(opaqueOnly + "-") && IsDateSuffix(model.SubStr(opaqueOnly.Length() + 1))))
                return false;

            return model.StartsWith("openai/gpt-image-") || model == "openai/gpt-5-image" || model == "openai/gpt-5-image-mini" ||
                model == "sourceful/riverflow-v2.5-pro";
        }

        bool UsesNativeTransparency(const PipelineNode& node, const String& portId /*= ""*/)
        {
            auto config = Read(node, portId);
            return (config.transparent || node.nodeType == "aiRemoveBg") && config.mode == "native";
        }

        const Vector<String>& SettingKeys()
        {
            static Vector<String> keys = { "transparentBg", "transparentMode", "chromaColor", "chromaTolerance", "chromaSoftness", "chromaSpill" };
            return keys;
        }

        const DataValue* SettingValue(const PipelineNode& node, const String& portId, const char* key)
        {
            // A partial own object still reads: a key it lacks comes from the node
            if (auto own = PipelineRegions::FindTransparency(node, portId))
            {
                if (auto value = own->FindMember(key))
                    return value;
            }

            return node.GetConfigValue(key);
        }

        Config Read(const PipelineNode& node, const String& portId /*= ""*/)
        {
            auto str = [&](const char* key, const String& def) { auto v = SettingValue(node, portId, key); return v ? PipelineUtils::ValueToString(*v, def) : def; };
            auto num = [&](const char* key, float def) { auto v = SettingValue(node, portId, key); return v ? PipelineUtils::ValueToNumber(*v, def) : def; };
            auto flag = SettingValue(node, portId, "transparentBg");

            // A framed edit keeps the rest of the image, background included; the stored keys apply again without the frame
            PipelineImageOps::CropRect region;
            Config config;
            config.transparent = flag && PipelineUtils::ValueToBool(*flag, false) && !PipelineEditRegion::Of(node, region);
            config.storedMode = str("transparentMode", "twoPass") == "chroma" ? "chroma" : "twoPass";
            config.mode = SupportsNativeTransparency(node.GetConfigString("model", GeminiProvider::defaultImageModel)) ?
                String("native") : config.storedMode;

            Color4 color;
            if (!PipelineUtils::ParseHexColor(str("chromaColor", "#00b140"), color))
                color = Color4(0, 177, 64, 255);

            config.chroma.color = color;
            config.chroma.tolerance = Math::Clamp(num("chromaTolerance", 30.0f), 0.0f, 100.0f);
            config.chroma.softness = Math::Clamp(num("chromaSoftness", 15.0f), 0.0f, 100.0f);
            config.chroma.spill = Math::Clamp(num("chromaSpill", 60.0f), 0.0f, 100.0f);
            config.colorName = PipelineUtils::ColorName(color);
            config.colorHex = PipelineUtils::ColorToHex(color).ToUpperCase();
            return config;
        }

        void WriteSettings(const PipelineNode& node, const String& portId, DataValue& target)
        {
            auto str = [&](const char* key, const String& def) { auto v = SettingValue(node, portId, key); return v ? PipelineUtils::ValueToString(*v, def) : def; };
            auto num = [&](const char* key, float def) { auto v = SettingValue(node, portId, key); return v ? PipelineUtils::ValueToNumber(*v, def) : def; };
            auto config = Read(node, portId);

            target.SetObject();
            target["transparentBg"] = config.transparent;
            target["transparentMode"] = config.storedMode;
            target["chromaColor"] = str("chromaColor", "#00b140");
            target["chromaTolerance"] = num("chromaTolerance", 30.0f);
            target["chromaSoftness"] = num("chromaSoftness", 15.0f);
            target["chromaSpill"] = num("chromaSpill", 60.0f);
        }

        String ChromaBgInstruction(const Config& c)
        {
            return "Place the described subject on a completely flat, uniform " + c.colorName + " (" + c.colorHex +
                ") background that fills the whole frame - a chroma-key backdrop. The background must be one single solid colour with no gradient, "
                "texture, vignette, shadow or reflection on it. The subject itself must NOT contain that " + c.colorName +
                " colour anywhere, and must keep crisp, clean edges against the backdrop.";
        }

        String ChromaEraseInstruction(const Config& c)
        {
            return "flat, solid, uniform " + c.colorName + " (" + c.colorHex + ")";
        }

        const Vector<String>& ChromaConfigKeys()
        {
            // chromaColor is not here: it is the backdrop the provider is asked to render, so a render
            // made against one key colour must never be reused for another
            static Vector<String> keys = { "chromaTolerance", "chromaSoftness", "chromaSpill" };
            return keys;
        }

        bool UsesChromaPostStep(const PipelineNode& node, const String& portId /*= ""*/)
        {
            bool chromaType = node.nodeType == "nanoBananaGen" || node.nodeType == "imageEdit" || node.nodeType == "imageExtract";
            if (!chromaType)
                return false;

            auto config = Read(node, portId);
            return config.transparent && config.mode == "chroma";
        }

        bool AnyChromaPostStep(const PipelineNode& node)
        {
            if (UsesChromaPostStep(node))
                return true;

            return node.outputs.Any([&](const PipelinePort& port) { return UsesChromaPostStep(node, port.id); });
        }

        String PortCacheSuffix(const PipelineNode& node, const String& portId, bool rawRender, Vector<String>& exclude)
        {
            bool chroma = UsesChromaPostStep(node, portId);
            String suffix;
            if (PipelineRegions::FindTransparency(node, portId))
            {
                // The key colour stays in: a render made against one colour is never reused for another
                auto config = Read(node, portId);
                exclude.Add(SettingKeys());
                suffix = "|bg:[" + String(config.transparent ? "true" : "false") + ",\"" + config.storedMode + "\",\"" + config.colorHex + "\"]";

                // What goes downstream is the cut, so its consumers follow the cut settings the raw render ignores
                if (chroma && !rawRender)
                {
                    suffix += "|cut:[" + (String)config.chroma.tolerance + "," + (String)config.chroma.softness + "," +
                        (String)config.chroma.spill + "]";
                }
            }
            else if (chroma && rawRender)
                exclude.Add(ChromaConfigKeys());

            if (chroma && rawRender)
                suffix += "|chroma-raw";

            return suffix;
        }

        Ref<Bitmap> ApplyChromaPostStep(const PipelineNode& node, const Bitmap& raw, const String& portId /*= ""*/)
        {
            auto config = Read(node, portId);
            auto cut = PipelineImageOps::ChromaKey(raw, config.chroma);
            if (!PipelineImageOps::HasContent(*cut))
                return EnsureRgba(Ref<Bitmap>(mmake<Bitmap>(raw)));

            if (node.nodeType != "imageExtract")
                return cut;

            return PipelineImageOps::CropToContent(*cut, PipelineImageOps::ContentMode::Alpha);
        }

        int ImageGenerations(const PipelineNode& node)
        {
            auto generations = [&](const String& portId)
            {
                auto config = Read(node, portId);
                bool transparent = node.nodeType == "aiRemoveBg" || config.transparent;
                return transparent && config.mode == "twoPass" ? 2 : 1;
            };

            if (node.nodeType != "imageExtract")
                return generations("");

            int total = 0;
            for (auto& region : PipelineRegions::Read(node))
                total += generations(region.id);

            return total;
        }

        Ref<Bitmap> MatteFromPair(const Bitmap& white, const Bitmap& black)
        {
            return PipelineImageOps::TwoPassMatte(white, black);
        }
    }

    bool ReadNodeCrop(const PipelineNode& node, const char* key, PipelineImageOps::CropRect& crop)
    {
        return PipelineImageOps::ParseCrop(node.GetConfigValue(key), crop);
    }

    Vector<ComposerLayerRef> ResolveComposerLayers(const PipelineNode& node)
    {
        Vector<ComposerLayerRef> layers;
        for (auto& port : node.inputs)
        {
            if (port.portType == PipelinePortType::Image)
                layers.Add({ port.id, port.id, port.name, false });
        }

        if (auto dups = node.GetConfigValue("dupLayers"))
        {
            if (dups->IsArray())
            {
                for (auto& d : *dups)
                {
                    if (!d.IsObject()) continue;
                    auto id = d.FindMember("id");
                    auto src = d.FindMember("srcPortId");
                    if (!id || !src) continue;
                    String srcId = PipelineUtils::ValueToString(*src);
                    auto source = layers.Find([&](const ComposerLayerRef& l) { return !l.dup && l.portId == srcId; });
                    if (!source) continue;
                    auto name = d.FindMember("name");
                    layers.Add({ PipelineUtils::ValueToString(*id), srcId,
                                 name ? PipelineUtils::ValueToString(*name) : (source->name.IsEmpty() ? String("layer") : source->name) + " copy", true });
                }
            }
        }

        Vector<ComposerLayerRef> ordered;
        if (auto order = node.GetConfigValue("layerOrder"))
        {
            if (order->IsArray())
            {
                for (auto& idValue : *order)
                {
                    String id = PipelineUtils::ValueToString(idValue);
                    auto layer = layers.Find([&](const ComposerLayerRef& l) { return l.id == id; });
                    if (layer && !ordered.Any([&](const ComposerLayerRef& l) { return l.id == id; }))
                        ordered.Add(*layer);
                }
            }
        }
        for (auto& layer : layers)
        {
            if (!ordered.Any([&](const ComposerLayerRef& l) { return l.id == layer.id; }))
                ordered.Add(layer);
        }
        return ordered;
    }

    bool ResolveSourceValue(const PipelineNode& node, const String& assetsPath, PipelineValue& value, String& error)
    {
        if (node.nodeType == "sourceText")
        {
            value = PipelineValue::Text(node.GetConfigString("text", ""));
            return true;
        }

        bool image = node.nodeType == "sourceImage";
        if (!image && node.nodeType != "sourceAudio")
        {
            error = node.nodeType + " is not a source node";
            return false;
        }

        String kind = image ? "sourceImage" : "sourceAudio";
        String assetPath = node.GetConfigString("assetPath", "").Trimed();
        String uploadId = node.GetConfigString("uploadId", "").Trimed();
        String path;
        if (!assetPath.IsEmpty())
            path = assetsPath + assetPath;
        else if (!uploadId.IsEmpty())
            path = PipelineUtils::GetUploadPath(uploadId);
        else
        {
            error = kind + (image ? ": no image chosen yet" : ": no audio chosen yet");
            return false;
        }

        String bytes = PipelineUtils::ReadFileBytes(path);
        if (bytes.IsEmpty())
        {
            error = kind + ": file not found: " + path;
            return false;
        }

        if (!image)
        {
            String ext = path.SubStr(path.FindLast(".") + 1).ToLowerCase();
            value = PipelineValue::Bytes(PipelinePortType::Audio, bytes, PipelineUtils::MimeForExtension(ext));
            return true;
        }

        auto bitmap = DecodeImageBytes(bytes);
        if (!bitmap)
        {
            error = "sourceImage: unsupported image format (png expected)";
            return false;
        }

        value = PipelineValue::Image(bitmap);
        return true;
    }
}
