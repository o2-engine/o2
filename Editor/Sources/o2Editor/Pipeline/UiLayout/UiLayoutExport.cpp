#include "o2Editor/stdafx.h"
#include "UiLayoutExport.h"

#include <set>

#include "o2/Assets/Assets.h"
#include "o2/EngineSettings.h"
#include "o2/Assets/Types/ActorAsset.h"
#include "o2/Assets/Types/FontStyleAsset.h"
#include "o2/Assets/Types/ImageAsset.h"
#include "o2/Assets/Types/VectorFontAsset.h"
#include "o2/Render/VectorFontEffects.h"
#include "o2/Scene/ActorCreationMode.h"
#include "o2/Utils/Coroutines/Awaiters.h"
#include "o2/Utils/Editor/EditorScope.h"
#include "o2/Utils/FileSystem/FileSystem.h"
#include "o2/Utils/Types/UID.h"
#include "o2Editor/Pipeline/PipelineUtils.h"
#include "o2Editor/Pipeline/UiLayout/UiLayoutAgent.h"
#include "o2Editor/Pipeline/UiLayout/UiLayoutWidgets.h"

namespace Editor::UiLayout
{
    namespace
    {
        void EnsureMeta(const String& fullPath, const String& type)
        {
            String metaPath = fullPath + ".meta";
            if (o2FileSystem.IsFileExist(metaPath))
                return;

            UID uid;
            uid.Randomize();
            Json value = Json::Object();
            value["mId"] = String(uid.ToString());
            Json meta = Json::Object();
            meta["Type"] = type;
            meta["Value"] = value;
            PipelineUtils::WriteFileBytes(metaPath, meta.Dump(4));
        }

        bool WriteIfChanged(const String& path, const String& bytes)
        {
            if (o2FileSystem.IsFileExist(path) && PipelineUtils::ReadFileBytes(path) == bytes)
                return false;
            return PipelineUtils::WriteFileBytes(path, bytes);
        }

        // The image meta keeps its uid; a nine-sliced sprite carries its borders as the asset's default slice, so it
        // stretches wherever it is used, not only inside the prototype
        void EnsureImageMeta(const String& fullPath, const std::optional<NineSlice>& nine)
        {
            String metaPath = fullPath + ".meta";
            Json meta;
            if (!o2FileSystem.IsFileExist(metaPath) || !Json::Parse(PipelineUtils::ReadFileBytes(metaPath), meta) || !meta.Get("Value"))
            {
                UID uid;
                uid.Randomize();
                Json value = Json::Object();
                value["mId"] = String(uid.ToString());
                meta = Json::Object();
                meta["Type"] = String("o2::ImageAsset::Meta");
                meta["Value"] = value;
            }
            Json& value = meta["Value"];
            if (nine)
            {
                Json border = Json::Object();
                border["left"] = nine->left;
                border["bottom"] = nine->bottom;
                border["right"] = nine->right;
                border["top"] = nine->top;
                value["sliceBorder"] = border;
                value["defaultMode"] = String("Sliced");
            }
            else
            {
                value.Remove("sliceBorder");
                value.Remove("defaultMode");
            }
            WriteIfChanged(metaPath, meta.Dump(4));
        }

        String HexOfColor(const Color4& c)
        {
            return HexOf(Rgb(c.r, c.g, c.b)).SubStr(1).ToLowerCase();
        }

        String StyleFile(const Color4& fill, const Color4& stroke, int radius)
        {
            return "stroke_" + HexOfColor(fill) + "_" + HexOfColor(stroke) + "_" + (String)radius + ".fntstyle";
        }

        class ExportResources: public UiWidgetResources
        {
        public:
            String              resources; // <folder>/<name>_Resources
            Map<String, String> files;     // Sprite file by lower-case asset name

            Ref<Sprite> SpriteOf(const String& asset) override
            {
                String key = asset;
                key.ReplaceAll("\\", "/");
                int slash = key.FindLast("/");
                if (slash >= 0)
                    key = key.SubStr(slash + 1);

                String file;
                if (!files.TryGetValue(key.ToLowerCase(), file))
                    return nullptr;
                return ImageSprite(resources + "/Sprites/" + file);
            }

            Ref<Sprite> WhiteSprite() override
            {
                return ImageSprite(resources + "/Sprites/white.png");
            }

