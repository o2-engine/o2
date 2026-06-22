#include "o2Editor/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Scene/Components/EditorTestComponent.h"
#include "o2/Utils/Editor/Attributes/DontDeleteAttribute.h"
#include "o2/Utils/Reflection/Type.h"

using namespace o2;

// @DONT_DELETE must reach the property field the same on a PROPERTY as on a plain field.
TEST(DontDeleteOnProperty, PropertyCarriesAttributeLikeField)
{
    auto& type = dynamic_cast<const ObjectType&>(TypeOf(EditorTestComponent));

    auto field = type.GetField("mSprite");      // Ref<Sprite> field, @DONT_DELETE (works)
    auto prop = type.GetField("spriteProp");    // PROPERTY(...), @DONT_DELETE
    ASSERT_NE(field, nullptr);
    ASSERT_NE(prop, nullptr);

    EXPECT_TRUE(field->HasAttribute<DontDeleteAttribute>());
    EXPECT_TRUE(prop->HasAttribute<DontDeleteAttribute>());
}
