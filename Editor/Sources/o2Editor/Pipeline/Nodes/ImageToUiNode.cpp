#include "o2Editor/stdafx.h"
#include "ImageToUiNode.h"

#include "o2/Assets/Assets.h"
#include "o2Editor/Pipeline/Nodes/PipelineNodesCommon.h"
#include "o2Editor/Pipeline/PipelineImageToUi.h"
#include "o2Editor/Pipeline/UiLayout/UiLayoutExport.h"
#include "o2Editor/Pipeline/UiLayout/UiLayoutRun.h"

namespace Editor
{
    using namespace UiLayout;

    // AI image to UI: a UI mockup in, a layout out. The node cuts the mockup's sprites, then runs XuToWei/Image-To-UI's
    // skill unchanged with the vision model as its agent; a fresh run also writes the o2 prototype of the layout. Config
    // edits holds the user's rounds of changes after a run: each continues the skill from the layout the previous one left
    class ImageToUiNode: public PipelineNodeBase
    {
    public:
        ImageToUiNode()
        {
            mSchema.type = PipelineImageToUi::type;
            mSchema.label = "AI image to UI";
            mSchema.category = PipelineNodeCategory::AI;
            mSchema.description = "Turn a UI mockup into a layout: cut its sprites, then let the model run the Image-To-UI skill - "
                "measure on its grid, write ui_structure.json, check, fix, review every element and finalize. In Unity the layout "
                "becomes a prefab, in o2 a prototype.";
            mSchema.inputs = { In("image", PipelinePortType::Image) };
            mSchema.outputs = { Out("preview", PipelinePortType::Image), Out("layout", PipelinePortType::Text) };
            mSchema.hasPlay = true;
            mSchema.dynamicOutputs = true;
        }

        Vector<String> SignatureExcludedKeys() const override
        {
            return { "assetPath" };
        }

        Coroutine<PipelineRunResult> Run(const Ref<PipelineExecContext>& ctx, const Map<String, PipelineValue>& inputs,
                                         const Ref<PipelineNode>& node) override
        {
            auto input = Input(inputs, "image");
            if (!input)
                co_return PipelineRunResult::Fail("imageToUi: input \"image\" is not connected");
            if (!input->IsImage())
                co_return PipelineRunResult::Fail("imageToUi: input must be an image");
            if (ctx->workDir.IsEmpty())
                co_return PipelineRunResult::Fail("imageToUi: the node needs its work folder");

            String designPng = input->GetPngBytes();
            UiImage size;
            if (!UiImage::Decode(designPng, size))
                co_return PipelineRunResult::Fail("imageToUi: the input is not a decodable image");

            auto cfg = PipelineImageToUi::ReadConfig(*node);
            auto trace = mmake<Trace>(ctx->workDir + "trace", size.w, size.h);

            auto ai = mmake<AiCtx>();
            ai->exec = ctx;
            ai->fresh = ctx->fresh;
            ai->textModel = cfg.model;
            ai->imageModel = cfg.imageModel;
            ai->seed = ctx->seed;
            ai->memo.get = ctx->memoGet;
            ai->memo.put = ctx->memoPut;

            ImageToUiConfig runConfig;
            runConfig.maxTurns = cfg.maxTurns;
            runConfig.sprites = cfg.sprites;
            runConfig.background = cfg.background;
            runConfig.name = cfg.screenName;
            runConfig.hint = cfg.hint;
            runConfig.python = ctx->settings.imageToUiPython;
            runConfig.edits = PipelineImageToUi::Edits(*node);
            runConfig.editTurns = PipelineImageToUi::EditTurns(cfg.maxTurns);

            ctx->Log("imageToUi: " + (String)size.w + "\xC3\x97" + (String)size.h + ", vision " + cfg.model + ", images " + cfg.imageModel +
                     ", at most " + (String)cfg.maxTurns + " agent turns" +
                     (runConfig.edits.IsEmpty() ? String() : ", " + (String)runConfig.edits.Count() + " round(s) of edits"));

            // Everything the run without edits depends on: rounds continue it only when this is unchanged
            Json base = Json::Object();
            base["model"] = cfg.model;
            base["imageModel"] = cfg.imageModel;
            base["maxTurns"] = cfg.maxTurns;
            base["sprites"] = cfg.sprites;
            base["background"] = cfg.background;
            base["name"] = cfg.screenName;
            base["hint"] = cfg.hint;
            base["seed"] = ctx->seed >= 0 ? Json(ctx->seed) : Json();
            ImageToUiRunOptions options;
            options.fresh = ctx->fresh;
            options.baseKey = ImageToUiBaseKey(designPng, base);

            ImageToUiResult res = co_await RunImageToUi(ai, designPng, runConfig, trace, ctx->workDir, options);
            if (!res.ok)
                co_return PipelineRunResult::Fail(res.error);

            const Json* turns = res.summary.Get("turns");
            const Json* checks = res.summary.Get("check_count");
            ctx->Log("imageToUi: " + (res.complete ? String("skill completed") : "NOT completed (" + (String)res.blockers.Count() + " blocker(s))") +
                     " - " + (turns ? turns->ToJsString() : String("0")) + " turns, " + (checks ? checks->ToJsString() : String("0")) +
                     " checks, " + (String)res.sprites.Count() + " sprites");
            for (auto& blocker : res.blockers)
                ctx->Log("imageToUi: blocker: " + blocker);

            if (!ctx->assetsPath.IsEmpty())
                co_await ImageToUiExport(ctx, cfg, res.layout, res.sprites);

            PipelineRunResult result;
            result.outputs["preview"] = PipelineValue::ImageBytes(res.preview);
            result.order.Add("preview");
            result.outputs["layout"] = PipelineValue::Text(res.layout);
            result.order.Add("layout");
            for (auto& sprite : res.sprites)
            {
                String name = PipelineImageToUi::SpriteName(sprite.key);
                result.outputs[name] = PipelineValue::ImageBytes(sprite.png);
                result.order.Add(name);
            }
            co_return result;
        }
    };

    Coroutine<bool> ImageToUiExport(Ref<PipelineExecContext> ctx, PipelineImageToUiConfig cfg, String layout, Vector<ImageToUiSprite> sprites)
    {
        ExportRequest request;
        request.assetsPath = ctx->assetsPath;
        request.folder = cfg.assetPath;
        request.name = cfg.screenName;
        request.layout = layout;
        request.sprites = sprites;

        ExportResult exported = co_await ExportPrototype(request);
        if (!exported.ok)
        {
            ctx->Log("imageToUi: export failed: " + exported.error);
            co_return false;
        }

        ctx->assetsChanged = true;
        ctx->Log("imageToUi: prototype " + (exported.prototype.IsEmpty() ? String("not written") : "Assets/" + exported.prototype) + ", " +
                 (String)exported.files.Count() + " files, " + (String)exported.widgets + " widgets");
        for (auto& warning : exported.warnings)
            ctx->Log("imageToUi: prototype: " + warning);
        co_return true;
    }

    void RegisterImageToUiNode()
    {
        PipelineNodeRegistry::Register(mmake<ImageToUiNode>());
    }
}