            void SetFont(const Ref<Label>& label, const String& fontFile) override
            {
                label->SetFontAsset(AssetRef<VectorFontAsset>(resources + "/Fonts/" + fontFile));
            }

            void SetStroke(const Ref<Label>& label, const Color4& fill, const Color4& stroke, int radius) override
            {
                label->SetFontStyleAsset(AssetRef<FontStyleAsset>(resources + "/FontStyles/" + StyleFile(fill, stroke, radius)));
            }

        private:
            Ref<Sprite> ImageSprite(const String& path)
            {
                if (!o2Assets.IsAssetExist(path))
                    return nullptr;
                return mmake<Sprite>(AssetRef<ImageAsset>(path));
            }
        };

        bool SamePath(const String& a, const String& b)
        {
            String x = AbsolutePath(a), y = AbsolutePath(b);
            while (HasSuffix(x, "/")) x = x.SubStr(0, x.Length() - 1);
            while (HasSuffix(y, "/")) y = y.SubStr(0, y.Length() - 1);
            return x == y;
        }
    }

    Map<String, UiSpriteInfo> SpriteInfos(const Vector<ImageToUiSprite>& sprites)
    {
        Map<String, UiSpriteInfo> infos;
        for (auto& sprite : sprites)
        {
            UiSpriteInfo info;
            info.nine = sprite.nine;
            UiImage img;
            if (UiImage::Decode(sprite.png, img))
            {
                info.w = img.w;
                info.h = img.h;
            }
            infos[(sprite.key + ".png").ToLowerCase()] = info;
        }
        return infos;
    }

