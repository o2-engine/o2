#include "o2/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Render/Camera.h"
#include "o2/Render/Render.h"
#include "o2/Render/Sprite.h"
#include "o2/Render/Text/Text.h"
#include "o2/Render/Text/VectorFont.h"
#include "o2/Assets/Assets.h"
#include "o2/Scene/UI/Widget.h"
#include "o2/Scene/UI/WidgetLayer.h"
#include "o2/Scene/UI/WidgetLayout.h"
#include "o2/Scene/UI/Widgets/ScrollArea.h"
#include "Scene/SceneTestHelpers.h"
#include "Scene/UI/UITestHelpers.h"

using namespace o2;

namespace
{
    // Root 200x200 at the origin with a 100x50 child centered in it, child transforms around its center
    struct RotatedRig
    {
        Ref<Widget> root;
        Ref<Widget> child;
        Ref<WidgetLayer> back;
        Ref<WidgetLayer> top;
    };

    RotatedRig MakeRig()
    {
        RotatedRig rig;
        rig.root = MakeWidget("root");
        *rig.root->layout = WidgetLayout::Based(BaseCorner::Center, Vec2F(200, 200));

        rig.child = MakeChildWidget(rig.root, "child");
        *rig.child->layout = WidgetLayout::Based(BaseCorner::Center, Vec2F(100, 50));
        rig.child->layout->SetPivot(Vec2F(0.5f, 0.5f)); // rotation and scale around the widget center

        rig.back = rig.child->AddLayer("back", mmake<Sprite>(Color4::White()), Layout::BothStretch());
        rig.top = rig.child->AddLayer("top", mmake<Sprite>(Color4::Red()), Layout::HorStretch(VerAlign::Top, 0, 0, 10));

        TickAndUpdateLayout(2);
        return rig;
    }

    // A sprite with its mesh open to the checks
    struct SpriteProbe: public Sprite
    {
        const Mesh& GetMeshForTest() const { return mMesh; }
    };

    void ExpectNear(const Vec2F& a, const Vec2F& b, float eps, const char* what)
    {
        EXPECT_NEAR(a.x, b.x, eps) << what;
        EXPECT_NEAR(a.y, b.y, eps) << what;
    }
}

// Without rotation or scale the layout is plain and layers keep their exact layout rects
TEST(WidgetTransform, PlainWidgetLayersUseLayoutRects)
{
    SceneCleanGuard guard;
    auto rig = MakeRig();

    EXPECT_TRUE(rig.child->layout->IsWorldTransformPlain());
    EXPECT_EQ(rig.back->GetDrawable()->GetRect(), rig.back->GetRect());
    EXPECT_EQ(rig.top->GetDrawable()->GetRect(), rig.top->GetRect());
}

// Rotating a widget rotates its layers around the widget pivot; the layout rect stays axis-aligned
TEST(WidgetTransform, RotationTurnsLayers)
{
    SceneCleanGuard guard;
    auto rig = MakeRig();

    rig.child->layout->SetAngleDegrees(90.0f);
    TickAndUpdateLayout(2);

    EXPECT_FALSE(rig.child->layout->IsWorldTransformPlain());

    // the layout rect is untouched by the rotation
    RectF layoutRect = rig.child->layout->GetWorldRect();
    EXPECT_NEAR(layoutRect.Width(), 100.0f, 0.01f);
    EXPECT_NEAR(layoutRect.Height(), 50.0f, 0.01f);

    // the stretched layer follows the widget world basis: x axis now points up
    Basis backBasis = rig.back->GetDrawable()->GetBasis();
    ExpectNear(backBasis.xv, Vec2F(0, 100), 0.1f, "back xv");
    ExpectNear(backBasis.yv, Vec2F(-50, 0), 0.1f, "back yv");
    ExpectNear(backBasis.origin, Vec2F(25, -50), 0.1f, "back origin");

    // the 10px top strip sits at the rotated top edge and keeps its thickness along the rotated y
    Basis topBasis = rig.top->GetDrawable()->GetBasis();
    ExpectNear(topBasis.xv, Vec2F(0, 100), 0.1f, "top xv");
    ExpectNear(topBasis.yv, Vec2F(-10, 0), 0.1f, "top yv");
    ExpectNear(topBasis.origin, Vec2F(-15, -50), 0.1f, "top origin");
}

