#pragma once

#include "o2/Utils/Math/Vector2.h"
#include "o2/Utils/Types/String.h"

using namespace o2;

namespace Editor
{
    // How the image-to-image nodes show their input next to their result: two panes, or one view split by a divider
    enum class PipelineIoView { SideBySide, Compare };

    // Parts grid of an extract node in the right pane of its row
    struct PipelineExtractGrid
    {
        float paneW = 0.0f;      // Width of each pane
        int   cols = 1;          // Columns of cells
        int   rows = 1;          // Rows of cells
        float cellW = 0.0f;      // Cell width
        float cellH = 0.0f;      // Cell image height: square cells, or the whole row for a single part
        float labelH = 0.0f;     // Caption height under each cell, none for a single part
        float offX = 0.0f;       // Where the first cell starts in the pane: the grid is centred
        float offY = 0.0f;
        float rowNatural = 0.0f; // Row height before a hand-sized card adds to it
        float rowH = 0.0f;       // Row height the grid sits in
    };

    // --------------------------------------------------------------------------------------------------
    // Input | result layout of the image-to-image nodes, the same as AssetsLine's and the Unity plugin's:
    // which nodes have it, the extract grid geometry, the height rules and the local view setting
    // --------------------------------------------------------------------------------------------------
    namespace PipelinePairLayout
    {
        constexpr float paneGap = 6.0f;         // Gap between the input and the result pane
        constexpr float gridGap = 6.0f;         // Gap between the part cells
        constexpr float minPaneCell = 64.0f;    // Narrowest part cell in the pane
        constexpr float resultHeight = 176.0f;  // Row height when either side has an image
        constexpr float emptyHeight = 64.0f;    // Row height when neither side has one
        constexpr float gridLabel = 20.0f;      // Caption height under a part cell
        constexpr float newNodeWidth = 480.0f;  // Width a new pair node is created with
        constexpr float dividerGrab = 6.0f;     // Half width of the strip that drags the compare divider on a drawing stage, screen pixels

        // Returns true for the node types that show their input next to their result
        bool IsPairNode(const String& type);

        // Returns the input port whose image the left pane shows
        String InputPortOf(const String& type);

        // Returns the width of each pane of a row
        float PaneWidth(float rowWidth);

        // Returns the row height before a hand-sized card adds to it
        float PairRowHeight(bool anyImage);

        // Returns the parts grid of an extract row. The natural row: as many columns of at least minPaneCell as fit,
        // square cells up to the result card's height. In the row (rowHeight above the natural height makes it taller)
        // one part fills the pane; several are square cells in the column count whose cells come out largest (the
        // fewest columns on a tie), the grid centred: AssetsLine's partsLayout
        PipelineExtractGrid ExtractGrid(float rowWidth, int parts, float rowHeight = 0.0f);

        // Returns the top-right corner of a part cell's image, where its output port sits, from the row's
        // top-left corner with y pointing down
        Vec2F ExtractCellCorner(const PipelineExtractGrid& grid, int index);

        // Returns true when a stored size leaves the height to the content
        bool IsAutoHeight(const Vec2F& size);

        // Returns the size a node is created with on the canvas: pair nodes get room for two panes and an automatic height
        Vec2F NewNodeSize(const String& type, const Vec2F& schemaSize);

        // Returns the height a resize stores: a drag of a side edge keeps an automatic height automatic
        float HeightAfterResize(bool verticalEdge, float storedHeight, float draggedHeight);

        // Returns true when the node shows the compare view: the setting asks for it, both images exist and no crop
        // frame is edited; the extract node always shows its parts beside the source
        bool ShowsCompare(PipelineIoView view, const String& type, bool hasInput, bool hasResult, bool cropOn);

        // Returns the path of the file the local view settings are kept in, under the pipeline work folder
        String GetViewPrefsPath();

        // Returns the view the image-to-image nodes use
        PipelineIoView GetIoView();

        // Stores the view and tells the open editors to rebuild their pair cards
        void SetIoView(PipelineIoView view);

        // Returns a number that changes whenever the view setting changes
        int GetIoViewVersion();

        // Returns where a node's compare divider sits, 0..1; kept for the session only
        float GetDivider(const String& nodeId);

        // Moves a node's compare divider
        void SetDivider(const String& nodeId, float value);
    }
}
// --- META ---

PRE_ENUM_META(Editor::PipelineIoView);
// --- END META ---
