#include "o2Editor/stdafx.h"

#include <cmath>
#include <random>

#include "o2Editor/Pipeline/Nodes/PipelineNodesCommon.h"
#include "o2Editor/Pipeline/PipelineUtils.h"
#include "o2Editor/Pipeline/UiDoc/UiDocAiShared.h"
#include "o2Editor/Pipeline/UiDoc/UiKitPrep.h"
#include "o2Editor/Pipeline/UiDoc/UiDocFp.h"

// The UI kit node's AI (backend/src/routes/uiAi.ts /kit-analyze): one vision call for the pictures, flat backdrops keyed
// locally, complex ones removed by the image model through the AI remove background node, nine-slice from the AI only
// where the pixels show no stretch
namespace Editor::UiDoc
{
    String KitAnalyzePrompt(int n, const Vector<String>& names, const Vector<String>& sizes)
    {
        Vector<String> types;
        for (auto& t : UiElementTypes())
            types.Add(t + " (" + UiElementHint(t) + ")");

        Vector<String> lines = {
            "You are preparing graphics for the UI kit of a game or app: the pictures will be used as UI elements (buttons, panels, "
            "frames, icons, bars\xE2\x80\xA6).",
            JsNum(n) + " picture(s) follow, in this order:",
        };
        for (int i = 0; i < names.Count(); i++)
            lines.Add(JsNum(i + 1) + ". input named \"" + names[i] + "\", " + (i < sizes.Count() ? sizes[i] : String()) + " px");
        Vector<String> rest = {
            "",
            "For each picture decide:",
            "- name: short snake_case name for the kit (btn_green, panel_dark, frame_gold, icon_coin, bar_hp_fill); keep the input "
            "name when it is already a good one",
            "- type: one of " + Join(types, "; "),
            "- assetType: one of " + Join(UiAssetTypes(), ", ") + " \xE2\x80\x94 sliced (nine-slice: corners keep their size, the "
            "middle stretches) for surfaces that change size: buttons, panels, frames, bars, fields, tabs; simple for icons, "
            "pictures, characters and full backgrounds; tiled for repeating patterns",
            "- variant: optional, e.g. primary, secondary, disabled, selected, small (empty when not obvious)",
            "- description: one short sentence \xE2\x80\x94 what it is and where in an interface it is used",
            "- background: transparent (already cut out: transparent or checker-free around the element) | flat (one plain colour "
            "fills everything around the element) | complex (scenery, gradient or texture behind the element)",
            "- nineSlice (sliced only): how many pixels from each edge of the picture (at the size given above) belong to the "
            "corners and borders that must keep their size \xE2\x80\x94 everything between them is stretched. Put each line just "
            "inside the flat middle, past the rounding, bevel, rim or ornament; null for simple / tiled",
            "",
            "Answer with JSON only, an array in the same order:",
            "[{\"index\":1,\"name\":\"...\",\"type\":\"...\",\"assetType\":\"...\",\"variant\":\"\",\"description\":\"...\","
            "\"background\":\"...\",\"nineSlice\":{\"left\":0,\"top\":0,\"right\":0,\"bottom\":0}}]",
        };
        lines.Add(rest);
        return Join(lines, "\n");
    }

    Json ParseJsonArrayAnswer(const String& text)
    {
        // text.replace(/```[a-z]*\n?|```/gi, '')
        const std::string& s0 = text;
        std::string s;
        for (size_t i = 0; i < s0.size();)
        {
            if (s0.compare(i, 3, "```") == 0)
            {
                i += 3;
                while (i < s0.size() && ((s0[i] >= 'a' && s0[i] <= 'z') || (s0[i] >= 'A' && s0[i] <= 'Z')))
                    i++;
                if (i < s0.size() && s0[i] == '\n')
                    i++;
                continue;
            }
            s += s0[i++];
        }
        size_t a = s.find('['), b = s.rfind(']');
        Json out;
        if (a == std::string::npos || b == std::string::npos || b <= a || !Json::Parse(String(s.substr(a, b - a + 1)), out) ||
            !out.IsArray())
            return Json::Array();
        return out;
    }

