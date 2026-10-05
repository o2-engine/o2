#include "o2/stdafx.h"

#include "o2/Render/VectorGraphics/SvgParser.h"
#include "o2/Render/VectorGraphics/VectorRasterizer.h"
#include "o2/Render/VectorGraphics/VectorTessellator.h"
#include "o2/Utils/Bitmap/BitmapCompare.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

using namespace o2;

namespace
{
    struct Options
    {
        float       scale = 1.0f;
        bool        hasScale = false;
        bool        unitsAsPixels = false;
        float       curveTolerance = VectorTessellationParams().curveTolerance;
        float       simplification = VectorTessellationParams().simplification;
        bool        antialiasing = true;
        int         tolerance = 8;
        Color4      background = Color4(255, 255, 255, 255);
        bool        hasBackground = false;
        bool        quiet = false;
        bool        stats = false;
        std::string outDir;
    };

    void PrintUsage()
    {
        printf("Usage:\n"
               "  o2SvgRasterizer <in.svg> <out.png> [options]\n"
               "  o2SvgRasterizer --batch <listfile> [options]\n"
               "  o2SvgRasterizer --compare <listfile> [--tolerance T] [--out-dir D] [options]\n"
               "options: --scale N, --no-aa, --bg RRGGBB, --curve-tolerance PX, --simplification LEVELS, --units-as-px,\n"
               "--quiet, --stats\n"
               "--simplification 0 keeps all vertices of the mesh\n"
               "--units-as-px reads pt, pc, mm, cm and in of the svg as pixels\n"
               "--stats prints triangles and vertices count of each converted svg\n"
               "listfile lines: svgPath<TAB>pngPath; --batch writes each png, --compare prints for each line\n"
               "similarity<TAB>meanAbsDiff<TAB>svgPath<TAB>pngPath, -1 when a file can't be read; without --scale\n"
               "the svg is scaled to the width of its png. Exit code is 1 when any file failed\n");
    }

    std::filesystem::path ToPath(const std::string& utf8)
    {
        return std::filesystem::path(std::u8string(utf8.begin(), utf8.end()));
    }

    bool LoadImage(const std::string& path, const Options& options, VectorImage& image)
    {
        std::ifstream file(ToPath(path), std::ios::binary);
        if (!file)
        {
            fprintf(stderr, "%s: can't open file\n", path.c_str());
            return false;
        }

        std::stringstream content;
        content << file.rdbuf();
        std::string text = content.str();

        SvgParseOptions parseOptions;
        parseOptions.unitsAsPixels = options.unitsAsPixels;

        String error;
        Vector<String> warnings;
        if (!SvgParser::Parse(text.c_str(), (UInt)text.size(), image, error, warnings, parseOptions))
        {
            fprintf(stderr, "%s: %s\n", path.c_str(), error.Data());
            return false;
        }

        if (!options.quiet)
        {
            for (const String& warning : warnings)
                fprintf(stderr, "%s: warning: %s\n", path.c_str(), warning.Data());
        }

        return true;
    }

    void Tessellate(const VectorImage& image, const Options& options, float scale, VectorMesh& mesh)
    {
        VectorTessellationParams params;
        params.pixelScale = Vec2F(scale, scale);
        params.antialiasing = options.antialiasing;
        params.curveTolerance = options.curveTolerance;
        params.simplification = options.simplification;
        VectorTessellator::Tessellate(image, mesh, params);
    }

    Ref<Bitmap> RenderStraight(const VectorMesh& mesh, float scale)
    {
        Ref<Bitmap> bitmap = VectorRasterizer::Rasterize(mesh, scale, Color4(0, 0, 0, 0));
        VectorRasterizer::Unpremultiply(*bitmap);
        return bitmap;
    }

    int Convert(const std::string& svgPath, const std::string& pngPath, const Options& options)
    {
        VectorImage image;
        if (!LoadImage(svgPath, options, image))
            return 1;

        VectorMesh mesh;
        Tessellate(image, options, options.scale, mesh);

        if (options.stats)
            printf("%u triangles, %u vertices\n", mesh.GetTrianglesCount(), (UInt)mesh.positions.Count());

        Ref<Bitmap> bitmap = options.hasBackground ?
            VectorRasterizer::Rasterize(mesh, options.scale, options.background) : RenderStraight(mesh, options.scale);

        if (!bitmap->Save(pngPath.c_str(), Bitmap::ImageType::Png))
        {
            fprintf(stderr, "%s: can't save file\n", pngPath.c_str());
            return 1;
        }

        return 0;
    }

    int ConvertBatch(const std::string& listPath, const Options& options)
    {
        std::ifstream list(ToPath(listPath));
        if (!list)
        {
            fprintf(stderr, "%s: can't open file\n", listPath.c_str());
            return 1;
        }

        int failed = 0;
        std::string line;
        while (std::getline(list, line))
        {
            if (!line.empty() && line.back() == '\r')
                line.pop_back();

            size_t tab = line.find('\t');
            if (line.empty() || tab == std::string::npos)
                continue;

            failed += Convert(line.substr(0, tab), line.substr(tab + 1), options);
        }

        return failed > 0 ? 1 : 0;
    }

