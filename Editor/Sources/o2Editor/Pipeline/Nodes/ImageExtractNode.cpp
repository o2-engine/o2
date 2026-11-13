#include "o2Editor/stdafx.h"
#include "ImageExtractNode.h"

#include <optional>

#include "o2Editor/Pipeline/Nodes/PipelineNodesCommon.h"
#include "o2Editor/Pipeline/PipelineExtractCheck.h"
#include "o2Editor/Pipeline/PipelinePngMeta.h"
#include "o2Editor/Pipeline/PipelineRegions.h"
#include "o2Editor/Pipeline/Providers/AiRouter.h"

namespace Editor
{
    namespace PipelineExtract
    {
        const char* const frameKey = "AssetsLine:frame";

        bool ReadPngFrame(const String& png, Frame& out)
        {
            auto text = PipelinePngMeta::ReadText(png);
            String value;
            if (!text.TryGetValue(frameKey, value))
                return false;

            auto parts = value.Split(",");
            if (parts.Count() != 4)
                return false;

            int v[4];
            for (int i = 0; i < 4; i++)
            {
                String part = parts[i].Trimed(" ");
                if (part.IsEmpty())
                    return false;
                for (int c = 0; c < part.Length(); c++)
                {
                    if (part[c] < '0' || part[c] > '9')
                        return false;
                }
                v[i] = atoi(part.Data());
            }
            if (!v[0] || !v[1])
                return false;

            out = Frame{ v[0], v[1], v[2], v[3] };
            return true;
        }

        static String BuildExtractPrompt(const String& part, const String& backdrop, bool native = false)
        {
            String result = native ? "The result must be almost entirely transparent with only the requested element visible." :
                "The result must be almost entirely that flat backdrop colour with only the requested element visible.";

            return "This image is a cropped region of a 2D game that contains the element to extract: " + part + ". "
                "Keep ONLY that element, exactly as it appears - same shape, position, size, colors, shading, texture and art style. Do NOT move, resize, recolor, restyle, or redraw it. "
                "If the description leaves something out - a label, text, numbers, an icon or another piece drawn on top of the element - remove that as well and restore the element's own surface underneath, continuing its color, texture and shading exactly, so it is left blank there. "
                "Erase EVERYTHING else in the crop to " + backdrop + ": the background AND any other objects, pieces, icons, text or fragments that are not the requested element. "
                "This is NOT background removal - remove the other objects as well. " + result + " If the element is partly hidden, plausibly complete it. "
                "Output only the resulting image.";
        }

        // The background plate of a screen: the scene alone, the UI on it removed and the scene continued
        static String BuildPlatePrompt(const String& part)
        {
            return "This image is a 2D game screen or a crop of one. Show only its background - " + part + ". "
                "Remove EVERY element drawn on top of it - panels, frames, plates, tiles, buttons, icons, bars, badges, characters, logos and all text - and continue the background naturally where they were. "
                "Keep the image size, framing, colors, lighting and texture of the background exactly; add nothing. Output only the image.";
        }

        // The result tagged with how much larger the model drew the part than the crop it was given (raw: the size it drew)
        // and, while the check is on, where the trimmed result sat in the frame (framed: what it was trimmed from)
        static PipelineRunResult Scaled(const Ref<Bitmap>& out, const Vec2I& crop, const Vec2I& raw, const Ref<Bitmap>& framed = nullptr,
                                        std::optional<PipelineImageOps::ContentMode> trimmed = std::nullopt, bool noteFrame = false)
        {
            String png = EncodeBitmapPng(*out);
            double scale = PipelineRegions::PartScale(crop.x, crop.y, raw.x, raw.y);
            if (scale > 0.0)
                png = PipelinePngMeta::WithDpi(png, std::nullopt, scale);

            // Only while the check is on: with it off a part's bytes are exactly what they were before the check existed
            int left, top, width, height;
            if (noteFrame && framed && trimmed && out != framed && PipelineImageOps::ContentBox(*framed, *trimmed, left, top, width, height))
            {
                Vec2I size = framed->GetSize();
                png = PipelinePngMeta::WriteText(png, { { frameKey, (String)size.x + "," + (String)size.y + "," + (String)left + "," + (String)top } });
            }

            return PipelineRunResult::Single(PipelineValue::ImageBytes(png));
        }

        Ref<Bitmap> RoiOf(const Bitmap& source, const PipelineNode& node, const String& portId)
        {
            auto region = PipelineRegions::RegionOfPort(node, portId);
            PipelineImageOps::CropRect roi{ region.x, region.y, region.w, region.h };
            bool hasRoi = region.x > 0.0f || region.y > 0.0f || region.w < 1.0f || region.h < 1.0f;
            return hasRoi ? PipelineImageOps::Crop(source, roi) : EnsureRgba(Ref<Bitmap>(mmake<Bitmap>(source)));
        }

