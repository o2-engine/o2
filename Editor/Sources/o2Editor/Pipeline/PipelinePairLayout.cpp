#include "o2Editor/stdafx.h"
#include "PipelinePairLayout.h"

#include "o2/Utils/FileSystem/FileSystem.h"
#include "o2/Utils/Serialization/DataValue.h"
#include "o2/Utils/Types/Containers/Map.h"
#include "o2Editor/Pipeline/PipelineUtils.h"

namespace Editor::PipelinePairLayout
{
    static PipelineIoView cachedView = PipelineIoView::SideBySide;
    static String         cachedPath;
    static int            viewVersion = 0;
    static Map<String, float> dividers;

    bool IsPairNode(const String& type)
    {
        return type == "imageEdit" || type == "imageExtract" || type == "aiRemoveBg" || type == "removeBackground" ||
            type == "imageOutline" || type == "imageShadow" || type == "imageGradient" || type == "imageColor" || type == "aiUpscale";
    }

    String InputPortOf(const String& type)
    {
        return type == "removeBackground" ? "white" : "image";
    }

    float PaneWidth(float rowWidth)
    {
        return (rowWidth - paneGap)*0.5f;
    }

    float PairRowHeight(bool anyImage)
    {
        return anyImage ? resultHeight : emptyHeight;
    }

    PipelineExtractGrid ExtractGrid(float rowWidth, int parts, float rowHeight /*= 0.0f*/)
    {
        int n = Math::Max(1, parts);
        PipelineExtractGrid g;
        g.paneW = PaneWidth(rowWidth);
        g.labelH = n == 1 ? 0.0f : gridLabel;
        int natCols = Math::Max(1, Math::Min(n, (int)Math::Floor((g.paneW + gridGap)/(minPaneCell + gridGap))));
        int natRows = Math::Max(1, (n + natCols - 1)/natCols);
        float cellNatural = Math::Min(resultHeight, Math::Round((g.paneW - gridGap*(natCols - 1))/natCols));
        float gridNatural = natRows*(cellNatural + g.labelH) + (natRows - 1)*gridGap;
        g.rowNatural = Math::Max(resultHeight, gridNatural);
        g.rowH = Math::Max(g.rowNatural, rowHeight);

        if (n == 1)
        {
            g.cellW = g.paneW;
            g.cellH = g.rowH;
            return g;
        }

        float side = -1.0f;
        for (int c = 1; c <= n; c++)
        {
            int r = (n + c - 1)/c;
            float s = Math::Floor(Math::Min((g.paneW - gridGap*(c - 1))/c, (g.rowH - r*g.labelH - gridGap*(r - 1))/r));
            if (s > side)
            {
                g.cols = c;
                g.rows = r;
                side = s;
            }
        }
        side = Math::Max(1.0f, side);
        g.cellW = g.cellH = side;
        g.offX = Math::Max(0.0f, Math::Floor((g.paneW - (g.cols*side + (g.cols - 1)*gridGap))*0.5f));
        g.offY = Math::Max(0.0f, Math::Floor((g.rowH - (g.rows*(side + g.labelH) + (g.rows - 1)*gridGap))*0.5f));
        return g;
    }

    Vec2F ExtractCellCorner(const PipelineExtractGrid& g, int index)
    {
        int col = index % g.cols, row = index / g.cols;
        return Vec2F(g.paneW + paneGap + g.offX + col*(g.cellW + gridGap) + g.cellW,
                     g.offY + row*(g.cellH + g.labelH + gridGap));
    }

    bool IsAutoHeight(const Vec2F& size)
    {
        return size.y <= 0.0f;
    }

    Vec2F NewNodeSize(const String& type, const Vec2F& schemaSize)
    {
        return IsPairNode(type) ? Vec2F(newNodeWidth, 0.0f) : schemaSize;
    }

    float HeightAfterResize(bool verticalEdge, float storedHeight, float draggedHeight)
    {
        if (!verticalEdge && storedHeight <= 0.0f)
            return 0.0f;

        return draggedHeight;
    }

    bool ShowsCompare(PipelineIoView view, const String& type, bool hasInput, bool hasResult, bool cropOn)
    {
        return view == PipelineIoView::Compare && type != "imageExtract" && hasInput && hasResult && !cropOn;
    }

    String GetViewPrefsPath()
    {
        return PipelineUtils::GetWorkPath() + "ViewPrefs.json";
    }

    PipelineIoView GetIoView()
    {
        // Read once per work folder: the cards ask on every rebuild
        String path = GetViewPrefsPath();
        if (path == cachedPath)
            return cachedView;

        cachedPath = path;
        cachedView = PipelineIoView::SideBySide;
        DataDocument doc;
        if (o2FileSystem.IsFileExist(path) && doc.LoadFromFile(path) && doc.IsObject())
        {
            auto value = doc.FindMember("ioView");
            if (value && value->IsString() && String(value->GetString()) == "compare")
                cachedView = PipelineIoView::Compare;
        }
        return cachedView;
    }

    void SetIoView(PipelineIoView view)
    {
        String path = GetViewPrefsPath();
        DataDocument doc;
        if (!o2FileSystem.IsFileExist(path) || !doc.LoadFromFile(path) || !doc.IsObject())
            doc.SetObject();

        doc["ioView"] = String(view == PipelineIoView::Compare ? "compare" : "side");
        o2FileSystem.FolderCreate(PipelineUtils::GetWorkPath(), true);
        doc.SaveToFile(path);

        cachedPath = path;
        cachedView = view;
        viewVersion++;
    }

    int GetIoViewVersion()
    {
        return viewVersion;
    }

    float GetDivider(const String& nodeId)
    {
        float value = 0.5f;
        return dividers.TryGetValue(nodeId, value) ? value : 0.5f;
    }

    void SetDivider(const String& nodeId, float value)
    {
        dividers[nodeId] = Math::Clamp01(value);
    }
}
// --- META ---

ENUM_META(Editor::PipelineIoView, Editor__PipelineIoView)
{
    ENUM_ENTRY(Compare);
    ENUM_ENTRY(SideBySide);
}
END_ENUM_META;
// --- END META ---
