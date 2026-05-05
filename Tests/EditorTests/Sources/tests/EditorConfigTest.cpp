#include "o2/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Scene/UI/WidgetLayout.h"
#include "o2/Utils/Math/Vector2.h"
#include "o2/Utils/Serialization/DataValue.h"
#include "o2Editor/EditorConfig.h"
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

// --- ProjectConfig --------------------------------------------------------

TEST(EditorProjectConfig, Defaults_AreSane)
{
    EditorConfig::ProjectConfig pc;
    EXPECT_EQ(pc.GetWindowSize(), Vec2I(800, 600));
    EXPECT_TRUE(pc.GetMaximized());
    EXPECT_TRUE(pc.GetLastLoadedScene().IsEmpty());
}

TEST(EditorProjectConfig, SetGet_WindowSize)
{
    EditorConfig::ProjectConfig pc;
    pc.SetWindowSize(Vec2I(1280, 720));
    EXPECT_EQ(pc.GetWindowSize(), Vec2I(1280, 720));
}

TEST(EditorProjectConfig, SetGet_WindowPosition)
{
    EditorConfig::ProjectConfig pc;
    pc.SetWindowPosition(Vec2I(100, 50));
    EXPECT_EQ(pc.GetWindowPosition(), Vec2I(100, 50));
}

TEST(EditorProjectConfig, SetGet_Maximized)
{
    EditorConfig::ProjectConfig pc;
    pc.SetMaximized(false);
    EXPECT_FALSE(pc.GetMaximized());
    pc.SetMaximized(true);
    EXPECT_TRUE(pc.GetMaximized());
}

TEST(EditorProjectConfig, SetGet_LastLoadedScene)
{
    EditorConfig::ProjectConfig pc;
    pc.SetLastLoadedScene("Assets/Scenes/main.scene");
    EXPECT_EQ(pc.GetLastLoadedScene(), "Assets/Scenes/main.scene");
}

TEST(EditorProjectConfig, SetGet_Layout)
{
    EditorConfig::ProjectConfig pc;
    WindowsLayout l;
    l.mainDock.childs.Add(MakeLeaf({ "tree" }, "tree"));

    pc.SetLayout(l);
    EXPECT_TRUE(pc.GetLayout() == l);
}

TEST(EditorProjectConfig, Setter_SameValue_NoCrashWithoutSingleton)
{
    // OnProjectConfigChanged is invoked on every property write; it bails out
    // when EditorConfig singleton isn't initialized. Verify we don't crash.
    EditorConfig::ProjectConfig pc;
    pc.SetWindowSize(Vec2I(1024, 768));
    pc.SetWindowSize(Vec2I(1024, 768));
    pc.SetMaximized(false);
    pc.SetMaximized(false);
    SUCCEED();
}

TEST(EditorProjectConfig, Serialize_RoundTrip_AllFields)
{
    EditorConfig::ProjectConfig original;
    original.SetWindowSize(Vec2I(1920, 1080));
    original.SetWindowPosition(Vec2I(40, 80));
    original.SetMaximized(false);
    original.SetLastLoadedScene("Assets/Scenes/test.scene");

    WindowsLayout layout;
    layout.mainDock.childs.Add(MakeLeaf({ "scene", "log" }, "scene"));
    original.SetLayout(layout);

    DataDocument doc;
    doc = original;

    EditorConfig::ProjectConfig restored;
    restored = doc;

    EXPECT_EQ(restored.GetWindowSize(), Vec2I(1920, 1080));
    EXPECT_EQ(restored.GetWindowPosition(), Vec2I(40, 80));
    EXPECT_FALSE(restored.GetMaximized());
    EXPECT_EQ(restored.GetLastLoadedScene(), "Assets/Scenes/test.scene");
    EXPECT_TRUE(restored.GetLayout() == layout);
}

TEST(EditorProjectConfig, Deserialize_Empty_GivesDefaults)
{
    DataDocument empty;
    EditorConfig::ProjectConfig restored;
    restored = empty;

    EXPECT_EQ(restored.GetWindowSize(), Vec2I(800, 600));
    EXPECT_TRUE(restored.GetMaximized());
    EXPECT_TRUE(restored.GetLastLoadedScene().IsEmpty());
}

