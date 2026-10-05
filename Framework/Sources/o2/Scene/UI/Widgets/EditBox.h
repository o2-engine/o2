#pragma once

#include "o2/Events/CursorAreaEventsListener.h"
#include "o2/Events/KeyboardEventsListener.h"
#include "o2/Render/Mesh.h"
#include "o2/Render/Sprite.h"
#include "o2/Render/Text.h"
#include "o2/Scene/UI/Widgets/ScrollArea.h"

namespace o2
{
    // --------------------
    // Text edit box widget
    // --------------------
    class EditBox: public ScrollArea, public KeyboardEventsListener
    {
    public:
        PROPERTIES(EditBox);
        PROPERTY(WString, text, SetText, GetText);                  // Text property
        PROPERTY(int, caret, SetCaretPosition, GetCaretPosition); // Caret position property

        PROPERTY(int, selectionBegin, SetSelectionBegin, GetSelectionBegin); // Selection begin index property
        PROPERTY(int, selectionEnd, SetSelectionEnd, GetSelectionEnd);         // Selection end index property

    public:
        Function<void(const WString&)> onChanged;         // Text changed event
        Function<void(const WString&)> onChangeCompleted; // Text changing completed event

    public:
        // Default constructor
        explicit EditBox(RefCounter* refCounter);

        // Copy-constructor
        EditBox(RefCounter* refCounter, const EditBox& other);

        // Destructor
        ~EditBox();

        // Copy-operator
        EditBox& operator=(const EditBox& other);

        // Draws widget
        void Draw() override;

        // Updates widget
        void Update(float dt) override;

        // Sets text
        void SetText(const WString& text);

        // returns text
        WString GetText() const;

        // Sets caret position
        void SetCaretPosition(int caretPosition);

        // Returns caret position
        int GetCaretPosition();

        // Sets selection begin and end indexes
        void Select(int begin, int end);

        // Sets selection begin position
        void SetSelectionBegin(int position);

        // Returns selection begin position
        int GetSelectionBegin() const;

        // Sets selection end position
        void SetSelectionEnd(int position);

        // Returns selection end position
        int GetSelectionEnd() const;

        // Resets selection
        void Deselect();

        // Selects all text
        void SelectAll();

        // Returns text drawable
        const Ref<Text>& GetTextDrawable();

        // Sets caret drawable
        void SetCaretDrawable(const Ref<IRectDrawable>& drawable);

        // Returns caret drawable as sprite, null when it is a drawable of another type
        Ref<Sprite> GetCaretDrawable();

        // Returns caret drawable
        const Ref<IRectDrawable>& GetCaretRectDrawable() const;

        // Sets selection color
        void SetSelectionColor(const Color4& color);

        // Returns selection color
        Color4 GetSelectionColor() const;

        // Sets filter characters for integer numbers
        void SetFilterInteger();

        // Sets filter characters for decimal numbers
        void SetFilterFloat();

        //Sets filter characters for names
        void SetFilterNames();

        // Sets available characters
        void SetAvailableSymbols(const WString& availableSymbols);

        // Returns available characters
        WString GetAvailableSymbols() const;

        // Sets max sizes of line and lines count
        void SetMaxSizes(int maxLineCharactersCount, int maxLinesCount);

        // Sets maximum characters in line
        void SetMaxLineCharactersCount(int count);

        // Returns maximum characters in line
        int GetMaxLineCharactersCount() const;

        // Sets maximum lines count
        void SetMaxLinesCount(int count);

        // Returns maximum lines count
        int GetMaxLinesCount() const;

        // Sets text multiline
        void SetMultiLine(bool multiline);

        // Returns is text multiline
        bool IsMultiLine() const;

        // Sets word wrapping
        void SetWordWrap(bool wordWrap);

        // returns is word wrapping
        bool IsWordWrap() const;

        // Sets caret blinking delay in seconds
        void SetCaretBlinkingDelay(float delay);

        // Returns caret blinking delay in seconds
        float GetCaretBlinkingDelay() const;

        // Returns is listener scrollable
        bool IsScrollable() const override;

        // Returns is this widget can be selected
        bool IsFocusable() const override;

        // Returns true if point is under drawable
        bool IsUnderPoint(const Vec2F& point) override;

        // Returns true when input events can be handled by down listeners, always returns false
        bool IsInputTransparent() const override;

