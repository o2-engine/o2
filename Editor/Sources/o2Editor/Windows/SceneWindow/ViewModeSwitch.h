#pragma once

#include "o2/Events/CursorAreaEventsListener.h"
#include "o2/Render/Text/Text.h"
#include "o2/Scene/UI/Widget.h"
#include "o2Editor/Windows/PipelineWindow/PipelineControls.h"

using namespace o2;

namespace Editor
{
    // ------------------------------------------------------------------------------------------------------
    // Rounded 2D/3D switch of the scene window: a track with both captions and a thumb that slides under the
    // chosen one. A click anywhere on it switches the mode
    // ------------------------------------------------------------------------------------------------------
    class ViewModeSwitch: public Widget, public CursorAreaEventsListener
    {
    public:
        Function<void(bool)> onSwitchByUser; // Called with the new value when the user clicks the switch

        static constexpr float slideSpeed = 14.0f; // How fast the thumb slides to the chosen side

    public:
        // Default constructor
        ViewModeSwitch(RefCounter* refCounter);

        // Copy-constructor
        ViewModeSwitch(RefCounter* refCounter, const ViewModeSwitch& other);

        // Copy operator
        ViewModeSwitch& operator=(const ViewModeSwitch& other);

        // Sets is 3D chosen; without the animation the thumb jumps to its side
        void SetValue(bool is3D, bool animate = true);

        // Returns is 3D chosen
        bool GetValue() const;

        // Returns where the thumb is: 0 under 2D, 1 under 3D
        float GetThumbPosition() const;

        // Returns the rectangle of the thumb
        RectF GetThumbRect() const;

        // Returns the rectangle of the track
        RectF GetTrackRect() const;

        // Draws the track, the thumb and the captions
        void Draw() override;

        // Slides the thumb
        void Update(float dt) override;

        // Returns true if point is in this object
        bool IsUnderPoint(const Vec2F& point) override;

        // Returns create menu category in editor
        static String GetCreateMenuCategory();

        SERIALIZABLE(ViewModeSwitch);
        CLONEABLE_REF(ViewModeSwitch);

    protected:
        bool  mValue = false;         // Is 3D chosen
        float mThumbPosition = 0.0f;  // Where the thumb is: 0 under 2D, 1 under 3D
        bool  mHovered = false;       // Is the cursor over the switch

        Ref<PipelineRoundedRect> mTrack;   // Rounded track
        Ref<PipelineRoundedRect> mThumb;   // Thumb under the chosen caption
        Ref<Text>                mLabel2D; // Caption of the left side
        Ref<Text>                mLabel3D; // Caption of the right side

    protected:
        // Makes the drawables
        void InitializeDrawables();

        // Switches the mode on a click
        void OnCursorReleased(const Input::Cursor& cursor) override;

        // Lights the track up
        void OnCursorEnter(const Input::Cursor& cursor) override;

        // Puts the track back
        void OnCursorExit(const Input::Cursor& cursor) override;
    };
}
// --- META ---

CLASS_BASES_META(Editor::ViewModeSwitch)
{
    BASE_CLASS(o2::Widget);
    BASE_CLASS(o2::CursorAreaEventsListener);
}
END_META;
CLASS_FIELDS_META(Editor::ViewModeSwitch)
{
    FIELD().PUBLIC().NAME(onSwitchByUser);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mValue);
    FIELD().PROTECTED().DEFAULT_VALUE(0.0f).NAME(mThumbPosition);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mHovered);
    FIELD().PROTECTED().NAME(mTrack);
    FIELD().PROTECTED().NAME(mThumb);
    FIELD().PROTECTED().NAME(mLabel2D);
    FIELD().PROTECTED().NAME(mLabel3D);
}
END_META;
CLASS_METHODS_META(Editor::ViewModeSwitch)
{

    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*);
    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*, const ViewModeSwitch&);
    FUNCTION().PUBLIC().SIGNATURE(void, SetValue, bool, bool);
    FUNCTION().PUBLIC().SIGNATURE(bool, GetValue);
    FUNCTION().PUBLIC().SIGNATURE(float, GetThumbPosition);
    FUNCTION().PUBLIC().SIGNATURE(RectF, GetThumbRect);
    FUNCTION().PUBLIC().SIGNATURE(RectF, GetTrackRect);
    FUNCTION().PUBLIC().SIGNATURE(void, Draw);
    FUNCTION().PUBLIC().SIGNATURE(void, Update, float);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsUnderPoint, const Vec2F&);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(String, GetCreateMenuCategory);
    FUNCTION().PROTECTED().SIGNATURE(void, InitializeDrawables);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCursorReleased, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCursorEnter, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCursorExit, const Input::Cursor&);
}
END_META;
// --- END META ---
