#pragma once

#include "o2/Events/CursorAreaEventsListener.h"
#include "o2/Events/KeyboardEventsListener.h"
#include "o2/Render/Sprite.h"
#include "o2/Scene/UI/Widget.h"
#include "o2/Utils/Function/SerializableFunction.h"
#include "o2/Utils/System/ShortcutKeys.h"

namespace o2
{
    class Text;

    // -------------
    // Button widget
    // -------------
    class Button: public Widget, public CursorAreaEventsListener, public KeyboardEventsListener
    {
    public:
        PROPERTIES(Button);
        PROPERTY(WString, caption, SetCaption, GetCaption); // Caption property. Searches "caption" layer and sets text
        PROPERTY(Ref<IRectDrawable>, icon, SetIcon, GetIconDrawable); // Icon drawable of the layer with name "icon"

    public:
        SerializableFunction<void()> onClick;       // Click event @SERIALIZABLE @SCRIPTABLE
        Function<bool(const Vec2F&)> isPointInside; // Checking pointer function. When this empty using default widget pointer check @EDITOR_IGNORE

    public:
        ShortcutKeys shortcut; // Shortcut keys

    public:
        // Default constructor @SCRIPTABLE
        Button(RefCounter* refCounter);

        // Copy-constructor
        Button(RefCounter* refCounter, const Button& other);

        // Assign operator
        Button& operator=(const Button& other);

        // Draws widget
        void Draw() override;

        // Sets caption of button. Searches text layer with name "caption". If can't find this layer, creates them @SCRIPTABLE
        void SetCaption(const WString& text);

        // Returns caption text from text layer "caption". Returns no data if layer isn't exist @SCRIPTABLE
        WString GetCaption() const;

        // Sets drawable of the layer "icon", does nothing when there is no such layer
        void SetIcon(const Ref<IRectDrawable>& drawable);

        // Returns icon as sprite, null when it is a drawable of another type
        Ref<Sprite> GetIcon() const;

        // Returns icon drawable
        Ref<IRectDrawable> GetIconDrawable() const;

        // Shows image by path in the icon layer, Sprite and VectorSprite are swapped by the kind of image
        void SetIconImage(const String& imagePath);

        // Returns is this widget can be selected
        bool IsFocusable() const override;

        // Returns true if point is in this object
        bool IsUnderPoint(const Vec2F& point) override;

        // Returns create menu group in editor
        static String GetCreateMenuGroup();

        SERIALIZABLE(Button);
        CLONEABLE_REF(Button);

    protected:
        WeakRef<Text>   mCaptionText; // Caption layer text
        WeakRef<IRectDrawable> mIconDrawable; // Icon layer drawable

    protected:
        // Called when cursor pressed on this. Sets state "pressed" to true
        void OnCursorPressed(const Input::Cursor& cursor) override;

        // Called when cursor released (only when cursor pressed this at previous time). Sets state "pressed" to false.
        // Called onClicked if cursor is still above this
        void OnCursorReleased(const Input::Cursor& cursor) override;

        // Called when cursor pressing was broken (when scrolled scroll area or some other)
        void OnCursorPressBreak(const Input::Cursor& cursor) override;

        // Called instead of press when the cursor is pressed again within the double click time; counts as a click
        void OnCursorDblClicked(const Input::Cursor& cursor) override;

        // Called when cursor enters this object. Sets state "select" to true
        void OnCursorEnter(const Input::Cursor& cursor) override;

        // Called when cursor exits this object. Sets state "select" to false
        void OnCursorExit(const Input::Cursor& cursor) override;

        // Called when key was pressed
        void OnKeyPressed(const Input::Key& key) override;

        // Called when key was released
        void OnKeyReleased(const Input::Key& key) override;

        // Called when layer added and updates drawing sequence
        void OnLayerAdded(const Ref<WidgetLayer>& layer) override;

        // Called when visible was changed
        void OnEnabled() override;

        // Called when visible was changed
        void OnDisabled() override;

        // Called when listener becomes interactable, disabled "inactive" state when exists
        void OnBecomeInteractable() override;

        // Called when listener stops interacting, enables "inactive" state when exists
        void OnBecomeNotInteractable() override;

        REF_COUNTERABLE_IMPL(Widget);
    };
}
// --- META ---

CLASS_BASES_META(o2::Button)
{
    BASE_CLASS(o2::Widget);
    BASE_CLASS(o2::CursorAreaEventsListener);
    BASE_CLASS(o2::KeyboardEventsListener);
}
END_META;
CLASS_FIELDS_META(o2::Button)
{
    FIELD().PUBLIC().NAME(caption);
    FIELD().PUBLIC().NAME(icon);
    FIELD().PUBLIC().SCRIPTABLE_ATTRIBUTE().SERIALIZABLE_ATTRIBUTE().NAME(onClick);
    FIELD().PUBLIC().EDITOR_IGNORE_ATTRIBUTE().NAME(isPointInside);
    FIELD().PUBLIC().NAME(shortcut);
    FIELD().PROTECTED().NAME(mCaptionText);
    FIELD().PROTECTED().NAME(mIconDrawable);
}
END_META;
CLASS_METHODS_META(o2::Button)
{

    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().CONSTRUCTOR(RefCounter*);
    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*, const Button&);
    FUNCTION().PUBLIC().SIGNATURE(void, Draw);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(void, SetCaption, const WString&);
    FUNCTION().PUBLIC().SCRIPTABLE_ATTRIBUTE().SIGNATURE(WString, GetCaption);
    FUNCTION().PUBLIC().SIGNATURE(void, SetIcon, const Ref<IRectDrawable>&);
    FUNCTION().PUBLIC().SIGNATURE(Ref<Sprite>, GetIcon);
    FUNCTION().PUBLIC().SIGNATURE(Ref<IRectDrawable>, GetIconDrawable);
    FUNCTION().PUBLIC().SIGNATURE(void, SetIconImage, const String&);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsFocusable);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsUnderPoint, const Vec2F&);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(String, GetCreateMenuGroup);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCursorPressed, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCursorReleased, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCursorPressBreak, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCursorDblClicked, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCursorEnter, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCursorExit, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnKeyPressed, const Input::Key&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnKeyReleased, const Input::Key&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnLayerAdded, const Ref<WidgetLayer>&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnEnabled);
    FUNCTION().PROTECTED().SIGNATURE(void, OnDisabled);
    FUNCTION().PROTECTED().SIGNATURE(void, OnBecomeInteractable);
    FUNCTION().PROTECTED().SIGNATURE(void, OnBecomeNotInteractable);
}
END_META;
// --- END META ---
