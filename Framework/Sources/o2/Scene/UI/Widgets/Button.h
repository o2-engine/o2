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
        PROPERTY(Sprite*, icon, SetIcon, GetIcon);          // Icon image asset setter. Searches sprite layer with name "icon" and sets image

    public:
        SerializableFunction<void()> onClick;       // Click event @SERIALIZABLE
        Function<bool(const Vec2F&)> isPointInside; // Checking pointer function. When this empty using default widget pointer check @EDITOR_IGNORE

    public:
        ShortcutKeys shortcut; // Shortcut keys

    public:
        // Default constructor
        Button();

        // Copy-constructor
        Button(const Button& other);

        // Assign operator
        Button& operator=(const Button& other);

        // Draws widget
        void Draw() override;

        // Sets caption of button. Searches text layer with name "caption". If can't find this layer, creates them
        void SetCaption(const WString& text);

        // Returns caption text from text layer "caption". Returns no data if layer isn't exist
        WString GetCaption() const;

        // Sets icon sprite. Searches sprite layer "icon". Creates a new icon if isn't exist
        void SetIcon(Sprite* sprite);

        // Returns icon sprite
        Sprite* GetIcon() const;

        // Returns is this widget can be selected
        bool IsFocusable() const override;

        // Returns true if point is in this object
        bool IsUnderPoint(const Vec2F& point) override;

        // Returns create menu group in editor
        static String GetCreateMenuGroup();

        SERIALIZABLE(Button);

    protected:
        Text*   mCaptionText = nullptr; // Caption layer text
        Sprite* mIconSprite = nullptr;  // Icon layer sprite

    protected:
        // Called when cursor pressed on this. Sets state "pressed" to true
        void OnCursorPressed(const Input::Cursor& cursor) override;

        // Called when cursor released (only when cursor pressed this at previous time). Sets state "pressed" to false.
        // Called onClicked if cursor is still above this
        void OnCursorReleased(const Input::Cursor& cursor) override;

        // Called when cursor pressing was broken (when scrolled scroll area or some other)
        void OnCursorPressBreak(const Input::Cursor& cursor) override;

        // Called when cursor enters this object. Sets state "select" to true
        void OnCursorEnter(const Input::Cursor& cursor) override;

        // Called when cursor exits this object. Sets state "select" to false
        void OnCursorExit(const Input::Cursor& cursor) override;

        // Called when key was pressed
        void OnKeyPressed(const Input::Key& key) override;

        // Called when key was released
        void OnKeyReleased(const Input::Key& key) override;

        // Called when layer added and updates drawing sequence
        void OnLayerAdded(WidgetLayer* layer) override;

        // Called when visible was changed
        void OnEnabled() override;

        // Called when visible was changed
        void OnDisabled() override;

        // Called when listener becomes interactable, disabled "inactive" state when exists
        void OnBecomeInteractable() override;

        // Called when listener stops interacting, enables "inactive" state when exists
        void OnBecomeNotInteractable() override;
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
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().NAME(onClick);
    FIELD().PUBLIC().EDITOR_IGNORE_ATTRIBUTE().NAME(isPointInside);
    FIELD().PUBLIC().NAME(shortcut);
    FIELD().PROTECTED().DEFAULT_VALUE(nullptr).NAME(mCaptionText);
    FIELD().PROTECTED().DEFAULT_VALUE(nullptr).NAME(mIconSprite);
}
END_META;
CLASS_METHODS_META(o2::Button)
{

    FUNCTION().PUBLIC().CONSTRUCTOR();
    FUNCTION().PUBLIC().CONSTRUCTOR(const Button&);
    FUNCTION().PUBLIC().SIGNATURE(void, Draw);
    FUNCTION().PUBLIC().SIGNATURE(void, SetCaption, const WString&);
    FUNCTION().PUBLIC().SIGNATURE(WString, GetCaption);
    FUNCTION().PUBLIC().SIGNATURE(void, SetIcon, Sprite*);
    FUNCTION().PUBLIC().SIGNATURE(Sprite*, GetIcon);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsFocusable);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsUnderPoint, const Vec2F&);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(String, GetCreateMenuGroup);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCursorPressed, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCursorReleased, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCursorPressBreak, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCursorEnter, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCursorExit, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnKeyPressed, const Input::Key&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnKeyReleased, const Input::Key&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnLayerAdded, WidgetLayer*);
    FUNCTION().PROTECTED().SIGNATURE(void, OnEnabled);
    FUNCTION().PROTECTED().SIGNATURE(void, OnDisabled);
    FUNCTION().PROTECTED().SIGNATURE(void, OnBecomeInteractable);
    FUNCTION().PROTECTED().SIGNATURE(void, OnBecomeNotInteractable);
}
END_META;
// --- END META ---