// Scaling a widget scales what is drawn, not the layout rect it occupies
TEST(WidgetTransform, ScaleGrowsLayersNotLayout)
{
    SceneCleanGuard guard;
    auto rig = MakeRig();

    rig.child->layout->SetScale(Vec3F(2.0f, 2.0f, 1.0f));
    TickAndUpdateLayout(2);

    RectF layoutRect = rig.child->layout->GetWorldRect();
    EXPECT_NEAR(layoutRect.Width(), 100.0f, 0.01f);
    EXPECT_NEAR(layoutRect.Height(), 50.0f, 0.01f);

    Basis backBasis = rig.back->GetDrawable()->GetBasis();
    ExpectNear(backBasis.xv, Vec2F(200, 0), 0.1f, "back xv");
    ExpectNear(backBasis.yv, Vec2F(0, 100), 0.1f, "back yv");
    ExpectNear(backBasis.origin, Vec2F(-100, -50), 0.1f, "back origin");
}

// Children inherit the parent rotation and scale through the world basis
TEST(WidgetTransform, ChildFollowsParentRotationAndScale)
{
    SceneCleanGuard guard;
    auto rig = MakeRig();

    auto grandChild = MakeChildWidget(rig.child, "grand");
    *grandChild->layout = WidgetLayout::BothStretch();
    auto grandLayer = grandChild->AddLayer("back", mmake<Sprite>(Color4::Blue()), Layout::BothStretch());

    rig.child->layout->SetAngleDegrees(90.0f);
    rig.child->layout->SetScale(Vec3F(2.0f, 2.0f, 1.0f));
    TickAndUpdateLayout(2);

    // grand child fills the child, so its world basis is the child's rotated and scaled basis
    Basis childBasis = rig.child->layout->GetWorldBasis();
    Basis grandBasis = grandChild->layout->GetWorldBasis();
    ExpectNear(grandBasis.xv, childBasis.xv, 0.1f, "grand xv");
    ExpectNear(grandBasis.yv, childBasis.yv, 0.1f, "grand yv");
    ExpectNear(grandBasis.origin, childBasis.origin, 0.1f, "grand origin");

    Basis grandLayerBasis = grandLayer->GetDrawable()->GetBasis();
    ExpectNear(grandLayerBasis.xv, childBasis.xv, 0.1f, "grand layer xv");
    ExpectNear(grandLayerBasis.origin, childBasis.origin, 0.1f, "grand layer origin");
}

// Hit testing follows the rotated shape: layers map world points back into the layout space
TEST(WidgetTransform, RotatedWidgetHitTestFollowsShape)
{
    SceneCleanGuard guard;
    auto rig = MakeRig();

    rig.child->layout->SetAngleDegrees(90.0f);
    TickAndUpdateLayout(2);

    // rotated 100x50 spans x in [-25, 25], y in [-50, 50]
    EXPECT_TRUE(rig.child->layout->IsPointInside(Vec2F(0, 45)));
    EXPECT_FALSE(rig.child->layout->IsPointInside(Vec2F(45, 0)));

    EXPECT_TRUE(rig.back->IsUnderPoint(Vec2F(0, 45)));
    EXPECT_FALSE(rig.back->IsUnderPoint(Vec2F(45, 0)));

    // top strip is the rotated top edge: x in [-25, -15]
    EXPECT_TRUE(rig.top->IsUnderPoint(Vec2F(-20, 0)));
    EXPECT_FALSE(rig.top->IsUnderPoint(Vec2F(20, 0)));
}

