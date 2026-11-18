#pragma once

#include "o2/Events/CursorAreaEventsListener.h"
#include "o2/Render/VectorSprite.h"
#include "o2/Scene/UI/Widget.h"
#include "o2Editor/Windows/PipelineWindow/PipelineControls.h"

using namespace o2;

namespace Editor
{
    // ---------------------------------------------------------------------------------------------------------------
    // Group of icon buttons on one track. Choice: a flat dark block slides under the chosen icon, none when nothing is
    // chosen. Flags: the block is under every icon that is on. The hovered icon gets a lighter block. A click reports
    // the segment
    // ---------------------------------------------------------------------------------------------------------------
    class SegmentedIcons: public Widget, public CursorAreaEventsListener
    {
    public:
        enum class Mode { Choice, Flags };

    public:
        Function<void(int)> onClicked; // Called with the index of the segment the user clicked

        static constexpr float segmentWidth = 24.0f;    // Width of a segment when there is room
        static constexpr float minSegmentWidth = 18.0f; // Narrowest a segment gets
        static constexpr float trackHeight = 20.0f;     // Height of the track
        static constexpr float trackRadius = 3.0f;      // Corners of the track, as the fields have
        static constexpr float blockRadius = 2.0f;      // Corners of the blocks
        static constexpr float blockInset = 2.0f;       // Gap between the track and a block
        static constexpr float iconSize = 16.0f;        // Side of an icon
        static constexpr float slideSpeed = 18.0f;      // How fast the block slides

    public:
        // Default constructor
        explicit SegmentedIcons(RefCounter* refCounter);

        // Copy-constructor
        SegmentedIcons(RefCounter* refCounter, const SegmentedIcons& other);

        // Copy operator
        SegmentedIcons& operator=(const SegmentedIcons& other);

        // Sets the mode
        void SetMode(Mode mode);

        // Returns the mode
        Mode GetMode() const;

        // Makes the segments of the icons; the layout takes their widths
        void SetSegments(const Vector<String>& icons);

        // Returns the icons of the segments
        const Vector<String>& GetSegments() const;

        // Chooses the segment, -1 chooses none; without the animation the block jumps there
        void SetSelected(int index, bool animate = true);

        // Returns the chosen segment, -1 when none is
        int GetSelected() const;

        // Turns a segment of the flags on or off
        void SetOn(int index, bool on);

        // Returns is the segment of the flags on
        bool IsOn(int index) const;

        // Returns the segment under the cursor, -1 when none is
        int GetHoveredSegment() const;

        // Returns the rectangle of a segment
        RectF GetSegmentRect(int index) const;

        // Returns the rectangle of the track
        RectF GetTrackRect() const;

        // Returns the rectangle of the sliding block
        RectF GetThumbRect() const;

        // Slides the block, fades the blocks of the flags
        void Update(float dt) override;

        // Draws the track, the blocks and the icons
        void Draw() override;

        // Returns true if point is in this object
        bool IsUnderPoint(const Vec2F& point) override;

        // Returns create menu category in editor
        static String GetCreateMenuCategory();

        SERIALIZABLE(SegmentedIcons);
        CLONEABLE_REF(SegmentedIcons);

    protected:
        Mode           mMode = Mode::Choice; // How the segments behave
        Vector<String> mIconNames;           // Icons of the segments
        Vector<int>    mOn;                  // Which segments of the flags are on, 1 when on
        Vector<float>  mOnCoefs;             // Their blocks, fading in and out

        int   mSelected = -1;        // Chosen segment of the choice
        float mThumbPosition = 0.0f; // Where the block is, in segments
        float mThumbCoef = 0.0f;     // How much of the block is shown: it fades when nothing is chosen
        int   mHovered = -1;         // Segment under the cursor
        int   mPressed = -1;         // Segment pressed by the cursor

        Ref<PipelineRoundedRect> mTrack; // Track
        Ref<PipelineRoundedRect> mBlock; // Block of the chosen and the hovered segment

        Vector<Ref<VectorSprite>> mIcons; // Icons of the segments

    protected:
        // Makes the drawables
        void InitializeDrawables();

        // Makes the icons of the segments
        void InitializeIcons();

        // Returns the segment at the point, -1 outside
        int GetSegmentAt(const Vec2F& point) const;

        // Returns the rectangle of a block over a segment, which may be between two
        RectF GetBlockRect(float position) const;