    // A random version-4 UUID (an upload's name)
    static String NewUuid()
    {
        static std::mt19937_64 rng(std::random_device{}());
        static const char* hex = "0123456789abcdef";
        std::string out;
        for (int i = 0; i < 36; i++)
        {
            if (i == 8 || i == 13 || i == 18 || i == 23) { out += '-'; continue; }
            if (i == 14) { out += '4'; continue; }
            int v = (int)(rng() % 16);
            out += hex[i == 19 ? (v & 3) | 8 : v];
        }
        return String(out);
    }

    // The image model paints the background over with a key colour (the AI remove background node in chroma mode); the
    // colour is keyed out here, as the executor's post step does
    static Coroutine<AiBytesResult> RemoveBackgroundAi(Ref<PipelineExecContext> ctx, String model, String png, String hint)
    {
        AiBytesResult res;
        auto impl = PipelineNodeRegistry::Get("aiRemoveBg");
        auto node = PipelineNodeRegistry::CreateNode("aiRemoveBg", Vec2F());
        if (!impl || !node)
        {
            res.error = "no AI remove background node";
            co_return res;
        }
        node->SetConfigString("model", model);
        node->SetConfigString("hint", hint);
        node->SetConfigBool("transparentBg", true);
        node->SetConfigString("transparentMode", "chroma");

        Map<String, PipelineValue> inputs;
        inputs["image"] = PipelineValue::ImageBytes(png);
        PipelineRunResult run = co_await impl->Run(ctx, inputs, node);
        auto out = run.outputs.find("out");
        if (!run.ok || out == run.outputs.end() || !out->second.IsImage())
        {
            res.error = run.ok ? String("the image model returned no picture") : run.error;
            co_return res;
        }
        auto bitmap = out->second.GetBitmap();
        if (!bitmap)
        {
            res.error = "the image model returned no picture";
            co_return res;
        }
        // The web keys this node's chroma render (aiRemoveBg is one of its chroma node types); o2's UsesChromaPostStep
        // leaves the node out, so the cut is made here from the node's own transparency settings
        auto tr = PipelineTransparency::Read(*node);
        if (tr.transparent && tr.mode == "chroma")
            bitmap = PipelineTransparency::ApplyChromaPostStep(*node, *bitmap);
        res.ok = true;
        res.data = PipelineValue::Image(bitmap).GetPngBytes();
        res.mimeType = "image/png";
        co_return res;
    }

