#pragma once

#include <future>
#include <memory>
#include <optional>

#include "o2/Render/TextureRef.h"
#include "o2Editor/Pipeline/UiLayout/UiLayoutPixels.h"
#include "o2Editor/Windows/PipelineWindow/UiLayout/UiDevicePicker.h"

namespace o2
{
    class Sprite;
}

namespace Editor
{
    // -----------------------------------------------------------------------------------------------
    // The workspace's stage (port of AssetsLine uiLayout/Stage.tsx): the screen on a board with a camera
    // (wheel zooms at the pointer, the middle / right button or Space + drag pans, the island has - / % /
    // + / fit; F frames the selected block) and Unity's rect tool: a click selects the deepest block, a
    // press inside the selection grabs it (a click there reaches the block under the pointer), a drag moves
    // it, the edges and corners resize it (Shift keeps the proportions, Alt resizes about the centre), edges
    // and centres snap to the parent and the siblings (Cmd / Ctrl: no snapping), the anchor's triangles go
    // to the parent's nine points keeping the block where it is; inside a row / column / grid a drag picks
    // the slot (a dashed ghost follows) and a resize sets w / h and drops grow. Every gesture is one hand
    // action; while it lasts the stage shows the result live. The screen is drawn by the layout renderer at
    // the size it takes on screen, again only when something it shows changed
    // -----------------------------------------------------------------------------------------------
    class UiWsStage : public UiWsPanel
    {
    public:
        // Default constructor
        explicit UiWsStage(RefCounter* refCounter);

        // Fits the screen into the board (and keeps fitting it while the board resizes)
        void Fit();

        // Frames a block (none: fits the screen)
        void Frame(const String& id);

        // Zooms by the factor about the board's centre
        void ZoomBy(float factor);

        // Returns the zoom: board pixels per design pixel
        float GetZoom() const { return mCam.k; }

        // Returns true while the camera follows the board's size
        bool IsFitted() const { return mAuto; }

        // The world point of a design point of the shown screen
        Vec2F DesignToWorld(double x, double y) const;

        // The design point of a world point
        Vec2F WorldToDesign(const Vec2F& point) const;

        // The world rectangle of a block of the shown screen (an empty one for none)
        RectF BlockRect(const String& id) const;

        // The world point of a resize handle of the selected block: n, s, e, w, ne, nw, se, sw
        Vec2F HandlePoint(const String& handle) const;

        // The world point inside the anchor triangle nearest the anchor's first corner (the gesture's grip)
        Vec2F AnchorGrip() const;

        // Returns true while Space is held (the left button pans)
        void SetSpaceHeld(bool held) { mSpace = held; }

        // The device chip of the island
        const Ref<UiDevicePicker>& GetDevicePicker() const { return mDevice; }

        // How many times the screen was drawn (tests: a frame without a change draws nothing)
        int GetRenderCount() const { return mRenderCount; }

        // The size the screen was last drawn at (pixels)
        Vec2I GetRenderedSize() const { return mTextureSize; }

        // How long the last drawing of the screen took, milliseconds
        float GetLastRenderMs() const { return mLastRenderMs; }

        // Renders what is pending now, at full size, instead of waiting for the gesture to end or the worker (tests)
        void FlushRender();

        // Returns true while the full-size drawing runs on the worker
        bool IsRendering() const { return mJob != nullptr; }

        void Update(float dt) override;
        void OnScrolled(float scroll) override;
        bool IsScrollable() const override { return true; }

    protected:
        struct Cam
        {
            float x = 0, y = 0, k = 0; // The screen's top-left in board pixels (y down), board pixels per design pixel
        };

        // A gesture in progress: the document as it would be, what to draw over it, the action it commits
        struct Live
        {
            UiDoc::UiDoc                 doc;
            Vector<UiDoc::UiOp>          ops;
            String                       label;
            Vector<UiDoc::UiSnapLine>    lines;
            String                       badge;
            std::optional<UiDoc::UiRect> ghost;
            std::optional<UiDoc::UiRect> anchorDots;
        };

        enum class DragKind { None, Pan, Move, Resize, Anchor };

        Cam   mCam;
        bool  mAuto = true;             // Fitted until the user pans or zooms; a new screen size fits again
        Vec2F mBoardSize;               // The board's size the camera was fitted to
        Vec2F mFittedFrame;             // The screen size the camera was fitted to
        bool  mSpace = false;