// Scene widgets round their layouts to whole units unless the rounding is switched off (editor edit mode)
TEST(WidgetTransform, SceneLayoutsRoundingSwitch)
{
    SceneCleanGuard guard;
    auto root = MakeWidget("root");
    *root->layout = WidgetLayout::Based(BaseCorner::Center, Vec2F(200, 200));
    auto child = MakeChildWidget(root, "child");
    *child->layout = WidgetLayout::Based(BaseCorner::LeftBottom, Vec2F(10.4f, 10.0f), Vec2F(3.3f, 0.0f));

    EXPECT_TRUE(WidgetLayout::IsSceneLayoutsRounding());
    TickAndUpdateLayout(2);
    EXPECT_NEAR(child->layout->GetWorldRect().Width(), 10.0f, 0.001f);
    EXPECT_NEAR(child->layout->GetWorldRect().left, -97.0f, 0.001f);

    WidgetLayout::SetSceneLayoutsRounding(false);
    child->layout->SetDirty();
    TickAndUpdateLayout(2);
    EXPECT_NEAR(child->layout->GetWorldRect().Width(), 10.4f, 0.001f);
    EXPECT_NEAR(child->layout->GetWorldRect().left, -96.7f, 0.001f);

    WidgetLayout::SetSceneLayoutsRounding(true);
    child->layout->SetDirty();
    TickAndUpdateLayout(2);
    EXPECT_NEAR(child->layout->GetWorldRect().Width(), 10.0f, 0.001f);
}

// Turning the rotation back to zero returns layer drawables to plain rects: no stale angle stays behind
TEST(WidgetTransform, RotationBackToZeroResetsLayers)
{
    SceneCleanGuard guard;
    auto rig = MakeRig();

    rig.child->layout->SetAngleDegrees(90.0f);
    TickAndUpdateLayout(2);
    EXPECT_FALSE(rig.child->layout->IsWorldTransformPlain());

    rig.child->layout->SetAngleDegrees(0.0f);
    TickAndUpdateLayout(2);
    EXPECT_TRUE(rig.child->layout->IsWorldTransformPlain());

    EXPECT_NEAR(rig.back->GetDrawable()->GetAngle(), 0.0f, 0.0001f);
    Basis backBasis = rig.back->GetDrawable()->GetBasis();
    ExpectNear(backBasis.xv, Vec2F(100, 0), 0.1f, "back xv");
    ExpectNear(backBasis.yv, Vec2F(0, 50), 0.1f, "back yv");
    ExpectNear(backBasis.origin, Vec2F(-50, -25), 0.1f, "back origin");
}

// Hit testing follows what is drawn: a child under a scaled parent covers its scaled area
TEST(WidgetTransform, ScaledParentHitTestCoversDrawnArea)
{
    SceneCleanGuard guard;
    auto rig = MakeRig();

    rig.root->layout->SetPivot(Vec2F(0.5f, 0.5f));
    rig.root->layout->SetScale(Vec3F(2.0f, 2.0f, 1.0f));
    TickAndUpdateLayout(2);

    // the child's 100x50 layout rect is drawn as 200x100 around the origin
    EXPECT_TRUE(rig.child->layout->IsPointInside(Vec2F(90.0f, 0.0f)));
    EXPECT_TRUE(rig.child->layout->IsPointInside(Vec2F(0.0f, 45.0f)));
    EXPECT_FALSE(rig.child->layout->IsPointInside(Vec2F(110.0f, 0.0f)));
    EXPECT_FALSE(rig.child->layout->IsPointInside(Vec2F(0.0f, 55.0f)));

    // a collapsed axis maps nothing back and must not produce NaN hits
    rig.root->layout->SetScale(Vec3F(0.0f, 2.0f, 1.0f));
    TickAndUpdateLayout(2);
    EXPECT_FALSE(rig.child->layout->IsPointInside(Vec2F(0.0f, 0.0f)));
}

