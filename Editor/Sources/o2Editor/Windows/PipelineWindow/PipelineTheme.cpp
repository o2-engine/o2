#include "o2Editor/stdafx.h"
#include "PipelineTheme.h"

#include "o2/Assets/Types/FontAsset.h"
#include "o2/Render/VectorFont.h"
#include "o2/Utils/FileSystem/FileSystem.h"
#include "o2/Utils/Serialization/DataValue.h"
#include "o2Editor/Pipeline/PipelinePairLayout.h"
#include "o2Editor/Pipeline/PipelineUtils.h"

namespace Editor
{
    static PipelinePalette MakeDark()
    {
        PipelinePalette p;
        p.dark = true;

        p.canvasInner = Color4(0x22, 0x26, 0x34, 255);
        p.canvasMid = Color4(0x18, 0x1A, 0x24, 255);
        p.canvasOuter = Color4(0x10, 0x11, 0x16, 255);
        p.gridDot = Color4(255, 255, 255, 15);
        p.gridDotMajor = Color4(255, 255, 255, 41);

        p.card = Color4(33, 36, 46, 237);
        p.cardSolid = Color4(0x21, 0x24, 0x2E, 255);
        p.cardStrokeTop = Color4(255, 255, 255, 56);
        p.cardStrokeMid = Color4(255, 255, 255, 20);
        p.cardStrokeBottom = Color4(255, 255, 255, 15);
        p.cardHighlight = Color4(255, 255, 255, 18);
        p.headerBand = Color4(0, 0, 0, 0);
        p.headerLine = Color4(0, 0, 0, 0);
        p.cardShadow = Color4(0, 0, 0, 102);
        p.shadowOffset = 12.0f;
        p.shadowBlur = 28.0f;
        p.pictureShadow = Color4(0, 0, 0, 89);
        p.pictureShadowOffset = 6.0f;
        p.pictureShadowBlur = 16.0f;

        p.bg = Color4(0x10, 0x11, 0x16, 255);
        p.bg2 = Color4(0x1B, 0x1D, 0x26, 255);
        p.bg3 = Color4(0x26, 0x2A, 0x36, 255);
        p.fg = Color4(0xF1, 0xF2, 0xF6, 255);
        p.fgDim = Color4(0x9A, 0xA0, 0xAE, 255);
        p.fg3 = Color4(0x6C, 0x72, 0x80, 255);
        p.border = Color4(255, 255, 255, 26);
        p.line = Color4(255, 255, 255, 22);
        p.field = Color4(255, 255, 255, 13);
        p.active = Color4(255, 255, 255, 23);
        p.surface = Color4(33, 36, 46, 237);

        p.accent = Color4(0xB7, 0x9C, 0xFF, 255);
        p.accentInk = Color4(0x1A, 0x10, 0x30, 255);
        p.accentHover = Color4(0xC9, 0xB4, 0xFF, 255);
        p.accentSoft = Color4(183, 156, 255, 41);

        p.portText = Color4(0x7F, 0xB2, 0xFF, 255);
        p.portImage = Color4(0xFF, 0xB0, 0x70, 255);
        p.portVideo = Color4(0xFF, 0x7A, 0xB6, 255);
        p.portAudio = Color4(0x6F, 0xE3, 0xCF, 255);

        p.green = Color4(0x62, 0xD5, 0x9A, 255);
        p.yellow = Color4(0xF1, 0xBE, 0x5B, 255);
        p.red = Color4(0xFF, 0x7A, 0x7A, 255);

        p.checkerA = Color4(0x1B, 0x1D, 0x25, 255);
        p.checkerB = Color4(0x22, 0x25, 0x2E, 255);
        return p;
    }

    static PipelinePalette MakeLight()
    {
        PipelinePalette p;
        p.dark = false;

        p.canvasInner = Color4(0xE1, 0xE8, 0xE8, 255);
        p.canvasMid = p.canvasInner;
        p.canvasOuter = p.canvasInner;
        p.gridDot = Color4(96, 125, 139, 71);
        p.gridDotMajor = Color4(96, 125, 139, 140);

        p.card = Color4(0xFB, 0xFB, 0xFB, 255);
        p.cardSolid = Color4(0xFB, 0xFB, 0xFB, 255);
        p.cardStrokeTop = Color4(0xB8, 0xC5, 0xCB, 255);
        p.cardStrokeMid = p.cardStrokeTop;
        p.cardStrokeBottom = p.cardStrokeTop;
        p.cardHighlight = Color4(0, 0, 0, 0);
        p.headerBand = Color4(0xEE, 0xF0, 0xF1, 255);
        p.headerLine = Color4(96, 125, 139, 71);
        p.cardShadow = Color4(38, 50, 56, 51);
        p.shadowOffset = 2.0f;
        p.shadowBlur = 8.0f;
        p.pictureShadow = Color4(38, 50, 56, 36);
        p.pictureShadowOffset = 2.0f;
        p.pictureShadowBlur = 6.0f;

        p.bg = Color4(0xE9, 0xED, 0xEE, 255);
        p.bg2 = Color4(0xF4, 0xF6, 0xF7, 255);
        p.bg3 = Color4(0xDD, 0xE3, 0xE5, 255);
        p.fg = Color4(0x60, 0x7D, 0x8B, 255);
        p.fgDim = Color4(0x8F, 0xA3, 0xAD, 255);
        p.fg3 = Color4(0xB0, 0xBE, 0xC5, 255);
        p.border = Color4(0xCF, 0xD8, 0xDC, 255);
        p.line = Color4(0xDE, 0xE4, 0xE8, 255);
        p.field = Color4(0xF7, 0xF7, 0xF7, 255);
        p.active = Color4(96, 125, 139, 31);
        p.surface = Color4(251, 251, 251, 247);

        p.accent = Color4(0x00, 0x96, 0x88, 255);
        p.accentInk = Color4(255, 255, 255, 255);
        p.accentHover = Color4(0x26, 0xA6, 0x9A, 255);
        p.accentSoft = Color4(0, 150, 136, 36);

        p.portText = Color4(0x21, 0x96, 0xF3, 255);
        p.portImage = Color4(0xFF, 0x98, 0x00, 255);
        p.portVideo = Color4(0x9C, 0x27, 0xB0, 255);
        p.portAudio = Color4(0x00, 0x96, 0x88, 255);

        p.green = Color4(0x4C, 0xAF, 0x50, 255);
        p.yellow = Color4(0xFF, 0xB3, 0x00, 255);
        p.red = Color4(0xE5, 0x53, 0x3D, 255);

        p.checkerA = Color4(0xAA, 0xAB, 0xAE, 255);
        p.checkerB = Color4(0xA2, 0xA3, 0xA6, 255);
        return p;
    }