        std::optional<Live> mLive;
        int                 mLiveVersion = 0;
        UiDoc::UiRects      mLiveRects;

        DragKind        mDrag = DragKind::None;
        bool            mMoved = false;     // The press travelled far enough to be a drag
        Vec2F           mPressWorld;
        Cam             mCam0;
        String          mDragId, mHit, mHandle;
        bool            mWasSelected = false;
        UiDoc::UiRects  mBaseRects;         // Boxes before the gesture
        UiDoc::UiRect   mR0, mP;            // The block's and its parent's boxes before the gesture
        UiDoc::UiSnapTargets mTargets;
        bool            mHasContext = false;
        String          mHoverBlock;
        bool            mOverSelection = false;

        Ref<UiDevicePicker> mDevice;        // The island's screen chip

        // The drawn screen
        Ref<Sprite> mSprite;
        TextureRef  mTexture;
        Vec2I       mTextureSize;
        String      mRenderedKey;
        String      mVersionKey;            // The versions the document key was made for
        String      mDocKey;                // The shown document as text
        float       mRenderedScale = 0.0f;
        float       mIdle = 0.0f;           // Seconds since the camera or the gesture last changed
        int         mRenderCount = 0;
        float       mLastRenderMs = 0.0f;

        // The full-size drawing on a worker: what it draws, at what scale, the picture once done
        struct RenderJob
        {
            String                           key;
            float                            scale = 1.0f;
            std::future<UiLayout::UiImage>   picture;
            float                            started = 0.0f;
        };
        std::shared_ptr<RenderJob> mJob;
        bool        mForceRender = false;

    protected:
        void DrawPanel(const UiWsDraw::Box& box) override;
        void DrawOverlay(const UiWsDraw::Box& box) override;
        void OnHotClick(const String& id, const Input::Cursor& cursor) override;

        void OnCursorPressed(const Input::Cursor& cursor) override;
        void OnCursorStillDown(const Input::Cursor& cursor) override;
        void OnCursorReleased(const Input::Cursor& cursor) override;
        void OnCursorPressBreak(const Input::Cursor& cursor) override;
        void OnCursorMoved(const Input::Cursor& cursor) override;
        void OnCursorExit(const Input::Cursor& cursor) override;
        void OnCursorRightMousePressed(const Input::Cursor& cursor) override;
        void OnCursorRightMouseStayDown(const Input::Cursor& cursor) override;
        void OnCursorRightMouseReleased(const Input::Cursor& cursor) override;
        void OnCursorMiddleMousePressed(const Input::Cursor& cursor) override;
        void OnCursorMiddleMouseStayDown(const Input::Cursor& cursor) override;
        void OnCursorMiddleMouseReleased(const Input::Cursor& cursor) override;

        // The screen's size: the preview frame
        Vec2F FrameSize() const;

        // The camera fitting a design rectangle into the board
        Cam FitCam(float x, float y, float w, float h) const;

        // Zooms by the factor about a board point (y down)
        void ZoomAt(const Vec2F& boardPoint, float factor);

        // The board point (y down) of a world point
        Vec2F ToBoard(const Vec2F& world) const;

        // The world rectangle of a design rectangle
        RectF ToWorld(const UiDoc::UiRect& r) const;

        // The document shown: the gesture's, else the context's
        const UiDoc::UiDoc& Shown() const;

        // The boxes of the shown document
        const UiDoc::UiRects& ShownRects() const;

        // The resize handle or the anchor under a world point: "n".."sw", "anchor" or ""
        String GripAt(const Vec2F& point) const;

        // Starts a gesture on a block: its parent and boxes before it, and what it snaps to
        bool BeginContext(const String& id);

        void BeginPan(const Input::Cursor& cursor);
        void DragTo(const Input::Cursor& cursor);
        void EndDrag(bool commit);

        // Draws the screen again when what it shows changed: live (a gesture, the camera moving) at a reduced size right away,
        // at rest at the full size on a worker
        void EnsureRender(const UiWsDraw::Box& box);

        // Shows a drawn picture
        void Upload(const UiLayout::UiImage& image, const String& key, float scale, float ms);

        // The island's tools and the zoom
        void DrawIsland(const UiWsDraw::Box& box);

        // The banners: peeking at a step, the agent working
        void DrawBanners(const UiWsDraw::Box& box);

        // Sets the cursor shape for what is under the point
        void UpdateCursor(const Vec2F& point);
    };
}
