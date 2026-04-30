#include "o2/stdafx.h"
#include <gtest/gtest.h>

#include "o2Editor/Actions/IAction.h"

using namespace o2;
using namespace Editor;

namespace
{
    class MoveAction : public IAction
    {
    public:
        int* target;
        int  from;
        int  to;
        int  redoCount = 0;
        int  undoCount = 0;

        MoveAction(int* t, int f, int to_) : target(t), from(f), to(to_) {}

        String GetName() const override { return "Move"; }
        void Redo() override { *target = to; ++redoCount; }
        void Undo() override { *target = from; ++undoCount; }

    public:
        bool TryMerge(const Ref<IAction>& other) override
        {
            auto m = DynamicCast<MoveAction>(other);
            if (m && m->target == target)
            {
                to = m->to;
                return true;
            }
            return false;
        }
    };

    class ScaleAction : public IAction
    {
    public:
        int* target;
        int  from;
        int  to;

        ScaleAction(int* t, int f, int to_) : target(t), from(f), to(to_) {}

        String GetName() const override { return "Scale"; }
        void Redo() override { *target = to; }
        void Undo() override { *target = from; }
    };

    class DefaultMergeAction : public IAction
    {
    public:
        String GetName() const override { return "Default"; }
        using IAction::TryMerge;
    };
}

TEST(IActionAppend, DefaultTryMergeReturnsFalse)
{
    auto a = mmake<DefaultMergeAction>();
    auto b = mmake<DefaultMergeAction>();
    EXPECT_FALSE(a->TryMerge(b));
}

TEST(IActionAppend, TryMergeAcceptsSameTypeAndUpdatesFinalState)
{
    int pos = 0;
    auto main = mmake<MoveAction>(&pos, 0, 10);
    auto delta = mmake<MoveAction>(&pos, 10, 15);

    EXPECT_TRUE(main->TryMerge(delta));
    EXPECT_EQ(main->to, 15);
    EXPECT_EQ(main->from, 0);
    EXPECT_EQ(pos, 0);
}

TEST(IActionAppend, TryMergeRejectsDifferentType)
{
    int pos = 0;
    auto move = mmake<MoveAction>(&pos, 0, 10);
    auto scale = mmake<ScaleAction>(&pos, 0, 99);
    EXPECT_FALSE(move->TryMerge(scale));
    EXPECT_EQ(move->to, 10);
}

TEST(IActionAppend, TryMergeRejectsForeignTarget)
{
    int posA = 0, posB = 0;
    auto a = mmake<MoveAction>(&posA, 0, 10);
    auto b = mmake<MoveAction>(&posB, 0, 5);
    EXPECT_FALSE(a->TryMerge(b));
    EXPECT_EQ(a->to, 10);
}

TEST(IActionAppend, AppendAppliesInnerRedoAndMergesFinalState)
{
    int pos = 0;
    auto main = mmake<MoveAction>(&pos, 0, 10);
    main->Redo();
    EXPECT_EQ(pos, 10);

    auto delta1 = mmake<MoveAction>(&pos, 10, 14);
    main->Append(delta1);
    EXPECT_EQ(pos, 14);
    EXPECT_EQ(delta1->redoCount, 1);
    EXPECT_EQ(main->to, 14);
    EXPECT_EQ(main->from, 0);

    auto delta2 = mmake<MoveAction>(&pos, 14, 20);
    main->Append(delta2);
    EXPECT_EQ(pos, 20);
    EXPECT_EQ(main->to, 20);
    EXPECT_EQ(main->from, 0);
}

TEST(IActionAppend, AfterAppendUndoRestoresOriginalFromState)
{
    int pos = 0;
    auto main = mmake<MoveAction>(&pos, 0, 10);
    main->Redo();
    main->Append(mmake<MoveAction>(&pos, 10, 25));
    main->Append(mmake<MoveAction>(&pos, 25, 50));
    EXPECT_EQ(pos, 50);

    main->Undo();
    EXPECT_EQ(pos, 0);
    EXPECT_EQ(main->undoCount, 1);
}

TEST(IActionAppend, AfterAppendRedoAppliesMergedFinalState)
{
    int pos = 0;
    auto main = mmake<MoveAction>(&pos, 0, 10);
    main->Redo();
    main->Append(mmake<MoveAction>(&pos, 10, 25));
    main->Undo();
    EXPECT_EQ(pos, 0);

    main->Redo();
    EXPECT_EQ(pos, 25);
    EXPECT_EQ(main->redoCount, 2);
}
