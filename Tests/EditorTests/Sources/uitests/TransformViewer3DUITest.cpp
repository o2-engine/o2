#include "o2Editor/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Scene/Actor.h"
#include "o2/Scene/UI/UIManager.h"
#include "o2/Scene/UI/Widgets/EditBox.h"
#include "o2Editor/Actions/PropertyChange.h"
#include "o2Editor/Properties/Basic/FloatProperty.h"
#include "o2Editor/Properties/Basic/Vector3FloatProperty.h"
#include "o2Editor/Windows/PropertiesWindow/ActorsViewer/DefaultActorTransformViewer.h"
#include "support/EditorTestScene.h"

using namespace o2;
using namespace Editor;
using namespace Editor::Tests;

namespace
{
    Ref<Widget> FindByNameDeep(const Ref<Widget>& w, const String& name)
    {
        if (w->GetName() == name)
            return w;
        if (auto r = w->FindInternalWidget(name))
            return r;
        for (auto& c : w->GetChildWidgets())
            if (auto r = FindByNameDeep(c, name))
                return r;
        return nullptr;
    }

    struct ViewerFixture
    {
        Ref<DefaultActorTransformViewer> viewer;
        Ref<FloatProperty>               positionZ;
        Ref<Vec3FProperty>               rotation3D;
        Ref<FloatProperty>               scaleZ;

        explicit ViewerFixture(const Ref<Actor>& actor)
        {
            viewer = mmake<DefaultActorTransformViewer>();
            viewer->SetTargetActors({ actor.Get() });
            viewer->Refresh();

            auto root = viewer->GetWidget();
            auto position = DynamicCast<Vec3FProperty>(FindByNameDeep(root, "position property"));
            auto scale = DynamicCast<Vec3FProperty>(FindByNameDeep(root, "scale property"));
            rotation3D = DynamicCast<Vec3FProperty>(FindByNameDeep(root, "rotation property"));
            positionZ = position ? position->GetZProperty() : nullptr;
            scaleZ = scale ? scale->GetZProperty() : nullptr;
        }
    };
}

TEST(TransformViewer3DUI, RefreshShowsActorValuesInDegrees)
{
    SceneCleanGuard guard;
    auto actor = MakeActor();
    actor->transform->SetPositionZ(5.0f);
    actor->transform->SetEulerAnglesDegrees(Vec3F(10.0f, 20.0f, 30.0f));
    actor->transform->SetScaleZ(2.0f);
    TickScene();

    ViewerFixture f(actor);
    ASSERT_NE(f.positionZ, nullptr);
    ASSERT_NE(f.rotation3D, nullptr);
    ASSERT_NE(f.scaleZ, nullptr);

    EXPECT_FLOAT_EQ(f.positionZ->GetCommonValue(), 5.0f);
    EXPECT_NEAR(f.rotation3D->GetXProperty()->GetCommonValue(), 10.0f, 1e-3f);
    EXPECT_NEAR(f.rotation3D->GetYProperty()->GetCommonValue(), 20.0f, 1e-3f);
    EXPECT_NEAR(f.rotation3D->GetZProperty()->GetCommonValue(), 30.0f, 1e-3f);
    EXPECT_FLOAT_EQ(f.scaleZ->GetCommonValue(), 2.0f);
}

// The 3D rotation z component is the same angle the 2D rotation field edits
TEST(TransformViewer3DUI, RotationZStaysInSyncWith2DAngle)
{
    SceneCleanGuard guard;
    auto actor = MakeActor();
    actor->transform->SetAngleDegrees(45.0f);
    TickScene();

    ViewerFixture f(actor);
    ASSERT_NE(f.rotation3D, nullptr);
    EXPECT_NEAR(f.rotation3D->GetZProperty()->GetCommonValue(), 45.0f, 1e-3f);

    f.rotation3D->GetZProperty()->GetEditBox()->onChangeCompleted("90");
    EXPECT_NEAR(actor->transform->GetAngleDegrees(), 90.0f, 1e-3f);
}

