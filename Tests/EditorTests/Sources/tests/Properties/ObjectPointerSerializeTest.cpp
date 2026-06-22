#include "o2Editor/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Render/IRectDrawable.h"
#include "o2/Render/Sprite.h"

using namespace o2;

// Serializing an object via its base IObject* (as ObjectPtrProperty::StoreValues does) must keep the
// derived object's data, not write Value: null.
TEST(ObjectPointerSerialize, BasePointerKeepsDerivedData)
{
    auto sprite = mmake<Sprite>();
    sprite->SetTransparency(0.25f);
    IObject* obj = dynamic_cast<IObject*>(sprite.Get());

    DataDocument doc;
    doc = obj;

    auto typeNode = doc.FindMember("Type");
    auto valueNode = doc.FindMember("Value");
    ASSERT_NE(typeNode, nullptr);
    ASSERT_NE(valueNode, nullptr);
    EXPECT_FALSE(valueNode->IsNull()) << "Value must hold the serialized object";

    Ref<IRectDrawable> restored;
    doc.Get(restored);
    ASSERT_NE(restored, nullptr);
    EXPECT_EQ(&restored->GetType(), &TypeOf(Sprite));
    EXPECT_NEAR(restored->GetTransparency(), 0.25f, 0.01f);
}
