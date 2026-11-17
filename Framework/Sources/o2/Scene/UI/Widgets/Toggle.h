#pragma once

#include "o2/Events/DrawableCursorEventsListener.h"
#include "o2/Events/KeyboardEventsListener.h"
#include "o2/Scene/UI/Widget.h"
#include "o2/Utils/System/ShortcutKeys.h"

namespace o2
{
    class Text;
    class Sprite;
    class Toggle;

    // ------------
    // Toggle group
    // ------------
    class ToggleGroup: public RefCounterable
    {
    public:
        enum class Type { OnlySingleTrue, VerOneClick, HorOneClick };

    public:
        Function<void(bool)> onPressed;  // Toggle group press event
        Function<void(bool)> onReleased; // Toggle group release event

    public:
        // Constructor by type
        ToggleGroup(Type type);

        // Destructor
        ~ToggleGroup();

        // Adds toggle to group
        void AddToggle(const Ref<Toggle>& toggle);

        // Removes toggle from group
        void RemoveToggle(Toggle* toggle);

        // Returns all toggles in group
        const Vector<WeakRef<Toggle>>& GetToggles() const;

        // Returns toggled toggles in group
        const Vector<WeakRef<Toggle>>& GetToggled() const;

    protected:
        bool mPressed = false;      // Is group in pressed state
        bool mPressedValue = false; // Group pressed value

        Vector<WeakRef<Toggle>> mToggles; // All toggles in group
        Vector<WeakRef<Toggle>> mToggled; // Toggled toggles in group

        Type mType; // Toggle group type

    protected:
        // Called when some toggle was toggled, 
        void OnToggled(const Ref<Toggle>& toggle);

        friend class Toggle;
    };

    class Toggle: public Widget, public DrawableCursorEventsListener, public KeyboardEventsListener
    {
    public:
        PROPERTIES(Toggle);
        PROPERTY(bool, value, SetValue, GetValue);                               // Current state value property
        PROPERTY(WString, caption, SetCaption, GetCaption);                      // Caption property. Searches text layer with name "caption" or creates them if he's not exist
        PROPERTY(Ref<ToggleGroup>, toggleGroup, SetToggleGroup, GetToggleGroup); // Toggle group property

    public:
        Function<void()>     onClick;        // Click event
        Function<void(bool)> onToggle;       // Toggle event
        Function<void(bool)> onToggleByUser; // Toggle by user event 

    public:
        ShortcutKeys shortcut;// Shortcut keys

    public:
        // Default constructor
        explicit Toggle(RefCounter* refCounter);

        // Copy-constructor
        Toggle(RefCounter* refCounter, const Toggle& other);

        // Assign operator
        Toggle& operator=(const Toggle& other);

        // Destructor
        ~Toggle();

        // Updates drawables, states and widget
        void Update(float dt) override;

        // Sets caption of button. Searches text layer with name "caption". If can't find this layer, creates them
        void SetCaption(const WString& text);

        // Returns caption text from text layer "caption". Returns no data if layer isn't exist
        WString GetCaption() const;

        // Sets current value
        void SetValue(bool value);

        // Sets value as unknown
        void SetValueUnknown();

        // Is value unknown
        bool IsValueUnknown() const;

        // Returns current value
        bool GetValue() const;

        // Sets toggle group
        void SetToggleGroup(const Ref<ToggleGroup>& toggleGroup);

        // Returns toggle group
        const Ref<ToggleGroup>& GetToggleGroup() const;

        // Returns is this widget can be selected
        bool IsFocusable() const override;

        // Returns create menu group in editor
        static String GetCreateMenuGroup();

        // Returns the icon of the type in the editor
        static String GetEditorIcon();

        SERIALIZABLE(Toggle);
        CLONEABLE_REF(Toggle);

    protected:
        bool mValue = false;        // Current value @SERIALIZABLE
        bool mValueUnknown = false; // Is value unknown @SERIALIZABLE

        Ref<Text>        mCaptionText; // Caption layer text
        Ref<WidgetLayer> mBackLayer;   // Background layer

        Ref<ToggleGroup> mToggleGroup; // Toggle group

    protected:
        // Called when deserialized
        void OnDeserialized(const DataValue& node) override;

        // Called when layer added and updates drawing sequence
        void OnLayerAdded(const Ref<WidgetLayer>& layer) override;

        // Called when visible was changed
        void OnEnabled() override;

        // Called when visible was changed
        void OnDisabled() override;

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
        
        REF_COUNTERABLE_IMPL(Widget);

        friend class ToggleGroup;
    };
}
// --- META ---

PRE_ENUM_META(o2::ToggleGroup::Type);

CLASS_BASES_META(o2::Toggle)
{
    BASE_CLASS(o2::Widget);
    BASE_CLASS(o2::DrawableCursorEventsListener);
    BASE_CLASS(o2::KeyboardEventsListener);
}
END_META;
CLASS_FIELDS_META(o2::Toggle)
{
    FIELD().PUBLIC().NAME(value);
    FIELD().PUBLIC().NAME(caption);
    FIELD().PUBLIC().NAME(toggleGroup);
    FIELD().PUBLIC().NAME(onClick);
    FIELD().PUBLIC().NAME(onToggle);
    FIELD().PUBLIC().NAME(onToggleByUser);
    FIELD().PUBLIC().NAME(shortcut);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(false).NAME(mValue);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(false).NAME(mValueUnknown);
    FIELD().PROTECTED().NAME(mCaptionText);
    FIELD().PROTECTED().NAME(mBackLayer);
    FIELD().PROTECTED().NAME(mToggleGroup);
}
END_META;
CLASS_METHODS_META(o2::Toggle)
{

    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*);
    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*, const Toggle&);
    FUNCTION().PUBLIC().SIGNATURE(void, Update, float);
    FUNCTION().PUBLIC().SIGNATURE(void, SetCaption, const WString&);
    FUNCTION().PUBLIC().SIGNATURE(WString, GetCaption);
    FUNCTION().PUBLIC().SIGNATURE(void, SetValue, bool);
    FUNCTION().PUBLIC().SIGNATURE(void, SetValueUnknown);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsValueUnknown);
    FUNCTION().PUBLIC().SIGNATURE(bool, GetValue);
    FUNCTION().PUBLIC().SIGNATURE(void, SetToggleGroup, const Ref<ToggleGroup>&);
    FUNCTION().PUBLIC().SIGNATURE(const Ref<ToggleGroup>&, GetToggleGroup);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsFocusable);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(String, GetCreateMenuGroup);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(String, GetEditorIcon);
    FUNCTION().PROTECTED().SIGNATURE(void, OnDeserialized, const DataValue&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnLayerAdded, const Ref<WidgetLayer>&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnEnabled);
    FUNCTION().PROTECTED().SIGNATURE(void, OnDisabled);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCursorPressed, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCursorReleased, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCursorPressBreak, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCursorEnter, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCursorExit, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnKeyPressed, const Input::Key&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnKeyReleased, const Input::Key&);
}
END_META;
// --- END META ---
