#include "o2/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Scene/UI/UIManager.h"
#include "o2/Scene/UI/Widgets/Spoiler.h"

using namespace o2;

// smoke: with editor styles loaded (UITestsMain), a styled editor widget actually builds
TEST(EditorUiSmoke, CreatesStyledSpoiler)
{
    EXPECT_TRUE(o2UI.GetWidgetStyle<Spoiler>("expand with caption") != nullptr) << "editor styles must be loaded";

    auto spoiler = o2UI.CreateWidget<Spoiler>("expand with caption");
    ASSERT_TRUE(spoiler != nullptr);
    EXPECT_GT(spoiler->GetLayers().Count() + spoiler->GetChildWidgets().Count(), 0)
        << "a styled spoiler must have real structure, not be an empty fallback widget";
}
