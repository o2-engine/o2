#include "o2/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Scene/Scene.h"
#include "o2/Scene/SceneLayer.h"
#include "o2Editor/Actions/LayerVisibility.h"
#include "support/EditorTestScene.h"

using namespace o2;
using namespace Editor;
using namespace Editor::Tests;

TEST(LayerVisibilityAction, CtorCapturesNameAndFlag)
{
    auto action = mmake<LayerVisibilityAction>("Foo", false);
    EXPECT_EQ(action->layerName, String("Foo"));
    EXPECT_FALSE(action->visible);
}

TEST(LayerVisibilityAction, RedoHides_UndoShows)
{
    SceneCleanGuard guard;
    auto layer = o2Scene.AddLayer("Foo");
    layer->SetVisible(true);

    auto action = mmake<LayerVisibilityAction>("Foo", false);

    action->Redo();
    EXPECT_FALSE(layer->IsVisible());

    action->Undo();
    EXPECT_TRUE(layer->IsVisible());
}

TEST(LayerVisibilityAction, RedoShows_UndoHides)
{
    SceneCleanGuard guard;
    auto layer = o2Scene.AddLayer("Foo");
    layer->SetVisible(false);

    auto action = mmake<LayerVisibilityAction>("Foo", true);

    action->Redo();
    EXPECT_TRUE(layer->IsVisible());

    action->Undo();
    EXPECT_FALSE(layer->IsVisible());
}

TEST(LayerVisibilityAction, ToleratesMissingLayer)
{
    SceneCleanGuard guard;
    auto action = mmake<LayerVisibilityAction>("Ghost", false);

    action->Redo();
    action->Undo();
    SUCCEED();
}

TEST(LayerVisibilityAction, GetNameDependsOnFlag)
{
    auto show = mmake<LayerVisibilityAction>("X", true);
    auto hide = mmake<LayerVisibilityAction>("X", false);
    EXPECT_EQ(show->GetName(), String("Show layer"));
    EXPECT_EQ(hide->GetName(), String("Hide layer"));
}
