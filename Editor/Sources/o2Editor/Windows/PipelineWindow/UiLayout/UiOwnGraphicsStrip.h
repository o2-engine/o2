#pragma once

#include "o2/Scene/UI/Widget.h"
#include "o2/Scene/UI/Widgets/Label.h"
#include "o2Editor/Pipeline/PipelineGraph.h"
#include "o2Editor/Windows/PipelineWindow/PipelineImageTheme.h"

using namespace o2;

namespace Editor
{
    // ---------------------------------------------------------------------------------------------
    // Where the cells of the layout card's own graphics strip go (AssetsLine UiLayoutBody.tsx OWN): as
    // many columns of at least 56 as fit the row, square cells up to 72, a caption under each
    // ---------------------------------------------------------------------------------------------
    struct UiOwnStripGeometry
    {
        static constexpr float gap = 6.0f, cellMin = 56.0f, cellMax = 72.0f, captionGap = 2.0f, caption = 18.0f;

        int   count = 0;
        int   cols = 0;
        int   rows = 0;
        float cell = 0.0f;
        float height = 0.0f;

        UiOwnStripGeometry(float rowWidth, int n);

        // The top-right corner of a cell from the strip's top-left, y down: where its output port sits
        Vec2F Corner(int index) const;
    };

    // ---------------------------------------------------------------------------------------------
    // One of the layout node's own graphics as a cell: its output port, the name the port has, the upload
    // its picture is in, what the cell's tooltip would say
    // ---------------------------------------------------------------------------------------------
    struct UiOwnCell
    {
        String portId;
        String name;
        String uploadId;
        String about; // type · kind · size, how it was made

        bool operator==(const UiOwnCell& other) const { return portId == other.portId && name == other.name && uploadId == other.uploadId; }
    };

    // The node's own graphics that have an output, in the outputs' order (the port rows keep the schema outputs)
    Vector<UiOwnCell> UiOwnCellsOf(const PipelineNode& node);

    // ---------------------------------------------------------------------------------------------
    // The AI UI layout card's strip under the screen: a cell per graphic the node made for itself (drawn
    // by the image model, cut from the reference) with its name under it; each graphic's output port sits
    // on its cell's top-right corner (the body places it, like the AI extract's parts). The strip never
    // scrolls - a port has to stay where a link can reach it - and is not there while the node made nothing.
    // The thumbnails come from the uploads
    // ---------------------------------------------------------------------------------------------
    class UiOwnGraphicsStrip : public Widget
    {
    public:
        // Default constructor
        explicit UiOwnGraphicsStrip(RefCounter* refCounter);

        // Shows the cells (made again only when they changed)
        void SetCells(const Vector<UiOwnCell>& cells);

        // The cells shown
        const Vector<UiOwnCell>& GetCells() const { return mCells; }

        // The index of a port's cell, -1 when it has none
        int GetCellIndex(const String& portId) const;

        // True and the drawn top-right corner of a cell's picture (world space), once the cell is laid out
        bool GetCellCorner(int index, Vec2F& corner) const;

        // True when a cell shows its picture (tests)
        bool HasPicture(int index) const;

        void UpdateSelfTransform() override;

    private:
        struct Cell
        {
            Ref<Widget>            root;
            Ref<PipelineCellImage> image;
            Ref<Label>             label;
        };

        Vector<UiOwnCell> mCells;
        Vector<Cell>      mWidgets;

    private:
        void LayoutCells();
    };
}