// A scroll area under a scaled parent clips where it is drawn, not to its unscaled layout rect
TEST(WidgetTransform, ScaledParentScrollAreaClipsItsDrawnArea)
{
    SceneCleanGuard guard;
    auto root = MakeWidget("root");
    *root->layout = WidgetLayout::Based(BaseCorner::Center, Vec2F(200, 200));
    root->layout->SetPivot(Vec2F(0.5f, 0.5f));
    root->layout->SetScale(Vec3F(2.0f, 2.0f, 1.0f));

    auto area = mmake<ScrollArea>();
    area->name = "area";
    root->AddChild(area);
    *area->layout = WidgetLayout::Based(BaseCorner::Center, Vec2F(100, 50));
    TickAndUpdateLayout(2);

    RectF world = area->layout->LayoutToWorldRect(area->layout->GetWorldRect());
    ExpectNear(world.LeftBottom(), Vec2F(-100, -50), 0.1f, "left bottom");
    ExpectNear(world.RightTop(), Vec2F(100, 50), 0.1f, "right top");

    o2Render.Begin();
    o2Render.SetCamera(Camera());
    root->Draw();
    auto infos = o2Render.GetScissorInfos();
    o2Render.End();

    ASSERT_FALSE(infos.IsEmpty());
    RectI clip = infos[0].scissorRect;
    EXPECT_EQ(clip.Width(), 200);
    EXPECT_EQ(clip.Height(), 100);

    // a turned widget clips its bounding box
    root->layout->SetScale(Vec3F(1.0f, 1.0f, 1.0f));
    area->layout->SetPivot(Vec2F(0.5f, 0.5f));
    area->layout->SetAngleDegrees(90.0f);
    TickAndUpdateLayout(2);
    world = area->layout->LayoutToWorldRect(area->layout->GetWorldRect());
    EXPECT_NEAR(world.Width(), 50.0f, 0.1f);
    EXPECT_NEAR(world.Height(), 100.0f, 0.1f);
}

// Under a scaled parent a child scrolled into a scroll area's view is laid out there and takes clicks
TEST(WidgetTransform, ScaledParentScrollAreaBringsAChildIntoView)
{
    SceneCleanGuard guard;
    auto root = MakeWidget("root");
    *root->layout = WidgetLayout::Based(BaseCorner::Center, Vec2F(200, 200));
    root->layout->SetPivot(Vec2F(0.5f, 0.5f));
    root->layout->SetScale(Vec3F(2.0f, 2.0f, 1.0f));

    auto area = mmake<ScrollArea>();
    area->name = "area";
    root->AddChild(area);
    *area->layout = WidgetLayout::Based(BaseCorner::Center, Vec2F(100, 50));

    // a strip three views wide: the last item starts outside the view
    auto item = MakeChildWidget(area, "item");
    *item->layout = WidgetLayout::Based(BaseCorner::LeftBottom, Vec2F(40, 50), Vec2F(250, 0));
    item->AddLayer("back", mmake<Sprite>(Color4::White()), Layout::BothStretch());
    TickAndUpdateLayout(3);
    EXPECT_FALSE(item->layout->IsPointInside(Vec2F(0, 0)));

    // scrolled to the end: the item takes the right edge of the view, 10..50 of the layout, 20..100 on screen. The scroll
    // moves the children itself, nothing else lays them out again
    area->SetScroll(Vec2F(190, 0));

    RectF rect = item->layout->LayoutToWorldRect(item->layout->GetWorldRect());
    ExpectNear(rect.Center(), Vec2F(60, 0), 0.5f, "item center");
    EXPECT_NEAR(rect.Width(), 80.0f, 0.5f);
    EXPECT_TRUE(item->layout->IsPointInside(Vec2F(60, 0)));

    // the hit test also asks the scissor it was drawn with
    o2Render.Begin();
    o2Render.SetCamera(Camera());
    root->Draw();
    o2Render.End();
    EXPECT_TRUE(item->IsUnderPoint(Vec2F(60, 0)));
    EXPECT_FALSE(item->IsUnderPoint(Vec2F(110, 0))) << "past the view's right edge";
}

