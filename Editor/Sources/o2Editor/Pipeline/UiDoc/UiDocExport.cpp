#include "o2Editor/stdafx.h"
#include "UiDocExport.h"

#include <cmath>
#include <functional>
#include <set>

#include "o2/Utils/FileSystem/FileSystem.h"
#include "o2Editor/Pipeline/PipelinePngMeta.h"
#include "o2Editor/Pipeline/PipelineUtils.h"
#include "o2Editor/Pipeline/UiDoc/UiDocFonts.h"
#include "o2Editor/Pipeline/UiDoc/UiDocUiml.h"
#include "o2Editor/Pipeline/UiLayout/UiLayoutPixels.h"
#include "o2Editor/Pipeline/UiDoc/UiDocFp.h"

namespace Editor::UiDoc
{
    const char* const UI_EXPORT_UNITY = "unity";
    const char* const UI_EXPORT_O2 = "o2";
    const char* const UI_EXPORT_JSON = "json";

    static const char* const kRoboto[] = { "Roboto-Regular.ttf", "Roboto-Medium.ttf", "Roboto-Bold.ttf", "LICENSE-Roboto.txt" };

    static bool IsRoboto(const String& file)
    {
        for (auto r : kRoboto)
        {
            if (file == r)
                return true;
        }
        return false;
    }

    static String CleanTarget(const String& target)
    {
        return target == UI_EXPORT_O2 || target == UI_EXPORT_JSON ? target : String(UI_EXPORT_UNITY);
    }

    // The node's assetPath as the folder inside Assets: trimmed, no leading or trailing slashes, Generated/UI when empty
    static String FolderOf(const String& assetPath)
    {
        String folder = JsTrim(assetPath);
        while (StartsWith(folder, "/"))
            folder = folder.SubStr(1);
        while (UiLayout::HasSuffix(folder, "/"))
            folder = folder.SubStr(0, folder.Length() - 1);
        return folder.IsEmpty() ? String("Generated/UI") : folder;
    }

    static String FontBytes(const String& file, Vector<String>* missing)
    {
        String home = UiFontsHome();
        String bytes = home.IsEmpty() ? String() : PipelineUtils::ReadFileBytes(home + "/" + file);
        if (bytes.IsEmpty() && missing)
            missing->Add(file);
        return bytes;
    }

    String ExportTargetOf(const Json& config)
    {
        const Json* v = config.IsObject() ? config.Get("exportFor") : nullptr;
        return CleanTarget(JsNullish(v) ? String() : JsString(v));
    }

    String ExportFileName(const UiDoc& doc, const String& target)
    {
        String t = CleanTarget(target);
        return t == UI_EXPORT_UNITY ? doc.name + ".unitypackage" : t == UI_EXPORT_O2 ? doc.name + "_o2.zip" : doc.name + ".zip";
    }

    Vector<UiExportFont> UnityExportFonts(const UiStructureResult& structure, Vector<String>* missing /*= nullptr*/)
    {
        // Unity gets every font the text styles use, with its licence
        Vector<String> used;
        for (auto& f : structure.fonts)
        {
            if (!IsRoboto(f))
                used.Add(f);
        }
        Vector<String> files;
        for (auto r : kRoboto)
            files.Add(r);
        files.Add(used);
        for (auto& font : UiFonts())
        {
            if (font.family != "Roboto" && font.faces.Any([&](const UiFontFace& x) { return used.Contains(x.file); }))
                files.Add(font.license);
        }

        Vector<UiExportFont> out;
        for (auto& file : files)
        {
            String data = FontBytes(file, missing);
            if (!data.IsEmpty())
                out.Add({ file, data });
        }
        return out;
    }

    UiExportInput UnityExportInput(const UiDoc& doc, const UiStructureResult& structure, const String& assetPath,
                                   const String& seed, const Vector<UiExportFont>& fonts)
    {
        UiExportInput input;
        input.name = doc.name;
        input.folder = FolderOf(assetPath);
        input.layout = structure.json;
        input.sprites = structure.sprites;
        input.fonts = fonts;
        input.seed = seed;
        // Laid out for its kit's device: the design always shows whole, another aspect ratio adds room (the preview's fit)
        input.canvasFit = "expand";
        return input;
    }

