#include "o2Editor/stdafx.h"
#include "o2Editor/Pipeline/Nodes/UiNodes.h"

#include "o2Editor/Pipeline/Nodes/PipelineNodesCommon.h"
#include "o2Editor/Pipeline/PipelinePngMeta.h"
#include "o2Editor/Pipeline/UiDoc/UiDocDevice.h"
#include "o2Editor/Pipeline/UiDoc/UiDocJs.h"
#include "o2Editor/Pipeline/UiDoc/UiDocOps.h"
#include "o2Editor/Pipeline/UiDoc/UiDocOwn.h"
#include "o2Editor/Pipeline/UiDoc/UiDocRender.h"
#include "o2Editor/Pipeline/UiDoc/UiKitPrep.h"
#include "o2Editor/Pipeline/UiDoc/UiStyleAi.h"
#include "o2Editor/Pipeline/UiDoc/UiStyleCheck.h"
#include "o2Editor/Pipeline/UiDoc/UiTextStyle.h"

namespace Editor
{
    namespace PipelineUiNodes
    {
        PipelineValue RenderLayoutPicture(const UiDoc::UiDoc& doc, const UiDoc::UiKit* kit)
        {
            bool skinned = kit && (!kit->elements.IsEmpty() || !kit->textStyles.IsEmpty());
            UiDoc::UiRenderOptions options;
            options.mode = skinned ? "kit" : "lines";
            options.kit = skinned ? kit : nullptr;
            auto picture = UiDoc::RenderLayout(doc, options);
            if (picture.IsEmpty())
                return PipelineValue();

            // The screen carries the layout's density, like the parts it was built from
            return PipelineValue::ImageBytes(PipelinePngMeta::WithDpi(picture.Png(), UiDoc::DocDpi(doc)));
        }

        PipelineValue OwnAssetPicture(const UiDoc::UiOwnAsset& asset)
        {
            String png = PipelineUtils::ReadFileBytes(PipelineUtils::GetUploadPath(asset.uploadId));
            if (png.IsEmpty())
                return PipelineValue();
            return PipelineValue::ImageBytes(PipelinePngMeta::WithDpi(png, asset.dpi));
        }

        std::optional<UiDoc::UiKit> OwnKit(const UiDoc::Json& config)
        {
            auto own = UiDoc::OwnKitOf(UiDoc::StepsOf(config));
            if (own.assets.IsEmpty() && own.styles.IsEmpty())
                return std::nullopt;

            UiDoc::UiKit kit;
            kit.name = "own";
            for (auto& a : own.assets)
            {
                String png = PipelineUtils::ReadFileBytes(PipelineUtils::GetUploadPath(a.uploadId));
                if (!png.IsEmpty())
                    kit.elements.Add(UiDoc::OwnElement(a, PipelineUtils::Base64Encode(png)));
            }
            kit.textStyles = own.styles;
            return kit;
        }

        std::optional<UiDoc::UiKit> LayoutKit(const Vector<PipelineValue>& kitValues, const UiDoc::Json& config)
        {
            Vector<UiDoc::UiKit> parsed;
            for (auto& v : kitValues)
            {
                if (v.IsValid() && v.type == PipelinePortType::UiKit)
                    parsed.Add(UiDoc::ParseKit(v.data));
            }
            auto own = OwnKit(config);
            if (own)
                parsed.Add(*own);

            Vector<const UiDoc::UiKit*> kits;
            for (auto& k : parsed)
                kits.Add(&k);
            return UiDoc::MergeKits(kits);
        }

        Vector<PipelineValue> KitInputs(const PipelineNode& node, const Map<String, PipelineValue>& inputsById,
                                        const Map<String, PipelineValue>& inputsByName)
        {
            Vector<PipelineValue> out;
            for (auto& port : node.inputs)
            {
                if (port.portType != PipelinePortType::UiKit)
                    continue;
                PipelineValue v;
                if (!inputsById.TryGetValue(port.id, v))
                    inputsByName.TryGetValue(port.name, v);
                if (v.IsValid())
                    out.Add(v);
            }
            return out;
        }

        String TextStyleOutput(const PipelineNode& node)
        {
            return UiDoc::TextStyleToJson(UiDoc::TextStyleFromConfig(node.config)).Dump();
        }