        Coroutine<PipelineRunResult> GeneratePart(Ref<PipelineExecContext> ctx, Ref<PipelineNode> node, String portId,
                                                  Ref<Bitmap> roi, String prompt, String extra)
        {
            auto ask = [&](const String& text) { return extra.IsEmpty() ? text : text + "\n\n" + extra; };
            String partName = ctx->outputPort.IsEmpty() ? String("out") : ctx->outputPort;
            bool noteFrame = PipelineGraph::IsRefCheckOn(*node);
            auto alpha = PipelineImageOps::ContentMode::Alpha, white = PipelineImageOps::ContentMode::White;

            // The part's own model and background settings, when it has them, replace the node's
            String model = PipelineRegions::PartModel(*node, portId);
            Vec2I cropSize = roi->GetSize();
            Vector<AiImageRef> references = { { "image/png", EncodeBitmapPng(*roi) } };
            auto tr = PipelineTransparency::Read(*node, portId);
            bool transparent = tr.transparent;
            ctx->Log("imageExtract . " + partName + ": model=" + model + " refs=" + (String)references.Count() + " transparent=" +
                     (transparent ? "true" : "false") + (extra.IsEmpty() ? String() : " clarifications=" + (String)extra.Length()));

            if (transparent && tr.mode == "native")
            {
                AiBytesResult png = co_await AiRouter::GenerateImage(ctx, model, ask(BuildExtractPrompt(prompt, PipelineTransparency::nativeBackdrop, true)), references, ctx->seed, true);
                if (!png.ok) co_return PipelineRunResult::Fail(png.error);
                auto rendered = DecodeImageBytes(png.data);
                if (!rendered) co_return PipelineRunResult::Fail("provider returned an undecodable image");

                if (!PipelineImageOps::HasContent(*rendered))
                    co_return Scaled(rendered, cropSize, rendered->GetSize());

                co_return Scaled(PipelineImageOps::CropToContent(*rendered, alpha), cropSize, rendered->GetSize(), rendered, alpha, noteFrame);
            }

            if (transparent && tr.mode == "chroma")
            {
                AiBytesResult raw = co_await AiRouter::GenerateImage(ctx, model, ask(BuildExtractPrompt(prompt, PipelineTransparency::ChromaEraseInstruction(tr))), references, ctx->seed);
                if (!raw.ok) co_return PipelineRunResult::Fail(raw.error);
                auto rawBmp = DecodeImageBytes(raw.data);
                if (!rawBmp) co_return PipelineRunResult::Fail("provider returned an undecodable image");
                co_return Scaled(rawBmp, cropSize, rawBmp->GetSize());
            }

            if (PipelineRegions::IsBackgroundPart(prompt, transparent))
            {
                // The plate behind a screen: opaque and the size of its box, so nothing is trimmed
                AiBytesResult plate = co_await AiRouter::GenerateImage(ctx, model, ask(BuildPlatePrompt(prompt)), references, ctx->seed);
                if (!plate.ok) co_return PipelineRunResult::Fail(plate.error);
                auto plateBmp = DecodeImageBytes(plate.data);
                if (!plateBmp) co_return PipelineRunResult::Fail("provider returned an undecodable image");
                co_return Scaled(plateBmp, cropSize, plateBmp->GetSize());
            }

            AiBytesResult whiteRender = co_await AiRouter::GenerateImage(ctx, model, ask(BuildExtractPrompt(prompt, "flat, solid, pure white (#FFFFFF)")), references, ctx->seed);
            if (!whiteRender.ok) co_return PipelineRunResult::Fail(whiteRender.error);
            auto whiteBmp = DecodeImageBytes(whiteRender.data);
            if (!whiteBmp) co_return PipelineRunResult::Fail("provider returned an undecodable image");

            if (!transparent)
                co_return Scaled(PipelineImageOps::CropToContent(*whiteBmp, white), cropSize, whiteBmp->GetSize(), whiteBmp, white, noteFrame);

            AiBytesResult black = co_await AiRouter::GenerateImage(ctx, model, PipelineTransparency::blackBgInstruction, { { "image/png", whiteRender.data } }, ctx->seed);
            if (!black.ok) co_return PipelineRunResult::Fail(black.error);
            auto blackBmp = DecodeImageBytes(black.data);
            if (!blackBmp) co_return PipelineRunResult::Fail("provider returned an undecodable image");

            auto recovered = PipelineTransparency::MatteFromPair(*whiteBmp, *blackBmp);
            if (PipelineImageOps::HasContent(*recovered))
                co_return Scaled(PipelineImageOps::CropToContent(*recovered, alpha), cropSize, whiteBmp->GetSize(), recovered, alpha, noteFrame);

            ctx->Log("imageExtract . " + partName + ": matte was empty - falling back to the white result");
            co_return Scaled(PipelineImageOps::CropToContent(*whiteBmp, white), cropSize, whiteBmp->GetSize(), whiteBmp, white, noteFrame);
        }
    }

