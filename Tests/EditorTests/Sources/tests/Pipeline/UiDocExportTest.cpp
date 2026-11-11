#include "o2Editor/stdafx.h"

#include <algorithm>
#include <cstring>
#include <functional>
#include <map>

#include "Network/NetworkTestHelpers.h"
#include "o2/Utils/FileSystem/FileSystem.h"
#include "o2Editor/Pipeline/PipelineUtils.h"
#include "o2Editor/Pipeline/UiDoc/UiDocExport.h"
#include "o2Editor/Pipeline/UiLayout/UiLayoutPixels.h"

// zlib's global Byte clashes with o2::Byte once the o2 namespace is open
#define Byte ZlibByte
#include "3rdPartyLibs/zlib/zlib.h"
#undef Byte

#include "UiDocTestKit.h"

#include "UiDocExportRefs.inc"

using namespace UiDocTest;
namespace PipelineUtils = Editor::PipelineUtils;
namespace PipelineZip = Editor::PipelineZip;
namespace UiLayout = Editor::UiLayout;

// The AI UI layout's Download: the Unity package, the JSON zip and the o2 prototype's files against what the web makes
// of the same layouts (UiDocExportRefs.inc, from UiDocGolden/export_refs.mts), and the entry point writing each target

namespace
{
    const Json& Refs()
    {
        static const Json refs = []() {
            std::string text;
            for (const char* part : kUiDocExportRefParts)
                text += part;
            Json out;
            String error;
            if (!Json::Parse(String(text), out, &error))
                ADD_FAILURE() << "the export references do not parse: " << error.Data();
            return out;
        }();
        return refs;
    }

    const Json& E(const char* key)
    {
        return M(Refs(), key);
    }

    // The kit of a structure case, as the golden script made it
    UiKit StructureKit(const Json& tag)
    {
        const Json& g = G("structure");
        if (tag.Str() == "A")
            return ParseKit(S(g, "kitA"));
        if (tag.Str() == "KIT")
        {
            UiKit kit = GoldenKit();
            for (auto& e : kit.elements)
                e.png = S(g, "png");
            return kit;
        }
        return ParseKit(tag.Str());
    }

    // A structure case of the golden data by name
    const Json* StructureCase(const String& name)
    {
        for (auto& c : M(G("structure"), "cases").Items())
        {
            if (S(c, "name") == name)
                return &c;
        }
        return nullptr;
    }

    // The fonts as the golden script stands them in
    Vector<UiExportFont> FakeFonts(const Json& files)
    {
        Vector<UiExportFont> out;
        for (auto& f : files.Items())
            out.Add({ f.Str(), "FONT " + f.Str() });
        return out;
    }

    std::string Bytes(const String& s)
    {
        return std::string(s.Data(), s.Length());
    }

    std::string Gunzip(const String& bytes)
    {
        z_stream zs;
        std::memset(&zs, 0, sizeof(zs));
        if (inflateInit2(&zs, 15 + 32) != Z_OK)
            return std::string();
        std::string out;
        char buffer[65536];
        zs.next_in = (Bytef*)bytes.Data();
        zs.avail_in = (uInt)bytes.Length();
        int status = Z_OK;
        while (status == Z_OK)
        {
            zs.next_out = (Bytef*)buffer;
            zs.avail_out = sizeof(buffer);
            status = inflate(&zs, Z_NO_FLUSH);
            out.append(buffer, sizeof(buffer) - zs.avail_out);
        }
        inflateEnd(&zs);
        return status == Z_STREAM_END ? out : std::string();
    }

    struct TarItem
    {
        std::string name, header, data;
    };

    std::vector<TarItem> ReadTar(const std::string& tar)
    {
        std::vector<TarItem> out;
        for (size_t o = 0; o + 512 <= tar.size();)
        {
            std::string header = tar.substr(o, 512);
            if (header.find_first_not_of('\0') == std::string::npos)
                break;
            std::string name = header.substr(0, 100).c_str();
            size_t size = std::stoul(header.substr(124, 11), nullptr, 8);
            out.push_back({ name, header, tar.substr(o + 512, size) });
            o += 512 + (size + 511) / 512 * 512;
        }
        return out;
    }

