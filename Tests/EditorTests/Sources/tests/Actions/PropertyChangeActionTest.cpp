#include "o2/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Scene/Scene.h"
#include "o2Editor/Actions/PropertyChange.h"
#include "support/EditorTestScene.h"

using namespace o2;
using namespace Editor;
using namespace Editor::Tests;

namespace
{
    Ref<PropertyChangeAction> MakeRenameAction(const Ref<Actor>& a, const String& before, const String& after)
    {
        DataDocument beforeData; beforeData = before;
        DataDocument afterData;  afterData  = after;
        return mmake<PropertyChangeAction>(AsEditable({ a }), "name",
                                           Vector<DataDocument>{ beforeData },
                                           Vector<DataDocument>{ afterData });
    }
}

TEST(PropertyChangeAction, CtorCapturesObjectIdsAndPath)
{
    SceneCleanGuard guard;
    auto a = MakeActor();
    auto b = MakeActor();

    DataDocument before; before = String("old");
    DataDocument after;  after  = String("new");

    auto action = mmake<PropertyChangeAction>(AsEditable({ a, b }), "name",
                                              Vector<DataDocument>{ before, before },
                                              Vector<DataDocument>{ after, after });

    ASSERT_EQ(action->objectsIds.Count(), 2);
    EXPECT_EQ(action->objectsIds[0], a->GetID());
    EXPECT_EQ(action->objectsIds[1], b->GetID());
    EXPECT_EQ(action->propertyPath, String("name"));
    EXPECT_EQ(action->beforeValues.Count(), 2);
    EXPECT_EQ(action->afterValues.Count(), 2);
}

TEST(PropertyChangeAction, RedoSetsAfterValue_UndoRestoresBeforeValue)
{
    SceneCleanGuard guard;
    auto a = MakeActor();
    TickScene();
    a->SetName("old");

    auto action = MakeRenameAction(a, "old", "new");

    a->SetName("new");

    action->Undo();
    EXPECT_EQ(a->GetName(), String("old"));

    action->Redo();
    EXPECT_EQ(a->GetName(), String("new"));
}

TEST(PropertyChangeAction, RoundTripsAcrossMultipleObjects)
{
    SceneCleanGuard guard;
    auto a = MakeActor();
    auto b = MakeActor();
    TickScene();
    a->SetName("a-old");
    b->SetName("b-old");

    DataDocument aBefore; aBefore = String("a-old");
    DataDocument aAfter;  aAfter  = String("a-new");
    DataDocument bBefore; bBefore = String("b-old");
    DataDocument bAfter;  bAfter  = String("b-new");

    auto action = mmake<PropertyChangeAction>(AsEditable({ a, b }), "name",
                                              Vector<DataDocument>{ aBefore, bBefore },
                                              Vector<DataDocument>{ aAfter,  bAfter });

    a->SetName("a-new");
    b->SetName("b-new");

    action->Undo();
    EXPECT_EQ(a->GetName(), String("a-old"));
    EXPECT_EQ(b->GetName(), String("b-old"));

    action->Redo();
    EXPECT_EQ(a->GetName(), String("a-new"));
    EXPECT_EQ(b->GetName(), String("b-new"));
}

TEST(PropertyChangeAction, MissingObjectIsTolerated)
{
    SceneCleanGuard guard;
    auto a = MakeActor();
    TickScene();

    DataDocument before; before = String("old");
    DataDocument after;  after  = String("new");

    auto action = mmake<PropertyChangeAction>(AsEditable({ a }), "name",
                                              Vector<DataDocument>{ before },
                                              Vector<DataDocument>{ after });

    o2Scene.DestroyActor(a);
    a = nullptr;
    o2Scene.UpdateDestroyingEntities();

    action->Redo();
    action->Undo();
}

TEST(PropertyChangeAction, GetNameIsStable)
{
    auto action = mmake<PropertyChangeAction>();
    EXPECT_EQ(action->GetName(), String("Property changed"));
}
