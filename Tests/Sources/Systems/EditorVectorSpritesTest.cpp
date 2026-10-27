#include "o2/stdafx.h"
#include <gtest/gtest.h>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <map>
#include <regex>
#include <set>
#include <sstream>

#include "o2/EngineSettings.h"
#include "o2/Render/VectorGraphics/SvgParser.h"
#include "o2/Render/VectorGraphics/VectorRasterizer.h"
#include "o2/Render/VectorGraphics/VectorTessellator.h"
#include "o2/Utils/Bitmap/BitmapCompare.h"
#include "o2/Utils/Serialization/DataValue.h"

using namespace o2;

// Every vector sprite of the editor UI against the raster original it replaces, and the image paths the editor refers to

namespace
{
    namespace fs = std::filesystem;

    const float minSimilarity = 0.98f;
    const float maxMeanDifference = 3.0f;
    const int tolerance = 24;

    struct Limits
    {
        float similarity;
        float meanDifference;
    };

    // Sprites below the common criterion, each with its own floor: measured similarity and mean difference
    const std::map<std::string, Limits> knownDeviations =
    {
    };

    // Raster images the editor code keeps although a vector twin exists: Sprite features VectorSprite has not
    const std::set<std::string> rasterOnlyUsages =
    {
        "ui/pipeline/checker.png",  // tiled
        "ui/UI4_animation_bar.png", // corner colors
    };

    std::string UiFolder()
    {
        return std::string(GetEditorAssetsPath()) + "ui";
    }

    std::string ReadText(const fs::path& path)
    {
        std::ifstream file(path, std::ios::binary);
        std::stringstream content;
        content << file.rdbuf();
        return content.str();
    }

    // Names of the sprites having both X.svg and X.png, relative to the ui folder and without extension
    std::vector<std::string> CollectPairs()
    {
        std::vector<std::string> res;
        std::error_code errorCode;
        for (auto& entry : fs::recursive_directory_iterator(UiFolder(), errorCode))
        {
            if (!entry.is_regular_file() || entry.path().extension() != ".svg")
                continue;

            fs::path png = entry.path();
            png.replace_extension(".png");
            if (!fs::exists(png))
                continue;

            fs::path name = fs::relative(entry.path(), UiFolder());
            name.replace_extension();
            res.push_back(name.generic_string());
        }

        std::sort(res.begin(), res.end());
        return res;
    }

    bool LoadVectorImage(const std::string& name, VectorImage& image)
    {
        std::string text = ReadText(UiFolder() + "/" + name + ".svg");
        String error;
        Vector<String> warnings;
        bool parsed = SvgParser::Parse(text.c_str(), (UInt)text.size(), image, error, warnings);
        EXPECT_TRUE(parsed) << name << ": " << error.Data();
        EXPECT_TRUE(warnings.IsEmpty()) << name << ": " << (warnings.IsEmpty() ? "" : warnings[0].Data());
        return parsed;
    }

    VectorTessellationParams ScalableParams()
    {
        VectorTessellationParams params;
        params.scalable = true;
        params.pixelScale = Vec2F(1, 1);
        return params;
    }

    // Sprites whose mesh built once for any scale is off the mesh of the scale by more than the common criterion:
    // translucent shapes one over another and parts thinner than a pixel
    const std::set<std::string> scalableDeviations =
    {
        "CurveHandle", "CurveHandleHover", "CurveHandlePressed", "CurveHandleSelected", "CurveSupportHandle",
        "UI4_keys_select border", "UI4_keys_select border_hover", "UI4_keys_select border_pressed", "UI4_map_key",
        "UI4_map_key_hover", "UI4_save_gray", "UI4_scene_icon", "UI4_shadow_separator", "pipeline/btn_region",
        "pipeline/menu_node_effect", "pipeline/menu_node_video", "pipeline/al_cmp_grip",
    };

    const Limits scalableDeviationLimits = { 0.8f, 9.0f };

