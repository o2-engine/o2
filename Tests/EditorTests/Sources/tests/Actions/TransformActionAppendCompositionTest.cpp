#include "o2/stdafx.h"
#include <gtest/gtest.h>

#include "o2Editor/Actions/IAction.h"
#include "o2Editor/Actions/Transform.h"
#include "support/EditorTestScene.h"

using namespace o2;
using namespace Editor;
using namespace Editor::Tests;

namespace
{
    Basis GetBasis(const Ref<Actor>& a)
    {
        return static_cast<const Actor*>(a.Get())->GetTransform();
    }

    bool NearB(const Basis& a, const Basis& b, float eps = 1e-3f)
    {
        return NearV(a.origin, b.origin, eps) && NearV(a.xv, b.xv, eps) && NearV(a.yv, b.yv, eps);
    }

    Ref<TransformAction> MakeBasisStep(const Vector<Ref<SceneEditableObject>>& objects, const Basis& delta)
    {
        auto step = mmake<TransformAction>(objects);
        step->doneTransforms = step->beforeTransforms;
        for (auto& t : step->doneTransforms)
            t.transform = t.transform * delta;
        return step;
    }
}

TEST(TransformActionAppendComposition, RotateAroundPivot_SingleActor_CoalescesNStepsIntoOneAction)
{
    SceneCleanGuard guard;
    auto a = MakeActor(Vec2F(10.0f, 0.0f));
    auto editable = AsEditable({a});

    Basis initial = GetBasis(a);

    auto main = mmake<TransformAction>(editable);
    ASSERT_EQ(main->beforeTransforms.Count(), 1);
    EXPECT_TRUE(main->beforeTransforms[0].transform == initial);
    EXPECT_EQ(main->doneTransforms.Count(), 0);

    const Vec2F pivot(0.0f, 0.0f);
    const float angleDelta = Math::Deg2rad(18.0f);
    const Basis rotation =
        Basis::Translated(pivot * -1.0f) *
        Basis::Rotated(-angleDelta) *
        Basis::Translated(pivot);

    Basis expected = initial;
    for (int i = 0; i < 5; i++)
    {
        auto step = MakeBasisStep(editable, rotation);
        main->Append(step);
        expected = expected * rotation;
    }

    ASSERT_EQ(main->doneTransforms.Count(), 1);
    EXPECT_TRUE(main->doneTransforms[0].transform == expected);

    EXPECT_TRUE(NearB(GetBasis(a), expected));

    main->Undo();
    EXPECT_TRUE(NearB(GetBasis(a), initial));

    main->Redo();
    EXPECT_TRUE(NearB(GetBasis(a), expected));
}

TEST(TransformActionAppendComposition, RotateAroundPivot_MultiActor_AllConverge)
{
    SceneCleanGuard guard;
    auto a = MakeActor(Vec2F(10.0f, 0.0f));
    auto b = MakeActor(Vec2F(0.0f, 20.0f));
    auto editable = AsEditable({a, b});

    Basis initialA = GetBasis(a);
    Basis initialB = GetBasis(b);

    auto main = mmake<TransformAction>(editable);

    const Vec2F pivot(5.0f, 5.0f);
    const float angleDelta = Math::Deg2rad(10.0f);
    const Basis rotation =
        Basis::Translated(pivot * -1.0f) *
        Basis::Rotated(-angleDelta) *
        Basis::Translated(pivot);

    Basis expectedA = initialA;
    Basis expectedB = initialB;
    for (int i = 0; i < 4; i++)
    {
        main->Append(MakeBasisStep(editable, rotation));
        expectedA = expectedA * rotation;
        expectedB = expectedB * rotation;
    }

    ASSERT_EQ(main->doneTransforms.Count(), 2);
    EXPECT_TRUE(main->doneTransforms[0].transform == expectedA);
    EXPECT_TRUE(main->doneTransforms[1].transform == expectedB);

    main->Undo();
    EXPECT_TRUE(NearB(GetBasis(a), initialA));
    EXPECT_TRUE(NearB(GetBasis(b), initialB));

    main->Redo();
    EXPECT_TRUE(NearB(GetBasis(a), expectedA));
    EXPECT_TRUE(NearB(GetBasis(b), expectedB));
}