        // Unique within the kit, case-insensitively: "name", "name_2"...
        static String Unique(Vector<String>& taken, const String& base)
        {
            String name = base;
            for (int k = 2; taken.Contains(name.ToLowerCase()); k++)
                name = base + "_" + (String)k;

            taken.Add(name.ToLowerCase());
            return name;
        }

        static bool PictureSize(const String& bytes, int& w, int& h)
        {
            if (PipelinePngMeta::Size(bytes, w, h))
                return true;

            UiLayout::UiImage image;
            if (!UiLayout::UiImage::Decode(bytes, image))
                return false;

            w = image.w;
            h = image.h;
            return true;
        }

        UiDoc::UiKit BuildKit(const PipelineNode& node, const Map<String, PipelineValue>& inputsById,
                              const Function<void(const String&)>& log)
        {
            auto items = UiDoc::KitItems(node.config);
            auto itemOf = [&](const String& portId) -> UiDoc::UiKitItem
            {
                for (auto& kv : items)
                {
                    if (kv.first == portId)
                        return kv.second;
                }
                return UiDoc::UiKitItem();
            };

            UiDoc::UiKit kit;
            auto target = UiDoc::TargetOfConfig(UiDoc::FromDataValue(node.config));
            kit.device = target;
            // Text styles are written for a 1080-px-wide screen: here they come to the device's pixels
            double styleScale = std::min(target.w, target.h)/UiDoc::TEXT_STYLE_SCREEN_W;
            Vector<String> names;
            int i = 0;
            for (auto& port : node.inputs)
            {
                i++;
                auto it = inputsById.find(port.id);
                if (it == inputsById.end() || !it->second.IsValid())
                    continue;

                const PipelineValue& value = it->second;
                UiDoc::UiKitItem item = itemOf(port.id);
                if (value.type == PipelinePortType::TextStyle)
                {
                    UiLayout::Json raw;
                    if (!UiLayout::Json::Parse(value.data.IsEmpty() ? String("{}") : value.data, raw))
                        raw = UiLayout::Json::Object();

                    auto style = UiDoc::ScaleTextStyle(UiDoc::NormalizeTextStyle(raw), styleScale);
                    // The style keeps its own name unless the kit renames it
                    UiLayout::Json name(!item.name.IsEmpty() ? item.name : !style.name.IsEmpty() ? style.name : port.name);
                    style.name = UiDoc::CleanKitName(&name, "style_" + (String)i);
                    kit.textStyles.Add(style);
                    continue;
                }

                if (!value.IsImage())
                    continue;

                UiLayout::Json rawName(!item.name.IsEmpty() ? item.name : port.name);
                String name = Unique(names, UiDoc::CleanKitName(&rawName, "element_" + (String)i));
                String type = UiDoc::UiElementTypes().Contains(item.type) ? item.type : UiDoc::GuessElementType(name);
                String assetType = UiDoc::UiAssetTypes().Contains(item.assetType) ? item.assetType : UiDoc::DefaultAssetType(type);

                String inputBytes = value.GetPngBytes();
                String data = inputBytes;
                // Input pixels per pixel of the picture that is prepared (the AI may have redrawn it at another size)
                double redrawn = 1;
                if (item.bg == "ai" && !item.preparedUploadId.IsEmpty())
                {
                    bool safeId = !item.preparedUploadId.Contains("/") && !item.preparedUploadId.Contains("\\") &&
                        !item.preparedUploadId.Contains("..");
                    if (item.preparedFrom == UiDoc::InputHash(inputBytes) && safeId)
                    {
                        String prepared = PipelineUtils::ReadFileBytes(PipelineUtils::GetUploadPath(item.preparedUploadId));
                        if (!prepared.IsEmpty())
                        {
                            data = prepared;
                            int aw = 0, ah = 0, bw = 0, bh = 0;
                            if (PictureSize(inputBytes, aw, ah) && PictureSize(prepared, bw, bh) && bw > 0)
                                redrawn = (double)aw/bw;
                        }
                        else if (log)
                            log("uiKit . " + name + ": the picture without its background is gone - using the input");
                    }
                    else if (log)
                        log("uiKit . " + name + ": the input changed since its background was removed - using the input as is");
                }

                UiDoc::UiPrepInput prepIn;
                prepIn.data = data;
                prepIn.assetType = assetType;
                prepIn.key = item.bg == "key";
                prepIn.trim = item.trim;
                prepIn.optimize = item.optimize;
                prepIn.nineSlice = UiDoc::NormalizeNine(&item.nineSlice);

                UiDoc::UiPrepOutput prep;
                if (!UiDoc::PrepareElement(prepIn, prep))
                {
                    if (log)
                        log("uiKit . " + name + ": the input is not a picture");
                    continue;
                }

                // The element's real size: its whole picture (trimmed, before a centre is collapsed) on the device's screen
                const UiDoc::Json* trimBox = UiDoc::JsGet(UiDoc::JsGet(prep.source, "trim"), "w") ? UiDoc::JsGet(prep.source, "trim") : nullptr;
                double naturalW = trimBox ? UiDoc::JsNumber(UiDoc::JsGet(trimBox, "w")) : prep.width;
                double naturalH = trimBox ? UiDoc::JsNumber(UiDoc::JsGet(trimBox, "h")) : prep.height;
                double wholeW = naturalW*redrawn, wholeH = naturalH*redrawn;
                auto partScale = PipelinePngMeta::ReadScale(inputBytes);
                auto tagged = PipelinePngMeta::ReadDpi(inputBytes);
                int fromW = 0, fromH = 0;
                bool fromSource = PipelinePngMeta::ReadSource(inputBytes, fromW, fromH);

                double onW = wholeW, onH = wholeH;
                String sizeFrom = "pixels";
                std::optional<double> share;
                if (item.size)
                {
                    onW = item.size->w*target.dpi;
                    onH = item.size->h*target.dpi;
                    sizeFrom = "item";
                }
                else if (partScale && fromSource)
                {
                    // Cut from a picture: that picture is the device's screen, shown whole on it
                    double k = UiDoc::SourceFit(target.Screen(), fromW, fromH) / *partScale;
                    onW = wholeW*k;
                    onH = wholeH*k;
                    sizeFrom = "source";
                    share = UiDoc::JsRound(wholeW / *partScale / fromW*1000)/1000;
                }
                else if (tagged)
                {
                    onW = wholeW*target.dpi / *tagged;
                    onH = wholeH*target.dpi / *tagged;
                    sizeFrom = "tag";
                }

                // The texture: the element at its density (the device's, unless it has its own). Never enlarged - a picture
                // with fewer pixels than that stays as it is, and its density says so
                std::optional<double> own = item.dpi;
                double want = own ? *own : target.dpi;
                double fx = onW*want/target.dpi/naturalW, fy = onH*want/target.dpi/naturalH;
                double width = prep.width, height = prep.height;
                auto nine = prep.nineSlice;
                String png = prep.png;
                double dpi = want;
                bool soft = false;
                if (std::max(fx, fy) > 1.02)
                {
                    dpi = want/std::max(fx, fy);
                    soft = true;
                }
                else if (std::min(fx, fy) < 0.98)
                {
                    width = std::max(1.0, UiDoc::JsRound(prep.width*fx));
                    height = std::max(1.0, UiDoc::JsRound(prep.height*fy));
                    if (nine)
                    {
                        nine = UiDoc::UiNine{ UiDoc::JsRound(nine->left*fx), UiDoc::JsRound(nine->top*fy), UiDoc::JsRound(nine->right*fx),
                                              UiDoc::JsRound(nine->bottom*fy) };
                        // The stretching middle keeps a pixel of its own
                        width = std::max(width, nine->left + nine->right + 1);
                        height = std::max(height, nine->top + nine->bottom + 1);
                    }
                    if (width != prep.width || height != prep.height)
                        png = prep.image.Resize((int)width, (int)height).Png();
                }

                auto r3 = [](double v) { return UiDoc::JsRound(v*1000)/1000; };
                UiDoc::UiKitElement el;
                el.name = name;
                el.type = type;
                el.assetType = assetType;
                el.width = width;
                el.height = height;
                el.png = PipelineUtils::Base64Encode(png);
                el.source = prep.source;
                el.port = port.id;
                auto clean = UiDoc::CleanDpi(dpi);
                el.dpi = clean ? *clean : target.dpi;
                el.dpiFrom = String(own ? "item" : "kit");
                el.inches = UiDoc::UiPair{ r3(onW/target.dpi), r3(onH/target.dpi) };
                el.sizeFrom = sizeFrom;
                el.share = share;
                el.soft = soft;
                if (width != prep.width || height != prep.height)
                    el.texture = UiDoc::UiPair{ prep.width, prep.height };
                if (!item.variant.IsEmpty())
                    el.variant = UiDoc::JsSlice(item.variant, 0, 40);
                UiDoc::Json purposeValue(item.purpose);
                String purpose = UiDoc::CleanPurpose(&purposeValue);
                if (!purpose.IsEmpty())
                    el.purpose = purpose;
                if (!item.description.IsEmpty())
                    el.description = UiDoc::JsSlice(item.description, 0, 300);
                if (nine)
                    el.nineSlice = nine;
                if (prep.detected && item.nineSlice.IsNull() && el.source.IsObject())
                    el.source["detected"] = UiDoc::NineToJson(*prep.detected);

                kit.elements.Add(el);
            }

            UiLayout::Json kitName;
            if (auto raw = node.GetConfigValue("kitName"))
                kitName = UiDoc::FromDataValue(*raw);
            kit.name = UiDoc::CleanKitName(kitName.IsNull() ? nullptr : &kitName, "kit");
            return kit;
        }
    }

