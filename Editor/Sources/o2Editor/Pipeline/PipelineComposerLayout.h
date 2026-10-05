#pragma once

#include "o2/Utils/Math/Vector2.h"
#include "o2/Utils/Types/Containers/Vector.h"
#include "o2/Utils/Types/String.h"
#include "o2Editor/Pipeline/PipelineGraph.h"

using namespace o2;

namespace Editor
{
    // Where a new composer layer goes: in front of every layer or behind them
    enum class PipelineLayerPlace { Front, Back };

    // Rows of the composer's layers list, tops from the panel top with y pointing down
    struct PipelineComposerRows
    {
        float         addTopRow = 0.0f;    // Top of "+ layer on top"
        Vector<float> rowTops;             // Top of each layer row, the front layer first
        float         emptyTop = 0.0f;     // Top of the hint shown while there are no layers
        float         settingsTop = 0.0f;  // Top of the open settings, under their row
        float         addBottomRow = 0.0f; // Top of "+ layer below"
        float         height = 0.0f;       // Height of the panel
    };

    // --------------------------------------------------------------------------------------------------------------
    // Rules of the composer card shared with AssetsLine and the Unity plugin: the rows of the layers list the input
    // ports sit on, the size a layer's file is exported at, where a new layer lands and the screen-sized frame
    // --------------------------------------------------------------------------------------------------------------
    namespace PipelineComposerLayout
    {
        constexpr float border = 1.0f;          // Frame of the panel
        constexpr float headHeight = 22.0f;     // "Layers · N"
        constexpr float listPadding = 4.0f;     // Above the first add row and under the last one
        constexpr float rowHeight = 28.0f;      // One layer row
        constexpr float rowGap = 3.0f;          // Between the rows
        constexpr float addRowHeight = 24.0f;   // An add row
        constexpr float settingsLine = 25.0f;   // Step of the lines in the open settings
        constexpr float settingsHeight = 101.0f; // Open settings: size, export size, opacity, 9-slice
        constexpr float nineHeight = 69.0f;     // More open settings with 9-slice on: insets, corners, hint
        constexpr float minFrameScale = 0.5f;   // The frame stops shrinking on the canvas when zoomed in past 2
        constexpr float maxFrameScale = 4.0f;   // And stops growing when zoomed out past 1/4

        // Returns the height of the settings opened under a row
        float SettingsHeight(bool nine);

        // Returns the rows of a list of the layers count; openRow is the display index of the row with open settings,
        // -1 for none. The list: "+ layer on top", the rows, "+ layer below"
        PipelineComposerRows Rows(int layers, int openRow = -1, bool openNine = false);

        // Returns true when the layer has an export size: both sides at least 1
        bool HasExportSize(float exportW, float exportH);

        // Returns the size a layer's own file is written at: its export size when it has one, else its work-area size;
        // whole pixels, at least 1
        Vec2I LayerExportSize(float w, float h, float exportW, float exportH);

        // Applies a typed export side (the width when horizontal). Below 1 clears both; the other side follows the layer's
        // proportions while nothing was set or the aspect is locked
        void TypeExportSide(float w, float h, bool lockAspect, bool horizontal, float value, float& exportW, float& exportH);

        // Returns the layer ids back to front with the new one in front of every layer or behind them
        Vector<String> OrderWithNewLayer(const PipelineNode& node, const String& newId, PipelineLayerPlace place);

        // Writes what a new input of a node adds besides the port: on a composer an image input takes its place in
        // "layerOrder" and becomes "selectedLayer"; nothing on any other node
        void PlaceNewLayer(PipelineNode& node, const String& portId, PipelinePortType type, PipelineLayerPlace place);

        // Returns the factor that keeps the selected layer's frame screen-sized: the size of a screen pixel in canvas
        // units (1 / camera zoom), within the limits
        float FrameScale(float pixelSize);
    }
}
// --- META ---

PRE_ENUM_META(Editor::PipelineLayerPlace);
// --- END META ---
