#include "o2/stdafx.h"

#include <gtest/gtest.h>

#include "o2/Scene/UI/UIManager.h"
#include "o2/Scene/UI/Widget.h"
#include "o2/Scene/UI/Widgets/Button.h"
#include "o2/Scene/UI/Widgets/HorizontalLayout.h"
#include "o2/Scene/UI/Widgets/Image.h"
#include "o2/Scene/UI/Widgets/Label.h"
#include "o2/Scene/UI/Widgets/Toggle.h"
#include "o2/Scene/UI/Widgets/VerticalLayout.h"
#include "tests/Scene/SceneTestHelpers.h"
#include "tests/Scene/UI/UITestHelpers.h"

using namespace o2;

// ===== Singleton =====

TEST(UIManager, SingletonInstanceIsAvailable)
{
    EXPECT_TRUE(UIManager::IsSingletonInitialzed());
}

// ===== Create widgets without style =====

TEST(UIManager, CreateWidgetByTypeReturnsNonNullWidget)
{
    SceneCleanGuard guard;
    auto w = o2UI.CreateWidget<Widget>("non_existing_style_42");
    ASSERT_TRUE(w);
    EXPECT_NE(w->GetType().GetName().find("Widget"), std::string::npos);
}

TEST(UIManager, CreateButtonReturnsButton)
{
    SceneCleanGuard guard;
    auto b = o2UI.CreateButton("press me");
    ASSERT_TRUE(b);
}

TEST(UIManager, CreateLabelReturnsLabel)
{
    SceneCleanGuard guard;
    auto l = o2UI.CreateLabel("hello");
    ASSERT_TRUE(l);
}

TEST(UIManager, CreateToggleReturnsToggle)
{
    SceneCleanGuard guard;
    auto t = o2UI.CreateToggle("on/off");
    ASSERT_TRUE(t);
}

TEST(UIManager, CreateHorLayoutReturnsHorizontalLayout)
{
    SceneCleanGuard guard;
    auto layout = o2UI.CreateHorLayout();
    ASSERT_TRUE(layout);
}

TEST(UIManager, CreateVerLayoutReturnsVerticalLayout)
{
    SceneCleanGuard guard;
    auto layout = o2UI.CreateVerLayout();
    ASSERT_TRUE(layout);
}

// ===== Style management =====

TEST(UIManager, AddWidgetStyleAndGetByType)
{
    auto styleSample = mmake<Widget>();
    styleSample->SetName("test_style_unique_42");
    UIStyleGuard<Widget> styleGuard(styleSample, "test_style_unique_42");

    auto found = o2UI.GetWidgetStyle<Widget>("test_style_unique_42");
    ASSERT_TRUE(found);
    EXPECT_EQ(found->GetName(), "test_style_unique_42");
}

TEST(UIManager, GetWidgetStyleForUnknownReturnsNull)
{
    auto found = o2UI.GetWidgetStyle<Button>("definitely_unknown_style_99");
    EXPECT_FALSE(found);
}

TEST(UIManager, RemoveWidgetStyleRemoves)
{
    auto sample = mmake<Toggle>();
    o2UI.AddWidgetStyle(sample, "remove_me_style");
    EXPECT_TRUE(o2UI.GetWidgetStyle<Toggle>("remove_me_style"));
    o2UI.RemoveWidgetStyle<Toggle>("remove_me_style");
    EXPECT_FALSE(o2UI.GetWidgetStyle<Toggle>("remove_me_style"));
}

TEST(UIManager, UIStyleGuardRemovesStyleOnScopeExit)
{
    auto sample = mmake<Widget>();
    sample->SetName("scoped_style_sample");
    {
        UIStyleGuard<Widget> guard(sample, "scoped_style_sample");
        EXPECT_TRUE(o2UI.GetWidgetStyle<Widget>("scoped_style_sample"));
    }
    EXPECT_FALSE(o2UI.GetWidgetStyle<Widget>("scoped_style_sample"));
}

// ===== Focus =====

TEST(UIManager, FocusWidgetSetsFocusedWidget)
{
    SceneCleanGuard guard;
    auto w = mmake<Widget>(ActorCreateMode::InScene);
    w->SetFocusable(true);
    TickFrame();
    o2UI.FocusWidget(w);
    EXPECT_EQ(o2UI.GetFocusedWidget(), w);
}

TEST(UIManager, FocusNullClearsFocusedWidget)
{
    SceneCleanGuard guard;
    auto w = mmake<Widget>(ActorCreateMode::InScene);
    w->SetFocusable(true);
    TickFrame();
    o2UI.FocusWidget(w);
    o2UI.FocusWidget(nullptr);
    EXPECT_FALSE(o2UI.GetFocusedWidget());
}

TEST(UIManager, FocusedWidgetReceivesOnFocusedCallback)
{
    SceneCleanGuard guard;
    auto w = mmake<Widget>(ActorCreateMode::InScene);
    w->SetFocusable(true);
    WidgetEventCounter counter;
    AttachEventCounter(w, &counter);
    TickFrame();

    o2UI.FocusWidget(w);
    // OnFocused/OnUnfocused fire on the next UIManager::Update tick.
    o2UI.Update();
    EXPECT_GE(counter.onFocusedCount, 1);
    EXPECT_TRUE(w->IsFocused());

    o2UI.FocusWidget(nullptr);
    o2UI.Update();
    EXPECT_GE(counter.onUnfocusedCount, 1);
    EXPECT_FALSE(w->IsFocused());
}

TEST(UIManager, FocusingNewWidgetUnfocusesPrevious)
{
    SceneCleanGuard guard;
    auto a = mmake<Widget>(ActorCreateMode::InScene);
    auto b = mmake<Widget>(ActorCreateMode::InScene);
    a->SetFocusable(true);
    b->SetFocusable(true);
    TickFrame();

    o2UI.FocusWidget(a);
    EXPECT_TRUE(a->IsFocused());

    o2UI.FocusWidget(b);
    EXPECT_FALSE(a->IsFocused());
    EXPECT_TRUE(b->IsFocused());
    EXPECT_EQ(o2UI.GetFocusedWidget(), b);
}