    // The files of an export against the web's: paths in order, texts as they are, PNGs byte for byte (a baked hue by
    // its pixels: the encoders differ)
    void ExpectFiles(const UiExportFiles& got, const Json& expected)
    {
        ASSERT_TRUE(got.ok) << got.error.Data();
        auto& files = M(expected, "files").Items();
        ASSERT_EQ(got.files.Count(), files.Count());
        for (int i = 0; i < files.Count(); i++)
        {
            String path = files[i].At(0)->Str();
            const String& want = files[i].At(1)->Str();
            EXPECT_STR_EQ(got.files[i].path, path);
            bool binary = UiLayout::HasSuffix(path, ".png") || UiLayout::HasSuffix(path, ".ttf") || UiLayout::HasSuffix(path, ".otf");
            if (!binary)
            {
                EXPECT_STR_EQ(got.files[i].data, want) << path.Data();
                continue;
            }
            const Json* pixels = M(expected, "pixels").Get(path);
            if (!pixels)
            {
                EXPECT_TRUE(Bytes(got.files[i].data) == Bytes(PipelineUtils::Base64Decode(want))) << path.Data();
                continue;
            }
            UiLayout::UiImage img;
            ASSERT_TRUE(UiLayout::UiImage::Decode(got.files[i].data, img)) << path.Data();
            String raw = PipelineUtils::Base64Decode(pixels->Str());
            ASSERT_EQ((size_t)raw.Length(), img.data.size()) << path.Data();
            EXPECT_EQ(0, std::memcmp(raw.Data(), img.data.data(), img.data.size())) << path.Data();
        }
        EXPECT_EQ(got.warnings, Strings(M(expected, "warnings")));
    }

    struct WorkDir
    {
        String path;

        WorkDir()
        {
            String relative = "./uidoc-export-" + (String)(int)(Math::Random()%1000000);
            o2FileSystem.FolderCreate(relative, true);
            path = o2FileSystem.CanonicalizePath(relative) + "/";
        }

        ~WorkDir()
        {
            o2FileSystem.FolderRemove(path, true);
        }
    };

    UiLayoutExportResult RunExport(const UiLayoutExportRequest& request)
    {
        auto co = ExportLayout(request);
        co.Start(JobThread::Main);
        EXPECT_TRUE(NetPumpUntil([&] { return co.IsDone(); }, 30.0f));
        return co.GetResult();
    }

    UiLayoutExportRequest RequestFor(const char* caseName, const String& target)
    {
        const Json* c = StructureCase(caseName);
        UiLayoutExportRequest request;
        request.doc = Parse(S(*c, "src"));
        if (!M(*c, "kit").IsNull())
            request.kit = StructureKit(M(*c, "kit"));
        request.target = target;
        request.nodeId = "node_o2";
        return request;
    }
}

TEST(UiDocExport, TargetsFileNamesAndFontNotes)
{
    Json config;
    ASSERT_TRUE(Json::Parse("{\"exportFor\":\"o2\"}", config));
    EXPECT_STR_EQ(ExportTargetOf(config), "o2");
    ASSERT_TRUE(Json::Parse("{\"exportFor\":\"json\"}", config));
    EXPECT_STR_EQ(ExportTargetOf(config), "json");
    ASSERT_TRUE(Json::Parse("{\"exportFor\":\"godot\"}", config));
    EXPECT_STR_EQ(ExportTargetOf(config), "unity");
    EXPECT_STR_EQ(ExportTargetOf(Json::Object()), "unity");

    UiDoc doc = Parse("screen Main Menu 100x100");
    EXPECT_STR_EQ(ExportFileName(doc, "unity"), doc.name + ".unitypackage");
    EXPECT_STR_EQ(ExportFileName(doc, "o2"), doc.name + "_o2.zip");
    EXPECT_STR_EQ(ExportFileName(doc, "json"), doc.name + ".zip");
    EXPECT_STR_EQ(ExportFileName(doc, "x"), doc.name + ".unitypackage");

    UiStructureResult st;
    st.fonts = { "Roboto-Bold.ttf", "Lobster-Regular.ttf" };
    EXPECT_EQ(O2FontNotes(st), Vector<String>({ "Lobster-Regular.ttf: the o2 export draws it with Roboto for now" }));
}