        // Returns create menu group in editor
        static String GetCreateMenuGroup();

        SERIALIZABLE(EditBox);
        CLONEABLE_REF(EditBox);

    protected:
        Color4  mSelectionColor = Color4(0.1f, 0.2f, 0.6f, 0.3f); // Text selection color @SERIALIZABLE

        WString mLastText;         // Last text
        WString mText;             // Current text @SERIALIZABLE
        WString mAvailableSymbols; // Available symbols @SERIALIZABLE

        Ref<Text>   mTextDrawable;  // Text drawable @SERIALIZABLE
        Ref<Mesh>   mSelectionMesh; // Selection mesh
        Ref<IRectDrawable> mCaretDrawable; // Caret drawable @SERIALIZABLE

        float mCaretBlinkDelay = 1.0f; // Caret blinking delay @SERIALIZABLE
        float mCaretBlinkTime = 0.0f;  // Caret blinking timer

        int  mSelectionBegin = 0;       // Selection begin index
        int  mSelectionEnd = 0;            // Selection end index
        bool mSelectingByWords = false; // Selection works by solid words
        int  mSelWordBegin = 0;         // Selection by words begin index
        int  mSelWordEnd = 0;           // Selection by words end index

        bool mMultiLine = true;          // True if text is multiline @SERIALIZABLE
        bool mWordWrap = false;          // True if text words wrapping @SERIALIZABLE
        int  mMaxLineChars = INT_MAX;  // Count of maximum characters in line @SERIALIZABLE
        int  mMaxLinesCount = INT_MAX; // Count of maximum lines count @SERIALIZABLE

        bool  mJustFocused = false;   // Is edit box selected at current frame
        float mLastClickTime = -1.0f; // Time of last clicking
        Vec2F mLastCursorPos;         // Last pressed cursor position

    protected:
        // Updates transparency for this and children widgets
        void UpdateTransparency() override;

        // Updates layers layouts, calls after updating widget layout
        void UpdateLayersLayouts() override;

        // Called when visible was changed
        void OnEnabled() override;

        // Called when visible was changed
        void OnDisabled() override;

        // Called when widget was selected
        void OnFocused() override;

        // Called when widget was deselected
        void OnUnfocused() override;

        // Updates scroll parameters: clip area, scroll size
        void UpdateScrollParams() override;

        // Called when cursor pressed on this
        void OnCursorPressed(const Input::Cursor& cursor) override;

        // Called when cursor released (only when cursor pressed this at previous time)
        void OnCursorReleased(const Input::Cursor& cursor) override;

        // Called when cursor pressing was broken (when scrolled scroll area or some other)
        void OnCursorPressBreak(const Input::Cursor& cursor) override;

        // Called when cursor released outside this(only when cursor pressed this at previous time)
        void OnCursorReleasedOutside(const Input::Cursor& cursor) override;

        // Called when cursor stay down during frame
        void OnCursorStillDown(const Input::Cursor& cursor) override;

        // Called when cursor enters this object
        void OnCursorEnter(const Input::Cursor& cursor) override;

        // Called when cursor exits this object
        void OnCursorExit(const Input::Cursor& cursor) override;

        // Called when right mouse button was pressed on this
        void OnCursorRightMousePressed(const Input::Cursor& cursor) override;

        // Called when right mouse button stay down on this
        void OnCursorRightMouseStayDown(const Input::Cursor& cursor) override;

        // Called when right mouse button was released (only when right mouse button pressed this at previous time)
        void OnCursorRightMouseReleased(const Input::Cursor& cursor) override;

        // Called when key was pressed
        void OnKeyPressed(const Input::Key& key) override;

        // Called when key was released
        void OnKeyReleased(const Input::Key& key) override;

        // Called when key stay down during frame
        void OnKeyStayDown(const Input::Key& key) override;

        // Returns text filtered by available characters set
        WString GetFilteredText(const WString& text);

        // Checks text for maximal characters in line and lines count
        void CheckCharactersAndLinesBounds();

        // Updates selection mesh and fixing selection bounds
        void UpdateSelectionAndCaret();

        // Returns coordinates of caret position by caret text position
        Vec2F GetTextCaretPosition(int position);

        //Returns of caret text position caret position by caret coordinates
        int GetTextCaretPosition(const Vec2F& point);

        // Updates cursor blinking
        void UpdateCaretBlinking(float dt);