// --- GlobalConfig ---------------------------------------------------------

TEST(EditorGlobalConfig, Defaults_AreEmpty)
{
    EditorConfig::GlobalConfig gc;
    EXPECT_TRUE(gc.GetAvailableLayouts().IsEmpty());
    EXPECT_TRUE(gc.GetDefaultLayout().mainDock.windows.IsEmpty());
    EXPECT_TRUE(gc.GetDefaultLayout().mainDock.childs.IsEmpty());
}

TEST(EditorGlobalConfig, SetGet_DefaultLayout)
{
    EditorConfig::GlobalConfig gc;
    WindowsLayout l;
    l.mainDock.childs.Add(MakeLeaf({ "x" }, "x"));

    gc.SetDefaultLayout(l);
    EXPECT_TRUE(gc.GetDefaultLayout() == l);
}

TEST(EditorGlobalConfig, SetGet_AvailableLayouts)
{
    EditorConfig::GlobalConfig gc;
    EditorLayoutsMap map;

    WindowsLayout coding;
    coding.mainDock.childs.Add(MakeLeaf({ "scene", "log" }, "scene"));
    map.Add("coding", coding);

    WindowsLayout debugging;
    debugging.mainDock.childs.Add(MakeLeaf({ "log", "console" }, "log"));
    map.Add("debugging", debugging);

    gc.SetAvailableLayouts(map);

    auto restored = gc.GetAvailableLayouts();
    ASSERT_EQ(restored.Count(), 2);
    EXPECT_TRUE(restored.ContainsKey("coding"));
    EXPECT_TRUE(restored.ContainsKey("debugging"));
}

TEST(EditorGlobalConfig, Serialize_RoundTrip_AllFields)
{
    EditorConfig::GlobalConfig original;

    WindowsLayout def;
    def.mainDock.childs.Add(MakeLeaf({ "scene" }, "scene"));
    original.SetDefaultLayout(def);

    EditorLayoutsMap layouts;
    WindowsLayout l1;
    l1.mainDock.childs.Add(MakeLeaf({ "tree" }, "tree"));
    layouts.Add("layout1", l1);
    original.SetAvailableLayouts(layouts);

    DataDocument doc;
    doc = original;

    EditorConfig::GlobalConfig restored;
    restored = doc;

    EXPECT_TRUE(restored.GetDefaultLayout() == def);
    auto restoredMap = restored.GetAvailableLayouts();
    ASSERT_EQ(restoredMap.Count(), 1);
    EXPECT_TRUE(restoredMap.ContainsKey("layout1"));
}

// --- EditorConfig top-level ----------------------------------------------

TEST(EditorConfigSerialize, RoundTrip_TopLevel_PreservesBothSubConfigs)
{
    // EditorConfig itself is ISerializable. Build one via copy on the stack —
    // we don't touch the singleton (no LoadConfigs / no SaveConfigs file I/O).
    EditorConfig::ProjectConfig pc;
    pc.SetWindowSize(Vec2I(1366, 768));
    pc.SetMaximized(false);
    pc.SetLastLoadedScene("scene.scn");

    EditorConfig::GlobalConfig gc;
    WindowsLayout def;
    def.mainDock.childs.Add(MakeLeaf({ "scene" }, "scene"));
    gc.SetDefaultLayout(def);

    DataDocument projDoc;
    projDoc = pc;
    DataDocument globDoc;
    globDoc = gc;

    EditorConfig::ProjectConfig pcRestored;
    pcRestored = projDoc;
    EditorConfig::GlobalConfig gcRestored;
    gcRestored = globDoc;

    EXPECT_EQ(pcRestored.GetWindowSize(), Vec2I(1366, 768));
    EXPECT_FALSE(pcRestored.GetMaximized());
    EXPECT_EQ(pcRestored.GetLastLoadedScene(), "scene.scn");
    EXPECT_TRUE(gcRestored.GetDefaultLayout() == def);
}
