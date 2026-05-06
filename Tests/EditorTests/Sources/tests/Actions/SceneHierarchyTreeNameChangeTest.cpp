#include "o2/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Scene/Scene.h"
#include "o2Editor/Actions/PropertyChange.h"
#include "support/EditorTestScene.h"

using namespace o2;
using namespace Editor;
using namespace Editor::Tests;

TEST(SceneHierarchyTreeRename, RedoSetsNewName_UndoRestoresOld_WithoutExternalSetName)
{
    SceneCleanGuard guard;
    auto a = MakeActor();
    a->SetName("before");
    TickScene();

    DataDocument prevData; prevData = String("before");
    DataDocument newData;  newData  = String("after");

    auto action = mmake<PropertyChangeAction>(AsEditable({ a }), "name",
                                              Vector<DataDocument>{ prevData },
                                              Vector<DataDocument>{ newData });
    action->Redo();
    EXPECT_EQ(a->GetName(), String("after"));

    action->Undo();
    EXPECT_EQ(a->GetName(), String("before"));

    action->Redo();
    EXPECT_EQ(a->GetName(), String("after"));
}

TEST(SceneHierarchyTreeRename, MultipleObjectsAppliedPerObject_WithoutExternalSetName)
{
    SceneCleanGuard guard;
    auto a = MakeActor();
    auto b = MakeActor();
    a->SetName("a-before");
    b->SetName("b-before");
    TickScene();

    DataDocument aBefore; aBefore = String("a-before");
    DataDocument aAfter;  aAfter  = String("a-after");
    DataDocument bBefore; bBefore = String("b-before");
    DataDocument bAfter;  bAfter  = String("b-after");

    auto action = mmake<PropertyChangeAction>(AsEditable({ a, b }), "name",
                                              Vector<DataDocument>{ aBefore, bBefore },
                                              Vector<DataDocument>{ aAfter,  bAfter });
    action->Redo();
    EXPECT_EQ(a->GetName(), String("a-after"));
    EXPECT_EQ(b->GetName(), String("b-after"));

    action->Undo();
    EXPECT_EQ(a->GetName(), String("a-before"));
    EXPECT_EQ(b->GetName(), String("b-before"));
}
