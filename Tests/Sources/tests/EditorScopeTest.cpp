#include "o2/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Scene/Actor.h"
#include "o2/Utils/Editor/EditorScope.h"

using namespace o2;

namespace
{
    // Restores both EditorScope depth and Actor's default creation mode after a test.
    class EditorScopeGuard
    {
    public:
        EditorScopeGuard()
        {
            mOriginalMode = Actor::GetDefaultCreationMode();
            mOriginalDepth = EditorScope::GetDepth();
            EditorScope::Exit(mOriginalDepth);
            Actor::SetDefaultCreationMode(ActorCreateMode::InScene);
        }

        ~EditorScopeGuard()
        {
            EditorScope::Exit(EditorScope::GetDepth());
            EditorScope::Enter(mOriginalDepth);
            Actor::SetDefaultCreationMode(mOriginalMode);
        }

    private:
        ActorCreateMode mOriginalMode;
        int             mOriginalDepth;
    };
}

TEST(EditorScope, FreshStateHasZeroDepthAndIsNotInScope)
{
    EditorScopeGuard guard;
    EXPECT_EQ(EditorScope::GetDepth(), 0);
    EXPECT_FALSE(EditorScope::IsInScope());
}

TEST(EditorScope, EnterIncreasesDepthAndExitDecreases)
{
    EditorScopeGuard guard;

    EditorScope::Enter();
    EXPECT_EQ(EditorScope::GetDepth(), 1);
    EXPECT_TRUE(EditorScope::IsInScope());

    EditorScope::Exit();
    EXPECT_EQ(EditorScope::GetDepth(), 0);
    EXPECT_FALSE(EditorScope::IsInScope());
}

TEST(EditorScope, EnterCountAccumulates)
{
    EditorScopeGuard guard;

    EditorScope::Enter(3);
    EXPECT_EQ(EditorScope::GetDepth(), 3);

    EditorScope::Enter(2);
    EXPECT_EQ(EditorScope::GetDepth(), 5);

    EditorScope::Exit(5);
    EXPECT_EQ(EditorScope::GetDepth(), 0);
}

TEST(EditorScope, EnterWithNonPositiveCountIsIgnored)
{
    EditorScopeGuard guard;

    EditorScope::Enter(0);
    EXPECT_EQ(EditorScope::GetDepth(), 0);

    EditorScope::Enter(-3);
    EXPECT_EQ(EditorScope::GetDepth(), 0);
}

TEST(EditorScope, EnterSwitchesActorDefaultCreationModeToNotInScene)
{
    EditorScopeGuard guard;

    EXPECT_EQ(Actor::GetDefaultCreationMode(), ActorCreateMode::InScene);

    EditorScope::Enter();
    EXPECT_EQ(Actor::GetDefaultCreationMode(), ActorCreateMode::NotInScene);

    EditorScope::Exit();
    EXPECT_EQ(Actor::GetDefaultCreationMode(), ActorCreateMode::InScene);
}

TEST(EditorScope, NestedExitKeepsActorModeNotInSceneUntilDepthZero)
{
    EditorScopeGuard guard;

    EditorScope::Enter(2);
    EXPECT_EQ(Actor::GetDefaultCreationMode(), ActorCreateMode::NotInScene);

    EditorScope::Exit();
    EXPECT_EQ(EditorScope::GetDepth(), 1);
    EXPECT_EQ(Actor::GetDefaultCreationMode(), ActorCreateMode::NotInScene);

    EditorScope::Exit();
    EXPECT_EQ(EditorScope::GetDepth(), 0);
    EXPECT_EQ(Actor::GetDefaultCreationMode(), ActorCreateMode::InScene);
}

TEST(EditorScope, PushOnStackEntersAndExitsAutomatically)
{
    EditorScopeGuard guard;
    EXPECT_EQ(EditorScope::GetDepth(), 0);

    {
        PushEditorScopeOnStack push;
        EXPECT_EQ(EditorScope::GetDepth(), 1);
        EXPECT_TRUE(EditorScope::IsInScope());
    }

    EXPECT_EQ(EditorScope::GetDepth(), 0);
}

TEST(EditorScope, PushOnStackHonorsCount)
{
    EditorScopeGuard guard;

    {
        PushEditorScopeOnStack push(4);
        EXPECT_EQ(EditorScope::GetDepth(), 4);
    }

    EXPECT_EQ(EditorScope::GetDepth(), 0);
}

TEST(EditorScope, PushOnStackNestsCleanly)
{
    EditorScopeGuard guard;

    PushEditorScopeOnStack outer;
    EXPECT_EQ(EditorScope::GetDepth(), 1);

    {
        PushEditorScopeOnStack inner(2);
        EXPECT_EQ(EditorScope::GetDepth(), 3);
    }

    EXPECT_EQ(EditorScope::GetDepth(), 1);
}

TEST(EditorScope, ForcePopRestoresDepthAfterScope)
{
    EditorScopeGuard guard;

    EditorScope::Enter(3);
    EXPECT_EQ(EditorScope::GetDepth(), 3);

    {
        ForcePopEditorScopeOnStack pop;
        EXPECT_EQ(EditorScope::GetDepth(), 0);
        EXPECT_FALSE(EditorScope::IsInScope());
    }

    EXPECT_EQ(EditorScope::GetDepth(), 3);
    EXPECT_TRUE(EditorScope::IsInScope());

    EditorScope::Exit(3);
}

TEST(EditorScope, ForcePopOnZeroDepthIsNoOp)
{
    EditorScopeGuard guard;

    {
        ForcePopEditorScopeOnStack pop;
        EXPECT_EQ(EditorScope::GetDepth(), 0);
    }

    EXPECT_EQ(EditorScope::GetDepth(), 0);
}