TEST(TransformActionAppendComposition, RotateSeparated_PreservesLegacyBasis)
{
    SceneCleanGuard guard;
    auto a = MakeActor(Vec2F(10.0f, 0.0f));
    auto editable = AsEditable({a});

    Basis initial = GetBasis(a);
    auto main = mmake<TransformAction>(editable);

    const float angleDelta = Math::Deg2rad(30.0f);
    const Basis legacySeparated = Basis::Rotated(-angleDelta);

    main->Append(MakeBasisStep(editable, legacySeparated));

    Basis expected = initial * legacySeparated;
    EXPECT_TRUE(NearB(GetBasis(a), expected));

    main->Undo();
    EXPECT_TRUE(NearB(GetBasis(a), initial));

    main->Redo();
    EXPECT_TRUE(NearB(GetBasis(a), expected));
}

TEST(TransformActionAppendComposition, ScaleAroundHandlesPos_RotatedAxis)
{
    SceneCleanGuard guard;
    auto a = MakeActor(Vec2F(10.0f, 0.0f));
    auto editable = AsEditable({a});

    Basis initial = GetBasis(a);
    auto main = mmake<TransformAction>(editable);

    const Vec2F handlesPos(20.0f, 30.0f);
    const float handlesAngle = Math::PI() / 4.0f;
    const Vec2F scale(1.5f, 0.8f);

    const Basis scaleBasis =
        Basis::Translated(handlesPos * -1.0f) *
        Basis::Rotated(-handlesAngle) *
        Basis::Scaled(scale) *
        Basis::Rotated(handlesAngle) *
        Basis::Translated(handlesPos);

    Basis expected = initial;
    for (int i = 0; i < 3; i++)
    {
        main->Append(MakeBasisStep(editable, scaleBasis));
        expected = expected * scaleBasis;
    }

    ASSERT_EQ(main->doneTransforms.Count(), 1);
    EXPECT_TRUE(main->doneTransforms[0].transform == expected);
    EXPECT_TRUE(NearB(GetBasis(a), expected));

    main->Undo();
    EXPECT_TRUE(NearB(GetBasis(a), initial));
}

TEST(TransformActionAppendComposition, FrameBasis_ArbitraryComposition)
{
    SceneCleanGuard guard;
    auto a = MakeActor(Vec2F(0.0f, 0.0f));
    auto b = MakeActor(Vec2F(50.0f, 0.0f));
    auto editable = AsEditable({a, b});

    Basis initialA = GetBasis(a);
    Basis initialB = GetBasis(b);

    auto main = mmake<TransformAction>(editable);

    const Basis frameDelta =
        Basis::Translated(Vec2F(2.0f, 3.0f)) *
        Basis::Rotated(Math::Deg2rad(7.0f)) *
        Basis::Scaled(Vec2F(1.05f, 0.95f));

    Basis expectedA = initialA;
    Basis expectedB = initialB;
    for (int i = 0; i < 6; i++)
    {
        main->Append(MakeBasisStep(editable, frameDelta));
        expectedA = expectedA * frameDelta;
        expectedB = expectedB * frameDelta;
    }

    EXPECT_TRUE(main->doneTransforms[0].transform == expectedA);
    EXPECT_TRUE(main->doneTransforms[1].transform == expectedB);

    main->Undo();
    EXPECT_TRUE(NearB(GetBasis(a), initialA));
    EXPECT_TRUE(NearB(GetBasis(b), initialB));

    main->Redo();
    EXPECT_TRUE(NearB(GetBasis(a), expectedA));
    EXPECT_TRUE(NearB(GetBasis(b), expectedB));
}

TEST(TransformActionAppendComposition, MultiStepCoalescingInvariant_BeforeIsFrozenDoneIsLatest)
{
    SceneCleanGuard guard;
    auto a = MakeActor(Vec2F(0.0f, 0.0f));
    auto editable = AsEditable({a});

    Basis initial = GetBasis(a);

    auto main = mmake<TransformAction>(editable);
    Basis frozenBefore = main->beforeTransforms[0].transform;
    EXPECT_TRUE(frozenBefore == initial);

    const Basis delta = Basis::Translated(Vec2F(1.0f, 0.0f));

    Basis expected = initial;
    for (int i = 0; i < 10; i++)
    {
        main->Append(MakeBasisStep(editable, delta));
        expected = expected * delta;
    }

    EXPECT_TRUE(main->beforeTransforms[0].transform == frozenBefore);
    EXPECT_TRUE(main->doneTransforms[0].transform == expected);
}
