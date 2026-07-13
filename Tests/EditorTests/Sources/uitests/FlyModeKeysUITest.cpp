#include "o2Editor/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Application/Input.h"
#include "o2/Events/EventSystem.h"
#include "o2/Events/ShortcutKeysListener.h"
#include "o2/Scene/UI/WidgetLayout.h"
#include "o2Editor/Windows/SceneWindow/SceneEditScreen.h"
#include "support/EditorTestScene.h"

using namespace o2;
using namespace Editor;
using namespace Editor::Tests;

namespace
{
    void PressAndReleaseKey(KeyboardKey key)
    {
        o2Input.OnKeyPressed(key);
        o2Input.PreUpdate();
        o2Events.Update();

        o2Input.OnKeyReleased(key);
        o2Input.PreUpdate();
        o2Events.Update();
        o2Input.Update(0.016f);
    }
}

// Regression for tool hotkeys firing while flying with RMB+WASD: while fly navigation is active
// no shortcut listener and no tool key handler must react, including the '3' mode toggle
TEST(FlyModeKeys, FlyNavigationSuppressesShortcutsAndToolKeys)
{
    SceneCleanGuard guard;
    auto& screen = o2EditorSceneScreen;

    *screen.layout = WidgetLayout::Based(BaseCorner::LeftBottom, Vec2F(800.0f, 600.0f));
    screen.UpdateSelfTransform();

    screen.SetView3DMode(true);

    // Stands for any tool-switch shortcut
    bool shortcutFired = false;
    auto listener = mmake<FunctionalShortcutKeysListener>([&]() { shortcutFired = true; });
    listener->SetShortcut(ShortcutKeys({ (KeyboardKey)'W' }));

    screen.SetFlyNavigation3D(true);
    EXPECT_TRUE(screen.IsFlyNavigation3D());
    EXPECT_TRUE(ShortcutKeysListenersManager::IsSuppressed());

    PressAndReleaseKey('W');
    EXPECT_FALSE(shortcutFired) << "shortcuts must not fire while flying";

    PressAndReleaseKey('3');
    EXPECT_TRUE(screen.IsView3DMode()) << "the '3' mode toggle must be gated while flying";

    screen.SetFlyNavigation3D(false);
    EXPECT_FALSE(screen.IsFlyNavigation3D());
    EXPECT_FALSE(ShortcutKeysListenersManager::IsSuppressed());

    PressAndReleaseKey('W');
    EXPECT_TRUE(shortcutFired) << "shortcuts work again after fly navigation ends";

    screen.SetView3DMode(false);
}