    int Compare(const std::string& listPath, const Options& options)
    {
        std::ifstream list(ToPath(listPath));
        if (!list)
        {
            fprintf(stderr, "%s: can't open file\n", listPath.c_str());
            return 1;
        }

        if (!options.outDir.empty())
        {
            std::error_code errorCode;
            std::filesystem::create_directories(ToPath(options.outDir), errorCode);
        }

        int failed = 0;
        std::string line;
        while (std::getline(list, line))
        {
            if (!line.empty() && line.back() == '\r')
                line.pop_back();

            size_t tab = line.find('\t');
            if (line.empty() || tab == std::string::npos)
                continue;

            std::string svgPath = line.substr(0, tab), pngPath = line.substr(tab + 1);

            float similarity = -1.0f, meanDifference = -1.0f;

            VectorImage image;
            Bitmap reference;
            if (LoadImage(svgPath, options, image) && reference.Load(pngPath.c_str(), Bitmap::ImageType::Png))
            {
                float scale = options.hasScale || image.size.x <= 0.0f ? options.scale :
                    (float)reference.GetSize().x/image.size.x;

                VectorMesh mesh;
                Tessellate(image, options, scale, mesh);

                Bitmap rendered(PixelFormat::R8G8B8A8, reference.GetSize());
                memset(rendered.GetData(), 0, (size_t)reference.GetSize().x*reference.GetSize().y*4);
                BitmapCompare::CompositeOver(rendered, options.background);
                VectorRasterizer::Rasterize(mesh, rendered, scale);

                Bitmap difference;
                BitmapCompareResult result = BitmapCompare::Compare(rendered, reference, options.tolerance,
                                                                    options.background,
                                                                    options.outDir.empty() ? nullptr : &difference);
                similarity = result.similarity;
                meanDifference = result.meanDifference;

                if (!options.outDir.empty())
                {
                    std::string name = std::filesystem::path(svgPath).stem().string();
                    std::string base = (std::filesystem::path(options.outDir)/name).string();
                    RenderStraight(mesh, scale)->Save((base + ".png").c_str(), Bitmap::ImageType::Png);
                    difference.Save((base + ".diff.png").c_str(), Bitmap::ImageType::Png);
                }
            }

            else
                failed++;

            printf("%.5f\t%.3f\t%s\t%s\n", similarity, meanDifference, svgPath.c_str(), pngPath.c_str());
            fflush(stdout);
        }

        return failed > 0 ? 1 : 0;
    }
}

int main(int argc, char** argv)
{
    Options options;
    std::string compareList, batchList;
    std::vector<std::string> paths;

    for (int i = 1; i < argc; i++)
    {
        std::string argument = argv[i];
        bool hasValue = i + 1 < argc;

        if (argument == "--no-aa")
            options.antialiasing = false;
        else if (argument == "--quiet")
            options.quiet = true;
        else if (argument == "--stats")
            options.stats = true;
        else if (argument == "--units-as-px")
            options.unitsAsPixels = true;
        else if (argument == "--scale" && hasValue)
        {
            options.scale = (float)atof(argv[++i]);
            options.hasScale = true;
        }
        else if (argument == "--curve-tolerance" && hasValue)
            options.curveTolerance = (float)atof(argv[++i]);
        else if (argument == "--simplification" && hasValue)
            options.simplification = (float)atof(argv[++i]);
        else if (argument == "--batch" && hasValue)
            batchList = argv[++i];
        else if (argument == "--tolerance" && hasValue)
            options.tolerance = atoi(argv[++i]);
        else if (argument == "--out-dir" && hasValue)
            options.outDir = argv[++i];
        else if (argument == "--compare" && hasValue)
            compareList = argv[++i];
        else if (argument == "--bg" && hasValue)
        {
            if (!SvgParser::ParseColor(String("#") + argv[++i], options.background))
            {
                fprintf(stderr, "invalid --bg value, RRGGBB expected\n");
                return 2;
            }

            options.background.a = 255;
            options.hasBackground = true;
        }
        else if (argument.compare(0, 2, "--") == 0)
        {
            PrintUsage();
            return 2;
        }
        else
            paths.push_back(argument);
    }

    if (options.scale <= 0.0f)
    {
        options.scale = 1.0f;
        options.hasScale = false;
    }

    if (!compareList.empty() && paths.empty())
        return Compare(compareList, options);

    if (!batchList.empty() && paths.empty())
        return ConvertBatch(batchList, options);

    if (paths.size() == 2 && compareList.empty() && batchList.empty())
        return Convert(paths[0], paths[1], options);

    PrintUsage();
    return 2;
}
