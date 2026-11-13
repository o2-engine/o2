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
#include "o2Editor/Pipeline/UiDoc/UiDocFonts.h"
#include "o2Editor/Pipeline/UiDoc/UiTextStyle.h"
#include "o2Editor/Pipeline/UiLayout/UiLayoutAgent.h"
#include "o2Editor/Pipeline/UiLayout/UiLayoutWidgets.h"

namespace Editor::UiLayout
{
    namespace
    {
        // FNV-1a over the UTF-8 bytes of seed \0 kind \0 key with the splitmix64 finalizer, as AssetsLine's o2 export
        // (shared/uiExport/o2.ts) derives its ids
        UInt64 SeedHash(const String& seed, const char* kind, const String& key)
        {
            std::string text = std::string(seed.Data()) + '\0' + kind + '\0' + key.Data();
            UInt64 h = 0xcbf29ce484222325ull;
            for (unsigned char c : text)
            {
                h ^= (UInt64)c;
                h *= 0x100000001b3ull;
            }
            h = (h ^ (h >> 30))*0xbf58476d1ce4e5b9ull;
            h = (h ^ (h >> 27))*0x94d049bb133111ebull;
            return h ^ (h >> 31);
        }

        // The asset uid of a path inside Assets for a seed (32 hex digits), empty without a seed
        String SeedUid(const String& seed, const String& path)
        {
            if (seed.IsEmpty())
                return String();
            char buffer[33];
            snprintf(buffer, sizeof(buffer), "%016llx%016llx", (unsigned long long)SeedHash(seed, "asset-a", path),
                     (unsigned long long)SeedHash(seed, "asset-b", path));
            return String(buffer);
        }

        // The actor id of a widget path for a seed: a non-zero 64-bit integer
        SceneUID SeedSceneUid(const String& seed, const String& key)
        {
            UInt64 h = SeedHash(seed, "scene", key);
            return h == 0 ? 1 : h;
        }

