#include "o2Editor/stdafx.h"
#include "UiDocExport.h"

#include <cmath>
#include <functional>

#include "o2Editor/Pipeline/UiDoc/UiDocFp.h"
#include "o2Editor/Pipeline/UiLayout/UiLayoutPixels.h"

namespace Editor::UiDoc
{
    String ExportFolderOf(const String& assetPath)
    {
        String folder = JsTrim(assetPath);
        folder.ReplaceAll("\\", "/");
        while (StartsWith(folder, "/"))
            folder = folder.SubStr(1);
        while (UiLayout::HasSuffix(folder, "/"))
            folder = folder.SubStr(0, folder.Length() - 1);
        return folder.IsEmpty() ? String("Generated/UI") : folder;
    }

    static String ScreenNameOf(const UiDoc& doc)
    {
        return doc.name.IsEmpty() ? String("Screen") : doc.name;
    }

    String ExportPrototypePathOf(const UiDoc& doc, const String& assetPath)
    {
        return ExportFolderOf(assetPath) + "/" + ScreenNameOf(doc) + ".proto";
    }

    UiLayout::ExportRequest PrototypeRequest(const UiDoc& doc, const UiStructureResult& structure, const String& assetPath,
                                             const String& assetsPath, const String& seed)
    {
        UiLayout::ExportRequest request;
        request.assetsPath = assetsPath;
        request.folder = ExportFolderOf(assetPath);
        request.name = ScreenNameOf(doc);
        request.seed = seed;

        // o2 draws a sliced sprite's borders in its own pixels: a sprite drawn for another density goes resampled to the
        // layout's, its borders with it
        std::map<std::string, double> scaled;
        for (auto& s : structure.sprites)
        {
            UiLayout::ImageToUiSprite sprite;
            sprite.key = UiLayout::HasSuffix(s.file, ".png") ? s.file.SubStr(0, s.file.Length() - 4) : s.file;
            sprite.png = s.png;
            double k = 1;
            if (s.density && std::fabs(*s.density - 1) >= 0.01 && *s.density > 0)
            {
                k = 1 / *s.density;
                UiLayout::UiImage img;
                if (UiLayout::UiImage::Decode(s.png, img))
                {
                    int w = std::max(1, (int)JsRound(s.width * k)), h = std::max(1, (int)JsRound(s.height * k));
                    sprite.png = img.Resize(w, h).Png();
                    scaled[JsToLower(s.file).Data()] = k;
                }
                else
                    k = 1;
            }
            if (s.nineSlice)
            {
                UiLayout::NineSlice nine;
                nine.left = (int)JsRound(s.nineSlice->left * k);
                nine.top = (int)JsRound(s.nineSlice->top * k);
                nine.right = (int)JsRound(s.nineSlice->right * k);
                nine.bottom = (int)JsRound(s.nineSlice->bottom * k);
                sprite.nine = nine;
            }
            request.sprites.Add(sprite);
        }

        if (scaled.empty() && structure.textStyles.IsEmpty())
        {
            request.layout = structure.json;
            return request;
        }

        // The borders a node asks for are in the sprite's own pixels: they follow the resampled sprite (the web passes
        // them unscaled, which draws the corners of a denser sprite too large). A styled text gets its look
        Json layout = structure.structure;
        std::function<void(Json&)> walk = [&](Json& v)
        {
            if (v.IsArray())
            {
                for (auto& item : v.Items())
                    walk(item);
                return;
            }
            if (!v.IsObject())
                return;
            const Json* asset = v.Get("asset");
            Json* nine = v.Get("nineSlice");
            if (asset && asset->IsString() && nine && nine->IsObject())
            {
                String file = asset->Str();
                int slash = file.FindLast("/");
                if (slash >= 0)
                    file = file.SubStr(slash + 1);
                auto it = scaled.find(JsToLower(file).Data());
                if (it != scaled.end())
                {
                    for (auto& side : nine->Members())
                    {
                        if (side.second.IsNumber())
                            side.second = Json(JsRound(side.second.Num() * it->second));
                    }
                }
            }
            const Json* type = v.Get("type");
            const Json* name = v.Get("name");
            Json look;
            if (type && type->IsString() && type->Str() == "text" && name && name->IsString() &&
                structure.textStyles.TryGetValue(name->Str(), look))
            {
                v["textStyle"] = look;
            }
            for (auto& m : v.Members())
                walk(m.second);
        };
        walk(layout);
        request.layout = layout.Dump(2);
        return request;
    }

    Coroutine<UiLayoutExportResult> ExportLayout(UiLayoutExportRequest request)
    {
        UiLayoutExportResult res;
        UiStructureResult structure = ToUiStructure(request.doc, request.kit ? &*request.kit : nullptr);
        auto exported = co_await UiLayout::ExportPrototype(PrototypeRequest(request.doc, structure, request.assetPath,
                                                                            request.assetsPath, request.nodeId));
        if (!exported.ok)
        {
            res.error = exported.error;
            co_return res;
        }

        res.warnings = structure.warnings;
        for (auto& warning : exported.warnings)
        {
            if (!res.warnings.Contains(warning))
                res.warnings.Add(warning);
        }
        res.written = exported.files;
        res.prototype = exported.prototype;
        res.ok = true;
        co_return res;
    }
}
