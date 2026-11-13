#pragma once

#include <future>
#include <memory>

#include "o2/Render/TextureRef.h"
#include "o2Editor/Pipeline/UiLayout/UiLayoutPixels.h"
#include "o2Editor/Windows/PipelineWindow/UiLayout/UiWsWidgets.h"

namespace Editor
{
    class PipelineEditor;
    class UiRefMenuPanel;

    // -----------------------------------------------------------------------------------------------
    // Comparing a layout with its reference picture (port of AssetsLine uiLayout/RefCompare.tsx's RefLayers
    // and refDiff.ts), the same on the workspace's stage and on the node's card: the reference laid over the
    // screen see-through (alpha), side by side across a divider the user drags (split: the reference left
    // of it, the layout right; the divider is the image nodes' compare divider - a dark hairline with a small
    // grip), or the per-pixel difference (diff: the reference dimmed to grey, every pixel that differs yellow
    // to red - the picture the layout agent's check shows, UiLayout::DiffPicture). The difference is worked
    // out on a worker a moment after the layout stops changing, from the layout drawn as shown at up to 1600
    // px on its longer side
    // -----------------------------------------------------------------------------------------------
    class UiRefLayers
    {
    public:
        // Follows the context: the reference's texture when its picture changed; in the diff mode the difference with the
        // layout as shown (view, ids), started once the layout has rested for a moment (resting: no gesture now)
        void Update(const UiWsContext& ctx, const String& view, bool showIds, float dt, bool resting);

        // Draws the comparison over the screen's world rectangle; split = the divider's place 0..1 (a live drag's or the
        // setting's), dividerActive = the grip comes forward
        void Draw(const RectF& screen, const UiDoc::Ws::UiRefSetting& setting, double split, bool dividerActive);

        // Starts the difference now, without the pause, and waits for it (tests)
        void FinishDiff(const UiWsContext& ctx, const String& view, bool showIds);

        // True once the difference for the layout shown is drawn
        bool IsDiffReady() const { return mDiffTexture && mDiffKey == mWantKey && !mWantKey.IsEmpty(); }

        // The size of the difference picture (pixels)
        Vec2I GetDiffSize() const { return mDiffSize; }

        // True once the reference picture is read
        bool HasReference() const { return (bool)mRefTexture; }

        // The divider's world x on the screen
        static float DividerX(const RectF& screen, double split);

        // The divider's place 0..1 at a world x
        static double SplitAt(const RectF& screen, float x);

        // True when a world point is on the divider (7 screen pixels either side; pixel = world units per screen pixel)
        static bool OnDivider(const RectF& screen, double split, const Vec2F& point, float pixel = 1.0f);

    protected:
        // ------------------------------------------------
        // A difference being worked out: its key, the picture
        // ------------------------------------------------
        struct DiffJob
        {
            String                         key;
            std::future<UiLayout::UiImage> picture;
        };

        String     mRefPng;       // The reference picture's bytes read last
        int        mRefVersion = 0;
        TextureRef mRefTexture;
        Vec2I      mRefSize;
        std::shared_ptr<UiLayout::UiImage> mRefImage; // Decoded once, shared with the workers

        int        mCtxVersion = -1; // The context's version the document key was made for
        String     mDocKey;
        String     mWantKey;      // The difference the layout shown needs
        float      mQuiet = 0.0f; // Seconds the wanted key has not changed
        String     mDiffKey;      // The difference drawn
        TextureRef mDiffTexture;
        Vec2I      mDiffSize;
        std::shared_ptr<DiffJob> mJob;

    protected:
        // The key of the difference the context needs (empty outside the diff mode)
        String WantedKey(const UiWsContext& ctx, const String& view, bool showIds);

        // Starts the difference on a worker
        void StartDiff(const UiWsContext& ctx, const String& view, bool showIds);

        // Shows a finished difference
        void TakeDiff();
    };

    // -----------------------------------------------------------------------------------------------
    // The comparison over the card's screen: lies over the preview and draws the layers on the picture's
    // rectangle; it takes the cursor only on the split's divider (a drag moves it, written when let go), so
    // the card under it keeps its clicks
    // -----------------------------------------------------------------------------------------------
    class UiRefOverlayView : public UiWsPanel
    {
    public:
        Function<RectF()> screenRect; // The world rectangle of the drawn screen (the preview's picture)

    public:
        // Default constructor
        explicit UiRefOverlayView(RefCounter* refCounter);

        // The layers (tests)
        UiRefLayers& GetLayers() { return mLayers; }

        // The divider's place while it is dragged, else the setting's
        double GetSplit() const;

        bool IsUnderPoint(const Vec2F& point) override;
        void Update(float dt) override;

    protected:
        UiRefLayers mLayers;
        double      mLive = -1.0; // The divider's place while dragged

    protected:
        void DrawPanel(const UiWsDraw::Box& box) override;
        void OnCursorPressed(const Input::Cursor& cursor) override;
        void OnCursorStillDown(const Input::Cursor& cursor) override;
        void OnCursorReleased(const Input::Cursor& cursor) override;
        void OnCursorPressBreak(const Input::Cursor& cursor) override;

        // True when the comparison is shown: on, with blocks
        bool Shown() const;
    };

    // -----------------------------------------------------------------------------------------------
    // The card's compare button (in its head row) with its menu under it: Off | Alpha | Split | Diff and the
    // amount (the reference's or the difference's opacity, or the divider's place) in one place - a popover,
    // so the card keeps its height. Disabled while the layout is empty; shown only with a reference
    // -----------------------------------------------------------------------------------------------
    class UiRefButton : public UiWsPanel
    {
    public:
        // Default constructor
        explicit UiRefButton(RefCounter* refCounter);

        // Opens or closes the menu
        void ToggleMenu();

        // Returns true while the menu is open
        bool IsMenuOpen() const;

        // The menu's panel (tests), null before the first open
        Ref<UiRefMenuPanel> GetMenuPanel() const { return mMenuPanel; }

        // True when the button can be pressed: a reference and blocks to compare it with
        bool IsUsable() const;

    protected:
        Ref<UiWsPopup>      mMenu;
        Ref<UiRefMenuPanel> mMenuPanel;

    protected:
        void DrawPanel(const UiWsDraw::Box& box) override;
        void OnHotClick(const String& id, const Input::Cursor& cursor) override;
    };

    // -----------------------------------------------------------------------------------------------
    // The comparison's controls as one column (the card's menu): the modes with Off first, and the amount
    // while it is on. Their writes are UI-only keys of the node
    // -----------------------------------------------------------------------------------------------
    class UiRefMenuPanel : public UiWsPanel
    {
    public:
        WeakRef<UiWsPopup> popup; // The popover it is shown in (it resizes with the amount row)

    public:
        // Default constructor
        explicit UiRefMenuPanel(RefCounter* refCounter);

        // The size it wants
        Vec2F GetContentSize() const;

        // Picks a mode as its segment does: off, alpha, split, diff
        void Pick(const String& mode);

        // Sets the amount as its slider does (0..1)
        void SetAmount(double value);

    protected:
        String mSliding; // The slider held

    protected:
        void DrawPanel(const UiWsDraw::Box& box) override;
        void OnHotClick(const String& id, const Input::Cursor& cursor) override;
        void OnCursorPressed(const Input::Cursor& cursor) override;
        void OnCursorStillDown(const Input::Cursor& cursor) override;
        void OnCursorReleased(const Input::Cursor& cursor) override;
    };
}