TEST(TransformViewer3DUI, PositionZEditCompletesWithUndoableAction)
{
    SceneCleanGuard guard;
    auto actor = MakeActor();
    actor->transform->SetPositionZ(1.0f);
    TickScene();

    ViewerFixture f(actor);
    ASSERT_NE(f.positionZ, nullptr);

    // Handler mirrors ActionsList::DoneActorPropertyChangeAction: make the action and Redo it
    int completed = 0;
    String path;
    Ref<PropertyChangeAction> action;
    f.viewer->onPropertyChangeCompleted = [&](const String& p, const Vector<DataDocument>& b, const Vector<DataDocument>& a) {
        completed++;
        path = p;
        action = mmake<PropertyChangeAction>(AsEditable({ actor }), p, b, a);
        action->Redo();
    };

    f.positionZ->GetEditBox()->onChangeCompleted("7");

    EXPECT_EQ(completed, 1);
    EXPECT_EQ(path, String("transform/positionZ"));
    EXPECT_FLOAT_EQ(actor->transform->GetPositionZ(), 7.0f);

    ASSERT_NE(action, nullptr);
    action->Undo();
    EXPECT_FLOAT_EQ(actor->transform->GetPositionZ(), 1.0f);
    action->Redo();
    EXPECT_FLOAT_EQ(actor->transform->GetPositionZ(), 7.0f);
}

TEST(TransformViewer3DUI, RotationEditCompletesWithUndoableDegreesAction)
{
    SceneCleanGuard guard;
    auto actor = MakeActor();
    actor->transform->SetEulerAnglesDegrees(Vec3F(0.0f, 0.0f, 30.0f));
    TickScene();

    ViewerFixture f(actor);
    ASSERT_NE(f.rotation3D, nullptr);

    int completed = 0;
    String path;
    Vector<DataDocument> before, after;
    f.viewer->onPropertyChangeCompleted = [&](const String& p, const Vector<DataDocument>& b, const Vector<DataDocument>& a) {
        completed++;
        path = p;
        before = b;
        after = a;
    };

    f.rotation3D->GetZProperty()->GetEditBox()->onChangeCompleted("90");

    EXPECT_EQ(completed, 1);
    EXPECT_EQ(path, String("transform/eulerAnglesDegrees"));
    EXPECT_NEAR(actor->transform->GetAngleDegrees(), 90.0f, 1e-3f);

    auto action = mmake<PropertyChangeAction>(AsEditable({ actor }), path, before, after);
    action->Undo();
    EXPECT_NEAR(actor->transform->GetAngleDegrees(), 30.0f, 1e-3f);
    action->Redo();
    EXPECT_NEAR(actor->transform->GetAngleDegrees(), 90.0f, 1e-3f);
}

TEST(TransformViewer3DUI, ScaleZEditCompletesWithUndoableAction)
{
    SceneCleanGuard guard;
    auto actor = MakeActor();
    actor->transform->SetScaleZ(1.0f);
    TickScene();

    ViewerFixture f(actor);
    ASSERT_NE(f.scaleZ, nullptr);

    // Handler mirrors ActionsList::DoneActorPropertyChangeAction: make the action and Redo it
    int completed = 0;
    String path;
    Ref<PropertyChangeAction> action;
    f.viewer->onPropertyChangeCompleted = [&](const String& p, const Vector<DataDocument>& b, const Vector<DataDocument>& a) {
        completed++;
        path = p;
        action = mmake<PropertyChangeAction>(AsEditable({ actor }), p, b, a);
        action->Redo();
    };

    f.scaleZ->GetEditBox()->onChangeCompleted("3");

    EXPECT_EQ(completed, 1);
    EXPECT_EQ(path, String("transform/scaleZ"));
    EXPECT_FLOAT_EQ(actor->transform->GetScaleZ(), 3.0f);

    ASSERT_NE(action, nullptr);
    action->Undo();
    EXPECT_FLOAT_EQ(actor->transform->GetScaleZ(), 1.0f);
    action->Redo();
    EXPECT_FLOAT_EQ(actor->transform->GetScaleZ(), 3.0f);
}
