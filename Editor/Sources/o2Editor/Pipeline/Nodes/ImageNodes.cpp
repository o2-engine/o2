#include "o2Editor/stdafx.h"
#include "o2Editor/Pipeline/Nodes/PipelineNodesCommon.h"

#include "o2/Utils/FileSystem/FileSystem.h"

namespace Editor
{
    // Defined in TextNodes.cpp
    String WriteFinishAsset(const Ref<PipelineExecContext>& ctx, const PipelineNode& node, const String& defaultName,
                            const String& ext, const String& bytes);

    static Color4 ConfigColor(const PipelineNode& node, const char* key, const Color4& def)
    {
        Color4 c;
        return PipelineUtils::ParseHexColor(node.GetConfigString(key, ""), c) ? c : def;
    }

    static PipelineRunResult ImageResult(const Ref<Bitmap>& bitmap)
    {
        if (!bitmap)
            return PipelineRunResult::Fail("image operation produced nothing");

        return PipelineRunResult::Single(PipelineValue::Image(bitmap));
    }

    // Image file or project image asset picked in the node
    class SourceImageNode : public PipelineNodeBase
    {
    public:
        SourceImageNode()
        {
            mSchema.type = "sourceImage";
            mSchema.label = "Image source";
            mSchema.category = PipelineNodeCategory::Source;
            mSchema.description = "Pick an image file or a project image asset - reused from cache on every run until you replace it.";
            mSchema.outputs = { Out("out", PipelinePortType::Image) };
        }

        Coroutine<PipelineRunResult> Run(const Ref<PipelineExecContext>& ctx, const Map<String, PipelineValue>& inputs,
                                         const Ref<PipelineNode>& node) override
        {
            PipelineValue value;
            String error;
            if (!ResolveSourceValue(*node, ctx->assetsPath, value, error))
                co_return PipelineRunResult::Fail(error);

            co_return PipelineRunResult::Single(value);
        }
    };

    // Alpha recovery from two renders of the same subject on white and on black
    class RemoveBackgroundNode : public PipelineNodeBase
    {
    public:
        RemoveBackgroundNode()
        {
            mSchema.type = "removeBackground";
            mSchema.label = "Remove background (two-pass)";
            mSchema.category = PipelineNodeCategory::Transform;
            mSchema.instant = true;
            mSchema.description = "Recover alpha from two identical shots on white and black backgrounds.";
            mSchema.inputs = { In("white", PipelinePortType::Image), In("black", PipelinePortType::Image) };
            mSchema.outputs = { Out("out", PipelinePortType::Image) };
        }

        Coroutine<PipelineRunResult> Run(const Ref<PipelineExecContext>& ctx, const Map<String, PipelineValue>& inputs,
                                         const Ref<PipelineNode>& node) override
        {
            auto white = Input(inputs, "white");
            auto black = Input(inputs, "black");
            if (!white || !black || !white->IsImage() || !black->IsImage())
                co_return PipelineRunResult::Fail("removeBackground: both \"white\" and \"black\" inputs are required");

            auto w = white->GetBitmap();
            auto b = black->GetBitmap();
            if (!w || !b)
                co_return PipelineRunResult::Fail("removeBackground: inputs must be decodable images");

            co_return ImageResult(PipelineImageOps::TwoPassMatte(*w, *b));
        }
    };

    // Hand-drawn overlay composited over an optional background image
    class DrawImageNode : public PipelineNodeBase
    {
    public:
        DrawImageNode()
        {
            mSchema.type = "drawImage";
            mSchema.label = "Draw";
            mSchema.category = PipelineNodeCategory::Transform;
            mSchema.description = "Hand-drawn overlay. Paint with the brush in the node. Optional background image sets the output size; the drawing is composited on top.";
            mSchema.inputs = { In("background", PipelinePortType::Image) };
            mSchema.outputs = { Out("out", PipelinePortType::Image) };
        }

