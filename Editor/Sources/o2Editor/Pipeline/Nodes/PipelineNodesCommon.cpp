#include "o2Editor/stdafx.h"
#include "PipelineNodesCommon.h"

namespace Editor
{
    namespace PipelineTransparency
    {
        const String whiteBgInstruction =
            "Place the described subject on a solid, uniform, pure white (#FFFFFF) background that fills the whole frame. "
            "Keep the subject fully opaque with crisp edges. Do not add shadows, gradients, reflections, vignettes, or any non-white background.";

        const String blackBgInstruction =
            "You are given an image of a subject on a white background. Reproduce it EXACTLY - identical subject, pose, position, scale, "
            "colors, lighting and details - changing ONLY the background to a solid, uniform, pure black (#000000) that fills the whole frame. "
            "Do not move, resize, recolor, or alter the subject in any way. Do not add shadows or gradients. Output only the resulting image.";

        Config Read(const PipelineNode& node)
        {
            Config config;
            config.mode = node.GetConfigString("transparentMode", "twoPass") == "chroma" ? "chroma" : "twoPass";

            Color4 color;
            if (!PipelineUtils::ParseHexColor(node.GetConfigString("chromaColor", "#00b140"), color))
                color = Color4(0, 177, 64, 255);

            config.chroma.color = color;
            config.chroma.tolerance = Math::Clamp(node.GetConfigNumber("chromaTolerance", 30.0f), 0.0f, 100.0f);
            config.chroma.softness = Math::Clamp(node.GetConfigNumber("chromaSoftness", 15.0f), 0.0f, 100.0f);
            config.chroma.spill = Math::Clamp(node.GetConfigNumber("chromaSpill", 60.0f), 0.0f, 100.0f);
            config.colorName = PipelineUtils::ColorName(color);
            config.colorHex = PipelineUtils::ColorToHex(color).ToUpperCase();
            return config;
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
            static Vector<String> keys = { "chromaColor", "chromaTolerance", "chromaSoftness", "chromaSpill" };
            return keys;
        }

        bool UsesChromaPostStep(const PipelineNode& node)
        {
            bool chromaType = node.nodeType == "nanoBananaGen" || node.nodeType == "imageEdit" || node.nodeType == "imageExtract";
            return chromaType && node.GetConfigBool("transparentBg", false) && node.GetConfigString("transparentMode", "twoPass") == "chroma";
        }

        Ref<Bitmap> ApplyChromaPostStep(const PipelineNode& node, const Bitmap& raw)
        {
            auto config = Read(node);
            auto cut = PipelineImageOps::ChromaKey(raw, config.chroma);
            if (!PipelineImageOps::HasContent(*cut))
                return EnsureRgba(Ref<Bitmap>(mmake<Bitmap>(raw)));

            if (node.nodeType != "imageExtract")
                return cut;

            return PipelineImageOps::CropToContent(*cut, PipelineImageOps::ContentMode::Alpha);
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
}