    // A text style for UI labels: its config IS the style, the output is the style as JSON
    class TextStyleNode : public PipelineNodeBase
    {
    public:
        TextStyleNode()
        {
            mSchema.type = "textStyle";
            mSchema.label = "Text style";
            mSchema.category = PipelineNodeCategory::Source;
            mSchema.instant = true;
            mSchema.description = "A text style for UI labels: font, size, colour or gradient, outline, shadow, spacing, case - set by hand or generated by AI from a request (pictures added as inputs are references). Connect it to a UI kit; the AI UI layout draws labels with it.";
            mSchema.outputs = { Out("style", PipelinePortType::TextStyle) };
            // Reference pictures for its AI (the lettering to match); the style itself never reads them
            mSchema.addableInputs = { PipelinePortType::Image };
            mSchema.defaultSize = Vec2F(360, 0);
        }

        void InitNode(const Ref<PipelineNode>& node) const override
        {
            node->SetConfigString("name", "title");
            node->SetConfigString("role", "title");
            node->SetConfigString("font", "Roboto");
            node->SetConfigNumber("size", 64);
        }

        Coroutine<PipelineRunResult> Run(const Ref<PipelineExecContext>& ctx, const Map<String, PipelineValue>& inputs,
                                         const Ref<PipelineNode>& node) override
        {
            co_return PipelineRunResult::Single(PipelineValue::TextOf(PipelinePortType::TextStyle, PipelineUiNodes::TextStyleOutput(*node)),
                                                "style");
        }
    };