    // Cuts a described element out of an image as a transparent sprite through an image model; a paid second step checks it
    // against the reference when the node asks for it
    class ImageExtractNode : public PipelineNodeBase
    {
    public:
        ImageExtractNode()
        {
            mSchema.type = "imageExtract";
            mSchema.label = "AI extract part";
            mSchema.category = PipelineNodeCategory::AI;
            mSchema.description = "Cut a described element out of an image as a clean, transparent game sprite; a box on the source marks where each part is.";
            mSchema.inputs = { In("image", PipelinePortType::Image) };
            mSchema.outputs = { Out("out", PipelinePortType::Image) };
            mSchema.perPortRun = true;
        }

        // Editing one part must not invalidate the others: the shared config is hashed without the
        // part list, and this part's own name and box are folded back in
        Vector<String> PortCacheExcludedKeys() const override { return { "regions", "roi", "prompt" }; }

        // A part with a model of its own is keyed by it instead of the node's; a part without one keeps the key it always had
        Vector<String> PortCacheExcludedKeysOf(const PipelineNode& node, const String& portId) const override
        {
            Vector<String> keys = PortCacheExcludedKeys();
            if (!PipelineRegions::OwnModel(node, portId).IsEmpty())
                keys.Add("model");

            return keys;
        }

        void InitNode(const Ref<PipelineNode>& node) const override
        {
            // New nodes cut transparent sprites with a model that draws the alpha itself, as AssetsLine's schema defaults
            node->SetConfigString("model", PipelineRegions::extractDefaultImageModel);
            node->SetConfigString("prompt", "");
            node->SetConfigBool("transparentBg", true);
            node->SetConfigString("transparentMode", "chroma");
        }

        bool SyncPorts(const Ref<PipelineNode>& node) const override
        {
            PipelineRegions::SyncPorts(*node);
            return true;
        }

        String PortCacheVariant(const PipelineNode& node, const String& portId) const override
        {
            auto region = PipelineRegions::RegionOfPort(node, portId);
            String variant = "region:" + region.name + ":" + (String)region.x + "," + (String)region.y + "," + (String)region.w + "," + (String)region.h;
            String model = PipelineRegions::OwnModel(node, portId);
            return model.IsEmpty() ? variant : variant + "|model:" + model;
        }

        // The check against the reference: a paid second step over a part's stored generation
        String RefineKey(const PipelineNode& node, const String& portId) const override
        {
            PipelineExtractCheck::Config check;
            if (!PipelineExtractCheck::Read(node, check))
                return "";
            return "ref-check:v1:" + check.model + ":" + (String)check.limit;
        }

        Coroutine<PipelineRefineResult> Refine(Ref<PipelineExecContext> ctx, Map<String, PipelineValue> inputs, Ref<PipelineNode> node,
                                               PipelineValue stored, String key) override
        {
            PipelineRefineResult res;
            res.value = stored;
            auto source = inputs.find("image");
            if (!stored.IsImage() || source == inputs.end() || !source->second.IsImage())
                co_return res;

            auto outcome = co_await PipelineExtractCheck::Run(ctx, node, source->second.GetPngBytes(), stored.data, key);
            res.value = PipelineValue::ImageBytes(outcome.data);
            res.keep = outcome.final;
            co_return res;
        }

        void RefineReplayed(const PipelineExecContext& ctx, const PipelineNode& node, const String& key) const override
        {
            PipelineExtractCheck::Replay(ctx, node, key);
        }

        Coroutine<PipelineRunResult> Run(const Ref<PipelineExecContext>& ctx, const Map<String, PipelineValue>& inputs,
                                         const Ref<PipelineNode>& node) override
        {
            auto imageVal = Input(inputs, "image");
            if (!imageVal || !imageVal->IsImage()) co_return PipelineRunResult::Fail("imageExtract: input \"image\" is not connected");

            // Each output port carries one part: its box is the region of interest, its name the prompt
            auto region = PipelineRegions::RegionOfPort(*node, ctx->outputPortId);
            String partName = ctx->outputPort.IsEmpty() ? String("out") : ctx->outputPort;
            String prompt = region.name.Trimed(" \n\r\t");
            if (prompt.IsEmpty()) co_return PipelineRunResult::Fail("imageExtract: no extraction prompt for \"" + partName + "\" - name the part to cut out");

            auto source = imageVal->GetBitmap();
            if (!source) co_return PipelineRunResult::Fail("imageExtract: input is not a decodable image");

            // A drawing left in an older pipeline is ignored: the extract has no drawing any more
            co_return co_await PipelineExtract::GeneratePart(ctx, node, region.id, PipelineExtract::RoiOf(*source, *node, region.id), prompt, "");
        }
    };

    void RegisterImageExtractNode()
    {
        PipelineNodeRegistry::Register(mmake<ImageExtractNode>());
    }
}
