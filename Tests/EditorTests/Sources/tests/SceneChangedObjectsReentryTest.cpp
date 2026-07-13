#include "o2/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Scene/Scene.h"
#include "support/EditorTestScene.h"

using namespace o2;
using namespace Editor::Tests;

// Regression: a changed widget's Update (SceneEditScreen) calls CheckChangedObjects again from
// inside the drain loop; without the reentrancy guard this recursed until stack overflow
TEST(SceneChangedObjects, CheckChangedObjectsIsReentrancySafe)
{
    SceneCleanGuard guard;
    auto a = MakeActor(Vec2F(0.0f, 0.0f));
    SetActorPos(a, Vec2F(10.0f, 0.0f));

    int calls = 0;
    auto handler = Function<void(const Vector<Ref<SceneEditableObject>>&)>(
        [&](const Vector<Ref<SceneEditableObject>>&)
        {
            calls++;
            if (calls < 5)
                o2Scene.CheckChangedObjects();
        });

    o2Scene.onObjectsChanged += handler;
    o2Scene.CheckChangedObjects();
    o2Scene.onObjectsChanged -= handler;

    EXPECT_EQ(calls, 1) << "reentrant CheckChangedObjects must be a no-op";
}
