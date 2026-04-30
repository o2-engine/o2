#include "o2/stdafx.h"
#include <gtest/gtest.h>

#include "o2Editor/Actions/MeshPoints.h"
#include "support/EditorTestScene.h"

using namespace o2;
using namespace Editor;
using namespace Editor::Tests;

namespace
{
    struct PointsModel
    {
        Vector<Vec2F> points;

        Vector<Vec2F> GetAll() const { return points; }
        void Set(int i, Vec2F p) { points[i] = p; }
    };

    bool NearVec(const Vec2F& a, const Vec2F& b, float eps = 1e-3f)
    {
        return NearV(a, b, eps);
    }
}

TEST(MeshPointsAction, RedoUndoReplaysSnapshots)
{
    PointsModel m;
    m.points = { Vec2F(0, 0), Vec2F(10, 0), Vec2F(20, 0) };

    auto setter = [&](int i, Vec2F p) { m.Set(i, p); };

    auto action = mmake<MeshPointsAction>(m.GetAll(), setter);
    m.points[1] = Vec2F(15, 5);
    action->Completed(m.GetAll());

    m.points[1] = Vec2F(-99, -99);

    action->Redo();
    EXPECT_TRUE(NearVec(m.points[1], Vec2F(15, 5)));

    action->Undo();
    EXPECT_TRUE(NearVec(m.points[1], Vec2F(10, 0)));
}

TEST(MeshPointsAction, AppendCoalescesIncrementalSteps)
{
    PointsModel m;
    m.points = { Vec2F(0, 0), Vec2F(10, 0) };

    auto setter = [&](int i, Vec2F p) { m.Set(i, p); };

    auto main = mmake<MeshPointsAction>(m.GetAll(), setter);

    Vector<Vec2F> step1Points = m.GetAll();
    step1Points[0] = Vec2F(5, 5);
    auto step1 = mmake<MeshPointsAction>(m.GetAll(), setter);
    step1->Completed(step1Points);
    main->Append(step1);
    EXPECT_TRUE(NearVec(m.points[0], Vec2F(5, 5)));

    Vector<Vec2F> step2Points = m.GetAll();
    step2Points[0] = Vec2F(7, 7);
    auto step2 = mmake<MeshPointsAction>(m.GetAll(), setter);
    step2->Completed(step2Points);
    main->Append(step2);
    EXPECT_TRUE(NearVec(m.points[0], Vec2F(7, 7)));

    main->Undo();
    EXPECT_TRUE(NearVec(m.points[0], Vec2F(0, 0)));

    main->Redo();
    EXPECT_TRUE(NearVec(m.points[0], Vec2F(7, 7)));
}

TEST(MeshPointsAction, TryMergeRejectsMismatchedPointCount)
{
    auto setter = [](int, Vec2F) {};

    auto main = mmake<MeshPointsAction>(Vector<Vec2F>{ Vec2F(0, 0), Vec2F(1, 0) }, setter);
    auto step = mmake<MeshPointsAction>(Vector<Vec2F>{ Vec2F(0, 0) }, setter);
    step->Completed(Vector<Vec2F>{ Vec2F(2, 2) });

    EXPECT_FALSE(main->TryMerge(step));
}

TEST(MeshPointsAction, TryMergeRejectsStepWithoutDoneCaptured)
{
    auto setter = [](int, Vec2F) {};

    auto main = mmake<MeshPointsAction>(Vector<Vec2F>{ Vec2F(0, 0) }, setter);
    auto step = mmake<MeshPointsAction>(Vector<Vec2F>{ Vec2F(0, 0) }, setter);

    EXPECT_FALSE(main->TryMerge(step));
}