    // Reads the text style of lettering in a picture: one vision call; the name and role set in the node are a local step
    // over the stored answer, and the check against the picture is a second step keyed on its own
    class TextStyleExtractNode : public PipelineNodeBase
    {
    public:
        TextStyleExtractNode()
        {
            mSchema.type = "textStyleExtract";
            mSchema.label = "AI text style";
            mSchema.category = PipelineNodeCategory::AI;
            mSchema.description = "Read the text style of lettering in a picture: the closest bundled font, size, flat or gradient fill and its layer effects - outlines, depth, shadows, glow, inner shadows, bevel - spacing, case. Frame the lettering on the picture to read just that part; optionally the result is drawn, compared with the picture and corrected. The result is a text style - connect it to a UI kit like a Text style node. A hint says which lettering to take when the picture has several.";
            mSchema.inputs = { In("image", PipelinePortType::Image) };
            mSchema.outputs = { Out("style", PipelinePortType::TextStyle) };
            mSchema.defaultSize = Vec2F(360, 0);
        }

        void InitNode(const Ref<PipelineNode>& node) const override
        {
            node->SetConfigString("model", UiDoc::TEXT_STYLE_AI_DEFAULT_MODEL);
        }

        Vector<String> ContentExcludedKeys(const PipelineNode& node) const override
        {
            Vector<String> keys = { "name", "role" };
            keys.Add(PipelineGraph::GetRefCheckKeys());
            return keys;
        }

