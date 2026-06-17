#include "o2Editor/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Scene/UI/Widget.h"
#include "o2/Scene/UI/WidgetLayer.h"
#include "o2/Scene/UI/WidgetLayout.h"
#include "o2/Utils/Math/Layout.h"
#include "support/EditorTestScene.h"

using namespace o2;
using namespace Editor;
using namespace Editor::Tests;

// WidgetLayer::OnChanged recomputes the layer layout so a deserialized edit (PropertyChangeAction
// Undo/Redo, which skips the setters) refreshes without a tick. SetLayout reproduces that write path.

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
}

TEST(WidgetLayerOnChangedRefresh, OnChangedRecomputesLayoutWithoutTick)
{
    SceneCleanGuard guard;
    Ref<Widget> w;
    auto layer = MakeFilledLayerWidget(Vec2F(100.0f, 100.0f), w);
    ASSERT_TRUE(NearV(layer->layout.size, Vec2F(100.0f, 100.0f)));

    // SetLayout writes the fields without recomputing — like PropertyChangeAction's deserialization.
    layer->SetLayout(Layout(Vec2F(0.0f, 0.0f), Vec2F(1.0f, 1.0f), Vec2F(20.0f, 30.0f), Vec2F(-20.0f, -30.0f)));
    layer->OnChanged();

    Vec2F size = layer->layout.size;
    EXPECT_TRUE(NearV(size, Vec2F(60.0f, 40.0f)))
        << "OnChanged must refresh the layer's resolved size without a scene tick. Actual = "
        << size.x << "," << size.y;
}

TEST(WidgetLayerOnChangedRefresh, OnChangedRestoreRecomputesLayoutWithoutTick)
{
    SceneCleanGuard guard;
    Ref<Widget> w;
    auto layer = MakeFilledLayerWidget(Vec2F(100.0f, 100.0f), w);

    layer->SetLayout(Layout(Vec2F(0.0f, 0.0f), Vec2F(1.0f, 1.0f), Vec2F(20.0f, 30.0f), Vec2F(-20.0f, -30.0f)));
    layer->OnChanged();
    ASSERT_TRUE(NearV(layer->layout.size, Vec2F(60.0f, 40.0f)));

    layer->SetLayout(Layout(Vec2F(0.0f, 0.0f), Vec2F(1.0f, 1.0f), Vec2F(0.0f, 0.0f), Vec2F(0.0f, 0.0f)));
    layer->OnChanged();

    Vec2F size = layer->layout.size;
    EXPECT_TRUE(NearV(size, Vec2F(100.0f, 100.0f)))
        << "OnChanged must refresh the restored size without a scene tick. Actual = "
        << size.x << "," << size.y;
}
