#pragma once

#include "o2Editor/Windows/AnimationWindow/KeyHandlesSheet.h"
#include "o2Editor/Actions/IAction.h"

namespace Editor
{
    class AnimationAddKeysAction : public IAction
    {
    public:
        AnimationAddKeysAction();
        AnimationAddKeysAction(const Map<String, Vector<UInt64>>& keys, const DataDocument& keysData, const Ref<KeyHandlesSheet>& editor);

        String GetName() const override;
        void Redo() override;
        void Undo() override;

        SERIALIZABLE(AnimationAddKeysAction);

    protected:
        Map<String, Vector<UInt64>> mKeys;
        DataDocument                mKeysData;
        WeakRef<KeyHandlesSheet>    mEditor;
    };

    class AnimationDeleteKeysAction : public IAction
    {
    public:
        AnimationDeleteKeysAction();
        AnimationDeleteKeysAction(const Map<String, Vector<UInt64>>& keys, const DataDocument& keysData, const Ref<KeyHandlesSheet>& editor);

        String GetName() const override;
        void Redo() override;
        void Undo() override;

        SERIALIZABLE(AnimationDeleteKeysAction);

    protected:
        Map<String, Vector<UInt64>> mKeys;
        DataDocument                mKeysData;
        WeakRef<KeyHandlesSheet>    mEditor;
    };

    class AnimationKeysChangeAction : public IAction
    {
    public:
        AnimationKeysChangeAction();
        AnimationKeysChangeAction(const Map<String, Vector<UInt64>>& keys,
                                  const DataDocument& beforeKeysData, const DataDocument& afterKeysData,
                                  const Ref<KeyHandlesSheet>& editor);

        String GetName() const override;
        void Redo() override;
        void Undo() override;

        SERIALIZABLE(AnimationKeysChangeAction);

    protected:
        Map<String, Vector<UInt64>> mKeys;
        DataDocument                mBeforeKeysData;
        DataDocument                mAfterKeysData;
        WeakRef<KeyHandlesSheet>    mEditor;
    };
}
// --- META ---

CLASS_BASES_META(Editor::AnimationAddKeysAction)
{
    BASE_CLASS(Editor::IAction);
}
END_META;
CLASS_FIELDS_META(Editor::AnimationAddKeysAction)
{
    FIELD().PROTECTED().NAME(mKeys);
    FIELD().PROTECTED().NAME(mKeysData);
    FIELD().PROTECTED().NAME(mEditor);
}
END_META;
CLASS_METHODS_META(Editor::AnimationAddKeysAction)
{

    typedef const Map<String, Vector<UInt64>>& _tmp1;

    FUNCTION().PUBLIC().CONSTRUCTOR();
    FUNCTION().PUBLIC().CONSTRUCTOR(_tmp1, const DataDocument&, const Ref<KeyHandlesSheet>&);
    FUNCTION().PUBLIC().SIGNATURE(String, GetName);
    FUNCTION().PUBLIC().SIGNATURE(void, Redo);
    FUNCTION().PUBLIC().SIGNATURE(void, Undo);
}
END_META;

CLASS_BASES_META(Editor::AnimationDeleteKeysAction)
{
    BASE_CLASS(Editor::IAction);
}
END_META;
CLASS_FIELDS_META(Editor::AnimationDeleteKeysAction)
{
    FIELD().PROTECTED().NAME(mKeys);
    FIELD().PROTECTED().NAME(mKeysData);
    FIELD().PROTECTED().NAME(mEditor);
}
END_META;
CLASS_METHODS_META(Editor::AnimationDeleteKeysAction)
{

    typedef const Map<String, Vector<UInt64>>& _tmp1;

    FUNCTION().PUBLIC().CONSTRUCTOR();
    FUNCTION().PUBLIC().CONSTRUCTOR(_tmp1, const DataDocument&, const Ref<KeyHandlesSheet>&);
    FUNCTION().PUBLIC().SIGNATURE(String, GetName);
    FUNCTION().PUBLIC().SIGNATURE(void, Redo);
    FUNCTION().PUBLIC().SIGNATURE(void, Undo);
}
END_META;

CLASS_BASES_META(Editor::AnimationKeysChangeAction)
{
    BASE_CLASS(Editor::IAction);
}
END_META;
CLASS_FIELDS_META(Editor::AnimationKeysChangeAction)
{
    FIELD().PROTECTED().NAME(mKeys);
    FIELD().PROTECTED().NAME(mBeforeKeysData);
    FIELD().PROTECTED().NAME(mAfterKeysData);
    FIELD().PROTECTED().NAME(mEditor);
}
END_META;
CLASS_METHODS_META(Editor::AnimationKeysChangeAction)
{

    typedef const Map<String, Vector<UInt64>>& _tmp1;

    FUNCTION().PUBLIC().CONSTRUCTOR();
    FUNCTION().PUBLIC().CONSTRUCTOR(_tmp1, const DataDocument&, const DataDocument&, const Ref<KeyHandlesSheet>&);
    FUNCTION().PUBLIC().SIGNATURE(String, GetName);
    FUNCTION().PUBLIC().SIGNATURE(void, Redo);
    FUNCTION().PUBLIC().SIGNATURE(void, Undo);
}
END_META;
// --- END META ---