        PipelineValue PostStep(const PipelineNode& node, const PipelineValue& value) const override
        {
            if (!value.IsTextLike())
                return value;
            String styled = UiDoc::WithStyleOverrides(value.data, node.GetConfigString("name", ""), node.GetConfigString("role", ""));
            return PipelineValue::TextOf(PipelinePortType::TextStyle, styled);
        }

        // The check against the picture: a paid second step over the stored style
        String RefineKey(const PipelineNode& node, const String& portId) const override
        {
            return UiDoc::StyleCheckKey(node);
        }

        Coroutine<PipelineRefineResult> Refine(Ref<PipelineExecContext> ctx, Map<String, PipelineValue> inputs, Ref<PipelineNode> node,
                                               PipelineValue stored, String key) override
        {
            PipelineRefineResult res;
            res.value = stored;
            auto image = inputs.find("image");
            if (!stored.IsTextLike() || image == inputs.end() || !image->second.IsImage())
                co_return res;

            auto outcome = co_await UiDoc::RunStyleCheck(ctx, node, image->second.GetPngBytes(), stored.data, key);
            res.value = PipelineValue::TextOf(PipelinePortType::TextStyle, outcome.data);
            res.keep = outcome.final;
            co_return res;
        }

        void RefineReplayed(const PipelineExecContext& ctx, const PipelineNode& node, const String& key) const override
        {
            UiDoc::ReplayStyleCheck(ctx, key);
        }

        Coroutine<PipelineRunResult> Run(const Ref<PipelineExecContext>& ctx, const Map<String, PipelineValue>& inputs,
                                         const Ref<PipelineNode>& node) override
        {
            auto image = Input(inputs, "image");
            if (!image)
                co_return PipelineRunResult::Fail("textStyleExtract: input \"image\" is not connected");
            if (!image->IsImage())
                co_return PipelineRunResult::Fail("textStyleExtract: input must be an image");

            String model = UiDoc::JsTrim(node->GetConfigString("model", ""));
            if (model.IsEmpty())
                model = UiDoc::TEXT_STYLE_AI_DEFAULT_MODEL;
            String hint = UiDoc::JsTrim(node->GetConfigString("hint", ""));
            ctx->Log("textStyleExtract: model=" + model + (hint.IsEmpty() ? String() : " hint=\"" + UiDoc::JsSlice(hint, 0, 80) + "\""));

            std::optional<PipelineImageOps::CropRect> frame;
            PipelineImageOps::CropRect crop;
            if (PipelineImageOps::ParseCrop(node->GetConfigValue("srcCrop"), crop))
                frame = crop;

            auto res = co_await UiDoc::ExtractTextStyle(ctx, image->GetPngBytes(), hint, model, frame, node->GetConfigString("name", ""),
                                                        node->GetConfigString("role", ""));
            if (!res.ok)
                co_return PipelineRunResult::Fail(res.error);

            auto style = UiDoc::NormalizeTextStyle(res.style);
            String look = UiDoc::StyleLook(style);
            ctx->Log("textStyleExtract: " + style.name + " - " + style.font + " " + UiDoc::JsNum(style.weight) + ", " +
                     UiDoc::JsNum(style.size) + "px, " + style.color + (look.IsEmpty() ? String() : ", " + look));
            co_return PipelineRunResult::Single(PipelineValue::TextOf(PipelinePortType::TextStyle, res.style.Dump()), "style");
        }
    };

    // The graphics of one interface style with their meta, and text styles, as one value
    class UiKitNode : public PipelineNodeBase
    {
    public:
        UiKitNode()
        {
            mSchema.type = "uiKit";
            mSchema.label = "UI kit";
            mSchema.category = PipelineNodeCategory::Transform;
            mSchema.instant = true;
            mSchema.partialInputs = true;
            mSchema.description = "Group the graphics of one UI style - buttons, panels, frames, icons, bars - with their meta: element type, asset type, nine-slice borders, name and where it is used; add text styles. Set it up by hand or with AI (it also cuts backgrounds and trims nine-slices). The AI UI layout draws with it.";
            mSchema.outputs = { Out("kit", PipelinePortType::UiKit) };
            mSchema.addableInputs = { PipelinePortType::Image, PipelinePortType::TextStyle };
            // The card is a grid of cells that takes as many columns as it is wide (PipelineKitLayout.h)
            mSchema.defaultSize = Vec2F(460, 0);
        }

