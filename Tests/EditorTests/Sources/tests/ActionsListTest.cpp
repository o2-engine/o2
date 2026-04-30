#include "o2/stdafx.h"
#include <gtest/gtest.h>

#include "o2Editor/Actions/ActionsList.h"
#include "o2Editor/Actions/Enable.h"

using namespace o2;
using namespace Editor;

namespace
{
    Ref<EnableAction> MakeEmptyEnableAction(bool enable = true)
    {
        return mmake<EnableAction>(Vector<Ref<SceneEditableObject>>(), enable);
    }
}

TEST(ActionsList, EmptyListHasZeroCounts)
{
    ActionsList list;
    EXPECT_EQ(list.GetUndoActionsCount(), 0);
    EXPECT_EQ(list.GetRedoActionsCount(), 0);
    EXPECT_EQ(list.GetLastActionName(), String(""));
    EXPECT_EQ(list.GetNextForwardActionName(), String(""));
}

TEST(ActionsList, DoneActionAppendsAndClearsForward)
{
    ActionsList list;

    auto first = MakeEmptyEnableAction(true);
    list.DoneAction(first);
    EXPECT_EQ(list.GetUndoActionsCount(), 1);
    EXPECT_EQ(list.GetRedoActionsCount(), 0);
    EXPECT_EQ(list.GetLastActionName(), first->GetName());

    list.UndoAction();
    EXPECT_EQ(list.GetUndoActionsCount(), 0);
    EXPECT_EQ(list.GetRedoActionsCount(), 1);

    auto second = MakeEmptyEnableAction(false);
    list.DoneAction(second);
    EXPECT_EQ(list.GetUndoActionsCount(), 1);
    EXPECT_EQ(list.GetRedoActionsCount(), 0);
    EXPECT_EQ(list.GetLastActionName(), second->GetName());
}

TEST(ActionsList, UndoMovesActionToForwardStack)
{
    ActionsList list;

    auto a = MakeEmptyEnableAction(true);
    auto b = MakeEmptyEnableAction(false);
    list.DoneAction(a);
    list.DoneAction(b);

    list.UndoAction();
    EXPECT_EQ(list.GetUndoActionsCount(), 1);
    EXPECT_EQ(list.GetRedoActionsCount(), 1);
    EXPECT_EQ(list.GetLastActionName(), a->GetName());
    EXPECT_EQ(list.GetNextForwardActionName(), b->GetName());
}

TEST(ActionsList, RedoRestoresUndoneAction)
{
    ActionsList list;

    auto a = MakeEmptyEnableAction(true);
    list.DoneAction(a);
    list.UndoAction();
    EXPECT_EQ(list.GetRedoActionsCount(), 1);

    list.RedoAction();
    EXPECT_EQ(list.GetUndoActionsCount(), 1);
    EXPECT_EQ(list.GetRedoActionsCount(), 0);
    EXPECT_EQ(list.GetLastActionName(), a->GetName());
}

TEST(ActionsList, UndoRedoOnEmptyListIsNoOp)
{
    ActionsList list;

    list.UndoAction();
    list.RedoAction();

    EXPECT_EQ(list.GetUndoActionsCount(), 0);
    EXPECT_EQ(list.GetRedoActionsCount(), 0);
}

TEST(ActionsList, ResetUndoActionsClearsBothStacks)
{
    ActionsList list;

    list.DoneAction(MakeEmptyEnableAction(true));
    list.DoneAction(MakeEmptyEnableAction(false));
    list.UndoAction();
    EXPECT_GT(list.GetUndoActionsCount(), 0);
    EXPECT_GT(list.GetRedoActionsCount(), 0);

    list.ResetUndoActions();
    EXPECT_EQ(list.GetUndoActionsCount(), 0);
    EXPECT_EQ(list.GetRedoActionsCount(), 0);
}

TEST(ActionsList, EnableActionNameDependsOnFlag)
{
    auto enable = MakeEmptyEnableAction(true);
    auto disable = MakeEmptyEnableAction(false);

    EXPECT_EQ(enable->GetName(), String("Enable actors"));
    EXPECT_EQ(disable->GetName(), String("Disable actors"));
}
