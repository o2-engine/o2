#include "o2Editor/stdafx.h"
#include "UiLayoutOwnAsset.h"

#include <random>

#include "o2Editor/Pipeline/Nodes/PipelineNodesCommon.h"
#include "o2Editor/Pipeline/PipelinePngMeta.h"
#include "o2Editor/Pipeline/PipelineRegions.h"
#include "o2Editor/Pipeline/PipelineUtils.h"
#include "o2Editor/Pipeline/UiDoc/UiKitPrep.h"
#include "o2Editor/Pipeline/UiLayout/UiLayoutPixels.h"

namespace Editor::UiLayout
{
    namespace D = ::Editor::UiDoc;

    // A random version-4 UUID (an upload's name)
    static String NewUploadId()
    {
        static std::mt19937_64 rng(std::random_device{}());
        static const char* hex = "0123456789abcdef";
        std::string out;
        for (int i = 0; i < 36; i++)
        {
            if (i == 8 || i == 13 || i == 18 || i == 23) { out += '-'; continue; }
            if (i == 14) { out += '4'; continue; }
            int v = (int)(rng()%16);
            out += hex[i == 19 ? (v & 3) | 8 : v];
        }
        return String(out) + ".png";
    }

    bool StoreOwnAsset(const OwnAssetInput& a, D::UiOwnAsset& out, String& error)
    {
        UiImage img;
        if (!UiImage::Decode(a.png, img))
        {
            error = "not a picture";
            return false;
        }

        std::optional<double> dpi = a.dpi;
        std::optional<D::UiNine> given = a.nineSlice;
        if (dpi && a.toDpi && *dpi > *a.toDpi*1.02)
        {
            // A picture denser than the layout is resampled to it (never the other way): the node's graphics are one to one
            // with its pixels like its kit's
            double k = *a.toDpi / *dpi;
            img = img.Resize(std::max(1, (int)JsRound(img.w*k)), std::max(1, (int)JsRound(img.h*k)));
            if (given)
                given = D::UiNine{ JsRound(given->left*k), JsRound(given->top*k), JsRound(given->right*k), JsRound(given->bottom*k) };
            dpi = a.toDpi;
        }

        String png;
        double width = 0, height = 0;
        std::optional<D::UiNine> nine;
        if (a.asIs)
        {
            png = img.Png();
            width = img.w;
            height = img.h;
            nine = given;
        }
        else
        {
            D::UiPrepInput prep;
            prep.image = img;
            prep.assetType = a.assetType;
            prep.key = false;
            prep.trim = true;
            prep.optimize = a.assetType == "sliced";
            prep.nineSlice = given;
            D::UiPrepOutput made;
            if (!D::PrepareElement(prep, made))
            {
                error = "the picture could not be prepared";
                return false;
            }
            png = made.png;
            width = made.width;
            height = made.height;
            nine = made.nineSlice;
        }

        String uploadId = NewUploadId();
        if (!PipelineUtils::WriteFileBytes(PipelineUtils::GetUploadPath(uploadId), png))
        {
            error = "the upload could not be written";
            return false;
        }

        out = D::UiOwnAsset();
        out.name = a.name;
        out.type = a.type;
        out.assetType = a.assetType;
        out.uploadId = uploadId;
        out.width = width;
        out.height = height;
        out.from = a.from;
        if (!a.description.IsEmpty())
            out.description = a.description.SubStr(0, 300);
        if (!a.purpose.IsEmpty())
            out.purpose = a.purpose.SubStr(0, 200);
        if (nine && a.assetType != "simple")
            out.nineSlice = nine;
        if (dpi)
            out.dpi = JsRound(*dpi);
        return true;
    }

