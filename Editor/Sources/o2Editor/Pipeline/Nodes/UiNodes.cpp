#include "o2Editor/stdafx.h"
#include "o2Editor/Pipeline/Nodes/UiNodes.h"

#include "o2Editor/Pipeline/Nodes/PipelineNodesCommon.h"
#include "o2Editor/Pipeline/PipelinePngMeta.h"
#include "o2Editor/Pipeline/UiDoc/UiDocDevice.h"
#include "o2Editor/Pipeline/UiDoc/UiDocJs.h"
#include "o2Editor/Pipeline/UiDoc/UiDocOps.h"
#include "o2Editor/Pipeline/UiDoc/UiDocRender.h"
#include "o2Editor/Pipeline/UiDoc/UiKitPrep.h"
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

                    auto style = UiDoc::NormalizeTextStyle(raw);
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
                // The density: set in the kit, else what the input carries (an AI extract part's), else the base
                std::optional<double> own = item.dpi;
                std::optional<double> dpi = own ? own : PipelinePngMeta::ReadDpi(inputBytes);
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
                            // The model may have drawn the picture without its background at another size
                            int aw = 0, ah = 0, bw = 0, bh = 0;
                            if (!own && dpi && PictureSize(inputBytes, aw, ah) && PictureSize(prepared, bw, bh) && aw > 0)
                                dpi = *dpi * bw / aw;
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

                UiDoc::UiKitElement el;
                el.name = name;
                el.type = type;
                el.assetType = assetType;
                el.width = prep.width;
                el.height = prep.height;
                el.png = PipelineUtils::Base64Encode(prep.png);
                el.source = prep.source;
                el.port = port.id;
                auto clean = dpi ? UiDoc::CleanDpi(*dpi) : std::nullopt;
                el.dpi = clean ? *clean : UiDoc::UI_BASE_DPI;
                el.dpiFrom = String(own ? "item" : dpi ? "input" : "base");
                if (!item.variant.IsEmpty())
                    el.variant = UiDoc::JsSlice(item.variant, 0, 40);
                if (!item.description.IsEmpty())
                    el.description = UiDoc::JsSlice(item.description, 0, 300);
                if (prep.nineSlice)
                    el.nineSlice = prep.nineSlice;
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
            mSchema.defaultSize = Vec2F(760, 0);
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
            mSchema.description = "Lay out a game or app screen with an AI agent: an adaptive hierarchy of blocks (anchors, rows, columns, grids) kept as text and drawn as line art; connect a UI kit and the screen is drawn with its graphics and text styles. Every agent action is a step you can undo. Exports a Unity prefab, an o2 prototype or JSON.";
            mSchema.inputs = { In("kit", PipelinePortType::UiKit) };
            mSchema.outputs = { Out("image", PipelinePortType::Image), Out("layout", PipelinePortType::Text) };
            mSchema.addableInputs = { PipelinePortType::Image, PipelinePortType::Text };
            mSchema.defaultSize = Vec2F(520, 0);
        }

        void InitNode(const Ref<PipelineNode>& node) const override
        {
            node->SetConfigString("agentModel", "gpt-6.1-sol");
        }

        String PortCacheVariant(const PipelineNode& node, const String& portId) const override
        {
            return "port:" + (portId.IsEmpty() ? String("image") : portId);
        }

        Coroutine<PipelineRunResult> Run(const Ref<PipelineExecContext>& ctx, const Map<String, PipelineValue>& inputs,
                                         const Ref<PipelineNode>& node) override
        {
            UiDoc::UiKit kit;
            bool hasKit = false;
            if (auto kitValue = Input(inputs, "kit"); kitValue && kitValue->type == PipelinePortType::UiKit)
            {
                kit = UiDoc::ParseKit(kitValue->data);
                hasKit = true;
            }

            auto applied = UiDoc::DocAt(node->config);
            if (!applied.errors.IsEmpty())
            {
                String first;
                for (int i = 0; i < applied.errors.Count() && i < 3; i++)
                    first += (i ? "; " : "") + applied.errors[i];
                ctx->Log("uiLayout: " + (String)applied.errors.Count() + " edit(s) did not apply: " + first);
            }

            String want = ctx->outputPort;
            PipelineRunResult result;
            if (want.IsEmpty() || want == "image")
            {
                PipelineValue picture = PipelineUiNodes::RenderLayoutPicture(applied.doc, hasKit ? &kit : nullptr);
                if (!picture.IsValid())
                    co_return PipelineRunResult::Fail("uiLayout: the layout could not be drawn");

                result.outputs["image"] = picture;
            }

            if (want.IsEmpty() || want == "layout")
                result.outputs["layout"] = PipelineValue::Text(UiDoc::DocToJson(applied.doc).Dump(2));

            co_return result;
        }
    };

    void RegisterUiNodes()
    {
        PipelineNodeRegistry::Register(mmake<TextStyleNode>());
        PipelineNodeRegistry::Register(mmake<UiKitNode>());
        PipelineNodeRegistry::Register(mmake<UiLayoutNode>());
    }
}
