#include "o2Editor/stdafx.h"
#include "ActionsList.h"

#include "o2Editor/Actions/PropertyChange.h"
#include "o2Editor/Windows/SceneWindow/SceneEditScreen.h"

namespace Editor
{

	ActionsList::ActionsList():
        ActionsList(nullptr)
	{}

    ActionsList::ActionsList(RefCounter* refCounter) :
        RefCounterable(refCounter)
    {}

	ActionsList::~ActionsList()
    {}

    int ActionsList::GetUndoActionsCount() const
    {
        return mActions.Count();
    }

    int ActionsList::GetRedoActionsCount() const
    {
        return mForwardActions.Count();
    }

    String ActionsList::GetLastActionName() const
    {
        if (mActions.Count() > 0)
            return mActions.Last()->GetName();

        return "";
    }

    String ActionsList::GetNextForwardActionName() const
    {
        if (mForwardActions.Count() > 0)
            return mForwardActions.Last()->GetName();

        return "";
    }

    void ActionsList::UndoAction()
    {
        if (mActions.Count() > 0)
        {
            auto last = mActions.Last();
            last->Undo();
            mForwardActions.Add(mActions.PopBack());
            OnActionUndo(last);
        }
    }

    void ActionsList::RedoAction()
    {
        if (mForwardActions.Count() > 0)
        {
            auto last = mForwardActions.Last();
            last->Redo();
            mActions.Add(mForwardActions.PopBack());
            OnActionDone(last);
        }
    }

    void ActionsList::DoneAction(const Ref<IAction>& action)
    {
        mActions.Add(action);

        mForwardActions.Clear();

        OnActionDone(action);
    }

    void ActionsList::DoneActorPropertyChangeAction(const String& path, const Vector<DataDocument>& prevValue,
                                                    const Vector<DataDocument>& newValue)
    {
        auto action = mmake<PropertyChangeAction>(o2EditorSceneScreen.GetSelectedObjects(), path, prevValue, newValue);

        DoneAction(action);
    }

    void ActionsList::ResetUndoActions()
    {
        mActions.Clear();
        mForwardActions.Clear();
    }

    const Vector<Ref<IAction>> ActionsList::GetUndoActions() const
    {
        return mActions;
    }

    const Vector<Ref<IAction>> ActionsList::GetRedoActions() const
    {
        return mForwardActions;
    }

}