    String OwnDrawPrompt(const D::UiAssetRequest& r, bool hasStyle)
    {
        String kind = r.type + " (" + D::UiElementHint(r.type) + ")";
        String shape = r.assetType == "sliced" ?
            String("It will be nine-sliced: draw it as one clean piece with plain, evenly coloured sides and middle between its corners, so it stretches without visible seams.") :
            r.assetType == "tiled" ? String("It will be tiled: draw one seamless repeat of the pattern.") : String();
        Vector<String> parts = {
            "Draw ONE game UI graphic for a UI kit: " + r.describe + ".",
            "Element: " + kind + (r.purpose.IsEmpty() ? String() : ", used as " + r.purpose) + ". Proportions about " +
                Json::FormatNumber(r.w) + ":" + Json::FormatNumber(r.h) + " (width:height).",
            "Show the element alone, straight on, filling the frame with a small even margin. No text, letters, digits or icons on it, no other elements, no scene, no mock-up around it.",
            shape,
            hasStyle ?
                String("Match the art style of the attached pictures exactly \xE2\x80\x94 the same rendering, outlines, shading, palette and level of detail \xE2\x80\x94 as if it belonged to the same UI kit.") :
                String("Clean, readable mobile-game UI style."),
        };
        String out;
        for (auto& p : parts)
        {
            if (p.IsEmpty())
                continue;
            out += (out.IsEmpty() ? "" : " ") + p;
        }
        return out;
    }

    // A context for one node run inside the agent's (the cut or the drawing of a graphic)
    static Ref<PipelineExecContext> NodeContext(const Ref<PipelineExecContext>& exec, const String& portId)
    {
        auto ctx = mmake<PipelineExecContext>();
        if (exec)
        {
            ctx->settings = exec->settings;
            ctx->log = exec->log;
            ctx->signalRetry = exec->signalRetry;
            ctx->isCancelled = exec->isCancelled;
            ctx->pipelineId = exec->pipelineId;
            ctx->nodeId = exec->nodeId;
        }
        ctx->outputPortId = portId;
        ctx->outputPort = portId;
        return ctx;
    }

    // The picture with its transparent margins cut off, when it has anything
    static Ref<Bitmap> Trimmed(const Ref<Bitmap>& bitmap)
    {
        if (!bitmap || !PipelineImageOps::HasContent(*bitmap))
            return bitmap;
        return PipelineImageOps::CropToContent(*bitmap, PipelineImageOps::ContentMode::Alpha);
    }

