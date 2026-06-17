#include "o2Editor/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Scene/Scene.h"
#include "o2/Scene/UI/Widget.h"
#include "o2/Scene/UI/WidgetLayer.h"
#include "o2/Scene/UI/WidgetLayout.h"
#include "o2/Utils/Math/Layout.h"
#include "o2Editor/Actions/PropertyChange.h"
#include "support/EditorTestScene.h"

using namespace o2;
using namespace Editor;
using namespace Editor::Tests;

// PropertyChangeAction works over a WidgetLayer (head viewer routes name/enabled/locked through it).

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

    Vector<Ref<SceneEditableObject>> AsEditableLayer(const Ref<WidgetLayer>& layer)
    {
        Vector<Ref<SceneEditableObject>> v;
        v.Add(DynamicCast<SceneEditableObject>(layer));
        return v;
    }
}

TEST(WidgetLayerPropertyChangeAction, NameRoundTripsOnLayer)
{
    SceneCleanGuard guard;
    Ref<Widget> w;
    auto layer = MakeFilledLayerWidget(Vec2F(100.0f, 100.0f), w);
    layer->name = "old";

    DataDocument before; before = String("old");
    DataDocument after;  after  = String("new");

    auto action = mmake<PropertyChangeAction>(AsEditableLayer(layer), "name",
                                              Vector<DataDocument>{ before },
                                              Vector<DataDocument>{ after });

    layer->name = "new";

    action->Undo();
    EXPECT_EQ(layer->name, String("old")) << "Undo must restore the layer name.";

    action->Redo();
    EXPECT_EQ(layer->name, String("new")) << "Redo must reapply the layer name.";
}
