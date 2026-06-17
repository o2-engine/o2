#include "o2Editor/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Scene/UI/Widget.h"
#include "o2/Scene/UI/WidgetLayer.h"
#include "o2/Scene/UI/WidgetLayerLayout.h"
#include "o2/Scene/UI/WidgetLayout.h"
#include "o2/Utils/Math/Layout.h"
#include "o2Editor/Actions/WidgetLayerLayout.h"
#include "support/EditorTestScene.h"

using namespace o2;
using namespace Editor;
using namespace Editor::Tests;

// WidgetLayerLayoutAction backs FitLayerByDrawable: round-trips a layer's size via Redo/Undo.

namespace
{
    Ref<WidgetLayer> MakeFilledLayerWidget(const Vec2F& widgetSize, Ref<Widget>& outWidget)
    {
        auto w = mmake<Widget>(ActorCreateMode::InScene);
        w->layout->anchorMin = Vec2F(0.0f, 0.0f);
        w->layout->anchorMax = Vec2F(0.0f, 0.0f);
        w->layout->offsetMin = Vec2F(0.0f, 0.0f);
        w->layout->offsetMax = widgetSize;

        // Add the layer before the first tick — Widget::OnAddToScene registers layers, OnLayerAdded does not.
        auto layer = w->AddLayer("layer", nullptr,
                                 Layout(Vec2F(0.0f, 0.0f), Vec2F(1.0f, 1.0f), Vec2F(), Vec2F()));
        TickScene();

        outWidget = w;
        return layer;
    }

    Ref<WidgetLayerLayoutAction> MakeFitAction(const Ref<WidgetLayer>& layer, const Vec2F& target)
    {
        Layout done = layer->GetLayout();
        Vec2F delta = target - layer->layout.size;
        done.offsetMin -= delta*0.5f;
        done.offsetMax += delta*0.5f;

        Vector<Ref<SceneEditableObject>> objects;
        objects.Add(DynamicCast<SceneEditableObject>(layer));
        Vector<Layout> doneLayouts;
        doneLayouts.Add(done);
        return mmake<WidgetLayerLayoutAction>(objects, doneLayouts);
    }
}

TEST(FitLayerByDrawableAction, RedoResizesLayerToTarget)
{
    SceneCleanGuard guard;
    Ref<Widget> w;
    auto layer = MakeFilledLayerWidget(Vec2F(100.0f, 100.0f), w);

    ASSERT_TRUE(NearV(layer->layout.size, Vec2F(100.0f, 100.0f)))
        << "Filled layer should start at the widget size.";

    auto action = MakeFitAction(layer, Vec2F(60.0f, 40.0f));
    action->Redo();
    TickScene();

    EXPECT_TRUE(NearV(layer->layout.size, Vec2F(60.0f, 40.0f)))
        << "Redo (the fit's mutation path) must resize the layer to the target.";
}

TEST(FitLayerByDrawableAction, UndoRestoresOriginalSize)
{
    SceneCleanGuard guard;
    Ref<Widget> w;
    auto layer = MakeFilledLayerWidget(Vec2F(100.0f, 100.0f), w);

    auto action = MakeFitAction(layer, Vec2F(60.0f, 40.0f));
    action->Redo();
    TickScene();
    ASSERT_TRUE(NearV(layer->layout.size, Vec2F(60.0f, 40.0f)));

    action->Undo();
    TickScene();
    EXPECT_TRUE(NearV(layer->layout.size, Vec2F(100.0f, 100.0f)))
        << "Undo must restore the pre-fit size.";
}

TEST(FitLayerByDrawableAction, UndoRedoCycleReplaysFit)
{
    SceneCleanGuard guard;
    Ref<Widget> w;
    auto layer = MakeFilledLayerWidget(Vec2F(100.0f, 100.0f), w);

    auto action = MakeFitAction(layer, Vec2F(60.0f, 40.0f));
    action->Redo();
    TickScene();

    action->Undo();
    TickScene();
    ASSERT_TRUE(NearV(layer->layout.size, Vec2F(100.0f, 100.0f)));

    action->Redo();
    TickScene();
    EXPECT_TRUE(NearV(layer->layout.size, Vec2F(60.0f, 40.0f)))
        << "Second Redo must replay the fit.";
}

TEST(FitLayerByDrawableAction, CtorCapturesBeforeAndDone)
{
    SceneCleanGuard guard;
    Ref<Widget> w;
    auto layer = MakeFilledLayerWidget(Vec2F(100.0f, 100.0f), w);

    auto action = MakeFitAction(layer, Vec2F(60.0f, 40.0f));

    ASSERT_EQ(action->objectsIds.Count(), 1);
    EXPECT_EQ(action->objectsIds[0], layer->GetID());
    ASSERT_EQ(action->beforeLayouts.Count(), 1);
    ASSERT_EQ(action->doneLayouts.Count(), 1);
    EXPECT_TRUE(NearV(action->beforeLayouts[0].offsetMin, Vec2F(0.0f, 0.0f)));
    EXPECT_TRUE(NearV(action->doneLayouts[0].offsetMin, Vec2F(20.0f, 30.0f)))
        << "Done layout must inset offsets symmetrically for the 60x40 target.";
}
