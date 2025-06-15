#pragma once

#include "o2/Events/CursorEventsArea.h"
#include "o2/Render/Font.h"
#include "o2/Scene/UI/Widget.h"
#include "o2/Utils/Editor/DragHandle.h"

using namespace o2;

namespace o2
{
    class HorizontalScrollBar;
}

namespace Editor
{
    FORWARD_CLASS_REF(AnimationWindow);

    // ------------------------------------
    // Draws time scale in animation window
    // ------------------------------------
    class AnimationTimeline : public Widget, public CursorAreaEventsListener
    {
    public:
        Function<void()> onViewChanged; // Called when scroll or zoom were changed

    public:
        // Default constructor
        AnimationTimeline(RefCounter* refCounter);

        // Copy-constructor
        AnimationTimeline(RefCounter* refCounter, const AnimationTimeline& other);

        // Destructor
        ~AnimationTimeline();

        // Copy-operator
        AnimationTimeline& operator=(const AnimationTimeline& other);

        // Draws time scale and captions
        void Draw() override;

        // Updates as regular widget and changes zoom by mouse wheel
        void Update(float dt) override;

        // Sets animation. Subscribes on animation duration change, controls playing time
        void SetAnimation(const Ref<AnimationClip>& animation, const Ref<AnimationPlayer>& player = nullptr);

        // Sets current time scroll in seconds
        void SetScroll(float scroll);

        // Sets view range, left view border by left, and right border by right
        void SetViewRange(float left, float right, bool force = true);

        // Sets red time cursor
        void SetTimeCursor(float time);

        // Returns time of red cursor
        float GetTimeCursor() const;

        // Returns current time scroll in seconds
        float GetScroll() const;

        // Sets time scale zoom
        void SetScale(float scale);

        // Returns time scale zoom
        float GetScale() const;

        // Sets lock on zoom and scroll
        void SetViewMoveDisabled(bool disabled);

        // Converts local time to world x position
        float LocalToWorld(float pos) const;

        // Converts world x position to local time
        float WorldToLocal(float pos) const;

        // Returns captions text drawable
        const Ref<Text>& GetText() const;

        // Sets scrollbar widget, used for view scroll
        void SetScrollBar(const Ref<HorizontalScrollBar>& scrollBar);

        // Returns scrollbar widget, used for view scroll
        const Ref<HorizontalScrollBar>& GetScrollBar() const;

        // Checks is this timeA and timeB are same on screen. Dependent on zoom, thershold - max pixels distance on screen
        bool IsSameTime(float timeA, float timeB, float threshold = 3.0f) const;

        // Returns true if point is in this object
        bool IsUnderPoint(const Vec2F& point) override;

        // Returns is listener scrollable
        bool IsScrollable() const override;

        // Returns true when input events can be handled by down listeners
        bool IsInputTransparent() const override;

        // Returns create menu category in editor
        static String GetCreateMenuCategory();

        SERIALIZABLE(AnimationTimeline);
        CLONEABLE_REF(AnimationTimeline);

    private:
        const float mTextOffset = 12.0f;              // Text position offset from top border
        const float mBigLineOffset = 13.0f;           // Big scale line offset from top border
        const float mSmallLineOffset = 15.0f;         // Small scale line offset from top border
        const float mOneSecondDefaultSize = 100.0f;   // One second with scale == 1 size in pixels
        const float mPerfectScaleSegmentSize = 10.0f; // The perfect size of scale segment in pixels. Scale period and segment size will be chosen more suitable
        const float mScaleOffset = 10.0f;             // First scale line offset from left border

        const Color4 mBigLineColor = Color4(16, 20, 23, 64);   // Big scale line color
        const Color4 mSmallLineColor = Color4(16, 20, 23, 38); // Small scale line color

        const float mScaleSmoothLerpCoef = 10.0f; // Scale smooth animation coefficient. More is faster
        const float mMinScale = 0.1f;             // Minimal view zoom 
        const float mMaxScale = 10.0f;            // Maximal view zoom
        const float mScaleSense = 0.1f/120.0f;    // Mouse wheel scroll sense coefficient

