#pragma once

#include "o2/Scene/UI/Widget.h"
#include "o2/Utils/Function/Function.h"
#include "o2Editor/Windows/PipelineWindow/PipelineComposerStage.h"

using namespace o2;

namespace o2
{
    class Button;
}

namespace Editor
{
    // -------------------------------------------------------------------------------------------------------------------
    // Layers list of the composer card, left of its work area: a row per layer, the front one first, each with its open
    // settings under it, then a "+ input" row. Heights are pinned, so the card places the input ports on the rows without
    // measuring them; a grip at the right end of a row drags it to another place in the stack
    // -------------------------------------------------------------------------------------------------------------------
    class PipelineComposerLayersPanel : public Widget
    {
    public:
        static constexpr float border = 1.0f;         // Frame of the panel
        static constexpr float headHeight = 22.0f;    // "Layers · N"
        static constexpr float listPadding = 4.0f;    // Above the first row and under the "+ input" row
        static constexpr float rowHeight = 28.0f;     // One layer row
        static constexpr float rowGap = 3.0f;         // Between the rows
        static constexpr float addRowHeight = 24.0f;  // The "+ input" row
        static constexpr float settingsHeight = 76.0f; // Open settings: size, opacity, 9-slice
        static constexpr float nineHeight = 69.0f;     // More open settings with 9-slice on: insets, corners, hint
        static constexpr float selectedTint = 0.16f;   // Accent strength behind the selected row

        Function<void(const String&, bool)>                            onConfigChanged; // A config key the panel wrote
        Function<void()>                                               onLayoutChanged; // The rows changed their heights
        Function<void(const String&)>                                  onDuplicate;     // Duplicate the layer
        Function<void(int, int)>                                       onMove;          // Move the layer at the stack index by the step
        Function<void(const String&, const String&, bool)>             onRemove;        // Remove the layer: id, port, copy
        Function<void(const String&, const String&, bool, const String&)> onRename;     // Rename the layer: id, port, copy, name
        Function<void(const Vector<String>&)>                          onReorder;       // New stack order of the layer ids, back first
        Function<void()>                                               onAddInput;      // Add an image input

    public:
        // Default constructor
        explicit PipelineComposerLayersPanel(RefCounter* refCounter);

        // Sets the composer node and its work area
        void Setup(const Ref<PipelineNode>& node, const Ref<PipelineComposerStage>& stage);

        // Rebuilds the rows from the layers
        void Rebuild();

        // Returns the height of the panel: it grows with its rows and never scrolls
        float GetContentHeight() const { return mContentHeight; }

        // Returns the middle of the row of the input port from the panel top, y down; false for a port without a row
        bool GetPortCenter(const String& portId, float& y) const;

        // Returns the middle of the "+ input" row from the panel top, y down
        float GetAddRowCenter() const { return mAddTop + addRowHeight*0.5f; }

        // Returns the "+ input" row
        const Ref<Button>& GetAddRow() const { return mAddRow; }

        // Moves the highlight to the selected layer's row without rebuilding the rows
        void UpdateSelection();

        // Returns the row of the layer
        Ref<Widget> FindRow(const String& layerId) const;

        // Returns the grip of the layer's row
        Ref<Widget> FindGrip(const String& layerId) const;

        // Starts dragging the layer's row by its grip
        void BeginRowDrag(const String& layerId);

        // Moves the dragged row's landing place to the point, canvas space
        void DragRowTo(const Vec2F& point);

        // Ends the drag; commits the new order when asked and it differs
        void EndRowDrag(bool commit);

        // Returns the display index the dragged row lands before, -1 while nothing is dragged
        int GetDropIndex() const { return mDragging.IsEmpty() ? -1 : mDropIndex; }

        // Draws the rows and the landing line of a dragged row
        void Draw() override;

        SERIALIZABLE(PipelineComposerLayersPanel);

