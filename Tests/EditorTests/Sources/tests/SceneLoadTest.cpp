#include "o2/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Scene/Actor.h"
#include "o2/Scene/Scene.h"
#include "o2/Utils/Serialization/DataValue.h"
#include "support/EditorTestScene.h"

using namespace o2;
using namespace Editor::Tests;

TEST(SceneLoad, RoundTripPreservesActorCount)
{
    SceneCleanGuard guard;

    auto a = MakeActor(Vec2F(10.0f, 0.0f));
    auto b = MakeActor(Vec2F(0.0f, 20.0f));
    auto c = MakeActor(Vec2F(30.0f, 30.0f));
    TickScene();

    DataDocument doc;
    o2Scene.Save(doc);

    o2Scene.Clear(true);
    o2Scene.UpdateDestroyingEntities();

    o2Scene.Load(doc);
    TickScene();

    int liveCount = 0;
    for (auto& [ptr, weak] : o2Scene.GetAllActors())
        if (weak.Lock()) ++liveCount;

    EXPECT_GE(liveCount, 3);
}

TEST(SceneLoad, RoundTripPreservesActorPositions)
{
    SceneCleanGuard guard;

    auto a = MakeActor(Vec2F(10.0f, 0.0f));
    auto b = MakeActor(Vec2F(0.0f, 20.0f));
    TickScene();

    DataDocument doc;
    o2Scene.Save(doc);

    o2Scene.Clear(true);
    o2Scene.UpdateDestroyingEntities();

    o2Scene.Load(doc);
    TickScene();

    Vector<Vec2F> restoredPositions;
    for (auto& [ptr, weak] : o2Scene.GetAllActors())
    {
        if (auto live = weak.Lock())
            restoredPositions.Add(live->transform->GetPosition2D());
    }

    bool foundA = false, foundB = false;
    for (auto& p : restoredPositions)
    {
        if (NearV(p, Vec2F(10.0f, 0.0f))) foundA = true;
        if (NearV(p, Vec2F(0.0f, 20.0f))) foundB = true;
    }

    EXPECT_TRUE(foundA);
    EXPECT_TRUE(foundB);
}