        const float mScrollSmoothCoef = 15.0f;        // Scroll smooth to target interpolation coefficient
        const float mScrollSpeedDecreaseCoef = 10.0f; // Scroll speed decrease coefficient. More - speed decreasing faster
        const float mScrollBorderBounceCoef = 10.0f;  // Smooth scroll bounds bounce coefficient

    private:
        Ref<AnimationWindow> mAnimationWindow; // Animation window

        Ref<AnimationPlayer> mPlayer;    // Animation player
        Ref<AnimationClip>   mAnimation; // Animation, used for sibscribing on duration change

        float mTimeCursor = 0; // Current time of red cursor

        float mSmoothViewScroll = 0.0f;          // Time scroll in seconds smoothed, tends to target value mViewScroll
        float mViewScroll = 0.0f;                // Time scroll in seconds
        bool  mDragViewScroll = false;           // It is true when user is dragging view with right button
        float mBeginDragViewScrollOffset = 0.0f; // Offset from cursor to view scroll before starting drag
        float mViewScrollSpeed = 0.0f;           // Speed of inertial scrolling, when user dragged view and released mouse

        float mViewZoom = 1.0f;       // Time scale zoom
        float mSmoothViewZoom = 1.0f; // Smoothed and animated time scale zoom
        bool  mViewHasZoomed = false; // True when area has scrolled at this frame, resets to false on update

        bool mViewMoveDisabled = false; // Is moving and zooming view disabled. It is disabling when editor has switched to curves edit mode

        float mDuration = 0.0f; // Scale length in seconds

        Ref<Font> mTextFont; // Captions font
        Ref<Text> mText;     // Captions text

        Ref<Sprite> mBeginMark; // Begin animation mark sprite. Begin is always at zero
        Ref<Sprite> mEndMark;   // End animation mark sprite, at duration

        Layout mBeginMarkLayout = Layout(Vec2F(0, 1), Vec2F(0, 1), Vec2F(-6.0f, 3.0f), Vec2F(6.0f, -21.0f));
        Layout mEndMarkLayout = Layout(Vec2F(0, 1), Vec2F(1, 1), Vec2F(-4.0f, 3.0f), Vec2F(6.0f, -21.0f));

        Ref<HorizontalScrollBar> mScrollBar; // Scroll bar. Limited by animation duration

        Ref<Sprite>           mTimeLine;           // Red time line
        Ref<CursorEventsArea> mTimeLineEventsArea; // Top area events listeners, used for moving red line of time

    private:
        // Updates duration and scrollbars
        void UpdateDuration(float duration);

        // Draws time scale with scroll and zoom
        void DrawTimeScale();

        // Chooses well view of time scale by zoom
        void ChooseScaleParams(int& bigLinePeriod, double& bigLineTimeAmount);

        // Tracks user's right mouse button and updates scroll by speed
        void UpdateScrolling(float dt);

        // Tracks user's mouse wheel and updates view scroll
        void UpdateZooming(float dt);

        // Updates scroll bar handle size by view width
        void UpdateScrollBarHandleSize();

        // Called when transformation was changed and updated. Updates scroll bar handle size
        void OnTransformUpdated() override;

        // Sets animation time by cursor screen position
        void SetAnimationTimeByCursor(const Input::Cursor& cursor);

        // Called when right mouse button was pressed on this, begin scrolling
        void OnCursorRightMousePressed(const Input::Cursor& cursor) override;

        // Called when right mouse button stay down on this, scrolling
        void OnCursorRightMouseStayDown(const Input::Cursor& cursor) override;

        // Called when right mouse button was released (only when right mouse button pressed this at previous time), ends scrolling
        void OnCursorRightMouseReleased(const Input::Cursor& cursor) override;

        // Called when scrolling
        void OnScrolled(float scroll) override;