TEST(UiDocExport, UnityFloatsAndGuidsAreTheWebs)
{
    // JSON writes -0 as 0: the web's "-0" is for -0
    for (auto& f : E("floats").Items())
    {
        double x = f.At(1)->Str() == "-0" ? -0.0 : f.At(0)->Num();
        EXPECT_STR_EQ(UnityFloat(x), f.At(1)->Str()) << x;
    }
    for (auto& g : E("guids").Items())
        EXPECT_STR_EQ(UnityGuid(g.At(0)->Str()), g.At(1)->Str());
}

TEST(UiDocExport, GoldenUnityExportOfTheLayoutsStructures)
{
    for (auto& c : E("unity").Items())
    {
        SCOPED_TRACE(S(c, "name").Data());
        const Json* sc = StructureCase(S(c, "name"));
        ASSERT_TRUE(sc);
        UiDoc doc = Parse(S(*sc, "src"));
        UiKit kit;
        if (!M(*sc, "kit").IsNull())
            kit = StructureKit(M(*sc, "kit"));
        auto st = ToUiStructure(doc, M(*sc, "kit").IsNull() ? nullptr : &kit);

        // The fonts the package takes: the bundled files, every one found
        Vector<String> missing;
        Vector<String> names;
        for (auto& f : UnityExportFonts(st, &missing))
            names.Add(f.file);
        EXPECT_EQ(names, Strings(M(c, "fonts")));
        EXPECT_TRUE(missing.IsEmpty());

        auto res = ExportUnity(UnityExportInput(doc, st, S(c, "assetPath"), "node_" + S(c, "name"), FakeFonts(M(c, "fonts"))));
        if (M(c, "error").IsString())
        {
            EXPECT_FALSE(res.ok);
            EXPECT_STR_EQ(res.error, S(c, "error"));
            continue;
        }
        ExpectFiles(res, c);

        if (!M(c, "package").IsArray())
            continue;

        // The package: the same tar entries (the order of the asset folders aside: the web sorts by locale)
        String error;
        String package = UnityPackage(res.files, &error);
        ASSERT_FALSE(package.IsEmpty()) << error.Data();
        auto entries = ReadTar(Gunzip(package));
        ASSERT_EQ(entries.size(), (size_t)M(c, "package").Count());
        std::map<std::string, const TarItem*> byName;
        for (auto& e : entries)
            byName[e.name] = &e;
        for (auto& want : M(c, "package").Items())
        {
            std::string name = want.At(0)->Str().Data();
            auto it = byName.find(name);
            ASSERT_TRUE(it != byName.end()) << name;
            EXPECT_TRUE(it->second->header == Bytes(PipelineUtils::Base64Decode(want.At(1)->Str()))) << name;
            EXPECT_EQ(it->second->data.size(), (size_t)want.At(2)->Num()) << name;
            if (want.At(3)->IsString())
                EXPECT_EQ(it->second->data, std::string(want.At(3)->Str().Data())) << name;
        }
        EXPECT_STR_EQ(entries.back().name, "packagemanagermanifest/asset");
    }
}

