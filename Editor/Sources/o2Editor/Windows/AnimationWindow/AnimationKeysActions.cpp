#include "o2Editor/stdafx.h"
#include "AnimationKeysActions.h"

namespace Editor
{
    AnimationAddKeysAction::AnimationAddKeysAction()
    {}

    AnimationAddKeysAction::AnimationAddKeysAction(const Map<String, Vector<UInt64>>& keys, const DataDocument& keysData,
                                                   const Ref<KeyHandlesSheet>& editor):
        mKeys(keys), mKeysData(keysData), mEditor(editor)
    {}

    String AnimationAddKeysAction::GetName() const
    {
        return "Add keys";
    }

    void AnimationAddKeysAction::Redo()
    {
        Map<String, Vector<UInt64>> keys;
        mEditor.Lock()->DeserializeKeys(mKeysData, keys, 0.0f, false);
        mEditor.Lock()->SetSelectedKeys(keys);
    }

    void AnimationAddKeysAction::Undo()
    {
        mEditor.Lock()->DeleteKeys(mKeys, false);
    }

    AnimationDeleteKeysAction::AnimationDeleteKeysAction()
    {}

    AnimationDeleteKeysAction::AnimationDeleteKeysAction(const Map<String, Vector<UInt64>>& keys, const DataDocument& keysData,
                                                         const Ref<KeyHandlesSheet>& editor) :
        mKeys(keys), mKeysData(keysData), mEditor(editor)
    {}

    String AnimationDeleteKeysAction::GetName() const
    {
        return "Delete keys";
    }

    void AnimationDeleteKeysAction::Redo()
    {
        mEditor.Lock()->DeleteKeys(mKeys, false);
    }

    void AnimationDeleteKeysAction::Undo()
    {
        Map<String, Vector<UInt64>> keys;
        mEditor.Lock()->DeserializeKeys(mKeysData, keys, 0.0f, false);
        mEditor.Lock()->SetSelectedKeys(keys);
    }

    AnimationKeysChangeAction::AnimationKeysChangeAction()
    {}

    AnimationKeysChangeAction::AnimationKeysChangeAction(const Map<String, Vector<UInt64>>& keys, const DataDocument& beforeKeysData,
                                                         const DataDocument& afterKeysData, const Ref<KeyHandlesSheet>& editor):
        mKeys(keys), mBeforeKeysData(beforeKeysData), mAfterKeysData(afterKeysData), mEditor(editor)
    {}

    String AnimationKeysChangeAction::GetName() const
    {
        return "Changed keys";
    }

    void AnimationKeysChangeAction::Redo()
    {
        auto editor = mEditor.Lock();

        editor->DeleteKeys(mKeys, false);

        Map<String, Vector<UInt64>> keys;
        editor->DeserializeKeys(mAfterKeysData, keys, 0.0f, false);
        editor->SetSelectedKeys(keys);
    }

    void AnimationKeysChangeAction::Undo()
    {
        auto editor = mEditor.Lock();

        editor->DeleteKeys(mKeys, false);

        Map<String, Vector<UInt64>> keys;
        editor->DeserializeKeys(mBeforeKeysData, keys, 0.0f, false);
        editor->SetSelectedKeys(keys);
    }
}
// --- META ---

DECLARE_CLASS(Editor::AnimationAddKeysAction, Editor__AnimationAddKeysAction);

DECLARE_CLASS(Editor::AnimationDeleteKeysAction, Editor__AnimationDeleteKeysAction);

DECLARE_CLASS(Editor::AnimationKeysChangeAction, Editor__AnimationKeysChangeAction);
// --- END META ---