    // Worst similarity and mean difference of the sprite over the backgrounds, as fit_check.py measures them
    bool Measure(const std::string& name, Limits& measured)
    {
        VectorImage image;
        Bitmap reference;
        if (!LoadVectorImage(name, image) || !reference.Load((UiFolder() + "/" + name + ".png").c_str(), Bitmap::ImageType::Png))
            return false;

        VectorMesh mesh;
        VectorTessellator::Tessellate(image, mesh);

        measured = { 1.0f, 0.0f };
        for (const Color4& background : { Color4(0x60, 0x60, 0x60, 255), Color4(255, 255, 255, 255), Color4(0, 0, 0, 255) })
        {
            Bitmap rendered(PixelFormat::R8G8B8A8, reference.GetSize());
            memset(rendered.GetData(), 0, (size_t)reference.GetSize().x*reference.GetSize().y*4);
            BitmapCompare::CompositeOver(rendered, background);
            VectorRasterizer::Rasterize(mesh, rendered, 1.0f);

            BitmapCompareResult result = BitmapCompare::Compare(rendered, reference, tolerance, background);
            if (!result.comparable)
                return false;

            measured.similarity = Math::Min(measured.similarity, result.similarity);
            measured.meanDifference = Math::Max(measured.meanDifference, result.meanDifference);
        }

        return true;
    }

    // A cast of a data value to String is ambiguous for MSVC
    String AsString(const DataValue& value)
    {
        String res;
        value.Get(res);
        return res;
    }

    DataDocument LoadMeta(const std::string& path)
    {
        DataDocument meta;
        EXPECT_TRUE(meta.LoadFromFile(path.c_str())) << path;
        return meta;
    }

    // Image paths in the string literals of the editor and framework sources, comments skipped
    std::map<std::string, std::string> CollectImageLiterals()
    {
        std::map<std::string, std::string> res;
        std::regex literal("\"(ui/[^\"]+\\.(png|svg))\"");

        std::string editorRoot = std::string(GetEditorAssetsPath()) + "../";
        for (const std::string& folder : { editorRoot + "Sources", editorRoot + "../Framework/Sources" })
        {
            std::error_code errorCode;
            for (auto& entry : fs::recursive_directory_iterator(folder, errorCode))
            {
                std::string extension = entry.path().extension().string();
                if (!entry.is_regular_file() || (extension != ".cpp" && extension != ".h" && extension != ".mm"))
                    continue;

                std::ifstream file(entry.path());
                std::string line;
                while (std::getline(file, line))
                {
                    size_t begin = line.find_first_not_of(" \t");
                    if (begin == std::string::npos || line.compare(begin, 2, "//") == 0 || line.find("\"ui/") == std::string::npos)
                        continue;

                    for (std::sregex_iterator it(line.begin(), line.end(), literal), end; it != end; ++it)
                        res[(*it)[1].str()] = entry.path().filename().string();
                }
            }
        }

        return res;
    }
}

TEST(EditorVectorSprites, ScalableMeshIsBuiltOnceForAnyScale)
{
    auto names = CollectPairs();
    ASSERT_GT(names.size(), 400u) << "no editor sprites at " << UiFolder();

    typedef std::chrono::steady_clock Clock;

    UInt64 exactTriangles[2] = { 0, 0 }, scalableTriangles = 0;
    double exactTime = 0, scalableTime = 0;
    float worstSimilarity = 1.0f;
    for (const std::string& name : names)
    {
        VectorImage image;
        ASSERT_TRUE(LoadVectorImage(name, image)) << name;

        auto start = Clock::now();
        VectorMesh scalable;
        VectorTessellator::Tessellate(image, scalable, ScalableParams());
        scalableTime += std::chrono::duration<double>(Clock::now() - start).count();
        scalableTriangles += scalable.GetTrianglesCount();

        int step = 0;
        for (float scale : { 1.0f, 2.0f })
        {
            VectorTessellationParams params;
            params.pixelScale = Vec2F(scale, scale);

            start = Clock::now();
            VectorMesh exact;
            VectorTessellator::Tessellate(image, exact, params);
            exactTime += std::chrono::duration<double>(Clock::now() - start).count();
            exactTriangles[step++] += exact.GetTrianglesCount();

            scalable.Rescale(Vec2F(scale, scale));

            Ref<Bitmap> expected = VectorRasterizer::Rasterize(exact, scale);
            Ref<Bitmap> actual = VectorRasterizer::Rasterize(scalable, scale);
            BitmapCompareResult result = BitmapCompare::Compare(*expected, *actual, tolerance);
            worstSimilarity = Math::Min(worstSimilarity, result.similarity);

            Limits limits = scalableDeviations.count(name) ? scalableDeviationLimits : Limits{ minSimilarity, maxMeanDifference };
            EXPECT_TRUE(result.similarity >= limits.similarity && result.meanDifference <= limits.meanDifference)
                << name << " at scale " << scale << ": similarity " << result.similarity << ", mean difference "
                << result.meanDifference;
        }
    }

    EXPECT_LT(scalableTriangles, exactTriangles[0]*3) << "a mesh for any scale must not cost much more than the one of a scale";

    printf("[   INFO   ] %d sprites: scalable %llu triangles in %.0f ms; exact %llu at 1x + %llu at 2x in %.0f ms; "
           "worst similarity %.4f\n", (int)names.size(), (unsigned long long)scalableTriangles, scalableTime*1000.0,
           (unsigned long long)exactTriangles[0], (unsigned long long)exactTriangles[1], exactTime*1000.0,
           worstSimilarity);
}