TEST(UiDocExport, GoldenUnityExportOfHandWrittenStructures)
{
    for (auto& c : E("unityHand").Items())
    {
        SCOPED_TRACE(S(c, "name").Data());
        UiExportInput input;
        input.name = S(c, "inputName");
        input.folder = S(c, "folder");
        input.layout = S(c, "layout");
        input.seed = S(c, "seed");
        input.fonts = FakeFonts(M(c, "fonts"));
        for (auto& s : M(c, "sprites").Items())
        {
            UiExportSprite sprite;
            sprite.file = S(s, "file");
            sprite.png = PipelineUtils::Base64Decode(S(s, "b64"));
            sprite.width = M(s, "width").Num();
            sprite.height = M(s, "height").Num();
            if (M(s, "nineSlice").IsObject())
            {
                const Json& n = M(s, "nineSlice");
                sprite.nineSlice = UiNine{ M(n, "left").Num(), M(n, "top").Num(), M(n, "right").Num(), M(n, "bottom").Num() };
            }
            if (M(s, "density").IsNumber())
                sprite.density = M(s, "density").Num();
            if (M(s, "dpi").IsNumber())
                sprite.dpi = M(s, "dpi").Num();
            input.sprites.Add(sprite);
        }

        auto res = ExportUnity(input);
        if (M(c, "error").IsString())
        {
            EXPECT_FALSE(res.ok);
            EXPECT_STR_EQ(res.error, S(c, "error"));
            continue;
        }
        ExpectFiles(res, c);
    }
}

TEST(UiDocExport, GoldenJsonZipEntries)
{
    for (auto& c : E("json").Items())
    {
        SCOPED_TRACE(S(c, "name").Data());
        const Json* sc = StructureCase(S(c, "name"));
        ASSERT_TRUE(sc);
        UiDoc doc = Parse(S(*sc, "src"));
        UiKit kit;
        if (!M(*sc, "kit").IsNull())
            kit = StructureKit(M(*sc, "kit"));
        auto st = ToUiStructure(doc, M(*sc, "kit").IsNull() ? nullptr : &kit);

        Vector<String> missing;
        auto entries = JsonExportEntries(doc, st, "preview-bytes", &missing);
        EXPECT_TRUE(missing.IsEmpty());
        auto& want = M(c, "entries").Items();
        ASSERT_EQ(entries.Count(), want.Count());
        for (int i = 0; i < want.Count(); i++)
        {
            String name = want[i].At(0)->Str();
            const Json& value = *want[i].At(1);
            EXPECT_STR_EQ(entries[i].name, name);
            if (value.IsNumber())
                EXPECT_EQ(entries[i].data.Length(), (int)value.Num()) << name.Data();
            else if (UiLayout::HasSuffix(name, ".png") && name.Contains("/sprites/"))
                EXPECT_TRUE(Bytes(entries[i].data) == Bytes(PipelineUtils::Base64Decode(value.Str()))) << name.Data();
            else
                EXPECT_STR_EQ(entries[i].data, value.Str()) << name.Data();
        }
    }
}

// The entry point the card and the workspace call: each target written where the request says
TEST(UiDocExport, TheEntryPointWritesEachTarget)
{
    WorkDir work;

    auto unity = RequestFor("screen_kitA", "unity");
    unity.outputPath = work.path + "out/" + ExportFileName(unity.doc, "unity");
    auto u = RunExport(unity);
    ASSERT_TRUE(u.ok) << u.error.Data();
    EXPECT_EQ(u.written, Vector<String>({ unity.outputPath }));
    auto entries = ReadTar(Gunzip(PipelineUtils::ReadFileBytes(unity.outputPath)));
    EXPECT_TRUE(std::any_of(entries.begin(), entries.end(), [](const TarItem& e) { return e.data == "Assets/Generated/UI/Menu.prefab"; }));
    // The real fonts went in: the package is far larger than the stand-ins
    EXPECT_GT(PipelineUtils::ReadFileBytes(unity.outputPath).Length(), 100000);

    auto json = RequestFor("dpi_kit", "json");
    json.outputPath = work.path + ExportFileName(json.doc, "json");
    json.picture = "preview-bytes";
    auto j = RunExport(json);
    ASSERT_TRUE(j.ok) << j.error.Data();
    Vector<PipelineZip::Entry> zip;
    String error;
    ASSERT_TRUE(PipelineZip::Read(PipelineUtils::ReadFileBytes(json.outputPath), zip, error)) << error.Data();
    Vector<String> names;
    for (auto& e : zip)
        names.Add(e.name);
    EXPECT_EQ(names, Vector<String>({ "M/ui_structure.json", "M/layout.uiml", "M/layout.json", "M/sprites/btn.png", "M/kit.json",
                                      "M/fonts/Roboto-Bold.ttf", "M/fonts/LICENSE-Roboto.txt", "M/preview.png" }));

    auto missingFile = RequestFor("screen_kitA", "unity");
    auto none = RunExport(missingFile);
    EXPECT_FALSE(none.ok);
    EXPECT_STR_EQ(none.error, "no file to write the export to");
}

