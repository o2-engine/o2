#include "o2/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Scene/UI/Widget.h"
#include "o2/Scene/UI/WidgetLayout.h"
#include "o2/Utils/Types/CommonTypes.h"
#include "o2Editor/UIRoot.h"
#include "o2Editor/Windows/DockWindowPlace.h"
#include "o2Editor/Windows/DockableWindow.h"
#include "support/EditorWindowsFixture.h"

using namespace o2;
using namespace Editor;
using namespace Editor::Tests;

namespace
{
    Ref<DockWindowPlace> AddRootDock()
    {
        auto root = MakeDock("main dock");
        EditorUIRoot.AddWidget(root);
        return root;
    }
}

class DockWindowPlaceArrange : public EditorWindowsFixture {};

TEST_F(DockWindowPlaceArrange, OneChild_AfterArrange_IsNotTabbed)
{
    auto root = AddRootDock();
    auto place = MakeDock("child");
    root->AddChild(place);

    auto wnd = MakeDockable("only");
    wnd->PlaceDock(place);

    EXPECT_TRUE(wnd->IsDocked());
    EXPECT_FALSE(wnd->IsTabActive());
}

TEST_F(DockWindowPlaceArrange, MultipleChildren_ExactlyOneActive)
{
    auto root = AddRootDock();
    auto place = MakeDock("child");
    root->AddChild(place);

    auto a = MakeDockable("a");
    auto b = MakeDockable("b");
    auto c = MakeDockable("c");

    a->PlaceDock(place);
    b->PlaceDock(place);
    c->PlaceDock(place);

    int activeCount = 0;
    if (a->IsTabActive()) activeCount++;
    if (b->IsTabActive()) activeCount++;
    if (c->IsTabActive()) activeCount++;
    EXPECT_EQ(activeCount, 1);
}

TEST_F(DockWindowPlaceArrange, SetActiveTab_DeactivatesOthers)
{
    auto root = AddRootDock();
    auto place = MakeDock("child");
    root->AddChild(place);

    auto a = MakeDockable("a");
    auto b = MakeDockable("b");
    auto c = MakeDockable("c");

    a->PlaceDock(place);
    b->PlaceDock(place);
    c->PlaceDock(place);

    place->SetActiveTab(b);
    EXPECT_FALSE(a->IsTabActive());
    EXPECT_TRUE(b->IsTabActive());
    EXPECT_FALSE(c->IsTabActive());

    place->SetActiveTab(a);
    EXPECT_TRUE(a->IsTabActive());
    EXPECT_FALSE(b->IsTabActive());
    EXPECT_FALSE(c->IsTabActive());
}

TEST_F(DockWindowPlaceArrange, SetActiveTab_Null_DeactivatesAll)
{
    auto root = AddRootDock();
    auto place = MakeDock("child");
    root->AddChild(place);

    auto a = MakeDockable("a");
    auto b = MakeDockable("b");
    a->PlaceDock(place);
    b->PlaceDock(place);

    place->SetActiveTab(nullptr);
    EXPECT_FALSE(a->IsTabActive());
    EXPECT_FALSE(b->IsTabActive());
}

TEST_F(DockWindowPlaceArrange, SetResizibleDir_StoresDirection)
{
    auto place = MakeDock("p");
    place->SetResizibleDir(TwoDirection::Horizontal, 1.5f, nullptr, nullptr);
    EXPECT_EQ(place->GetResizibleDir(), TwoDirection::Horizontal);

    place->SetResizibleDir(TwoDirection::Vertical, 1.5f, nullptr, nullptr);
    EXPECT_EQ(place->GetResizibleDir(), TwoDirection::Vertical);
}

TEST_F(DockWindowPlaceArrange, ArrangeChildWindows_NoChildren_DoesNotCrash)
{
    auto place = MakeDock("empty");
    place->ArrangeChildWindows();
    EXPECT_TRUE(place->GetChildWidgets().IsEmpty());
}