        REF_COUNTERABLE_IMPL(Widget);

        friend class AnimationWindow;

        template<typename AnimationTrackType>
        friend class KeyFramesTrackControl;
    };
}
// --- META ---

CLASS_BASES_META(Editor::AnimationTimeline)
{
    BASE_CLASS(o2::Widget);
    BASE_CLASS(o2::CursorAreaEventsListener);
}
END_META;
CLASS_FIELDS_META(Editor::AnimationTimeline)
{
    FIELD().PUBLIC().NAME(onViewChanged);
    FIELD().PRIVATE().DEFAULT_VALUE(12.0f).NAME(mTextOffset);
    FIELD().PRIVATE().DEFAULT_VALUE(13.0f).NAME(mBigLineOffset);
    FIELD().PRIVATE().DEFAULT_VALUE(15.0f).NAME(mSmallLineOffset);
    FIELD().PRIVATE().DEFAULT_VALUE(100.0f).NAME(mOneSecondDefaultSize);
    FIELD().PRIVATE().DEFAULT_VALUE(10.0f).NAME(mPerfectScaleSegmentSize);
    FIELD().PRIVATE().DEFAULT_VALUE(10.0f).NAME(mScaleOffset);
    FIELD().PRIVATE().DEFAULT_VALUE(Color4(16, 20, 23, 64)).NAME(mBigLineColor);
    FIELD().PRIVATE().DEFAULT_VALUE(Color4(16, 20, 23, 38)).NAME(mSmallLineColor);
    FIELD().PRIVATE().DEFAULT_VALUE(10.0f).NAME(mScaleSmoothLerpCoef);
    FIELD().PRIVATE().DEFAULT_VALUE(0.1f).NAME(mMinScale);
    FIELD().PRIVATE().DEFAULT_VALUE(10.0f).NAME(mMaxScale);
    FIELD().PRIVATE().DEFAULT_VALUE(0.1f/120.0f).NAME(mScaleSense);
    FIELD().PRIVATE().DEFAULT_VALUE(15.0f).NAME(mScrollSmoothCoef);
    FIELD().PRIVATE().DEFAULT_VALUE(10.0f).NAME(mScrollSpeedDecreaseCoef);
    FIELD().PRIVATE().DEFAULT_VALUE(10.0f).NAME(mScrollBorderBounceCoef);
    FIELD().PRIVATE().NAME(mAnimationWindow);
    FIELD().PRIVATE().NAME(mPlayer);
    FIELD().PRIVATE().NAME(mAnimation);
    FIELD().PRIVATE().DEFAULT_VALUE(0).NAME(mTimeCursor);
    FIELD().PRIVATE().DEFAULT_VALUE(0.0f).NAME(mSmoothViewScroll);
    FIELD().PRIVATE().DEFAULT_VALUE(0.0f).NAME(mViewScroll);
    FIELD().PRIVATE().DEFAULT_VALUE(false).NAME(mDragViewScroll);
    FIELD().PRIVATE().DEFAULT_VALUE(0.0f).NAME(mBeginDragViewScrollOffset);
    FIELD().PRIVATE().DEFAULT_VALUE(0.0f).NAME(mViewScrollSpeed);
    FIELD().PRIVATE().DEFAULT_VALUE(1.0f).NAME(mViewZoom);
    FIELD().PRIVATE().DEFAULT_VALUE(1.0f).NAME(mSmoothViewZoom);
    FIELD().PRIVATE().DEFAULT_VALUE(false).NAME(mViewHasZoomed);
    FIELD().PRIVATE().DEFAULT_VALUE(false).NAME(mViewMoveDisabled);
    FIELD().PRIVATE().DEFAULT_VALUE(0.0f).NAME(mDuration);
    FIELD().PRIVATE().NAME(mTextFont);
    FIELD().PRIVATE().NAME(mText);
    FIELD().PRIVATE().NAME(mBeginMark);
    FIELD().PRIVATE().NAME(mEndMark);
    FIELD().PRIVATE().DEFAULT_VALUE(Layout(Vec2F(0, 1), Vec2F(0, 1), Vec2F(-6.0f, 3.0f), Vec2F(6.0f, -21.0f))).NAME(mBeginMarkLayout);
    FIELD().PRIVATE().DEFAULT_VALUE(Layout(Vec2F(0, 1), Vec2F(1, 1), Vec2F(-4.0f, 3.0f), Vec2F(6.0f, -21.0f))).NAME(mEndMarkLayout);
    FIELD().PRIVATE().NAME(mScrollBar);
    FIELD().PRIVATE().NAME(mTimeLine);
    FIELD().PRIVATE().NAME(mTimeLineEventsArea);
}
END_META;
CLASS_METHODS_META(Editor::AnimationTimeline)
{

    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*);
    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*, const AnimationTimeline&);
    FUNCTION().PUBLIC().SIGNATURE(void, Draw);
    FUNCTION().PUBLIC().SIGNATURE(void, Update, float);
    FUNCTION().PUBLIC().SIGNATURE(void, SetAnimation, const Ref<AnimationClip>&, const Ref<AnimationPlayer>&);
    FUNCTION().PUBLIC().SIGNATURE(void, SetScroll, float);
    FUNCTION().PUBLIC().SIGNATURE(void, SetViewRange, float, float, bool);
    FUNCTION().PUBLIC().SIGNATURE(void, SetTimeCursor, float);
    FUNCTION().PUBLIC().SIGNATURE(float, GetTimeCursor);
    FUNCTION().PUBLIC().SIGNATURE(float, GetScroll);
    FUNCTION().PUBLIC().SIGNATURE(void, SetScale, float);
    FUNCTION().PUBLIC().SIGNATURE(float, GetScale);
    FUNCTION().PUBLIC().SIGNATURE(void, SetViewMoveDisabled, bool);
    FUNCTION().PUBLIC().SIGNATURE(float, LocalToWorld, float);
    FUNCTION().PUBLIC().SIGNATURE(float, WorldToLocal, float);
    FUNCTION().PUBLIC().SIGNATURE(const Ref<Text>&, GetText);
    FUNCTION().PUBLIC().SIGNATURE(void, SetScrollBar, const Ref<HorizontalScrollBar>&);
    FUNCTION().PUBLIC().SIGNATURE(const Ref<HorizontalScrollBar>&, GetScrollBar);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsSameTime, float, float, float);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsUnderPoint, const Vec2F&);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsScrollable);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsInputTransparent);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(String, GetCreateMenuCategory);
    FUNCTION().PRIVATE().SIGNATURE(void, UpdateDuration, float);
    FUNCTION().PRIVATE().SIGNATURE(void, DrawTimeScale);
    FUNCTION().PRIVATE().SIGNATURE(void, ChooseScaleParams, int&, double&);
    FUNCTION().PRIVATE().SIGNATURE(void, UpdateScrolling, float);
    FUNCTION().PRIVATE().SIGNATURE(void, UpdateZooming, float);
    FUNCTION().PRIVATE().SIGNATURE(void, UpdateScrollBarHandleSize);
    FUNCTION().PRIVATE().SIGNATURE(void, OnTransformUpdated);
    FUNCTION().PRIVATE().SIGNATURE(void, SetAnimationTimeByCursor, const Input::Cursor&);
    FUNCTION().PRIVATE().SIGNATURE(void, OnCursorRightMousePressed, const Input::Cursor&);
    FUNCTION().PRIVATE().SIGNATURE(void, OnCursorRightMouseStayDown, const Input::Cursor&);
    FUNCTION().PRIVATE().SIGNATURE(void, OnCursorRightMouseReleased, const Input::Cursor&);
    FUNCTION().PRIVATE().SIGNATURE(void, OnScrolled, float);
}
END_META;
// --- END META ---