        Coroutine<PipelineRunResult> Run(const Ref<PipelineExecContext>& ctx, const Map<String, PipelineValue>& inputs,
                                         const Ref<PipelineNode>& node) override
        {
            auto bg = Input(inputs, "background");
            Ref<Bitmap> overlay = DecodeImageBytes(PipelineUtils::DataUrlToBytes(node->GetConfigString("drawing", "")));

            if (bg && bg->IsImage())
            {
                auto base = bg->GetBitmap();
                if (!base)
                    co_return PipelineRunResult::Fail("drawImage: background is not a decodable image");
                if (!overlay)
                    co_return ImageResult(base);
                co_return ImageResult(PipelineImageOps::CompositeOverlay(*base, *overlay));
            }

            if (overlay)
                co_return ImageResult(overlay);

            int w = Math::Max(1, (int)node->GetConfigNumber("dw", 512));
            int h = Math::Max(1, (int)node->GetConfigNumber("dh", 512));
            co_return ImageResult(PipelineImageOps::Blank(w, h));
        }
    };

    // Silhouette stroke of a transparent image
    class ImageOutlineNode : public PipelineNodeBase
    {
    public:
        ImageOutlineNode()
        {
            mSchema.type = "imageOutline";
            mSchema.label = "Outline";
            mSchema.category = PipelineNodeCategory::Transform;
            mSchema.instant = true;
            mSchema.description = "Stroke the silhouette of a transparent image: colour, thickness, position (outside / inside / centered) and softness. Works offline.";
            mSchema.inputs = { In("image", PipelinePortType::Image) };
            mSchema.outputs = { Out("out", PipelinePortType::Image) };
        }

        Coroutine<PipelineRunResult> Run(const Ref<PipelineExecContext>& ctx, const Map<String, PipelineValue>& inputs,
                                         const Ref<PipelineNode>& node) override
        {
            auto image = Input(inputs, "image");
            if (!image || !image->IsImage()) co_return PipelineRunResult::Fail("imageOutline: input \"image\" is not connected");
            auto bmp = image->GetBitmap();
            if (!bmp) co_return PipelineRunResult::Fail("imageOutline: input is not a decodable image");

            PipelineImageOps::OutlineOptions opt;
            opt.color = ConfigColor(*node, "color", Color4::Black());
            opt.width = Math::Clamp(node->GetConfigNumber("width", 4), 0.0f, 256.0f);
            String position = node->GetConfigString("position", "outside");
            opt.position = position == "inside" ? "inside" : position == "center" ? "center" : "outside";
            opt.opacity = Math::Clamp(node->GetConfigNumber("opacity", 1), 0.0f, 1.0f);
            opt.softness = Math::Clamp(node->GetConfigNumber("softness", 0), 0.0f, 64.0f);
            ctx->Log("imageOutline: width=" + (String)opt.width + "px position=" + opt.position);
            co_return ImageResult(PipelineImageOps::Outline(*bmp, opt));
        }
    };

    // Drop shadow, glow or inner shadow of a transparent image
    class ImageShadowNode : public PipelineNodeBase
    {
    public:
        ImageShadowNode()
        {
            mSchema.type = "imageShadow";
            mSchema.label = "Shadow";
            mSchema.category = PipelineNodeCategory::Transform;
            mSchema.instant = true;
            mSchema.description = "Drop a shadow (or a glow) behind a transparent image - colour, direction, distance, blur and spread - or cast it inside the shape. Works offline.";
            mSchema.inputs = { In("image", PipelinePortType::Image) };
            mSchema.outputs = { Out("out", PipelinePortType::Image) };
        }