// The o2 target into a project's assets that are not the editor's: sprites, fonts and metas written with the uids the
// web's o2 export derives from the node id, the .proto left to the editor that owns the assets
TEST(UiDocExport, TheO2ExportWritesTheResourcesWithTheWebsUids)
{
    WorkDir work;
    auto o2 = RequestFor("screen_kitA", "o2");
    o2.assetsPath = work.path;
    auto res = RunExport(o2);
    ASSERT_TRUE(res.ok) << res.error.Data();
    EXPECT_TRUE(res.prototype.IsEmpty());
    EXPECT_TRUE(res.warnings.Any([](const String& w) { return w.Contains("the .proto not"); }));
    EXPECT_TRUE(res.written.Contains("Generated/UI/Menu_Resources/Sprites/btn_red.png"));

    int compared = 0;
    for (auto& f : M(E("o2"), "files").Items())
    {
        String path = f.At(0)->Str().SubStr(7); // Without "Assets/"
        String meta = PipelineUtils::ReadFileBytes(work.path + path);
        if (meta.IsEmpty())
            continue;
        Json value;
        ASSERT_TRUE(Json::Parse(meta, value)) << path.Data();
        EXPECT_STR_EQ(S(M(value, "Value"), "mId"), f.At(1)->Str()) << path.Data();
        compared++;
    }
    EXPECT_GE(compared, 8);
}

// A sprite drawn for another density goes to o2 resampled to the layout's, its borders (the meta's and the ones the
// layout gives it) with it
TEST(UiDocExport, TheO2ExportResamplesDenserSprites)
{
    const Json* c = StructureCase("dpi_kit");
    UiDoc doc = Parse(S(*c, "src"));
    UiKit kit = StructureKit(M(*c, "kit"));
    auto st = ToUiStructure(doc, &kit);
    ASSERT_EQ(st.sprites.Count(), 1);
    ASSERT_TRUE(st.sprites[0].density);
    EXPECT_EQ(*st.sprites[0].density, 2);

    auto request = O2ExportRequest(doc, st, "", "/x/", "seed");
    EXPECT_STR_EQ(request.folder, "Generated/UI");
    EXPECT_STR_EQ(request.seed, "seed");
    ASSERT_EQ(request.sprites.Count(), 1);
    UiLayout::UiImage img;
    ASSERT_TRUE(UiLayout::UiImage::Decode(request.sprites[0].png, img));
    EXPECT_EQ(img.w, (int)JsRound(st.sprites[0].width / 2));
    EXPECT_EQ(img.h, (int)JsRound(st.sprites[0].height / 2));
    ASSERT_TRUE(request.sprites[0].nine);
    EXPECT_EQ(request.sprites[0].nine->left, 20);
    EXPECT_EQ(request.sprites[0].nine->top, 20);

    Json layout;
    ASSERT_TRUE(Json::Parse(request.layout, layout));
    std::function<const Json*(const Json&)> find = [&](const Json& v) -> const Json*
    {
        if (v.IsObject() && v.Get("nineSlice") && v.Get("asset"))
            return &v;
        for (auto& m : v.Members())
            if (auto r = find(m.second)) return r;
        for (auto& i : v.Items())
            if (auto r = find(i)) return r;
        return nullptr;
    };
    const Json* node = find(layout);
    ASSERT_TRUE(node);
    EXPECT_EQ(M(M(*node, "nineSlice"), "left").Num(), 20);
    EXPECT_EQ(M(M(*node, "nineSlice"), "bottom").Num(), 20);
}
