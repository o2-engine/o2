#include "o2/stdafx.h"
#include <gtest/gtest.h>

#include "o2Editor/Actions/SplineKeys.h"
#include "support/EditorTestScene.h"

using namespace o2;
using namespace Editor;
using namespace Editor::Tests;

namespace
{
    Spline::Key MakeKey(const Vec2F& value)
    {
        return Spline::Key(value, 0.0f, value + Vec2F(-1, 0), value + Vec2F(1, 0));
    }

    Ref<Spline> MakeSpline(const Vector<Vec2F>& positions)
    {
        auto sp = mmake<Spline>();
        Vector<Spline::Key> keys;
        for (auto& p : positions)
            keys.Add(MakeKey(p));
        sp->SetKeys(keys);
        return sp;
    }
}

TEST(SplineKeysAction, CtorCapturesBeforeKeys)
{
    auto sp = MakeSpline({ Vec2F(0, 0), Vec2F(10, 0), Vec2F(20, 0) });
    auto action = mmake<SplineKeysAction>(sp);

    ASSERT_EQ(action->beforeKeys.Count(), 3);
    EXPECT_TRUE(action->beforeKeys[1].value == Vec2F(10, 0));
    EXPECT_FALSE(action->doneCaptured);
}

TEST(SplineKeysAction, RedoUndoRestoresFullKeysSnapshot)
{
    auto sp = MakeSpline({ Vec2F(0, 0), Vec2F(10, 0) });
    auto action = mmake<SplineKeysAction>(sp);

    auto modified = sp->GetKeys();
    modified[1].value = Vec2F(50, 5);
    sp->SetKeys(modified);
    action->Completed();

    auto messed = sp->GetKeys();
    messed[1].value = Vec2F(-99, -99);
    sp->SetKeys(messed);

    action->Redo();
    EXPECT_TRUE(sp->GetKeys()[1].value == Vec2F(50, 5));

    action->Undo();
    EXPECT_TRUE(sp->GetKeys()[1].value == Vec2F(10, 0));
}

TEST(SplineKeysAction, AppendCoalescesIncrementalSteps)
{
    auto sp = MakeSpline({ Vec2F(0, 0) });
    auto main = mmake<SplineKeysAction>(sp);

    {
        auto modified = sp->GetKeys();
        modified[0].value = Vec2F(1, 0);
        sp->SetKeys(modified);
        auto step = mmake<SplineKeysAction>(sp);
        step->Completed();
        main->Append(step);
    }
    {
        auto modified = sp->GetKeys();
        modified[0].value = Vec2F(5, 0);
        sp->SetKeys(modified);
        auto step = mmake<SplineKeysAction>(sp);
        step->Completed();
        main->Append(step);
    }

    EXPECT_TRUE(sp->GetKeys()[0].value == Vec2F(5, 0));

    main->Undo();
    EXPECT_TRUE(sp->GetKeys()[0].value == Vec2F(0, 0));

    main->Redo();
    EXPECT_TRUE(sp->GetKeys()[0].value == Vec2F(5, 0));
}

TEST(SplineKeysAction, TryMergeRejectsForeignSpline)
{
    auto a = MakeSpline({ Vec2F(0, 0) });
    auto b = MakeSpline({ Vec2F(0, 0) });

    auto main = mmake<SplineKeysAction>(a);
    auto step = mmake<SplineKeysAction>(b);
    step->Completed();

    EXPECT_FALSE(main->TryMerge(step));
}

TEST(SplineKeysAction, TryMergeRejectsStepWithoutDoneCaptured)
{
    auto sp = MakeSpline({ Vec2F(0, 0) });

    auto main = mmake<SplineKeysAction>(sp);
    auto step = mmake<SplineKeysAction>(sp);

    EXPECT_FALSE(main->TryMerge(step));
}