        Coroutine<PipelineRunResult> Run(const Ref<PipelineExecContext>& ctx, const Map<String, PipelineValue>& inputs,
                                         const Ref<PipelineNode>& node) override
        {
            auto image = Input(inputs, "image");
            if (!image || !image->IsImage()) co_return PipelineRunResult::Fail("imageShadow: input \"image\" is not connected");
            auto bmp = image->GetBitmap();
            if (!bmp) co_return PipelineRunResult::Fail("imageShadow: input is not a decodable image");

            PipelineImageOps::ShadowOptions opt;
            opt.color = ConfigColor(*node, "color", Color4::Black());
            opt.opacity = Math::Clamp(node->GetConfigNumber("opacity", 0.6f), 0.0f, 1.0f);
            opt.angle = node->GetConfigNumber("angle", 45);
            opt.distance = Math::Clamp(node->GetConfigNumber("distance", 8), 0.0f, 512.0f);
            opt.blur = Math::Clamp(node->GetConfigNumber("blur", 8), 0.0f, 256.0f);
            opt.spread = Math::Clamp(node->GetConfigNumber("spread", 0), 0.0f, 128.0f);
            opt.inner = node->GetConfigBool("inner", false);
            co_return ImageResult(PipelineImageOps::Shadow(*bmp, opt));
        }
    };

    // Gradient overlay or gradient map over an image
    class ImageGradientNode : public PipelineNodeBase
    {
    public:
        ImageGradientNode()
        {
            mSchema.type = "imageGradient";
            mSchema.label = "Gradient";
            mSchema.category = PipelineNodeCategory::Transform;
            mSchema.instant = true;
            mSchema.description = "Lay a linear or radial gradient over the image - as an overlay in any blend mode, or as a gradient MAP that recolours it by luminance. Works offline.";
            mSchema.inputs = { In("image", PipelinePortType::Image) };
            mSchema.outputs = { Out("out", PipelinePortType::Image) };
        }

        Coroutine<PipelineRunResult> Run(const Ref<PipelineExecContext>& ctx, const Map<String, PipelineValue>& inputs,
                                         const Ref<PipelineNode>& node) override
        {
            auto image = Input(inputs, "image");
            if (!image || !image->IsImage()) co_return PipelineRunResult::Fail("imageGradient: input \"image\" is not connected");
            auto bmp = image->GetBitmap();
            if (!bmp) co_return PipelineRunResult::Fail("imageGradient: input is not a decodable image");

            PipelineImageOps::GradientOptions opt;
            String blend = node->GetConfigString("blend", "normal");
            opt.blend = (blend == "multiply" || blend == "screen" || blend == "overlay" || blend == "map") ? blend : String("normal");
            opt.kind = node->GetConfigString("kind", "linear") == "radial" ? "radial" : "linear";
            opt.color1 = ConfigColor(*node, "color1", Color4::White());
            opt.color2 = ConfigColor(*node, "color2", Color4::Black());
            opt.alpha1 = Math::Clamp(node->GetConfigNumber("alpha1", 1), 0.0f, 1.0f);
            opt.alpha2 = Math::Clamp(node->GetConfigNumber("alpha2", 1), 0.0f, 1.0f);
            opt.angle = node->GetConfigNumber("angle", 90);
            opt.opacity = Math::Clamp(node->GetConfigNumber("opacity", 1), 0.0f, 1.0f);
            opt.clipToAlpha = node->GetConfigBool("clipToAlpha", true);
            co_return ImageResult(PipelineImageOps::Gradient(*bmp, opt));
        }
    };

    // Brightness, contrast, saturation, hue, tint, grayscale and invert adjustments
    class ImageColorNode : public PipelineNodeBase
    {
    public:
        ImageColorNode()
        {
            mSchema.type = "imageColor";
            mSchema.label = "Color adjust";
            mSchema.category = PipelineNodeCategory::Transform;
            mSchema.instant = true;
            mSchema.description = "Recolour an image: brightness, contrast, saturation, hue rotation, a tint or a full colorize, plus grayscale and invert. Alpha is preserved. Works offline.";
            mSchema.inputs = { In("image", PipelinePortType::Image) };
            mSchema.outputs = { Out("out", PipelinePortType::Image) };
        }

