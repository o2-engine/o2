#include "o2/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Render/Sprite.h"
#include "o2/Scene/UI/Widget.h"
#include "o2/Scene/UI/WidgetLayer.h"
#include "o2/Scene/UI/WidgetLayout.h"
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
