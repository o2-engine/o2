#include "o2/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Scene/UI/Widget.h"
#include "o2/Scene/UI/WidgetLayout.h"
#include "o2Editor/UIRoot.h"
#include "o2Editor/Windows/DockWindowPlace.h"
#include "o2Editor/Windows/DockableWindow.h"
#include "support/EditorWindowsFixture.h"

using namespace o2;
using namespace Editor;
using namespace Editor::Tests;

namespace
{
    Ref<DockWindowPlace> MakeRootDock(const String& name = "main dock")
    {
        auto root = MakeDock(name);
        EditorUIRoot.AddWidget(root);
        return root;
    }
}

TEST_F(EditorWindowsFixture, Construct_NotDocked)
{
    auto wnd = MakeDockable("w");
    EXPECT_FALSE(wnd->IsDocked());
    EXPECT_FALSE(wnd->IsTabActive());
}

TEST_F(EditorWindowsFixture, PlaceDock_AddsToDock_AndMarks)
{
    auto root = MakeRootDock();
    auto childPlace = MakeDock("child");
    root->AddChild(childPlace);

    auto wnd = MakeDockable("w");
    wnd->PlaceDock(childPlace);

    EXPECT_TRUE(wnd->IsDocked());
    ASSERT_EQ(childPlace->GetChildWidgets().Count(), 1);
    EXPECT_EQ(childPlace->GetChildWidgets()[0].Get(), wnd.Get());
}

TEST_F(EditorWindowsFixture, PlaceDock_TwoWindows_BothDockedAsTabs)
{
    auto root = MakeRootDock();
    auto childPlace = MakeDock("child");
    root->AddChild(childPlace);

    auto a = MakeDockable("a");
    auto b = MakeDockable("b");

    a->PlaceDock(childPlace);
    b->PlaceDock(childPlace);

    EXPECT_TRUE(a->IsDocked());
    EXPECT_TRUE(b->IsDocked());
    ASSERT_EQ(childPlace->GetChildWidgets().Count(), 2);

    int activeCount = (a->IsTabActive() ? 1 : 0) + (b->IsTabActive() ? 1 : 0);
    EXPECT_EQ(activeCount, 1);
}

TEST_F(EditorWindowsFixture, SetTabActive_SwitchesActive)
{
    auto root = MakeRootDock();
    auto childPlace = MakeDock("child");
    root->AddChild(childPlace);

    auto a = MakeDockable("a");
    auto b = MakeDockable("b");
    auto c = MakeDockable("c");

    a->PlaceDock(childPlace);
    b->PlaceDock(childPlace);
    c->PlaceDock(childPlace);

    b->SetTabActive();

    EXPECT_FALSE(a->IsTabActive());
    EXPECT_TRUE(b->IsTabActive());
    EXPECT_FALSE(c->IsTabActive());

    c->SetTabActive();

    EXPECT_FALSE(a->IsTabActive());
    EXPECT_FALSE(b->IsTabActive());
    EXPECT_TRUE(c->IsTabActive());
}

TEST_F(EditorWindowsFixture, Undock_FromTabbedPlace_ReducesChildren)
{
    auto root = MakeRootDock();
    auto childPlace = MakeDock("child");
    root->AddChild(childPlace);

    auto a = MakeDockable("a");
    auto b = MakeDockable("b");

    a->PlaceDock(childPlace);
    b->PlaceDock(childPlace);

    a->Undock();

    EXPECT_FALSE(a->IsDocked());
    ASSERT_EQ(childPlace->GetChildWidgets().Count(), 1);
    EXPECT_EQ(childPlace->GetChildWidgets()[0].Get(), b.Get());

    auto parent = a->GetParent().Lock();
    ASSERT_TRUE(parent != nullptr);
    EXPECT_EQ(parent.Get(), EditorUIRoot.GetRootWidget().Get());
}

TEST_F(EditorWindowsFixture, Undock_OnNonDocked_IsNoOp)
{
    auto wnd = MakeDockable("w");
    wnd->Undock();
    EXPECT_FALSE(wnd->IsDocked());
}

TEST_F(EditorWindowsFixture, SetCaption_RecalculatesTabWidth_WhenAutoCalc)
{
    auto wnd = MakeDockable("w");
    wnd->SetAutoCalcuclatingTabWidth(true);

    wnd->SetCaption(L"a");
    float narrow = wnd->GetTabWidth();

    wnd->SetCaption(L"a much longer window caption text");
    float wide = wnd->GetTabWidth();

    EXPECT_GE(wide, narrow);
}

TEST_F(EditorWindowsFixture, SetTabWidth_ManualOverride)
{
    auto wnd = MakeDockable("w");
    wnd->SetAutoCalcuclatingTabWidth(false);
    wnd->SetTabWidth(77.0f);
    EXPECT_FLOAT_EQ(wnd->GetTabWidth(), 77.0f);
}