        Coroutine<PipelineRunResult> Run(const Ref<PipelineExecContext>& ctx, const Map<String, PipelineValue>& inputs,
                                         const Ref<PipelineNode>& node) override
        {
            auto image = Input(inputs, "image");
            if (!image || !image->IsImage()) co_return PipelineRunResult::Fail("imageColor: input \"image\" is not connected");
            auto bmp = image->GetBitmap();
            if (!bmp) co_return PipelineRunResult::Fail("imageColor: input is not a decodable image");

            PipelineImageOps::ColorOptions opt;
            opt.brightness = node->GetConfigNumber("brightness", 0);
            opt.contrast = node->GetConfigNumber("contrast", 0);
            opt.saturation = node->GetConfigNumber("saturation", 0);
            opt.hue = node->GetConfigNumber("hue", 0);
            opt.tint = ConfigColor(*node, "tint", Color4(255, 136, 0, 255));
            opt.tintStrength = Math::Clamp(node->GetConfigNumber("tintStrength", 0), 0.0f, 1.0f);
            opt.colorize = node->GetConfigBool("colorize", false);
            opt.grayscale = node->GetConfigBool("grayscale", false);
            opt.invert = node->GetConfigBool("invert", false);
            co_return ImageResult(PipelineImageOps::AdjustColor(*bmp, opt));
        }
    };

    // Renders the arranged image layers into one image of the work area size
    class ComposerNode : public PipelineNodeBase
    {
    public:
        ComposerNode()
        {
            mSchema.type = "composer";
            mSchema.label = "Composer";
            mSchema.category = PipelineNodeCategory::Output;
            mSchema.instant = true;
            mSchema.description = "Collect several images as layers and arrange them (move / scale / rotate / flip / 9-slice / hide / reorder / duplicate) on a fixed-size work area, then render the arrangement to one image. Add image inputs with the + button.";
            mSchema.outputs = { Out("out", PipelinePortType::Image) };
            mSchema.addableInputs = { PipelinePortType::Image };
            mSchema.defaultSize = Vec2F(720, 560);
        }

        Coroutine<PipelineRunResult> Run(const Ref<PipelineExecContext>& ctx, const Map<String, PipelineValue>& inputs,
                                         const Ref<PipelineNode>& node) override
        {
            int canvasW = Math::Clamp((int)node->GetConfigNumber("canvasW", 1024), 16, 8192);
            int canvasH = Math::Clamp((int)node->GetConfigNumber("canvasH", 1024), 16, 8192);
            auto placements = node->GetConfigValue("layers");

            Vector<PipelineImageOps::PlacedLayer> placed;
            for (auto& layer : ResolveComposerLayers(*node))
            {
                auto it = ctx->inputsById.find(layer.portId);
                if (it == ctx->inputsById.end() || !it->second.IsValid())
                    continue;

                if (!it->second.IsImage())
                    co_return PipelineRunResult::Fail("composer: layer \"" + layer.name + "\" must be an image");

                auto bitmap = it->second.GetBitmap();
                if (!bitmap)
                    continue;

                const DataValue* stored = placements && placements->IsObject() ? placements->FindMember(layer.id.Data()) : nullptr;
                auto num = [&](const char* key, float def)
                {
                    auto m = stored ? stored->FindMember(key) : nullptr;
                    return m ? PipelineUtils::ValueToNumber(*m, def) : def;
                };
                auto flag = [&](const char* key)
                {
                    auto m = stored ? stored->FindMember(key) : nullptr;
                    return m ? (m->IsBoolean() ? (bool)*m : PipelineUtils::ValueToNumber(*m, 0) != 0) : false;
                };

                if (stored && flag("hidden"))
                    continue;

                PipelineImageOps::PlacedLayer p;
                p.image = bitmap;
                if (stored)
                {
                    p.x = num("x", 0); p.y = num("y", 0);
                    p.w = Math::Max(1.0f, num("w", 1)); p.h = Math::Max(1.0f, num("h", 1));
                    p.rotation = num("rot", 0);
                    p.flipH = flag("flipH"); p.flipV = flag("flipV");
                    p.opacity = num("opacity", 1);
                    p.nine = flag("nine");
                    if (auto slice = stored->FindMember("slice"))
                    {
                        auto sn = [&](const char* key) { auto m = slice->IsObject() ? slice->FindMember(key) : nullptr; return m ? Math::Max(0, (int)PipelineUtils::ValueToNumber(*m, 0)) : 0; };
                        p.slice = { sn("l"), sn("t"), sn("r"), sn("b") };
                    }
                    p.sliceScale = num("sliceScale", 1);
                }
                else
                {
                    Vec2I size = bitmap->GetSize();
                    float scale = Math::Min(1.0f, Math::Min((float)canvasW / size.x, (float)canvasH / size.y));
                    p.w = size.x * scale; p.h = size.y * scale;
                    p.x = (canvasW - p.w) / 2.0f; p.y = (canvasH - p.h) / 2.0f;
                }
                placed.Add(p);
            }

            Color4 bg;
            bool hasBg = node->GetConfigBool("outBgEnabled", false) && PipelineUtils::ParseHexColor(node->GetConfigString("outBg", ""), bg);
            ctx->Log("composer: " + (String)placed.Count() + " layer(s) -> " + (String)canvasW + "x" + (String)canvasH);
            co_return ImageResult(PipelineImageOps::ComposeLayers(canvasW, canvasH, placed, hasBg ? &bg : nullptr));
        }
    };