// A scaled parent magnifies what the layers draw: the drawable keeps the layout size and gets the scale, so a text lays its
// words out as before and its letters grow, a sliced sprite's corners grow with it
TEST(WidgetTransform, ScaledParentMagnifiesTextAndSlicedCorners)
{
    SceneCleanGuard guard;
    auto rig = MakeRig();
    auto font = mmake<VectorFont>(o2Assets.GetBuiltAssetsPath() + "debugFont.ttf");
    auto text = mmake<Text>(Ref<Font>(font));
    text->SetHeight(14);
    text->SetText("Hamburgefonstiv quick fox");
    rig.child->AddLayer("text", text, Layout::BothStretch());
    TickAndUpdateLayout(2);
    Vec2F plainSize = text->GetRealSize();

    rig.root->layout->SetPivot(Vec2F(0.5f, 0.5f));
    rig.root->layout->SetScale(Vec3F(2.0f, 2.0f, 1.0f));
    TickAndUpdateLayout(2);

    for (auto& drawable : { DynamicCast<IRectDrawable>(text), rig.back->GetDrawable() })
    {
        ExpectNear(drawable->GetScale2D(), Vec2F(2, 2), 0.001f, "scale");
        ExpectNear(drawable->GetSize2D(), Vec2F(100, 50), 0.01f, "layout size");
        EXPECT_NEAR(drawable->GetBasis().xv.Length(), 200.0f, 0.1f);
    }
    ExpectNear(text->GetRealSize(), plainSize, 0.5f, "the words keep their layout");

    // back to a plain parent: no scale left on the layers
    rig.root->layout->SetScale(Vec3F(1.0f, 1.0f, 1.0f));
    TickAndUpdateLayout(2);
    ExpectNear(text->GetScale2D(), Vec2F(1, 1), 0.001f, "scale reset");
    ExpectNear(text->GetSize2D(), Vec2F(100, 50), 0.01f, "size after reset");

    // a 10px corner of a sliced sprite twice as large is 20px wide; the middle takes the rest
    SpriteProbe sliced;
    sliced.SetMode(SpriteMode::Sliced);
    sliced.SetSliceBorder(BorderI(10, 10, 10, 10));
    sliced.SetScale2D(Vec2F(2.0f, 2.0f));
    sliced.SetSize2D(Vec2F(100, 50));
    sliced.SetPosition(Vec2F());
    auto verts = const_cast<Mesh&>(sliced.GetMeshForTest()).GetVertices<Vertex>();
    EXPECT_NEAR(verts[1].x - verts[0].x, 20.0f, 0.01f);
    EXPECT_NEAR(verts[3].x - verts[0].x, 200.0f, 0.01f);
}

// The editor's transform of a layer is where it is drawn: the scale and rotation of the widget and its parents carry
// its layout rectangle there; setting it moves the layout so the layer is drawn at the new place
TEST(WidgetTransform, LayerTransformIsWhereItIsDrawn)
{
    SceneCleanGuard guard;
    auto rig = MakeRig();

    rig.root->layout->SetScale(Vec3F(1.5f, 1.5f, 1.0f));
    rig.child->layout->SetAngle(Math::Deg2rad(30.0f));
    TickAndUpdateLayout(2);
    ASSERT_FALSE(rig.child->layout->IsWorldTransformPlain());

    for (auto& layer : { rig.back, rig.top })
    {
        Basis transform = layer->GetTransform();
        Basis drawn = layer->GetDrawable()->GetBasis();
        ExpectNear(transform.origin, drawn.origin, 0.05f, "origin");
        ExpectNear(transform.xv, drawn.xv, 0.05f, "xv");
        ExpectNear(transform.yv, drawn.yv, 0.05f, "yv");
    }

    // Moved by 30 in the world: 20 in the layout of a widget scaled 1.5 times
    Basis before = rig.top->GetTransform();
    Vec2F offsetBefore = rig.top->layout.offsetMin;
    Basis moved = before;
    moved.origin += before.xv.Normalized()*30.0f;
    rig.top->SetTransform(moved);
    TickAndUpdateLayout(2);

    ExpectNear(rig.top->GetTransform().origin, moved.origin, 0.05f, "moved origin");
    ExpectNear(rig.top->GetDrawable()->GetBasis().origin, moved.origin, 0.05f, "drawn there");
    EXPECT_NEAR((rig.top->layout.offsetMin - offsetBefore).Length(), 20.0f, 0.05f);
}