TEST(EditorVectorSprites, EverySpriteMatchesItsRasterOriginal)
{
    auto names = CollectPairs();
    ASSERT_GT(names.size(), 400u) << "no editor sprites at " << UiFolder();

    int exact = 0;
    double similaritySum = 0;
    for (const std::string& name : names)
    {
        Limits measured;
        ASSERT_TRUE(Measure(name, measured)) << name;

        Limits limits = { minSimilarity, maxMeanDifference };
        auto deviation = knownDeviations.find(name);
        if (deviation != knownDeviations.end())
        {
            limits = deviation->second;
            EXPECT_TRUE(measured.similarity < minSimilarity || measured.meanDifference > maxMeanDifference)
                << name << " meets the common criterion now, remove it from knownDeviations: similarity "
                << measured.similarity << ", mean difference " << measured.meanDifference;
        }

        EXPECT_TRUE(measured.similarity >= limits.similarity && measured.meanDifference <= limits.meanDifference)
            << name << ": similarity " << measured.similarity << ", mean difference " << measured.meanDifference;

        similaritySum += measured.similarity;
        if (measured.similarity >= 1.0f)
            exact++;
    }

    for (auto& deviation : knownDeviations)
        EXPECT_TRUE(std::find(names.begin(), names.end(), deviation.first) != names.end()) << deviation.first << " is not a sprite";

    printf("  %d sprites, mean similarity %.4f at tolerance %d, %d fully within the tolerance, %d known deviations\n",
           (int)names.size(), similaritySum/(double)names.size(), tolerance, exact, (int)knownDeviations.size());
}

TEST(EditorVectorSprites, SimplifiedMeshesDrawTheSameWithLessTriangles)
{
    const float minReduction = 0.1f;
    const int maxDifference = 2;

    auto names = CollectPairs();
    ASSERT_GT(names.size(), 400u) << "no editor sprites at " << UiFolder();

    std::vector<UInt> full, simplified;
    double fullTime = 0, simplifiedTime = 0;
    for (const std::string& name : names)
    {
        VectorImage image;
        ASSERT_TRUE(LoadVectorImage(name, image)) << name;

        VectorTessellationParams params;
        VectorMesh simplifiedMesh, fullMesh;

        auto start = std::chrono::steady_clock::now();
        VectorTessellator::Tessellate(image, simplifiedMesh, params);
        simplifiedTime += std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();

        params.simplification = 0.0f;
        start = std::chrono::steady_clock::now();
        VectorTessellator::Tessellate(image, fullMesh, params);
        fullTime += std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();

        EXPECT_LE(simplifiedMesh.GetTrianglesCount(), fullMesh.GetTrianglesCount()) << name;
        full.push_back(fullMesh.GetTrianglesCount());
        simplified.push_back(simplifiedMesh.GetTrianglesCount());

        Color4 background(0x60, 0x60, 0x60, 255);
        auto fullBitmap = VectorRasterizer::Rasterize(fullMesh, 1.0f, background);
        auto simplifiedBitmap = VectorRasterizer::Rasterize(simplifiedMesh, 1.0f, background);

        BitmapCompareResult result = BitmapCompare::Compare(*fullBitmap, *simplifiedBitmap, 0, background);
        ASSERT_TRUE(result.comparable) << name;
        EXPECT_LE(result.maxDifference, maxDifference) << name;
    }

    auto describe = [](std::vector<UInt> counts)
    {
        std::sort(counts.begin(), counts.end());

        UInt64 sum = 0;
        for (UInt count : counts)
            sum += count;

        return String::Format("median %u, mean %.1f, p90 %u, max %u, sum %llu", counts[counts.size()/2],
                              (double)sum/(double)counts.size(), counts[counts.size()*9/10], counts.back(),
                              (unsigned long long)sum);
    };

    UInt64 fullSum = 0, simplifiedSum = 0;
    for (size_t i = 0; i < full.size(); i++)
    {
        fullSum += full[i];
        simplifiedSum += simplified[i];
    }

    printf("  %d sprites, triangles of all vertices: %s\n", (int)full.size(), describe(full).Data());
    printf("  %d sprites, triangles simplified:      %s\n", (int)full.size(), describe(simplified).Data());
    printf("  tessellation of all vertices %.0f ms, with the simplification %.0f ms\n", fullTime, simplifiedTime);

    EXPECT_LE((float)simplifiedSum, (float)fullSum*(1.0f - minReduction));
}

