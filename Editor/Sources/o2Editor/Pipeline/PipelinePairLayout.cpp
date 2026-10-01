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
        g.cols = Math::Max(1, Math::Min(n, (int)Math::Floor((g.paneW + gridGap)/(minPaneCell + gridGap))));
        g.cellW = (g.paneW - gridGap*(g.cols - 1))/g.cols;
        g.rows = Math::Max(1, (n + g.cols - 1)/g.cols);

        float cellNatural = Math::Min(resultHeight, Math::Round(g.cellW));
        float gridNatural = g.rows*(cellNatural + g.labelH) + (g.rows - 1)*gridGap;
        g.rowNatural = Math::Max(resultHeight, gridNatural);
        g.rowH = Math::Max(g.rowNatural, rowHeight);
        g.cellH = (g.rowH - g.rows*g.labelH - (g.rows - 1)*gridGap)/g.rows;
        return g;
    }

    Vec2F ExtractCellCorner(const PipelineExtractGrid& g, int index)
    {
        int col = index % g.cols, row = index / g.cols;
        return Vec2F(g.paneW + paneGap + col*(g.cellW + gridGap) + g.cellW, row*(g.cellH + g.labelH + gridGap) + g.cellH);
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
