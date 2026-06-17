#include "o2Editor/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Scene/Scene.h"
#include "o2/Scene/UI/Widget.h"
#include "o2/Scene/UI/WidgetLayer.h"
#include "o2/Scene/UI/WidgetLayerLayout.h"
#include "o2/Scene/UI/WidgetLayout.h"
#include "o2/Utils/Math/Layout.h"
#include "o2Editor/Actions/PropertyChange.h"
#include "support/EditorTestScene.h"

using namespace o2;
using namespace Editor;
using namespace Editor::Tests;

// The layout viewer builds a PropertyChangeAction from each property field's value path on
// completion. These pin which paths actually resolve on a WidgetLayer so the viewer labels its
// fields with undoable paths (position/size live under "layout", not "transform").

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

    Ref<PropertyChangeAction> MakeVec2Action(const Ref<WidgetLayer>& layer, const String& path,
                                             const Vec2F& before, const Vec2F& after)
    {
        DataDocument db; db = before;
        DataDocument da; da = after;
        Vector<Ref<SceneEditableObject>> objects;
        objects.Add(DynamicCast<SceneEditableObject>(layer));
        return mmake<PropertyChangeAction>(objects, path, Vector<DataDocument>{ db }, Vector<DataDocument>{ da });
    }
}

TEST(WidgetLayerLayoutPath, OffsetMaxPathRoundTrips)
{
    SceneCleanGuard guard;
    Ref<Widget> w;
    auto layer = MakeFilledLayerWidget(Vec2F(100.0f, 100.0f), w);

    auto action = MakeVec2Action(layer, "layout/offsetMax", Vec2F(0.0f, 0.0f), Vec2F(-20.0f, -30.0f));
    action->Redo();
    EXPECT_TRUE(NearV(layer->layout.offsetMax, Vec2F(-20.0f, -30.0f)));
    action->Undo();
    EXPECT_TRUE(NearV(layer->layout.offsetMax, Vec2F(0.0f, 0.0f)));
}

TEST(WidgetLayerLayoutPath, LayoutSizePathRoundTrips)
{
    SceneCleanGuard guard;
    Ref<Widget> w;
    auto layer = MakeFilledLayerWidget(Vec2F(100.0f, 100.0f), w);
    ASSERT_TRUE(NearV(layer->layout.size, Vec2F(100.0f, 100.0f)));

    auto action = MakeVec2Action(layer, "layout/size", Vec2F(100.0f, 100.0f), Vec2F(60.0f, 40.0f));
    action->Redo();
    Vec2F redo = layer->layout.size;
    EXPECT_TRUE(NearV(redo, Vec2F(60.0f, 40.0f)))
        << "'layout/size' must drive the layer size. actual = " << redo.x << "," << redo.y;
}

TEST(WidgetLayerLayoutPath, LayoutPositionPathRoundTrips)
{
    SceneCleanGuard guard;
    Ref<Widget> w;
    auto layer = MakeFilledLayerWidget(Vec2F(100.0f, 100.0f), w);
    Vec2F p0 = layer->layout.position;

    auto action = MakeVec2Action(layer, "layout/position", p0, p0 + Vec2F(15.0f, 25.0f));
    action->Redo();
    Vec2F redo = layer->layout.position;
    EXPECT_TRUE(NearV(redo, p0 + Vec2F(15.0f, 25.0f)))
        << "'layout/position' must drive the layer position. actual = " << redo.x << "," << redo.y;
}

TEST(WidgetLayerLayoutPath, TransformSizePathIsNoOpOnLayer)
{
    SceneCleanGuard guard;
    Ref<Widget> w;
    auto layer = MakeFilledLayerWidget(Vec2F(100.0f, 100.0f), w);

    auto action = MakeVec2Action(layer, "transform/size", Vec2F(100.0f, 100.0f), Vec2F(60.0f, 40.0f));
    action->Redo();
    EXPECT_TRUE(NearV(layer->layout.size, Vec2F(100.0f, 100.0f)))
        << "'transform/size' does not exist on a WidgetLayer — it must not be the field's path.";
}