        // Adds rectangle geometry to selection mesh
        void AddSelectionRect(const RectF& rect);

        // Checks scroll to cursor
        void CheckScrollingToCaret();

        // Jump selection over word
        void JumpSelection(bool forward, bool selecting);

        // Checks character typing by keyboard key
        void CheckCharacterTyping(KeyboardKey key);

        // Check for erasing commands
        void CheckCharactersErasing(KeyboardKey key);

        // Checks caret moving commands
        void CheckCaretMoving(KeyboardKey key);

        // Checks clipboard commands (copy cut paste)
        void CheckClipboard(KeyboardKey key);

        // Moves mSelectionEnd position and mSelectionBegin, when selecting
        void MoveCaret(int newPosition, bool selecting);
        
        REF_COUNTERABLE_IMPL(ScrollArea);
    };
}
// --- META ---

CLASS_BASES_META(o2::EditBox)
{
    BASE_CLASS(o2::ScrollArea);
    BASE_CLASS(o2::KeyboardEventsListener);
}
END_META;
CLASS_FIELDS_META(o2::EditBox)
{
    FIELD().PUBLIC().NAME(text);
    FIELD().PUBLIC().NAME(caret);
    FIELD().PUBLIC().NAME(selectionBegin);
    FIELD().PUBLIC().NAME(selectionEnd);
    FIELD().PUBLIC().NAME(onChanged);
    FIELD().PUBLIC().NAME(onChangeCompleted);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(Color4(0.1f, 0.2f, 0.6f, 0.3f)).NAME(mSelectionColor);
    FIELD().PROTECTED().NAME(mLastText);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().NAME(mText);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().NAME(mAvailableSymbols);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().NAME(mTextDrawable);
    FIELD().PROTECTED().NAME(mSelectionMesh);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().NAME(mCaretDrawable);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(1.0f).NAME(mCaretBlinkDelay);
    FIELD().PROTECTED().DEFAULT_VALUE(0.0f).NAME(mCaretBlinkTime);
    FIELD().PROTECTED().DEFAULT_VALUE(0).NAME(mSelectionBegin);
    FIELD().PROTECTED().DEFAULT_VALUE(0).NAME(mSelectionEnd);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mSelectingByWords);
    FIELD().PROTECTED().DEFAULT_VALUE(0).NAME(mSelWordBegin);
    FIELD().PROTECTED().DEFAULT_VALUE(0).NAME(mSelWordEnd);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(true).NAME(mMultiLine);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(false).NAME(mWordWrap);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(INT_MAX).NAME(mMaxLineChars);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(INT_MAX).NAME(mMaxLinesCount);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mJustFocused);
    FIELD().PROTECTED().DEFAULT_VALUE(-1.0f).NAME(mLastClickTime);
    FIELD().PROTECTED().NAME(mLastCursorPos);
}
END_META;
CLASS_METHODS_META(o2::EditBox)
{

    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*);
    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*, const EditBox&);
    FUNCTION().PUBLIC().SIGNATURE(void, Draw);
    FUNCTION().PUBLIC().SIGNATURE(void, Update, float);
    FUNCTION().PUBLIC().SIGNATURE(void, SetText, const WString&);
    FUNCTION().PUBLIC().SIGNATURE(WString, GetText);
    FUNCTION().PUBLIC().SIGNATURE(void, SetCaretPosition, int);
    FUNCTION().PUBLIC().SIGNATURE(int, GetCaretPosition);
    FUNCTION().PUBLIC().SIGNATURE(void, Select, int, int);
    FUNCTION().PUBLIC().SIGNATURE(void, SetSelectionBegin, int);
    FUNCTION().PUBLIC().SIGNATURE(int, GetSelectionBegin);
    FUNCTION().PUBLIC().SIGNATURE(void, SetSelectionEnd, int);
    FUNCTION().PUBLIC().SIGNATURE(int, GetSelectionEnd);
    FUNCTION().PUBLIC().SIGNATURE(void, Deselect);
    FUNCTION().PUBLIC().SIGNATURE(void, SelectAll);
    FUNCTION().PUBLIC().SIGNATURE(const Ref<Text>&, GetTextDrawable);
    FUNCTION().PUBLIC().SIGNATURE(void, SetCaretDrawable, const Ref<IRectDrawable>&);
    FUNCTION().PUBLIC().SIGNATURE(Ref<Sprite>, GetCaretDrawable);
    FUNCTION().PUBLIC().SIGNATURE(const Ref<IRectDrawable>&, GetCaretRectDrawable);
    FUNCTION().PUBLIC().SIGNATURE(void, SetSelectionColor, const Color4&);
    FUNCTION().PUBLIC().SIGNATURE(Color4, GetSelectionColor);
    FUNCTION().PUBLIC().SIGNATURE(void, SetFilterInteger);
    FUNCTION().PUBLIC().SIGNATURE(void, SetFilterFloat);
    FUNCTION().PUBLIC().SIGNATURE(void, SetFilterNames);
    FUNCTION().PUBLIC().SIGNATURE(void, SetAvailableSymbols, const WString&);
    FUNCTION().PUBLIC().SIGNATURE(WString, GetAvailableSymbols);
    FUNCTION().PUBLIC().SIGNATURE(void, SetMaxSizes, int, int);
    FUNCTION().PUBLIC().SIGNATURE(void, SetMaxLineCharactersCount, int);
    FUNCTION().PUBLIC().SIGNATURE(int, GetMaxLineCharactersCount);
    FUNCTION().PUBLIC().SIGNATURE(void, SetMaxLinesCount, int);
    FUNCTION().PUBLIC().SIGNATURE(int, GetMaxLinesCount);
    FUNCTION().PUBLIC().SIGNATURE(void, SetMultiLine, bool);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsMultiLine);
    FUNCTION().PUBLIC().SIGNATURE(void, SetWordWrap, bool);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsWordWrap);
    FUNCTION().PUBLIC().SIGNATURE(void, SetCaretBlinkingDelay, float);
    FUNCTION().PUBLIC().SIGNATURE(float, GetCaretBlinkingDelay);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsScrollable);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsFocusable);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsUnderPoint, const Vec2F&);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsInputTransparent);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(String, GetCreateMenuGroup);
    FUNCTION().PROTECTED().SIGNATURE(void, UpdateTransparency);
    FUNCTION().PROTECTED().SIGNATURE(void, UpdateLayersLayouts);
    FUNCTION().PROTECTED().SIGNATURE(void, OnEnabled);
    FUNCTION().PROTECTED().SIGNATURE(void, OnDisabled);
    FUNCTION().PROTECTED().SIGNATURE(void, OnFocused);
    FUNCTION().PROTECTED().SIGNATURE(void, OnUnfocused);
    FUNCTION().PROTECTED().SIGNATURE(void, UpdateScrollParams);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCursorPressed, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCursorReleased, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCursorPressBreak, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCursorReleasedOutside, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCursorStillDown, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCursorEnter, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCursorExit, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCursorRightMousePressed, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCursorRightMouseStayDown, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCursorRightMouseReleased, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnKeyPressed, const Input::Key&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnKeyReleased, const Input::Key&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnKeyStayDown, const Input::Key&);
    FUNCTION().PROTECTED().SIGNATURE(WString, GetFilteredText, const WString&);
    FUNCTION().PROTECTED().SIGNATURE(void, CheckCharactersAndLinesBounds);
    FUNCTION().PROTECTED().SIGNATURE(void, UpdateSelectionAndCaret);
    FUNCTION().PROTECTED().SIGNATURE(Vec2F, GetTextCaretPosition, int);
    FUNCTION().PROTECTED().SIGNATURE(int, GetTextCaretPosition, const Vec2F&);
    FUNCTION().PROTECTED().SIGNATURE(void, UpdateCaretBlinking, float);
    FUNCTION().PROTECTED().SIGNATURE(void, AddSelectionRect, const RectF&);
    FUNCTION().PROTECTED().SIGNATURE(void, CheckScrollingToCaret);
    FUNCTION().PROTECTED().SIGNATURE(void, JumpSelection, bool, bool);
    FUNCTION().PROTECTED().SIGNATURE(void, CheckCharacterTyping, KeyboardKey);
    FUNCTION().PROTECTED().SIGNATURE(void, CheckCharactersErasing, KeyboardKey);
    FUNCTION().PROTECTED().SIGNATURE(void, CheckCaretMoving, KeyboardKey);
    FUNCTION().PROTECTED().SIGNATURE(void, CheckClipboard, KeyboardKey);
    FUNCTION().PROTECTED().SIGNATURE(void, MoveCaret, int, bool);
}
END_META;
// --- END META ---
