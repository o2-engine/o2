#include "o2Editor/stdafx.h"
#include "PipelineComposerLayout.h"

#include "o2Editor/Pipeline/Nodes/PipelineNodesCommon.h"

namespace Editor::PipelineComposerLayout
{
    float SettingsHeight(bool nine)
    {
        return settingsHeight + (nine ? nineHeight : 0.0f);
    }

    PipelineComposerRows Rows(int layers, int openRow /*= -1*/, bool openNine /*= false*/)
    {
        PipelineComposerRows rows;
        float y = border + headHeight + listPadding;
        rows.addTopRow = y;
        y += addRowHeight + rowGap;

        rows.emptyTop = y;
        if (layers <= 0)
            y += rowHeight + rowGap;

        for (int i = 0; i < layers; i++)
        {
            rows.rowTops.Add(y);
            y += rowHeight;
            if (i == openRow)
            {
                rows.settingsTop = y;
                y += SettingsHeight(openNine);
            }
            y += rowGap;
        }

        rows.addBottomRow = y;
        rows.height = y + addRowHeight + listPadding + border;
        return rows;
    }

    bool HasExportSize(float exportW, float exportH)
    {
        return std::isfinite(exportW) && std::isfinite(exportH) && exportW >= 1.0f && exportH >= 1.0f;
    }

    Vec2I LayerExportSize(float w, float h, float exportW, float exportH)
    {
        if (HasExportSize(exportW, exportH))
            return Vec2I((int)Math::Round(exportW), (int)Math::Round(exportH));

        return Vec2I(Math::Max(1, (int)Math::Round(w)), Math::Max(1, (int)Math::Round(h)));
    }

    void TypeExportSide(float w, float h, bool lockAspect, bool horizontal, float value, float& exportW, float& exportH)
    {
        if (!(value >= 1.0f))
        {
            exportW = exportH = 0.0f;
            return;
        }

        bool follow = lockAspect || !HasExportSize(exportW, exportH);
        float ratio = h > 0.0f ? w/h : 1.0f;
        if (horizontal)
        {
            exportW = value;
            if (follow)
                exportH = Math::Max(1.0f, Math::Round(value/ratio));
        }
        else
        {
            exportH = value;
            if (follow)
                exportW = Math::Max(1.0f, Math::Round(value*ratio));
        }
    }

    Vector<String> OrderWithNewLayer(const PipelineNode& node, const String& newId, PipelineLayerPlace place)
    {
        Vector<String> ids;
        if (place == PipelineLayerPlace::Back)
            ids.Add(newId);

        for (auto& layer : ResolveComposerLayers(node))
        {
            if (layer.id != newId)
                ids.Add(layer.id);
        }

        if (place == PipelineLayerPlace::Front)
            ids.Add(newId);

        return ids;
    }

    void PlaceNewLayer(PipelineNode& node, const String& portId, PipelinePortType type, PipelineLayerPlace place)
    {
        if (node.nodeType != "composer" || type != PipelinePortType::Image)
            return;

        auto ids = OrderWithNewLayer(node, portId, place);
        node.RemoveConfig("layerOrder");
        auto& order = node.config["layerOrder"];
        order.SetArray();
        for (auto& id : ids)
            order.AddElement() = id;

        node.SetConfigString("selectedLayer", portId);
    }

    float FrameScale(float pixelSize)
    {
        return Math::Clamp(pixelSize > 0.0f ? pixelSize : 1.0f, minFrameScale, maxFrameScale);
    }
}
// --- META ---

ENUM_META(Editor::PipelineLayerPlace, Editor__PipelineLayerPlace)
{
    ENUM_ENTRY(Back);
    ENUM_ENTRY(Front);
}
END_ENUM_META;
// --- END META ---