    Coroutine<OwnDrawResult> DrawOwnAsset(OwnDrawContext ctx, D::UiAssetRequest r, String name)
    {
        OwnDrawResult res;
        if (r.box && ctx.hasReference)
        {
            // The reference shows it: keep that very artwork (the AI extract node's cut), without what is drawn on it
            double bx = std::max(0.0, std::min(ctx.refW - 2, r.box->x)), by = std::max(0.0, std::min(ctx.refH - 2, r.box->y));
            double bw = std::max(2.0, r.box->w), bh = std::max(2.0, r.box->h);
            double padX = bw*0.08, padY = bh*0.08;
            double x0 = std::max(0.0, bx - padX), y0 = std::max(0.0, by - padY);
            double x1 = std::min(ctx.refW, bx + bw + padX), y1 = std::min(ctx.refH, by + bh + padY);

            auto impl = PipelineNodeRegistry::Get("imageExtract");
            auto node = PipelineNodeRegistry::CreateNode("imageExtract", Vec2F());
            if (!impl || !node)
            {
                res.error = "no AI extract node";
                co_return res;
            }
            node->SetConfigString("model", ctx.imageModel);
            node->SetConfigBool("transparentBg", true);
            node->SetConfigString("transparentMode", "chroma");
            PipelineExtractRegion region;
            region.id = "r";
            region.name = r.describe + " \xE2\x80\x94 the bare element, without any text, label, number or icon drawn on it";
            region.x = (float)(x0/ctx.refW);
            region.y = (float)(y0/ctx.refH);
            region.w = (float)((x1 - x0)/ctx.refW);
            region.h = (float)((y1 - y0)/ctx.refH);
            PipelineRegions::Write(*node, { region });

            Map<String, PipelineValue> inputs;
            inputs["image"] = PipelineValue::ImageBytes(ctx.referencePng);
            PipelineRunResult run = co_await impl->Run(NodeContext(ctx.exec, "r"), inputs, node);
            auto out = run.outputs.find("out");
            if (!run.ok || out == run.outputs.end() || !out->second.IsImage())
            {
                res.error = run.ok ? String("the image model returned no picture") : run.error;
                co_return res;
            }

            String raw = out->second.GetPngBytes();
            double scale = PipelinePngMeta::ReadScale(raw).value_or(1.0);
            auto bitmap = out->second.GetBitmap();
            if (bitmap && PipelineTransparency::UsesChromaPostStep(*node))
                bitmap = PipelineTransparency::ApplyChromaPostStep(*node, *bitmap);
            bitmap = Trimmed(bitmap);
            if (!bitmap)
            {
                res.error = "the image model returned no picture";
                co_return res;
            }
            String png = PipelineValue::Image(bitmap).GetPngBytes();
            // The frame's ratio, unless the model clearly did not keep the element's size inside its frame
            scale = PipelineRegions::MeasuredPartScale(ctx.referencePng, region, png, scale).value_or(scale);

            OwnAssetInput in;
            in.png = png;
            in.name = name;
            in.type = r.type;
            in.assetType = r.assetType;
            in.description = r.describe;
            in.purpose = r.purpose;
            in.dpi = ctx.refDpi*scale;
            in.toDpi = ctx.refDpi;
            in.from = "cut";
            res.ok = StoreOwnAsset(in, res.asset, res.error);
            co_return res;
        }

        auto impl = PipelineNodeRegistry::Get("nanoBananaGen");
        auto node = PipelineNodeRegistry::CreateNode("nanoBananaGen", Vec2F());
        if (!impl || !node)
        {
            res.error = "no image generation node";
            co_return res;
        }
        node->SetConfigString("model", ctx.imageModel);
        node->SetConfigBool("transparentBg", true);
        node->SetConfigString("transparentMode", "chroma");

        Map<String, PipelineValue> inputs;
        Vector<String> refs = ctx.styleRefs;
        while (refs.Count() > 3)
            refs.PopBack();
        inputs["prompt"] = PipelineValue::Text(OwnDrawPrompt(r, !refs.IsEmpty()));
        node->inputs.Clear();
        node->inputs.Add(PipelinePort{ "prompt", "prompt", PipelinePortType::Text });
        auto ctxRun = NodeContext(ctx.exec, "");
        for (int i = 0; i < refs.Count(); i++)
        {
            String id = "ref" + (String)i;
            node->inputs.Add(PipelinePort{ id, id, PipelinePortType::Image });
            inputs[id] = PipelineValue::ImageBytes(refs[i]);
            ctxRun->inputsById[id] = inputs[id];
        }
        ctxRun->inputsById["prompt"] = inputs["prompt"];

        PipelineRunResult run = co_await impl->Run(ctxRun, inputs, node);
        auto out = run.outputs.find("out");
        if (!run.ok || out == run.outputs.end() || !out->second.IsImage())
        {
            res.error = run.ok ? String("the image model returned no picture") : run.error;
            co_return res;
        }
        auto bitmap = out->second.GetBitmap();
        if (bitmap && PipelineTransparency::UsesChromaPostStep(*node))
            bitmap = PipelineTransparency::ApplyChromaPostStep(*node, *bitmap);
        bitmap = Trimmed(bitmap);
        if (!bitmap)
        {
            res.error = "the image model returned no picture";
            co_return res;
        }

        // Drawn at whatever size the model likes: its density is what shows it at the size asked for
        Vec2I size = bitmap->GetSize();
        double scale = std::max(size.x/std::max(1.0, r.w), size.y/std::max(1.0, r.h));
        OwnAssetInput in;
        in.png = PipelineValue::Image(bitmap).GetPngBytes();
        in.name = name;
        in.type = r.type;
        in.assetType = r.assetType;
        in.description = r.describe;
        in.purpose = r.purpose;
        in.dpi = ctx.layoutDpi*scale;
        in.toDpi = ctx.layoutDpi;
        in.from = "drawn";
        res.ok = StoreOwnAsset(in, res.asset, res.error);
        co_return res;
    }
}
