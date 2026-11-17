#include "o2/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Scene/UI/WidgetLayer.h"

using namespace o2;

// Widget layers are drawn as one flat list: a sublayer of a disabled layer must not draw (the play badge of a video
// icon stayed on the recycled icons of the Assets window)
TEST(WidgetLayerParentEnabled, ADisabledParentHidesItsSublayers)
{
    auto badge = mmake<WidgetLayer>();
    auto fill = mmake<WidgetLayer>();
    auto glyph = mmake<WidgetLayer>();
    badge->AddChild(fill);
    fill->AddChild(glyph);

    EXPECT_TRUE(glyph->IsEnabledWithParentLayers());

    badge->SetEnabled(false);
    EXPECT_FALSE(badge->IsEnabledWithParentLayers());
    EXPECT_FALSE(fill->IsEnabledWithParentLayers());
    EXPECT_FALSE(glyph->IsEnabledWithParentLayers());
    EXPECT_TRUE(glyph->IsEnabled());

    badge->SetEnabled(true);
    EXPECT_TRUE(glyph->IsEnabledWithParentLayers());

    glyph->SetEnabled(false);
    EXPECT_FALSE(glyph->IsEnabledWithParentLayers());
    EXPECT_TRUE(fill->IsEnabledWithParentLayers());
}