    Vector<PipelineZip::Entry> JsonExportEntries(const UiDoc& doc, const UiStructureResult& structure, const String& picture,
                                                 Vector<String>* missing /*= nullptr*/)
    {
        String dir = doc.name;
        double layoutDpi = DocDpi(doc);
        Vector<PipelineZip::Entry> entries;
        entries.Add({ dir + "/ui_structure.json", structure.json });
        entries.Add({ dir + "/layout.uiml", DocToUiml(doc) });
        entries.Add({ dir + "/layout.json", DocToJson(doc).Dump(2) });

        // Each sprite keeps its pixels and says its density (pHYs + our tag); kit.json lists it for an importer
        for (auto& s : structure.sprites)
            entries.Add({ dir + "/sprites/" + s.file, PipelinePngMeta::WithDpi(s.png, s.dpi ? *s.dpi : layoutDpi) });

        Json manifest = Json::Object();
        manifest["layoutDpi"] = layoutDpi;
        Json sprites = Json::Array();
        for (auto& s : structure.sprites)
        {
            Json item = Json::Object();
            item["file"] = s.file;
            item["width"] = s.width;
            item["height"] = s.height;
            item["dpi"] = s.dpi ? *s.dpi : layoutDpi;
            item["pixelsPerLayoutPixel"] = s.density ? *s.density : 1.0;
            if (s.nineSlice)
            {
                Json nine = Json::Object();
                nine["left"] = s.nineSlice->left;
                nine["top"] = s.nineSlice->top;
                nine["right"] = s.nineSlice->right;
                nine["bottom"] = s.nineSlice->bottom;
                item["nineSlice"] = nine;
            }
            sprites.Add(item);
        }
        manifest["sprites"] = sprites;
        entries.Add({ dir + "/kit.json", manifest.Dump(2) });

        Vector<String> fonts;
        for (auto& f : structure.fonts)
        {
            if (!fonts.Contains(f))
                fonts.Add(f);
        }
        Vector<String> licenses;
        for (auto& font : UiFonts())
        {
            if (font.faces.Any([&](const UiFontFace& x) { return fonts.Contains(x.file); }) && !licenses.Contains(font.license))
                licenses.Add(font.license);
        }
        for (auto& f : fonts)
            entries.Add({ dir + "/fonts/" + f, FontBytes(f, missing) });
        for (auto& f : licenses)
        {
            if (!fonts.Contains(f))
                entries.Add({ dir + "/fonts/" + f, FontBytes(f, missing) });
        }

        if (!picture.IsEmpty())
            entries.Add({ dir + "/preview.png", picture });
        return entries;
    }

    Vector<String> O2FontNotes(const UiStructureResult& structure)
    {
        Vector<String> notes;
        for (auto& f : structure.fonts)
        {
            if (!StartsWith(JsToLower(f), "roboto-"))
                notes.Add(f + ": the o2 export draws it with Roboto for now");
        }
        return notes;
    }

    UiLayout::ExportRequest O2ExportRequest(const UiDoc& doc, const UiStructureResult& structure, const String& assetPath,
                                            const String& assetsPath, const String& seed)
    {
        UiLayout::ExportRequest request;
        request.assetsPath = assetsPath;
        request.folder = FolderOf(assetPath);
        request.name = doc.name.IsEmpty() ? String("Screen") : doc.name;
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

        if (scaled.empty())
        {
            request.layout = structure.json;
            return request;
        }

        // The borders a node asks for are in the sprite's own pixels: they follow the resampled sprite (the web passes
        // them unscaled, which draws the corners of a denser sprite too large)
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
            for (auto& m : v.Members())
                walk(m.second);
        };
        walk(layout);
        request.layout = layout.Dump(2);
        return request;
    }

    static bool WriteOutput(const String& path, const String& bytes, String& error)
    {
        if (path.IsEmpty())
        {
            error = "no file to write the export to";
            return false;
        }
        String parent = o2FileSystem.GetParentPath(path);
        if (!parent.IsEmpty())
            o2FileSystem.FolderCreate(parent, true);
        if (!PipelineUtils::WriteFileBytes(path, bytes))
        {
            error = "could not write " + path;
            return false;
        }
        return true;
    }

    Coroutine<UiLayoutExportResult> ExportLayout(UiLayoutExportRequest request)
    {
        UiLayoutExportResult res;
        String target = CleanTarget(request.target);
        UiStructureResult structure = ToUiStructure(request.doc, request.kit ? &*request.kit : nullptr);
        Vector<String> missing;

        if (target == UI_EXPORT_JSON)
        {
            auto entries = JsonExportEntries(request.doc, structure, request.picture, &missing);
            if (!WriteOutput(request.outputPath, PipelineZip::Write(entries, true), res.error))
                co_return res;
            res.warnings = structure.warnings;
        }
        else if (target == UI_EXPORT_UNITY)
        {
            auto fonts = UnityExportFonts(structure, &missing);
            auto exported = ExportUnity(UnityExportInput(request.doc, structure, request.assetPath, request.nodeId, fonts));
            if (!exported.ok)
            {
                res.error = exported.error;
                co_return res;
            }
            String packError;
            String package = UnityPackage(exported.files, &packError);
            if (package.IsEmpty())
            {
                res.error = packError;
                co_return res;
            }
            if (!WriteOutput(request.outputPath, package, res.error))
                co_return res;
            res.warnings = structure.warnings;
            res.warnings.Add(exported.warnings);
        }
        else
        {
            auto exported = co_await UiLayout::ExportPrototype(O2ExportRequest(request.doc, structure, request.assetPath,
                                                                               request.assetsPath, request.nodeId));
            if (!exported.ok)
            {
                res.error = exported.error;
                co_return res;
            }
            res.warnings = structure.warnings;
            res.warnings.Add(O2FontNotes(structure));
            res.warnings.Add(exported.warnings);
            res.written = exported.files;
            res.prototype = exported.prototype;
            res.ok = true;
            co_return res;
        }

        for (auto& file : missing)
            res.warnings.Add(file + ": the bundled font is missing, not exported");
        res.written.Add(request.outputPath);
        res.ok = true;
        co_return res;
    }
}