    // Terminal node saving the image as an image asset, with optional crop and rescale
    class FinishImageNode : public PipelineNodeBase
    {
    public:
        FinishImageNode()
        {
            mSchema.type = "finishImage";
            mSchema.label = "Finish - image";
            mSchema.category = PipelineNodeCategory::Output;
            mSchema.description = "Terminal node. Press Play to evaluate the pipeline; the image is saved as an ImageAsset in the Assets folder. Optionally rescale or crop the result.";
            mSchema.inputs = { In("in", PipelinePortType::Image) };
            mSchema.hasPlay = true;
        }

        Coroutine<PipelineRunResult> Run(const Ref<PipelineExecContext>& ctx, const Map<String, PipelineValue>& inputs,
                                         const Ref<PipelineNode>& node) override
        {
            auto value = Input(inputs, "in");
            if (!value || !value->IsImage())
                co_return PipelineRunResult::Fail("finishImage: input \"in\" is not connected");

            auto bitmap = value->GetBitmap();
            if (!bitmap)
                co_return PipelineRunResult::Fail("finishImage: input is not a decodable image");

            PipelineImageOps::CropRect crop;
            if (node->GetConfigBool("cropEnabled", false) && ReadNodeCrop(*node, "crop", crop))
            {
                bitmap = PipelineImageOps::Crop(*bitmap, crop);
                ctx->Log("finishImage: cropped to " + (String)bitmap->GetSize().x + "x" + (String)bitmap->GetSize().y);
            }

            int w = (int)Math::Round(node->GetConfigNumber("resizeW", 0));
            int h = (int)Math::Round(node->GetConfigNumber("resizeH", 0));
            if (node->GetConfigBool("resize", false) && w > 0 && h > 0)
            {
                bitmap = PipelineImageOps::Resize(*bitmap, Vec2I(w, h));
                ctx->Log("finishImage: rescaled to " + (String)w + "x" + (String)h);
            }

            PipelineValue result = PipelineValue::Image(bitmap);
            if (WriteFinishAsset(ctx, *node, "Generated/output", "png", result.data).IsEmpty())
                co_return PipelineRunResult::Fail("finishImage: failed to write the asset file");

            co_return PipelineRunResult::Single(result, "result");
        }
    };

    void RegisterImageNodes()
    {
        PipelineNodeRegistry::Register(mmake<SourceImageNode>());
        PipelineNodeRegistry::Register(mmake<RemoveBackgroundNode>());
        PipelineNodeRegistry::Register(mmake<DrawImageNode>());
        PipelineNodeRegistry::Register(mmake<ImageOutlineNode>());
        PipelineNodeRegistry::Register(mmake<ImageShadowNode>());
        PipelineNodeRegistry::Register(mmake<ImageGradientNode>());
        PipelineNodeRegistry::Register(mmake<ImageColorNode>());
        PipelineNodeRegistry::Register(mmake<ComposerNode>());
        PipelineNodeRegistry::Register(mmake<FinishImageNode>());
    }
}