TEST(EditorVectorSprites, SizeEqualsRasterOriginal)
{
    for (const std::string& name : CollectPairs())
    {
        VectorImage image;
        Bitmap reference;
        ASSERT_TRUE(LoadVectorImage(name, image)) << name;
        ASSERT_TRUE(reference.Load((UiFolder() + "/" + name + ".png").c_str(), Bitmap::ImageType::Png)) << name;
        EXPECT_EQ(Vec2F(image.size), Vec2F(reference.GetSize())) << name;
    }
}

TEST(EditorVectorSprites, MetaRepeatsSlicesAndModeOfRasterOriginal)
{
    std::map<std::string, std::string> ids;
    std::error_code errorCode;
    for (auto& entry : fs::recursive_directory_iterator(GetEditorAssetsPath(), errorCode))
    {
        if (!entry.is_regular_file() || entry.path().extension() != ".meta")
            continue;

        DataDocument meta = LoadMeta(entry.path().string());
        std::string id = AsString(meta["Value"]["mId"]).Data();
        if (id.empty())
            continue;

        auto inserted = ids.insert({ id, entry.path().generic_string() });
        EXPECT_TRUE(inserted.second) << "asset id " << id << " is shared by " << inserted.first->second << " and "
            << entry.path().generic_string();
    }

    for (const std::string& name : CollectPairs())
    {
        DataDocument vectorMeta = LoadMeta(UiFolder() + "/" + name + ".svg.meta");
        DataDocument rasterMeta = LoadMeta(UiFolder() + "/" + name + ".png.meta");

        EXPECT_EQ(AsString(vectorMeta["Type"]), String("o2::VectorImageAsset::Meta")) << name;
        EXPECT_EQ(AsString(vectorMeta["Value"]["mId"]).Length(), 32) << name;
        EXPECT_FALSE(vectorMeta["Value"].FindMember("atlasId")) << name;

        BorderI vectorSlices, rasterSlices;
        if (auto slices = vectorMeta["Value"].FindMember("sliceBorder"))
            vectorSlices = *slices;

        if (auto slices = rasterMeta["Value"].FindMember("sliceBorder"))
            rasterSlices = *slices;

        EXPECT_EQ(vectorSlices, rasterSlices) << name;

        String vectorMode, rasterMode;
        if (auto mode = vectorMeta["Value"].FindMember("defaultMode"))
            vectorMode = *mode;

        if (auto mode = rasterMeta["Value"].FindMember("defaultMode"))
            rasterMode = *mode;

        EXPECT_EQ(vectorMode, rasterMode) << name;
    }
}

TEST(EditorVectorSprites, ImagePathsOfEditorCodeResolveToAssets)
{
    auto literals = CollectImageLiterals();
    ASSERT_GT(literals.size(), 300u);

    std::string assets = GetEditorAssetsPath();
    int vectorCount = 0;
    for (auto& literal : literals)
    {
        const std::string& path = literal.first;
        EXPECT_TRUE(fs::exists(assets + path)) << path << " in " << literal.second;
        EXPECT_TRUE(fs::exists(assets + path + ".meta")) << path << " in " << literal.second;

        bool isVector = path.size() > 4 && path.compare(path.size() - 4, 4, ".svg") == 0;
        if (isVector)
        {
            vectorCount++;
            EXPECT_TRUE(fs::exists(assets + path.substr(0, path.size() - 3) + "png"))
                << path << " has no raster original to fall back to";
        }
        else if (!rasterOnlyUsages.count(path))
        {
            EXPECT_FALSE(fs::exists(assets + path.substr(0, path.size() - 3) + "svg"))
                << path << " in " << literal.second << " has a vector twin, use it";
        }

        // PipelineNodeWidget::MenuIconForType builds the menu icon path from the node icon one
        size_t node = path.find("/node_");
        if (node != std::string::npos)
        {
            std::string menuIcon = path;
            menuIcon.replace(node, 6, "/menu_node_");
            EXPECT_TRUE(fs::exists(assets + menuIcon)) << menuIcon;
        }
    }

    EXPECT_GT(vectorCount, 300);
}