    Color4 PipelinePalette::PortColor(PipelinePortType type) const
    {
        switch (type)
        {
            case PipelinePortType::Image: return portImage;
            case PipelinePortType::Video: return portVideo;
            case PipelinePortType::Audio: return portAudio;
            default: return portText;
        }
    }

    namespace PipelineTheme
    {
        static String cachedPath;
        static PipelineThemeKind cachedKind = PipelineThemeKind::Dark;
        static int version = 0;

        static PipelineThemeKind ReadKind()
        {
            // Read once per work folder: every control asks
            String path = PipelinePairLayout::GetViewPrefsPath();
            if (path == cachedPath)
                return cachedKind;

            cachedPath = path;
            cachedKind = PipelineThemeKind::Dark;
            DataDocument doc;
            if (o2FileSystem.IsFileExist(path) && doc.LoadFromFile(path) && doc.IsObject())
            {
                auto value = doc.FindMember("theme");
                if (value && value->IsString() && String(value->GetString()) == "light")
                    cachedKind = PipelineThemeKind::Light;
            }
            return cachedKind;
        }

        const PipelinePalette& Of(PipelineThemeKind kind)
        {
            static const PipelinePalette dark = MakeDark();
            static const PipelinePalette light = MakeLight();
            return kind == PipelineThemeKind::Light ? light : dark;
        }

        const PipelinePalette& Get()
        {
            return Of(ReadKind());
        }

        PipelineThemeKind GetKind()
        {
            return ReadKind();
        }

        void SetKind(PipelineThemeKind kind)
        {
            String path = PipelinePairLayout::GetViewPrefsPath();
            DataDocument doc;
            if (!o2FileSystem.IsFileExist(path) || !doc.LoadFromFile(path) || !doc.IsObject())
                doc.SetObject();

            doc["theme"] = String(kind == PipelineThemeKind::Light ? "light" : "dark");
            o2FileSystem.FolderCreate(PipelineUtils::GetWorkPath(), true);
            doc.SaveToFile(path);

            bool changed = ReadKind() != kind;
            cachedPath = path;
            cachedKind = kind;
            if (changed)
                version++;
        }

        int GetVersion()
        {
            // A work folder switch (tests, another project) may bring another stored theme
            static String versionPath;
            static PipelineThemeKind versionKind = PipelineThemeKind::Dark;
            PipelineThemeKind kind = ReadKind();
            if (versionPath != cachedPath || versionKind != kind)
            {
                if (!versionPath.IsEmpty())
                    version++;
                versionPath = cachedPath;
                versionKind = kind;
            }
            return version;
        }

        // The pipeline fonts take sizes in half pixels and draw unhinted, as the browser does
        static const String& Configured(const String& path)
        {
            static Vector<String> done;
            if (done.Contains(path))
                return path;

            AssetRef<FontAsset> asset(path);
            if (asset)
            {
                if (auto font = DynamicCast<VectorFont>(asset->GetFont()))
                {
                    font->SetResolution(fontResolution);
                    font->SetHinting(false);
                    done.Add(path);
                }
            }
            return path;
        }

        String Font(int weight /*= 400*/)
        {
            static const String semiBold = "ui/pipeline/fonts/Geologica-SemiBold.ttf";
            static const String medium = "ui/pipeline/fonts/Geologica-Medium.ttf";
            static const String regular = "ui/pipeline/fonts/Geologica-Regular.ttf";
            if (weight >= 600)
                return Configured(semiBold);
            if (weight >= 500)
                return Configured(medium);
            return Configured(regular);
        }

        String MonoFont()
        {
            static const String mono = "ui/pipeline/fonts/IBMPlexMono-Regular.ttf";
            return Configured(mono);
        }

        int Size(float pixels)
        {
            return Math::RoundToInt(pixels * 72.0f / fontResolution);
        }

        Color4 Alpha(const Color4& color, float alpha)
        {
            return Color4(color.r, color.g, color.b, (int)Math::Clamp(color.a * alpha, 0.0f, 255.0f));
        }
    }
}
// --- META ---

ENUM_META(Editor::PipelineThemeKind, Editor__PipelineThemeKind)
{
    ENUM_ENTRY(Dark);
    ENUM_ENTRY(Light);
}
END_ENUM_META;
// --- END META ---
