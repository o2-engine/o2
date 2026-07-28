#include "o2/stdafx.h"
#include "EditorScope.h"

#include "o2/Events/EventSystem.h"
#include "o2/Utils/Debug/Assert.h"

namespace o2
{
    void EditorScope::Enter(int count /*= 1*/)
    {
        if (count > 0)
        {
            Actor::SetDefaultCreationMode(ActorCreateMode::NotInScene);

            mDepth += count;
        }
    }

    void EditorScope::Exit(int count /*= 1*/)
    {
        if (count > 0)
        {
            mDepth -= count;

            if (mDepth == 0)
                Actor::SetDefaultCreationMode(ActorCreateMode::InScene);
            else
                Actor::SetDefaultCreationMode(ActorCreateMode::NotInScene);

            Assert(mDepth >= 0, "Editor scope Enter/Exit mismatch");
        }
    }

    int EditorScope::mDepth = 0;

    PushEditorScopeOnStack::PushEditorScopeOnStack(int count /*= 1*/):
        mDepth(count)
    {
        EditorScope::Enter(count);
    }

    PushEditorScopeOnStack::~PushEditorScopeOnStack()
    {
        EditorScope::Exit(mDepth);
    }

    ForcePopEditorScopeOnStack::ForcePopEditorScopeOnStack()
    {
        mDepth = EditorScope::GetDepth();
        EditorScope::Exit(mDepth);
    }

    ForcePopEditorScopeOnStack::~ForcePopEditorScopeOnStack()
    {
        EditorScope::Enter(mDepth);
    }

}