    Coroutine<ExportResult> ExportPrototype(ExportRequest request)
    {
        co_await SwitchToMain();

        ExportResult res;
        Json structure;
        if (!Json::Parse(request.layout, structure) || !structure.Get("root"))
        {
            res.error = "the layout is not a ui_structure.json";
            co_return res;
        }

        UiWidgetPlan plan;
        String planError;
        if (!PlanWidgets(structure, SpriteInfos(request.sprites), plan, &planError))
        {
            res.error = "the layout does not resolve: " + planError;
            co_return res;
        }
        res.warnings = plan.warnings;

        if (request.assetsPath.IsEmpty())
        {
            res.error = "no assets folder to write into";
            co_return res;
        }

        String folder = request.folder.IsEmpty() ? String("Generated/UI") : request.folder;
        String resources = folder + "/" + request.name + "_Resources";
        String protoRel = folder + "/" + request.name + ".proto";
        String root = request.assetsPath;
        if (!HasSuffix(root, "/"))
            root += "/";

        o2FileSystem.FolderCreate(root + resources + "/Sprites", true);
        o2FileSystem.FolderCreate(root + resources + "/Fonts", true);
        for (auto& f : Vector<String>{ folder, resources, resources + "/Sprites", resources + "/Fonts" })
            EnsureMeta(root + f, "o2::DefaultAssetMeta<o2::FolderAsset>");

        // Sprites: the run's own PNGs; a sprite the layout no longer uses goes
        ExportResources exportResources;
        exportResources.resources = resources;
        std::set<std::string> keep;
        for (auto& sprite : request.sprites)
        {
            String file = sprite.key + ".png";
            String rel = resources + "/Sprites/" + file;
            WriteIfChanged(root + rel, sprite.png);
            EnsureImageMeta(root + rel, sprite.nine);
            keep.insert(file.Data());
            exportResources.files[file.ToLowerCase()] = file;
            res.files.Add(rel);
        }

        bool flat = plan.nodes.Any([](const UiPlanNode& p)
        {
            return p.type == "rect" || (p.asset.IsEmpty() && !p.color.IsEmpty() && p.type != "text" && p.type != "image");
        });
        if (flat)
        {
            String rel = resources + "/Sprites/white.png";
            WriteIfChanged(root + rel, UiImage::Blank(8, 8, 255, 255, 255, 255).Png());
            EnsureMeta(root + rel, "o2::ImageAsset::Meta");
            keep.insert("white.png");
            res.files.Add(rel);
        }

        for (auto& file : o2FileSystem.GetFolderInfo(root + resources + "/Sprites").files)
        {
            String name = o2FileSystem.GetPathWithoutDirectories(file.path);
            if (HasSuffix(name, ".png") && !keep.count(name.Data()))
            {
                o2FileSystem.FileDelete(root + resources + "/Sprites/" + name);
                o2FileSystem.FileDelete(root + resources + "/Sprites/" + name + ".meta");
            }
        }

        // Fonts: the Roboto files the layout names, with their licence
        std::set<std::string> fonts;
        Vector<Pair<String, int>> strokes;
        for (auto& p : plan.nodes)
        {
            if (p.widget != "Label")
                continue;

            fonts.insert(p.fontFile.Data());
            if (!p.strokeColor.IsEmpty())
            {
                Pair<String, int> stroke(p.color + "|" + p.strokeColor, Math::Max(1, (int)JsRound(p.strokeWidth)));
                if (!strokes.Contains(stroke))
                    strokes.Add(stroke);
            }
        }

        String fontSource = FindRepoPath("o2/Editor/Assets/ui/pipeline/fonts");
        fontSource = fontSource.IsEmpty() ? String(GetEditorAssetsPath()) + "ui/pipeline/fonts/" : fontSource + "/";
        for (auto& font : fonts)
        {
            String rel = resources + "/Fonts/" + String(font);
            WriteIfChanged(root + rel, PipelineUtils::ReadFileBytes(fontSource + String(font)));
            EnsureMeta(root + rel, "o2::VectorFontAsset::Meta");
            res.files.Add(rel);
        }
        if (!fonts.empty())
        {
            String rel = resources + "/Fonts/LICENSE-Roboto.txt";
            WriteIfChanged(root + rel, PipelineUtils::ReadFileBytes(fontSource + "LICENSE-Roboto.txt"));
            EnsureMeta(root + rel, "o2::DefaultAssetMeta<o2::BinaryAsset>");
        }

        if (!SamePath(root, o2Assets.GetAssetsPath()))
        {
            res.warnings.Add("the prototype is written by the editor whose assets these are: sprites and fonts written, the .proto not");
            res.ok = true;
            co_return res;
        }

        // Outline styles, made natively; one file per fill, stroke and radius
        for (auto& stroke : strokes)
        {
            int bar = stroke.first.Find("|");
            Color4 fill = Color4::Black(), color = Color4::Black();
            PipelineUtils::ParseHexColor(stroke.first.SubStr(0, bar), fill);
            PipelineUtils::ParseHexColor(stroke.first.SubStr(bar + 1), color);
            fill.a = color.a = 255;

            String rel = resources + "/FontStyles/" + StyleFile(fill, color, stroke.second);
            res.files.Add(rel);
            if (o2Assets.IsAssetExist(rel) || o2FileSystem.IsFileExist(root + rel))
                continue;

            o2FileSystem.FolderCreate(root + resources + "/FontStyles", true);
            EnsureMeta(root + resources + "/FontStyles", "o2::DefaultAssetMeta<o2::FolderAsset>");
            auto style = mmake<FontStyleAsset>();
            style->AddEffect<FontColorEffect>(fill);
            style->AddEffect<FontStrokeEffect>((float)stroke.second, color, 100);
            style->Save(rel);
        }

        o2Assets.RebuildAssets();

        {
            UiWidgetOptions options;
            options.explicitDepths = true;
            options.layer = "UI";
            AssetRef<ActorAsset> existing;
            if (o2Assets.IsAssetExist(protoRel))
            {
                existing = AssetRef<ActorAsset>(protoRel);
                if (existing && existing->GetActor())
                    options.keepIds = CollectActorIds(existing->GetActor());
            }

            UiWidgetTree tree = BuildWidgets(plan, exportResources, options);
            UpdateWidgetTree(tree.root);
            res.warnings = tree.warnings;
            res.widgets = tree.widgets.Count();

            PushEditorScopeOnStack scope;
            if (existing)
            {
                existing->SetActor(tree.root);
                existing->Save();
            }
            else
            {
                auto asset = mmake<ActorAsset>(tree.root);
                asset->Save(protoRel);
            }
            tree = UiWidgetTree();
        }

        res.prototype = protoRel;
        res.files.Insert(protoRel, 0);
        o2Assets.RebuildAssets();
        res.ok = true;
        co_return res;
    }
}