    protected:
        Ref<PipelineNode>          mNode;          // Composer node
        Ref<PipelineComposerStage> mStage;         // Its work area: layers, placements, selection
        Vector<String>             mDisplayIds;    // Layer ids in display order, the front one first
        Vector<String>             mDisplayPorts;  // Input port of each displayed layer, empty for a copy
        Vector<float>              mRowTops;       // Top of each displayed row from the panel top
        Vector<Ref<Widget>>        mRows;          // Displayed row widgets
        Vector<Ref<Widget>>        mGrips;         // Grip of each displayed row
        Ref<Button>                mAddRow;        // "+ input"
        float                      mAddTop = 0.0f; // Top of the "+ input" row
        float                      mContentHeight = 0.0f; // Height of the panel
        String                     mDragging;      // Layer whose row is dragged, empty when none
        int                        mDropIndex = 0; // Display index the dragged row lands before

    protected:
        // Returns the height of the settings opened under a row
        static float SettingsHeight(bool nine);

        // Creates the row of a layer
        Ref<Widget> MakeRow(const ComposerLayerRef& layer, int stackIndex, int count, bool selected, bool open);

        // Adds the open settings of the layer at the top
        void AddSettings(const ComposerLayerRef& layer, float top);
    };
}
// --- META ---

CLASS_BASES_META(Editor::PipelineComposerLayersPanel)
{
    BASE_CLASS(o2::Widget);
}
END_META;
CLASS_FIELDS_META(Editor::PipelineComposerLayersPanel)
{
    FIELD().PUBLIC().NAME(onConfigChanged);
    FIELD().PUBLIC().NAME(onLayoutChanged);
    FIELD().PUBLIC().NAME(onDuplicate);
    FIELD().PUBLIC().NAME(onMove);
    FIELD().PUBLIC().NAME(onRemove);
    FIELD().PUBLIC().NAME(onRename);
    FIELD().PUBLIC().NAME(onReorder);
    FIELD().PUBLIC().NAME(onAddInput);
    FIELD().PROTECTED().NAME(mNode);
    FIELD().PROTECTED().NAME(mStage);
    FIELD().PROTECTED().NAME(mDisplayIds);
    FIELD().PROTECTED().NAME(mDisplayPorts);
    FIELD().PROTECTED().NAME(mRowTops);
    FIELD().PROTECTED().NAME(mRows);
    FIELD().PROTECTED().NAME(mGrips);
    FIELD().PROTECTED().NAME(mAddRow);
    FIELD().PROTECTED().DEFAULT_VALUE(0.0f).NAME(mAddTop);
    FIELD().PROTECTED().DEFAULT_VALUE(0.0f).NAME(mContentHeight);
    FIELD().PROTECTED().NAME(mDragging);
    FIELD().PROTECTED().DEFAULT_VALUE(0).NAME(mDropIndex);
}
END_META;
CLASS_METHODS_META(Editor::PipelineComposerLayersPanel)
{

    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*);
    FUNCTION().PUBLIC().SIGNATURE(void, Setup, const Ref<PipelineNode>&, const Ref<PipelineComposerStage>&);
    FUNCTION().PUBLIC().SIGNATURE(void, Rebuild);
    FUNCTION().PUBLIC().SIGNATURE(float, GetContentHeight);
    FUNCTION().PUBLIC().SIGNATURE(bool, GetPortCenter, const String&, float&);
    FUNCTION().PUBLIC().SIGNATURE(float, GetAddRowCenter);
    FUNCTION().PUBLIC().SIGNATURE(const Ref<Button>&, GetAddRow);
    FUNCTION().PUBLIC().SIGNATURE(void, UpdateSelection);
    FUNCTION().PUBLIC().SIGNATURE(Ref<Widget>, FindRow, const String&);
    FUNCTION().PUBLIC().SIGNATURE(Ref<Widget>, FindGrip, const String&);
    FUNCTION().PUBLIC().SIGNATURE(void, BeginRowDrag, const String&);
    FUNCTION().PUBLIC().SIGNATURE(void, DragRowTo, const Vec2F&);
    FUNCTION().PUBLIC().SIGNATURE(void, EndRowDrag, bool);
    FUNCTION().PUBLIC().SIGNATURE(int, GetDropIndex);
    FUNCTION().PUBLIC().SIGNATURE(void, Draw);
    FUNCTION().PROTECTED().SIGNATURE_STATIC(float, SettingsHeight, bool);
    FUNCTION().PROTECTED().SIGNATURE(Ref<Widget>, MakeRow, const ComposerLayerRef&, int, int, bool, bool);
    FUNCTION().PROTECTED().SIGNATURE(void, AddSettings, const ComposerLayerRef&, float);
}
END_META;
// --- END META ---