        Coroutine<PipelineRunResult> Run(const Ref<PipelineExecContext>& ctx, const Map<String, PipelineValue>& inputs,
                                         const Ref<PipelineNode>& node) override
        {
            Map<String, PipelineValue> byId = ctx->inputsById;
            if (byId.empty())
            {
                for (auto& port : node->inputs)
                {
                    if (auto v = Input(inputs, port.name))
                        byId[port.id] = *v;
                }
            }

            auto kit = PipelineUiNodes::BuildKit(*node, byId, [&](const String& m) { ctx->Log(m); });
            ctx->Log("uiKit: " + (String)kit.elements.Count() + " graphic(s), " + (String)kit.textStyles.Count() + " text style(s)");
            co_return PipelineRunResult::Single(PipelineValue::TextOf(PipelinePortType::UiKit, UiDoc::KitToJson(kit).Dump()), "kit");
        }
    };

    // An adaptive screen layout made of edit steps; its outputs are the screen drawn with the kit and the layout as JSON
    class UiLayoutNode : public PipelineNodeBase
    {
    public:
        UiLayoutNode()
        {
            mSchema.type = "uiLayout";
            mSchema.label = "AI UI layout";
            mSchema.category = PipelineNodeCategory::AI;
            mSchema.instant = true;
            mSchema.partialInputs = true;
            mSchema.perPortRun = true;
            mSchema.description = "Lay out a game or app screen with an AI agent, or rebuild one from a mockup: an adaptive hierarchy of blocks (anchors, rows, columns, grids) kept as text. Connect UI kits and the screen is drawn with their graphics and text styles; what no kit has - graphics, text styles - the node makes itself, and every graphic it made is an output of its own. Every action is a step you can undo. Saves the screen into the project as an o2 prototype with its sprites, fonts and text styles.";
            mSchema.inputs = { In("kit", PipelinePortType::UiKit) };
            mSchema.outputs = { Out("image", PipelinePortType::Image), Out("layout", PipelinePortType::Text) };
            mSchema.addableInputs = { PipelinePortType::Image, PipelinePortType::Text, PipelinePortType::UiKit };
            mSchema.defaultSize = Vec2F(520, 0);
        }

        void InitNode(const Ref<PipelineNode>& node) const override
        {
            node->SetConfigString("agentModel", "gpt-6.1-sol");
        }

        // Where Save to project writes changes nothing the node draws
        Vector<String> SignatureExcludedKeys() const override
        {
            return { "assetPath" };
        }

        // The graphic behind one of the node's outputs; none for its schema outputs and a graphic that is gone
        static std::optional<UiDoc::LayoutOwnPort> OwnPortOf(const PipelineNode& node, const String& portId)
        {
            auto port = node.outputs.FindOrDefault([&](const PipelinePort& p) { return p.id == portId; });
            if (port.id.IsEmpty() || UiDoc::IsLayoutFixedOutput(port.name))
                return std::nullopt;
            return UiDoc::LayoutOwnPortByName(UiDoc::FromDataValue(node.config), port.name);
        }

        String PortCacheVariant(const PipelineNode& node, const String& portId) const override
        {
            // A graphic's output is its stored picture: the steps around it are not part of its key
            if (auto own = OwnPortOf(node, portId))
                return UiDoc::OwnGraphicSig(*own);
            return "port:" + (portId.IsEmpty() ? String("image") : portId);
        }

        Vector<String> PortCacheExcludedKeysOf(const PipelineNode& node, const String& portId) const override
        {
            if (OwnPortOf(node, portId))
                return { "steps" };
            return {};
        }

