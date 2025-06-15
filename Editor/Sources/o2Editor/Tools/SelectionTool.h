#pragma once

#include "o2Editor/Tools/IEditorTool.h"

using namespace o2;

namespace o2
{
    class Sprite;
    class SceneEditableObject;
}

namespace Editor
{
    // ---------------------
    // Editor selection tool
    // ---------------------
    class SelectionTool: public IEditTool
    {
    public:
        // Default constructor
        SelectionTool();

        // Destructor
        ~SelectionTool();

        IOBJECT(SelectionTool);

    protected:
        Ref<Sprite> mSelectionSprite; // Selection frame sprite

        Vector<Ref<SceneEditableObject>> mCurrentSelectingObjects; // Current selecting objects (when cursor pressed, but not released yet)
        Vector<Ref<SceneEditableObject>> mBeforeSelectingObjects;  // Before selection objects array

        Vec2F mPressPoint;               // Press point before selecting
        bool  mSelectingByFrame = false; // Is selecting objects now

    protected:
        // Returns toggle in menu panel icon name
        String GetPanelIcon() const override;

        // Returns shortcut keys for toggle
        ShortcutKeys GetShortcut() const override;

        // Draws tool
        void DrawScene() override;

        // Draws tool
        void DrawScreen() override;

        // Updates tool
        void Update(float dt) override;

        // Called when tool was enabled
        void OnEnabled() override;

        // Called when tool was disabled
        void OnDisabled() override;

        // Called when objects selection was changed
        void OnObjectsSelectionChanged(const Vector<Ref<SceneEditableObject>>& objects) override;

        // Called when cursor pressed on this
        void OnCursorPressed(const Input::Cursor& cursor) override;

        // Called when cursor released (only when cursor pressed this at previous time)
        void OnCursorReleased(const Input::Cursor& cursor) override;

        // Called when cursor pressing was broken (when scrolled scroll area or some other)
        void OnCursorPressBreak(const Input::Cursor& cursor) override;

        // Called when cursor stay down during frame
        void OnCursorStillDown(const Input::Cursor& cursor) override;

        // Called when cursor moved on this (or moved outside when this was pressed)
        void OnCursorMoved(const Input::Cursor& cursor) override;

        // Called when key was pressed
        void OnKeyPressed(const Input::Key& key) override;
    };

}
// --- META ---

CLASS_BASES_META(Editor::SelectionTool)
{
    BASE_CLASS(Editor::IEditTool);
}
END_META;
CLASS_FIELDS_META(Editor::SelectionTool)
{
    FIELD().PROTECTED().NAME(mSelectionSprite);
    FIELD().PROTECTED().NAME(mCurrentSelectingObjects);
    FIELD().PROTECTED().NAME(mBeforeSelectingObjects);
    FIELD().PROTECTED().NAME(mPressPoint);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mSelectingByFrame);
}
END_META;
CLASS_METHODS_META(Editor::SelectionTool)
{

    FUNCTION().PUBLIC().CONSTRUCTOR();
    FUNCTION().PROTECTED().SIGNATURE(String, GetPanelIcon);
    FUNCTION().PROTECTED().SIGNATURE(ShortcutKeys, GetShortcut);
    FUNCTION().PROTECTED().SIGNATURE(void, DrawScene);
    FUNCTION().PROTECTED().SIGNATURE(void, DrawScreen);
    FUNCTION().PROTECTED().SIGNATURE(void, Update, float);
    FUNCTION().PROTECTED().SIGNATURE(void, OnEnabled);
    FUNCTION().PROTECTED().SIGNATURE(void, OnDisabled);
    FUNCTION().PROTECTED().SIGNATURE(void, OnObjectsSelectionChanged, const Vector<Ref<SceneEditableObject>>&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCursorPressed, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCursorReleased, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCursorPressBreak, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCursorStillDown, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCursorMoved, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnKeyPressed, const Input::Key&);
}
END_META;
// --- END META ---