        // Draws a block
        void DrawBlock(const RectF& rect, const Color4& color, float alpha);

        // Clicks the segment as the user does
        void Click(int segment);

        void OnCursorPressed(const Input::Cursor& cursor) override;
        void OnCursorReleased(const Input::Cursor& cursor) override;

        // A quick second click is a double click for the listener, but for the icons it is a click on one of them
        void OnCursorDblClicked(const Input::Cursor& cursor) override;

        void OnCursorPressBreak(const Input::Cursor& cursor) override;
        void OnCursorMoved(const Input::Cursor& cursor) override;
        void OnCursorEnter(const Input::Cursor& cursor) override;
        void OnCursorExit(const Input::Cursor& cursor) override;
    };
}
// --- META ---

PRE_ENUM_META(Editor::SegmentedIcons::Mode);

CLASS_BASES_META(Editor::SegmentedIcons)
{
    BASE_CLASS(o2::Widget);
    BASE_CLASS(o2::CursorAreaEventsListener);
}
END_META;
CLASS_FIELDS_META(Editor::SegmentedIcons)
{
    FIELD().PUBLIC().NAME(onClicked);
    FIELD().PROTECTED().DEFAULT_VALUE(Mode::Choice).NAME(mMode);
    FIELD().PROTECTED().NAME(mIconNames);
    FIELD().PROTECTED().NAME(mOn);
    FIELD().PROTECTED().NAME(mOnCoefs);
    FIELD().PROTECTED().DEFAULT_VALUE(-1).NAME(mSelected);
    FIELD().PROTECTED().DEFAULT_VALUE(0.0f).NAME(mThumbPosition);
    FIELD().PROTECTED().DEFAULT_VALUE(0.0f).NAME(mThumbCoef);
    FIELD().PROTECTED().DEFAULT_VALUE(-1).NAME(mHovered);
    FIELD().PROTECTED().DEFAULT_VALUE(-1).NAME(mPressed);
    FIELD().PROTECTED().NAME(mTrack);
    FIELD().PROTECTED().NAME(mBlock);
    FIELD().PROTECTED().NAME(mIcons);
}
END_META;
CLASS_METHODS_META(Editor::SegmentedIcons)
{

    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*);
    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*, const SegmentedIcons&);
    FUNCTION().PUBLIC().SIGNATURE(void, SetMode, Mode);
    FUNCTION().PUBLIC().SIGNATURE(Mode, GetMode);
    FUNCTION().PUBLIC().SIGNATURE(void, SetSegments, const Vector<String>&);
    FUNCTION().PUBLIC().SIGNATURE(const Vector<String>&, GetSegments);
    FUNCTION().PUBLIC().SIGNATURE(void, SetSelected, int, bool);
    FUNCTION().PUBLIC().SIGNATURE(int, GetSelected);
    FUNCTION().PUBLIC().SIGNATURE(void, SetOn, int, bool);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsOn, int);
    FUNCTION().PUBLIC().SIGNATURE(int, GetHoveredSegment);
    FUNCTION().PUBLIC().SIGNATURE(RectF, GetSegmentRect, int);
    FUNCTION().PUBLIC().SIGNATURE(RectF, GetTrackRect);
    FUNCTION().PUBLIC().SIGNATURE(RectF, GetThumbRect);
    FUNCTION().PUBLIC().SIGNATURE(void, Update, float);
    FUNCTION().PUBLIC().SIGNATURE(void, Draw);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsUnderPoint, const Vec2F&);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(String, GetCreateMenuCategory);
    FUNCTION().PROTECTED().SIGNATURE(void, InitializeDrawables);
    FUNCTION().PROTECTED().SIGNATURE(void, InitializeIcons);
    FUNCTION().PROTECTED().SIGNATURE(int, GetSegmentAt, const Vec2F&);
    FUNCTION().PROTECTED().SIGNATURE(RectF, GetBlockRect, float);
    FUNCTION().PROTECTED().SIGNATURE(void, DrawBlock, const RectF&, const Color4&, float);
    FUNCTION().PROTECTED().SIGNATURE(void, Click, int);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCursorPressed, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCursorReleased, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCursorDblClicked, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCursorPressBreak, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCursorMoved, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCursorEnter, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCursorExit, const Input::Cursor&);
}
END_META;
// --- END META ---
