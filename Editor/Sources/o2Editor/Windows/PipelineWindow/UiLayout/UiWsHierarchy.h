#pragma once

#include <optional>

#include "o2Editor/Windows/PipelineWindow/UiLayout/UiWsWidgets.h"

namespace Editor
{
    class UiWsAddMenuPanel;

    // -----------------------------------------------------------------------------------------------
    // The workspace's hierarchy (port of the Layers part of AssetsLine UiWorkspace.tsx): the screen and
    // its blocks as a tree with type icons and kit thumbnails (a yellow dot where the kit has no graphic);
    // the add menu puts a block into the selected one (or after a selected leaf); a row is dragged to land
    // above / below a row (its top or bottom third, a line at that depth) or inside a block that holds others
    // (its middle); below an open container is above its first child; held over a folded block it opens;
    // the list scrolls near its edges. Rename by a double-click, F2 or the pencil (a taken name is refused,
    // the selection follows); hide / delete per row
    // -----------------------------------------------------------------------------------------------
    class UiWsHierarchy : public UiWsPanel
    {
    public:
        // Default constructor
        explicit UiWsHierarchy(RefCounter* refCounter);

        // Starts renaming a block (the screen's id renames the screen)
        void StartRename(const String& id);

        // Returns the block being renamed, "" when none
        const String& GetRenaming() const { return mRenaming; }

        // Returns the rename field (null while nothing is renamed)
        const Ref<EditBox>& GetRenameField() const { return mRenameField; }

        // Ends the rename: keep applies the name typed (when it is free)
        void EndRename(bool keep);

        // The world rectangle of a block's row (an empty one when it is not shown)
        RectF RowRect(const String& id) const { return HotRect("row:" + id); }

        // Folds or unfolds a block's children
        void ToggleFold(const String& id);

        // Returns true when a block's children are folded
        bool IsFolded(const String& id) const { return mFolded.Contains(id); }

        // Adds a block of the word, as the add menu does
        void AddBlock(const String& word);

        // Opens the add menu
        void OpenAddMenu();

        // Where a row dragged to the world point lands (tests); false when nowhere
        bool DropTarget(const String& dragId, const Vec2F& point, UiDoc::Ws::UiDropAt& at);

        // The landing place of the row being dragged now
        std::optional<UiDoc::Ws::UiDropAt> GetDragTarget() const { return mDropAt; }

        void Update(float dt) override;
        void OnScrolled(float scroll) override;
        bool IsScrollable() const override { return true; }

    protected:
        struct Row
        {
            String id;
            int    depth = 0;
            bool   root = false;
        };

        Vector<Row>    mRows;
        Vector<String> mFolded;
        float          mScroll = 0.0f;
        float          mContentHeight = 0.0f;

        String       mRenaming;
        Ref<EditBox> mRenameField;
        bool         mRenameBad = false;

        // A row being dragged
        String                             mDragging;      // The block pressed (a drag once it moved 4 px)
        bool                               mDragStarted = false;
        Vec2F                              mDragPoint;
        std::optional<UiDoc::Ws::UiDropAt> mDropAt;
        String                             mUnfoldId;
        float                              mUnfoldTime = 0.0f;

        Ref<UiWsPopup>        mAddMenu;
        Ref<UiWsAddMenuPanel> mAddPanel;

    protected:
        void DrawPanel(const UiWsDraw::Box& box) override;
        void DrawOverlay(const UiWsDraw::Box& box) override;
        void OnHotClick(const String& id, const Input::Cursor& cursor) override;
        void OnHotDoubleClick(const String& id, const Input::Cursor& cursor) override;

        void OnCursorPressed(const Input::Cursor& cursor) override;
        void OnCursorStillDown(const Input::Cursor& cursor) override;
        void OnCursorReleased(const Input::Cursor& cursor) override;
        void OnCursorPressBreak(const Input::Cursor& cursor) override;

        // The rows of the tree, folded blocks without their children
        void BuildRows();

        // The list's area (world)
        RectF ListRect() const;

        // Moves the dragged row to the point: its landing place, the auto-scroll, the unfolding
        void DragRowTo(const Vec2F& point, float dt);

        // Places the rename field over the row
        void PlaceRenameField(const RectF& row, float nameLeft);
    };

    // -------------------------------------------------------------
    // The add menu: two columns of block words with their type icons
    // -------------------------------------------------------------
    class UiWsAddMenuPanel : public UiWsPanel
    {
    public:
        Function<void(const String&)> onPick;

    public:
        explicit UiWsAddMenuPanel(RefCounter* refCounter): UiWsPanel(refCounter) {}

        // The height it wants
        static float ContentHeight();

    protected:
        void DrawPanel(const UiWsDraw::Box& box) override;
        void OnHotClick(const String& id, const Input::Cursor& cursor) override;
    };
}
