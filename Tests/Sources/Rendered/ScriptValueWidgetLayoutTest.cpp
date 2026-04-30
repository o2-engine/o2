#include "o2/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Scripts/ScriptValue.h"
#include "o2/Scripts/ScriptEngine.h"

#if IS_SCRIPTING_SUPPORTED
#include "o2/Scene/UI/Widgets/Image.h"
#include "o2/Scene/UI/WidgetLayout.h"
#include "o2/Utils/Math/Vector2.h"

using namespace o2;

// Mirrors Assets/Scripts/Reel.js: image.GetLayout().Set(o2.WidgetLayout.Based("Center", size, offset)).
// Lives in the rendered tier because it depends on UI styles being loaded — without them
// `WidgetLayout.Based` falls back to defaults and the offsets don't match.
TEST(ScriptValue, ReelJsStyleGetLayoutSetWidgetLayout) {
    auto image = mmake<Image>();
    ScriptValue rotatingImage = ScriptValue::EmptyObject();
    rotatingImage.SetProperty("image", image->GetScriptValue());
    o2Scripts.GetGlobal().SetProperty("rotatingImage", rotatingImage);
    o2Scripts.CollectGarbage();

    o2Scripts.Eval(
        "var size = new Vec2(80.0, 40.0);"
        "var imageOffset = 10.0;"
        "rotatingImage.image.GetLayout().Set("
        "    o2.WidgetLayout.Based(\"Center\", size, new Vec2(0.0, imageOffset)));"
    );

    EXPECT_FLOAT_EQ(image->layout->GetAnchorMin().x, 0.5f);
    EXPECT_FLOAT_EQ(image->layout->GetAnchorMin().y, 0.5f);
    EXPECT_FLOAT_EQ(image->layout->GetAnchorMax().x, 0.5f);
    EXPECT_FLOAT_EQ(image->layout->GetAnchorMax().y, 0.5f);
    EXPECT_FLOAT_EQ(image->layout->GetOffsetMin().x, -40.0f);
    EXPECT_FLOAT_EQ(image->layout->GetOffsetMin().y, -10.0f);
    EXPECT_FLOAT_EQ(image->layout->GetOffsetMax().x, 40.0f);
    EXPECT_FLOAT_EQ(image->layout->GetOffsetMax().y, 30.0f);
}

#endif // IS_SCRIPTING_SUPPORTED
