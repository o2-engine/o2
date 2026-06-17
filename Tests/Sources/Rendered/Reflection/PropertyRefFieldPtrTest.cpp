#include "o2/stdafx.h"

#include <gtest/gtest.h>

#include "o2/Render/Sprite.h"
#include "o2/Scene/UI/WidgetLayer.h"
#include "o2/Utils/Reflection/FieldInfo.h"
#include "o2/Utils/Reflection/Type.h"
#include "o2/Utils/Serialization/DataValue.h"

using namespace o2;

// A property whose value is a Ref<> (e.g. WidgetLayer::drawable = PROPERTY(Ref<IRectDrawable>, ...))
// must resolve nested field paths on Undo/Redo: TPropertyType::GetFieldPtr unwraps the Ref through
// its ReferenceType, like a raw-pointer property is unwrapped. Pre-fix it returned nullptr for any
// Ref<> property, so "drawable/transparency" never resolved.

TEST(PropertyRefFieldPtr, ResolvesNestedPathThroughRefProperty)
{
    auto layer = mmake<WidgetLayer>();
    auto sprite = mmake<Sprite>();
    sprite->SetTransparency(0.3f);
    layer->SetDrawable(sprite);

    const FieldInfo* fi = nullptr;
    void* ptr = GetTypeOf<WidgetLayer>().GetFieldPtr(layer.Get(), "drawable/transparency", fi);

    ASSERT_NE(ptr, nullptr) << "GetFieldPtr must resolve through the Ref<IRectDrawable> 'drawable' property.";
    ASSERT_NE(fi, nullptr);

    DataDocument d; d = 0.7f;
    fi->Deserialize(ptr, d);
    EXPECT_NEAR(sprite->GetTransparency(), 0.7f, 0.01f) // alpha is stored as a byte
        << "Writing through the resolved field ptr must change the drawable's transparency.";
}

TEST(PropertyRefFieldPtr, ResolvesEnabledThroughRefProperty)
{
    auto layer = mmake<WidgetLayer>();
    auto sprite = mmake<Sprite>();
    sprite->SetEnabled(true);
    layer->SetDrawable(sprite);

    const FieldInfo* fi = nullptr;
    void* ptr = GetTypeOf<WidgetLayer>().GetFieldPtr(layer.Get(), "drawable/enabled", fi);

    ASSERT_NE(ptr, nullptr);
    DataDocument d; d = false;
    fi->Deserialize(ptr, d);
    EXPECT_FALSE(sprite->IsEnabled());
}