        // The schema outputs as the node has them, then one image output per graphic the node made for itself (derived from
        // the steps, like an AI extract's parts from its regions); a graphic's output the node had keeps its port id
        bool SyncPorts(const Ref<PipelineNode>& node) const override
        {
            Vector<PipelinePort> fixed;
            Vector<Pair<String, String>> had;
            for (auto& p : node->outputs)
            {
                if (UiDoc::IsLayoutFixedOutput(p.name))
                    fixed.Add(p);
                else
                    had.Add(Pair<String, String>(p.name, p.id));
            }
            for (auto& port : mSchema.outputs)
            {
                if (!fixed.Any([&](const PipelinePort& p) { return p.name == port.name; }))
                    fixed.Add(PipelinePort(PipelineNode::GenerateId(), port.name, port.portType, false));
            }

            Vector<PipelinePort> outputs = fixed;
            for (auto& own : UiDoc::LayoutOwnOutputs(UiDoc::FromDataValue(node->config), had))
                outputs.Add(PipelinePort(own.id, own.name, PipelinePortType::Image, false));

            bool same = outputs.Count() == node->outputs.Count();
            for (int i = 0; same && i < outputs.Count(); i++)
                same = outputs[i].id == node->outputs[i].id && outputs[i].name == node->outputs[i].name;
            if (!same)
                node->outputs = outputs;
            return true;
        }

        Coroutine<PipelineRunResult> Run(const Ref<PipelineExecContext>& ctx, const Map<String, PipelineValue>& inputs,
                                         const Ref<PipelineNode>& node) override
        {
            String want = ctx->outputPort;
            PipelineRunResult result;

            // One of the node's own graphics, asked for alone: nothing else is read
            if (!want.IsEmpty() && !UiDoc::IsLayoutFixedOutput(want))
            {
                auto own = UiDoc::LayoutOwnPortByName(UiDoc::FromDataValue(node->config), want);
                if (!own)
                    co_return PipelineRunResult::Fail("uiLayout: the layout has no graphic \"" + want + "\" at its current step");
                PipelineValue picture = PipelineUiNodes::OwnAssetPicture(own->asset);
                if (!picture.IsValid())
                    co_return PipelineRunResult::Fail("uiLayout: the picture of the graphic \"" + want + "\" is gone");
                result.outputs[want] = picture;
                co_return result;
            }

            // Every kit wired in, in port order (two added inputs may share a name: by id), then the node's own
            auto merged = PipelineUiNodes::LayoutKit(PipelineUiNodes::KitInputs(*node, ctx->inputsById, inputs),
                                                     UiDoc::FromDataValue(node->config));
            UiDoc::UiKit kit = merged ? *merged : UiDoc::UiKit();
            bool hasKit = merged.has_value();

            auto applied = UiDoc::DocAt(node->config);
            if (!applied.errors.IsEmpty())
            {
                String first;
                for (int i = 0; i < applied.errors.Count() && i < 3; i++)
                    first += (i ? "; " : "") + applied.errors[i];
                ctx->Log("uiLayout: " + (String)applied.errors.Count() + " edit(s) did not apply: " + first);
            }


            if (want.IsEmpty() || want == "image")
            {
                PipelineValue picture = PipelineUiNodes::RenderLayoutPicture(applied.doc, hasKit ? &kit : nullptr);
                if (!picture.IsValid())
                    co_return PipelineRunResult::Fail("uiLayout: the layout could not be drawn");

                result.outputs["image"] = picture;
            }

            if (want.IsEmpty() || want == "layout")
                result.outputs["layout"] = PipelineValue::Text(UiDoc::DocToJson(applied.doc).Dump(2));

            // A run of the whole node: the graphics behind the ports it has; one whose picture is gone is skipped
            if (want.IsEmpty())
            {
                auto config = UiDoc::FromDataValue(node->config);
                for (auto& port : node->outputs)
                {
                    if (UiDoc::IsLayoutFixedOutput(port.name))
                        continue;
                    auto own = UiDoc::LayoutOwnPortByName(config, port.name);
                    PipelineValue picture = own ? PipelineUiNodes::OwnAssetPicture(own->asset) : PipelineValue();
                    if (picture.IsValid())
                        result.outputs[port.name] = picture;
                    else
                        ctx->Log("uiLayout: no picture for the output \"" + port.name + "\"");
                }
            }

            co_return result;
        }
    };

    void RegisterUiNodes()
    {
        PipelineNodeRegistry::Register(mmake<TextStyleNode>());
        PipelineNodeRegistry::Register(mmake<TextStyleExtractNode>());
        PipelineNodeRegistry::Register(mmake<UiKitNode>());
        PipelineNodeRegistry::Register(mmake<UiLayoutNode>());
    }
}
