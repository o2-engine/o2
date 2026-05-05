#include "o2/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Scene/UI/Widget.h"
#include "o2/Scene/UI/WidgetLayout.h"
#include "o2/Utils/Serialization/DataValue.h"
#include "o2Editor/UIRoot.h"
#include "o2Editor/Windows/DockWindowPlace.h"
#include "o2Editor/Windows/DockableWindow.h"
#include "o2Editor/Windows/WindowsLayout.h"
#include "support/EditorWindowsFixture.h"

using namespace o2;
using namespace Editor;
using namespace Editor::Tests;

class WindowsLayoutRetrieve : public EditorWindowsFixture {};

TEST_F(WindowsLayoutRetrieve, EmptyDock_RecordsAnchors_NoChildren)
{
    auto root = MakeDock("root");
    EditorUIRoot.AddWidget(root);

    WindowsLayout::WindowDockPlaceInfo info;
    info.RetrieveLayout(root);

    EXPECT_TRUE(info.windows.IsEmpty());
    EXPECT_TRUE(info.childs.IsEmpty());
}

TEST_F(WindowsLayoutRetrieve, OneDockedWindow_RecordsName)
{
    auto root = MakeDock("root");
    EditorUIRoot.AddWidget(root);
    auto child = MakeDock("child");
    root->AddChild(child);

    auto wnd = MakeDockable("inspector");
    wnd->PlaceDock(child);

    WindowsLayout::WindowDockPlaceInfo info;
    info.RetrieveLayout(root);

    ASSERT_EQ(info.childs.Count(), 1);
    ASSERT_EQ(info.childs[0].windows.Count(), 1);
    EXPECT_EQ(info.childs[0].windows[0], "inspector");
}

TEST_F(WindowsLayoutRetrieve, ActiveTab_RecordedInActiveField)
{
    auto root = MakeDock("root");
    EditorUIRoot.AddWidget(root);
    auto child = MakeDock("child");
    root->AddChild(child);

    auto a = MakeDockable("a");
    auto b = MakeDockable("b");
    auto c = MakeDockable("c");
    a->PlaceDock(child);
    b->PlaceDock(child);
    c->PlaceDock(child);

    child->SetActiveTab(b);

    WindowsLayout::WindowDockPlaceInfo info;
    info.RetrieveLayout(root);

    ASSERT_EQ(info.childs.Count(), 1);
    EXPECT_EQ(info.childs[0].active, "b");
    EXPECT_EQ(info.childs[0].windows.Count(), 3);
}

TEST_F(WindowsLayoutRetrieve, NestedSplit_RecordsTree)
{
    auto root = MakeDock("root");
    EditorUIRoot.AddWidget(root);

    auto leftCol = MakeDock("left");
    leftCol->layout->anchorMin = Vec2F(0.0f, 0.0f);
    leftCol->layout->anchorMax = Vec2F(0.3f, 1.0f);

    auto rightCol = MakeDock("right");
    rightCol->layout->anchorMin = Vec2F(0.3f, 0.0f);
    rightCol->layout->anchorMax = Vec2F(1.0f, 1.0f);

    root->AddChild(leftCol);
    root->AddChild(rightCol);

    auto leftWnd = MakeDockable("tree");
    leftWnd->PlaceDock(leftCol);
    auto rightWnd = MakeDockable("scene");
    rightWnd->PlaceDock(rightCol);

    WindowsLayout::WindowDockPlaceInfo info;
    info.RetrieveLayout(root);

    ASSERT_EQ(info.childs.Count(), 2);
    EXPECT_EQ(info.childs[0].windows.Count(), 1);
    EXPECT_EQ(info.childs[0].windows[0], "tree");
    EXPECT_EQ(info.childs[1].windows.Count(), 1);
    EXPECT_EQ(info.childs[1].windows[0], "scene");
}

TEST_F(WindowsLayoutRetrieve, DisabledDockedWindow_Skipped)
{
    auto root = MakeDock("root");
    EditorUIRoot.AddWidget(root);
    auto child = MakeDock("child");
    root->AddChild(child);

    auto a = MakeDockable("visible");
    auto b = MakeDockable("hidden");
    a->PlaceDock(child);
    b->PlaceDock(child);

    b->SetEnabled(false);

    WindowsLayout::WindowDockPlaceInfo info;
    info.RetrieveLayout(root);

    ASSERT_EQ(info.childs.Count(), 1);
    EXPECT_EQ(info.childs[0].windows.Count(), 1);
    EXPECT_EQ(info.childs[0].windows[0], "visible");
}

TEST_F(WindowsLayoutRetrieve, NonDockableChildWidget_Ignored)
{
    auto root = MakeDock("root");
    EditorUIRoot.AddWidget(root);

    auto plainWidget = mmake<Widget>();
    plainWidget->name = "plain";
    root->AddChild(plainWidget);

    WindowsLayout::WindowDockPlaceInfo info;
    info.RetrieveLayout(root);

    EXPECT_TRUE(info.windows.IsEmpty());
    EXPECT_TRUE(info.childs.IsEmpty());
}

TEST_F(WindowsLayoutRetrieve, FullSnapshot_TopLevelWrapper)
{
    auto root = MakeDock("root");
    EditorUIRoot.AddWidget(root);

    auto child = MakeDock("child");
    root->AddChild(child);
    auto wnd = MakeDockable("inspector");
    wnd->PlaceDock(child);

    WindowsLayout layout;
    layout.mainDock.RetrieveLayout(root);

    ASSERT_EQ(layout.mainDock.childs.Count(), 1);
    EXPECT_EQ(layout.mainDock.childs[0].windows[0], "inspector");
}

TEST_F(WindowsLayoutRetrieve, Snapshot_RoundTripsViaSerialization)
{
    auto root = MakeDock("root");
    EditorUIRoot.AddWidget(root);

    auto leftCol = MakeDock("left");
    leftCol->layout->anchorMin = Vec2F(0.0f, 0.0f);
    leftCol->layout->anchorMax = Vec2F(0.3f, 1.0f);
    auto rightCol = MakeDock("right");
    rightCol->layout->anchorMin = Vec2F(0.3f, 0.0f);
    rightCol->layout->anchorMax = Vec2F(1.0f, 1.0f);
    root->AddChild(leftCol);
    root->AddChild(rightCol);

    auto a = MakeDockable("tree");
    auto b = MakeDockable("assets");
    a->PlaceDock(leftCol);
    b->PlaceDock(leftCol);
    auto c = MakeDockable("scene");
    c->PlaceDock(rightCol);

    WindowsLayout snapshot;
    snapshot.mainDock.RetrieveLayout(root);

    DataDocument doc;
    doc = snapshot;

    WindowsLayout restored;
    restored = doc;

    EXPECT_TRUE(snapshot == restored);
    ASSERT_EQ(restored.mainDock.childs.Count(), 2);
    EXPECT_EQ(restored.mainDock.childs[0].windows.Count(), 2);
    EXPECT_EQ(restored.mainDock.childs[1].windows[0], "scene");
}