    Coroutine<UiKitAnalyzeResponse> AnalyzeUiKit(Ref<PipelineExecContext> ctx, UiKitAnalyzeRequest request)
    {
        UiKitAnalyzeResponse res;
        struct Pic
        {
            String portId, name, png, size;
        };
        Vector<Pic> pics;
        for (auto& p : request.pictures)
        {
            UiImage img;
            if (p.png.IsEmpty() || !UiImage::Decode(p.png, img))
                continue;
            pics.Add(Pic{ p.portId, p.name.IsEmpty() ? "element_" + JsNum(pics.Count() + 1) : p.name, p.png,
                          JsNum(img.w) + "x" + JsNum(img.h) });
        }
        if (pics.IsEmpty())
        {
            res.error = "Connect pictures to the kit (and run the nodes that make them) first";
            co_return res;
        }

        auto modelOf = [&](const String& given, const char* key, const char* def) {
            String m = JsTrim(given);
            if (m.IsEmpty())
                m = JsTrim(JsStringOr(JsGet(request.config, key)));
            return m.IsEmpty() ? String(def) : m;
        };
        String model = modelOf(request.model, "kitModel", UI_KIT_AI_DEFAULT_MODEL);
        String imageModel = modelOf(request.imageModel, "kitImageModel", UI_KIT_AI_DEFAULT_IMAGE_MODEL);
        res.model = model;

        // The sizes the model sees (its nine-slice answer is in those pixels)
        Vector<AiImageRef> images;
        Vector<UiSize> seen;
        Vector<String> names, sizes;
        for (auto& p : pics)
        {
            String small = PictureForModel(p.png, 768);
            UiImage img;
            UiImage::Decode(small, img);
            images.Add(AiImageRef{ "image/png", small });
            seen.Add(UiSize{ (double)std::max(1, img.w), (double)std::max(1, img.h) });
            names.Add(p.name);
            sizes.Add(JsNum(seen.Last().w) + "x" + JsNum(seen.Last().h));
        }

        AiTextResult answer = co_await UiAiText(ctx, model, KitAnalyzePrompt(pics.Count(), names, sizes), images);
        if (!answer.ok)
        {
            res.error = answer.error;
            co_return res;
        }

        Json rows = ParseJsonArrayAnswer(answer.text);
        for (int i = 0; i < pics.Count(); i++)
        {
            const Json* row = nullptr;
            for (auto& r : rows.Items())
            {
                if (JsNumber(JsGet(r, "index")) == i + 1)
                {
                    row = &r;
                    break;
                }
            }
            if (!row)
                row = rows.At(i);
            if (!row || !JsTruthy(row) || !JsIsObjectLike(row))
            {
                res.notes.Add(pics[i].name + ": the model said nothing about it");
                continue;
            }

            const Json* typeV = JsGet(row, "type");
            const Json* assetV = JsGet(row, "assetType");
            String type = typeV && typeV->IsString() && UiElementTypes().Contains(typeV->Str()) ? typeV->Str() : String();
            String assetType = assetV && assetV->IsString() && UiAssetTypes().Contains(assetV->Str()) ? assetV->Str() : String();
            String bgKind = JsToLower(JsStringOr(JsGet(row, "background")));
            auto aiNine = NormalizeNine(JsGet(row, "nineSlice"));

            Json item = Json::Object();
            Json inputName(pics[i].name);
            item["name"] = CleanKitName(JsGet(row, "name"), CleanKitName(&inputName, "element_" + JsNum(i + 1)));
            if (!type.IsEmpty())
                item["type"] = type;
            if (!assetType.IsEmpty())
                item["assetType"] = assetType;
            String variant = JsSlice(JsTrim(JsStringOr(JsGet(row, "variant"))), 0, 40);
            if (!variant.IsEmpty())
                item["variant"] = variant;
            String description = JsSlice(JsTrim(JsStringOr(JsGet(row, "description"))), 0, 300);
            if (!description.IsEmpty())
                item["description"] = description;
            item["bg"] = bgKind == "flat" ? "key" : "keep";
            item["ai"] = true;
            item["background"] = bgKind;
            String itemName = item["name"].Str();

            if (bgKind == "complex")
            {
                if (request.removeBg)
                {
                    String hint = !description.IsEmpty() ? description : itemName;
                    AiBytesResult cut = co_await RemoveBackgroundAi(ctx, imageModel, pics[i].png, hint);
                    if (cut.ok)
                    {
                        String id = NewUuid() + ".png";
                        if (PipelineUtils::WriteFileBytes(PipelineUtils::GetUploadsPath() + id, cut.data))
                        {
                            item["bg"] = "ai";
                            Json prepared = Json::Object();
                            prepared["uploadId"] = id;
                            prepared["from"] = InputHash(pics[i].png);
                            item["prepared"] = prepared;
                        }
                        else
                            res.notes.Add(itemName + ": the background could not be removed (the upload could not be written)");
                    }
                    else
                        res.notes.Add(itemName + ": the background could not be removed (" + cut.error + ")");
                }
                else
                    res.notes.Add(itemName + ": the background is not plain \xE2\x80\x94 remove it with AI (\xE2\x9C\xA6 with background) or upstream");
            }

            // Nine-slice: the pixels decide where they show a uniform stretch (exact); where they do not, the AI's
            // measurement does. Both uniform: left to the node
            String effectiveAsset = !assetType.IsEmpty() ? assetType : !type.IsEmpty() ? DefaultAssetType(type) : String();
            if (effectiveAsset == "sliced" && aiNine)
            {
                String pic = pics[i].png;
                if (item["bg"].Str() == "ai" && item.Has("prepared"))
                    pic = PipelineUtils::ReadFileBytes(PipelineUtils::GetUploadsPath() + item["prepared"]["uploadId"].Str());
                UiInputNine local;
                if (!DetectInputNine(pic, item["bg"].Str() == "key", local))
                    res.notes.Add(itemName + ": nine-slice not checked (the picture does not decode)");
                else if (!local.uniformX || !local.uniformY)
                {
                    double kx = local.w/seen[i].w, ky = local.h/seen[i].h;
                    UiNine nine;
                    nine.left = local.uniformX ? local.nine.left : JsRound(aiNine->left*kx);
                    nine.right = local.uniformX ? local.nine.right : JsRound(aiNine->right*kx);
                    nine.top = local.uniformY ? local.nine.top : JsRound(aiNine->top*ky);
                    nine.bottom = local.uniformY ? local.nine.bottom : JsRound(aiNine->bottom*ky);
                    if (nine.left + nine.right < local.w - 1 && nine.top + nine.bottom < local.h - 1)
                    {
                        item["nineSlice"] = NineToJson(nine);
                        item["nineFrom"] = "ai";
                    }
                }
            }
            res.items[pics[i].portId] = item;
        }
        res.ok = true;
        co_return res;
    }

