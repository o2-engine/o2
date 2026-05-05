#include "o2/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Scene/UI/WidgetLayout.h"
#include "o2/Utils/Serialization/DataValue.h"
#include "o2Editor/Windows/WindowsLayout.h"

using namespace o2;
using namespace Editor;

namespace
{
    using DockInfo = WindowsLayout::WindowDockPlaceInfo;

    DockInfo MakeLeaf(std::initializer_list<String> windows, const String& active = "",
                      const RectF& anchors = { 0.0f, 0.0f, 1.0f, 1.0f })
    {
        DockInfo info;
        info.anchors = anchors;
        info.active = active;
        for (auto& w : windows)
            info.windows.Add(w);
        return info;
    }
}

TEST(WindowsLayoutData, Empty_Equals_Default)
{
    WindowsLayout a, b;
    EXPECT_TRUE(a == b);
    EXPECT_TRUE(a.mainDock.windows.IsEmpty());
    EXPECT_TRUE(a.mainDock.childs.IsEmpty());
    EXPECT_TRUE(a.windows.IsEmpty());
}

TEST(WindowsLayoutData, Equality_DetectsAnchorMismatch)
{
    WindowsLayout a, b;
    a.mainDock.childs.Add(MakeLeaf({ "x" }, "x", { 0.0f, 0.0f, 0.5f, 1.0f }));
    b.mainDock.childs.Add(MakeLeaf({ "x" }, "x", { 0.0f, 0.0f, 0.4f, 1.0f }));
    EXPECT_FALSE(a == b);
}

TEST(WindowsLayoutData, Equality_IgnoresActiveTab)
{
    // active is derived UI state (which tab is currently selected), not layout shape.
    // operator== compares anchors/childs/windows only — see WindowsLayout.cpp.
    WindowsLayout a, b;
    a.mainDock = MakeLeaf({ "x", "y" }, "x");
    b.mainDock = MakeLeaf({ "x", "y" }, "y");
    EXPECT_TRUE(a == b);
}

TEST(WindowsLayoutData, Equality_DetectsWindowOrder)
{
    WindowsLayout a, b;
    a.mainDock = MakeLeaf({ "x", "y" }, "x");
    b.mainDock = MakeLeaf({ "y", "x" }, "x");
    EXPECT_FALSE(a == b);
}

TEST(WindowsLayoutData, Equality_DetectsChildCountMismatch)
{
    WindowsLayout a, b;
    a.mainDock.childs.Add(MakeLeaf({ "x" }));
    b.mainDock.childs.Add(MakeLeaf({ "x" }));
    b.mainDock.childs.Add(MakeLeaf({ "y" }));
    EXPECT_FALSE(a == b);
}

TEST(WindowsLayoutData, Equality_DetectsNonDockedDifference)
{
    WindowsLayout a, b;
    a.windows.Add("floating", WidgetLayout({ 0.0f, 0.0f }, { 0.5f, 0.5f }, { 0, 0 }, { 0, 0 }));
    b.windows.Add("floating", WidgetLayout({ 0.0f, 0.0f }, { 0.6f, 0.5f }, { 0, 0 }, { 0, 0 }));
    EXPECT_FALSE(a == b);
}

TEST(WindowsLayoutData, Serialize_RoundTrip_Empty)
{
    WindowsLayout original;

    DataDocument doc;
    doc = original;

    WindowsLayout restored;
    restored = doc;

    EXPECT_TRUE(original == restored);
}

TEST(WindowsLayoutData, Serialize_RoundTrip_Nested)
{
    WindowsLayout original;
    original.mainDock.anchors = { 0.0f, 0.0f, 1.0f, 1.0f };

    auto leftCol = MakeLeaf({ "tree", "assets" }, "tree", { 0.0f, 0.0f, 0.3f, 1.0f });
    auto rightCol = MakeLeaf({}, "", { 0.3f, 0.0f, 1.0f, 1.0f });
    rightCol.childs.Add(MakeLeaf({ "scene" }, "scene", { 0.0f, 0.3f, 1.0f, 1.0f }));
    rightCol.childs.Add(MakeLeaf({ "log", "console" }, "log", { 0.0f, 0.0f, 1.0f, 0.3f }));

    original.mainDock.childs.Add(leftCol);
    original.mainDock.childs.Add(rightCol);

    DataDocument doc;
    doc = original;

    WindowsLayout restored;
    restored = doc;

    ASSERT_TRUE(original == restored);
    ASSERT_EQ(restored.mainDock.childs.Count(), 2);
    EXPECT_EQ(restored.mainDock.childs[1].childs.Count(), 2);
    EXPECT_EQ(restored.mainDock.childs[1].childs[0].active, "scene");
}

TEST(WindowsLayoutData, Serialize_RoundTrip_NonDockedWindows)
{
    WindowsLayout original;
    original.windows.Add("inspector",
        WidgetLayout({ 0.1f, 0.1f }, { 0.4f, 0.6f }, { 0, 0 }, { 0, 0 }));
    original.windows.Add("hierarchy",
        WidgetLayout({ 0.6f, 0.1f }, { 0.9f, 0.6f }, { 0, 0 }, { 0, 0 }));

    DataDocument doc;
    doc = original;

    WindowsLayout restored;
    restored = doc;

    ASSERT_EQ(restored.windows.Count(), 2);
    EXPECT_TRUE(restored.windows.ContainsKey("inspector"));
    EXPECT_TRUE(restored.windows.ContainsKey("hierarchy"));
    EXPECT_TRUE(original == restored);
}

TEST(WindowsLayoutData, Deserialize_Empty_GivesDefaults)
{
    DataDocument empty;
    WindowsLayout restored;
    restored = empty;

    EXPECT_TRUE(restored.mainDock.windows.IsEmpty());
    EXPECT_TRUE(restored.mainDock.childs.IsEmpty());
    EXPECT_TRUE(restored.windows.IsEmpty());
}

TEST(WindowDockPlaceInfo, Equality_AsymmetricChildSize)
{
    DockInfo a, b;
    a.childs.Add(MakeLeaf({ "x" }));
    b.childs.Add(MakeLeaf({ "x" }));
    b.childs.Add(MakeLeaf({ "y" }));
    EXPECT_FALSE(a == b);
}
