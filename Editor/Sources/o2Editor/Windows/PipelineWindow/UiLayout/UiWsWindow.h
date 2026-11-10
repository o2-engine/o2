#pragma once

#include "o2/Events/KeyboardEventsListener.h"
#include "o2Editor/Windows/PipelineWindow/UiLayout/UiWsHierarchy.h"
#include "o2Editor/Windows/PipelineWindow/UiLayout/UiWsSide.h"
#include "o2Editor/Windows/PipelineWindow/UiLayout/UiWsStage.h"

namespace Editor
{
    class PipelineEditor;

    // ------------------------------------------------------------
    // A rectangle in screen points from the top-left corner, y down
    // ------------------------------------------------------------
    struct UiWsFrame
    {
        float x = 0, y = 0, w = 0, h = 0;

        float Right() const { return x + w; }
        float Bottom() const { return y + h; }
    };

    // -----------------------------------------------------------------------------------------------
    // Grips over the window's panels: the resize edges and the dividers between the panels. Only its
    // zones take the cursor; a press, a drag and a release in one go to the window
    // -----------------------------------------------------------------------------------------------
    class UiWsGrips : public UiWsPanel
    {
    public:
        Vector<Pair<RectF, String>>                 zones;     // Set by the window every frame
        Function<void(const String&, const Vec2F&)> onPress;   // The zone pressed and the point
        Function<void(const Vec2F&)>                onDrag;    // The point while held
        Function<void()>                            onRelease;
        Function<void(const String&)>               onDouble;  // The zone double-clicked
        String                                      active;    // The zone dragged now

    public:
        explicit UiWsGrips(RefCounter* refCounter): UiWsPanel(refCounter) {}

        bool IsUnderPoint(const Vec2F& point) override;

    protected:
        void DrawPanel(const UiWsDraw::Box& box) override;
        void OnCursorPressed(const Input::Cursor& cursor) override;
        void OnCursorStillDown(const Input::Cursor& cursor) override;
        void OnCursorReleased(const Input::Cursor& cursor) override;
        void OnCursorDblClicked(const Input::Cursor& cursor) override;
    };

    // -----------------------------------------------------------------------------------------------
    // The AI UI layout workspace's window (the frame of AssetsLine UiWorkspace.tsx): over the editor,
    // a dimmed backdrop (a press on it closes the window), moved by its header, resized from any edge or
    // corner (at least 860 x 520), maximized by its button or a double-click on the header; its size, the
    // maximized state and the side panels' widths (the dividers drag, a double-click gives the default
    // back; the stage keeps 280 px) are kept in the pipeline work folder's ViewPrefs.json. The header: the
    // title, the screen's name and size, the kit coverage chip with its popover, undo / redo, the export
    // target, Download, maximize and close. The keys while the window has the focus: Escape (the peek, then
    // the window), Cmd/Ctrl+Z / Shift+Z / Y, Delete, F2, F, the arrows (Shift: 10), Space + drag
    // -----------------------------------------------------------------------------------------------
    class UiWsWindow : public UiWsPanel, public KeyboardEventsListener
    {
    public:
        Function<void()>                                   onClosed;   // Called when the window closes
        Function<void(const String& prompt, const String&)> onSend;     // The agent request (prompt, selected block)
        Function<void()>                                   onDownload; // Download pressed (none: the button is disabled)

    public:
        // Default constructor
        explicit UiWsWindow(RefCounter* refCounter);

        // Shows the window for the node
        void Bind(const Ref<PipelineEditor>& editor, const String& nodeId);

        // Hides the window
        void Close();

        // Returns true while it is shown
        bool IsShown() const { return mShown; }

        // The node it shows
        const String& GetNodeId() const { return mNodeId; }

        // Maximizes or restores the window
        void ToggleMaximized();

        // Returns true while maximized
        bool IsMaximized() const { return mMax; }

        // The window's rectangle in screen points from the top-left corner (y down)
        UiWsFrame GetFrame() const;

        // Moves and resizes the window (screen points from the top-left, y down), kept on the screen and at least 860 x 520
        void SetFrame(float x, float y, float w, float h);

        // The side panels' widths
        float GetLeftWidth() const { return mLeft; }
        float GetRightWidth() const { return mRight; }

        // Sets a side panel's width as its divider does (side: left | right; 0 = the default)
        void SetPanelWidth(const String& side, float width);

        // Opens or closes the coverage popover
        void ToggleCoverage();

        const Ref<UiWsContext>& GetContext() const { return mCtx; }
        const Ref<UiWsHierarchy>& GetHierarchy() const { return mHierarchy; }
        const Ref<UiWsStage>& GetStage() const { return mStage; }
        const Ref<UiWsSidePanel>& GetSide() const { return mSide; }
        const Ref<UiWsCoveragePanel>& GetCoveragePanel() const { return mCoverage; }

        // Handles a key as the window does (tests): its code with the modifiers held now
        void PressKey(int keyCode);

        bool IsFocusable() const override { return true; }
        void Update(float dt) override;

    protected:
        Ref<UiWsContext>       mCtx;
        Ref<UiWsHierarchy>     mHierarchy;
        Ref<UiWsStage>         mStage;
        Ref<UiWsSidePanel>     mSide;
        Ref<UiWsGrips>         mGrips;
        Ref<UiWsPopup>         mCoveragePopup;
        Ref<UiWsCoveragePanel> mCoverage;

        String mNodeId;
        bool   mShown = false;
        bool   mHasFocus = false;

        float mX = 0, mY = 0, mW = 1200, mH = 800; // The restored frame, screen points from the top-left
        bool  mMax = false;
        float mLeft = 264, mRight = 400;           // Side panel widths shown
        float mSavedLeft = 0, mSavedRight = 0;     // Widths kept (0 = the default)

        String mDrag;                // head, rz:<edges>, split:left|right
        Vec2F  mDragStart;
        UiWsFrame mFrameStart;
        float  mPanelStart = 0;
        float  mPoll = 0.0f;
        int    mThemeVersion = -1;

    protected:
        void DrawPanel(const UiWsDraw::Box& box) override;
        void DrawOverlay(const UiWsDraw::Box& box) override;
        void OnHotClick(const String& id, const Input::Cursor& cursor) override;
        void OnHotDoubleClick(const String& id, const Input::Cursor& cursor) override;

        void OnCursorPressed(const Input::Cursor& cursor) override;
        void OnCursorStillDown(const Input::Cursor& cursor) override;
        void OnCursorReleased(const Input::Cursor& cursor) override;

        // A drag of the header, an edge or a divider
        void BeginDrag(const String& zone, const Vec2F& point);
        void DragTo(const Vec2F& point);
        void EndDrag();

        void OnKeyPressed(const Input::Key& key) override;
        void OnKeyReleased(const Input::Key& key) override;
        void OnKeyStayDown(const Input::Key& key) override;

        // The screen size in points
        Vec2F ScreenSize() const;

        // The frame shown (the maximized one while maximized)
        UiWsFrame ShownFrame() const;

        // Lays the panels out in the frame
        void LayoutPanels();

        // Clamps the side panels to the window's width
        void ClampPanels();

        // Reads and writes the remembered size, state and widths
        void LoadPrefs();
        void SavePrefs() const;

        // True while the window takes the keys
        bool TakesKeys() const;

        // Handles a key; returns true when it was the window's
        bool HandleKey(int keyCode, bool repeat);
    };
}