    Json MergeKitAnalysis(const Json& config, const Json& items)
    {
        Json out = config.IsObject() ? config : Json::Object();
        const Json* curItems = JsGet(out, "items");
        Json next = curItems && curItems->IsObject() ? *curItems : Json::Object();
        if (!items.IsObject())
            return out;

        for (auto& m : items.Members())
        {
            const String& pid = m.first;
            Json clean = m.second;
            clean.Remove("background");
            const Json* curPtr = next.Get(pid);
            Json cur = curPtr && curPtr->IsObject() ? *curPtr : Json::Object();

            // A background removed earlier stays removed when this pass did not redo it
            bool keepAi = JsStringOr(JsGet(cur, "bg")) == "ai" && JsTruthy(JsGet(cur, "prepared")) && !JsTruthy(clean.Get("prepared"));
            // Borders someone set by hand stay; otherwise the AI's measurement (or none - the node finds
            // them) replaces the last one
            const Json* curNine = JsGet(cur, "nineSlice");
            bool handNine = JsTruthy(curNine) && JsStringOr(JsGet(cur, "nineFrom")) != "ai";
            Json merged = cur;
            for (auto& f : clean.Members())
                merged[f.first] = f.second;
            if (handNine)
            {
                merged["nineSlice"] = *curNine;
                merged.Remove("nineFrom");
            }
            else
            {
                const Json* aiNine = clean.Get("nineSlice");
                merged["nineSlice"] = JsNullish(aiNine) ? Json() : *aiNine;
                if (!JsTruthy(aiNine) || !clean.Has("nineFrom"))
                    merged.Remove("nineFrom");
            }
            if (keepAi)
            {
                merged["bg"] = "ai";
                merged["prepared"] = *JsGet(cur, "prepared");
            }
            next[pid] = merged;
        }
        out["items"] = next;
        return out;
    }

    void MergeKitAnalysis(DataValue& config, const Json& items)
    {
        Json merged = MergeKitAnalysis(FromDataValue(config), items);
        SetConfigJson(config, "items", *merged.Get("items"));
    }
}