        void EnsureMeta(const String& fullPath, const String& type, const String& seededUid = "")
        {
            String metaPath = fullPath + ".meta";
            if (o2FileSystem.IsFileExist(metaPath))
                return;

            UID uid;
            uid.Randomize();
            Json value = Json::Object();
            value["mId"] = seededUid.IsEmpty() ? String(uid.ToString()) : seededUid;
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
        void EnsureImageMeta(const String& fullPath, const std::optional<NineSlice>& nine, const String& seededUid = "")
        {
            String metaPath = fullPath + ".meta";
            Json meta;
            if (!o2FileSystem.IsFileExist(metaPath) || !Json::Parse(PipelineUtils::ReadFileBytes(metaPath), meta) || !meta.Get("Value"))
            {
                UID uid;
                uid.Randomize();
                Json value = Json::Object();
                value["mId"] = seededUid.IsEmpty() ? String(uid.ToString()) : seededUid;
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

        class ExportResources: public UiWidgetResources
        {
        public:
            String              resources; // <folder>/<name>_Resources
            Map<String, String> files;     // Sprite file by lower-case asset name
            Map<String, String> fonts;     // The font file written for a font the plan names

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
                String file = fontFile;
                fonts.TryGetValue(fontFile, file);
                label->SetFontAsset(AssetRef<VectorFontAsset>(resources + "/Fonts/" + file));
            }

            void SetLook(const Ref<Label>& label, const UiTextLook& look) override
            {
                label->SetFontStyleAsset(AssetRef<FontStyleAsset>(resources + "/FontStyles/" + FontStyleFileOf(look)));
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

        // A file of the bundled UI fonts (a font or a licence): the layout renderer's fonts, else the editor's own copy
        String BundledFontBytes(const String& file)
        {
            String home = ::Editor::UiDoc::UiFontsHome();
            String bytes = home.IsEmpty() ? String() : PipelineUtils::ReadFileBytes(home + "/" + file);
            if (!bytes.IsEmpty())
                return bytes;

            String editorFonts = FindRepoPath("o2/Editor/Assets/ui/pipeline/fonts");
            editorFonts = editorFonts.IsEmpty() ? String(GetEditorAssetsPath()) + "ui/pipeline/fonts/" : editorFonts + "/";
            return PipelineUtils::ReadFileBytes(editorFonts + file);
        }

        // The licence file of a bundled font's family, empty for a font outside the catalogue
        String FontLicenceOf(const String& fontFile)
        {
            for (auto& font : ::Editor::UiDoc::UiFonts())
            {
                if (font.faces.Any([&](const ::Editor::UiDoc::UiFontFace& face) { return face.file == fontFile; }))
                    return font.license;
            }
            return String();
        }

        Color4 LookColor(const String& hex, double opacity)
        {
            Color4 color = Color4::Black();
            PipelineUtils::ParseHexColor(hex, color);
            color.a = (int)JsRound(255*Math::Clamp(opacity, 0.0, 1.0));
            return color;
        }

        int StrokeRadius(double width)
        {
            return Math::Max(1, (int)JsRound(width));
        }

        String HexOfColor(const String& hex)
        {
            Color4 c = LookColor(hex, 1);
            return HexOf(Rgb(c.r, c.g, c.b)).SubStr(1).ToLowerCase();
        }
    }

    Ref<FontStyle> FontStyleOf(const UiTextLook& look)
    {
        auto style = mmake<FontStyle>();
        Color4 from = LookColor(look.fill, 1);
        if (look.fillTo.IsEmpty())
            style->AddEffect<FontColorEffect>(from);
        else
        {
            // o2 runs a gradient over each glyph from its first colour, rows bottom-up; the direction goes to the nearest axis
            Color4 to = LookColor(look.fillTo, 1);
            double a = std::fmod(std::fmod(look.angle, 360.0) + 360.0, 360.0);
            if (a >= 45 && a < 135)
                style->AddEffect<FontGradientEffect>(to, from, 0.0f, 0.0f, Vec2F());
            else if (a >= 225 && a < 315)
                style->AddEffect<FontGradientEffect>(from, to, 0.0f, 0.0f, Vec2F());
            else if (a >= 135 && a < 225)
                style->AddEffect<FontGradientEffect>(to, from, -90.0f, 0.0f, Vec2F());
            else
                style->AddEffect<FontGradientEffect>(from, to, -90.0f, 0.0f, Vec2F());
        }

        for (auto& e : look.effects)
        {
            if (e.type == "stroke")
                style->AddEffect<FontStrokeEffect>((float)StrokeRadius(e.width), LookColor(e.color, e.opacity), 100);
            else
            {
                // A blur under one pixel copies the glyph as it is (a zero radius divides by zero)
                style->AddEffect<FontShadowEffect>((float)Math::Max(0.5, e.blur), Vec2I((int)JsRound(e.x), (int)JsRound(e.y)),
                                                   LookColor(e.color, e.opacity));
            }
        }
        return style;
    }

    String FontStyleDataOf(const UiTextLook& look)
    {
        DataDocument data;
        FontStyleOf(look)->Serialize(data);
        return data.SaveAsString();
    }

    String FontStyleFileOf(const UiTextLook& look)
    {
        if (look.fillTo.IsEmpty() && look.effects.Count() == 1 && look.effects[0].type == "stroke" && look.effects[0].opacity >= 1)
        {
            return "stroke_" + HexOfColor(look.fill) + "_" + HexOfColor(look.effects[0].color) + "_" +
                (String)StrokeRadius(look.effects[0].width) + ".fntstyle";
        }

        UInt64 h = 0xcbf29ce484222325ull;
        String data = FontStyleDataOf(look);
        for (int i = 0; i < data.Length(); i++)
        {
            h ^= (UInt64)(unsigned char)data[i];
            h *= 0x100000001b3ull;
        }
        char buffer[17];
        snprintf(buffer, sizeof(buffer), "%016llx", (unsigned long long)h);
        return "text_" + String(buffer) + ".fntstyle";
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
            EnsureMeta(root + f, "o2::DefaultAssetMeta<o2::FolderAsset>", SeedUid(request.seed, f));

        // Sprites: the run's own PNGs; a sprite the layout no longer uses goes
        ExportResources exportResources;
        exportResources.resources = resources;
        std::set<std::string> keep;
        for (auto& sprite : request.sprites)
        {
            String file = sprite.key + ".png";
            String rel = resources + "/Sprites/" + file;
            WriteIfChanged(root + rel, sprite.png);
            EnsureImageMeta(root + rel, sprite.nine, SeedUid(request.seed, rel));
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
            EnsureMeta(root + rel, "o2::ImageAsset::Meta", SeedUid(request.seed, rel));
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

        // Fonts: every bundled font the labels use, with its family's licence (a font that cannot be read: Roboto instead)
        Vector<String> fonts, licences;
        Vector<UiTextLook> looks;
        for (auto& p : plan.nodes)
        {
            if (p.widget != "Label")
                continue;

            if (!fonts.Contains(p.fontFile))
                fonts.Add(p.fontFile);
            if (p.look && !looks.Contains(*p.look))
                looks.Add(*p.look);
        }

        for (auto& font : fonts)
        {
            String file = font, bytes = BundledFontBytes(font);
            if (bytes.IsEmpty())
            {
                file = FontFileOf(WeightOfFont(font));
                bytes = BundledFontBytes(file);
                res.warnings.Add(font + ": the bundled font is missing, its labels use " + file);
            }
            exportResources.fonts[font] = file;
            if (bytes.IsEmpty())
                continue;

            String rel = resources + "/Fonts/" + file;
            WriteIfChanged(root + rel, bytes);
            EnsureMeta(root + rel, "o2::VectorFontAsset::Meta", SeedUid(request.seed, rel));
            if (!res.files.Contains(rel))
                res.files.Add(rel);
            String licence = FontLicenceOf(file);
            if (!licence.IsEmpty() && !licences.Contains(licence))
                licences.Add(licence);
        }
        for (auto& licence : licences)
        {
            String rel = resources + "/Fonts/" + licence;
            WriteIfChanged(root + rel, BundledFontBytes(licence));
            EnsureMeta(root + rel, "o2::DefaultAssetMeta<o2::BinaryAsset>", SeedUid(request.seed, rel));
            res.files.Add(rel);
        }

        // Text looks: one font style each, named by what it draws
        if (!looks.IsEmpty())
        {
            o2FileSystem.FolderCreate(root + resources + "/FontStyles", true);
            EnsureMeta(root + resources + "/FontStyles", "o2::DefaultAssetMeta<o2::FolderAsset>",
                       SeedUid(request.seed, resources + "/FontStyles"));
        }
        for (auto& look : looks)
        {
            String rel = resources + "/FontStyles/" + FontStyleFileOf(look);
            WriteIfChanged(root + rel, FontStyleDataOf(look));
            EnsureMeta(root + rel, "o2::DefaultAssetMeta<o2::FontStyleAsset>", SeedUid(request.seed, rel));
            res.files.Add(rel);
        }

        if (!SamePath(root, o2Assets.GetAssetsPath()))
        {
            res.warnings.Add("the prototype is written by the editor whose assets these are: sprites, fonts and text styles written, the .proto not");
            res.ok = true;
            co_return res;
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

            // A seeded export gives the widgets it makes anew the ids AssetsLine's o2 export gives them (a path that occurs
            // twice keeps random ones: the ids are kept by path)
            if (!request.seed.IsEmpty())
            {
                Map<String, int> uses;
                for (auto& p : plan.nodes)
                    uses[p.widgetPath] = uses.ContainsKey(p.widgetPath) ? uses[p.widgetPath] + 1 : 1;
                for (auto& p : plan.nodes)
                {
                    if (uses[p.widgetPath] == 1 && !options.keepIds.ContainsKey(p.widgetPath))
                        options.keepIds[p.widgetPath] = SeedSceneUid(request.seed, p.widgetPath);
                }
            }

            UiWidgetTree tree = BuildWidgets(plan, exportResources, options);
            UpdateWidgetTree(tree.root);
            for (auto& warning : tree.warnings)
            {
                if (!res.warnings.Contains(warning))
                    res.warnings.Add(warning);
            }
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
