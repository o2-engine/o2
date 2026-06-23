#include "o2Editor/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Scene/Components/EditorTestComponent.h"
#include "o2/Utils/Reflection/Type.h"

using namespace o2;

TEST(VectorElementPath, GetFieldPtrResolvesNestedFloat)
{
    auto comp = mmake<EditorTestComponent>();
    comp->mTestInsideVector.Add(EditorTestComponent::TestInside());

    auto& type = dynamic_cast<const ObjectType&>(comp->GetType());
    void* obj = type.DynamicCastFromIObject(dynamic_cast<IObject*>(comp.Get()));

    const FieldInfo* fi = nullptr;
    void* ptr = type.GetFieldPtr(obj, "mTestInsideVector/0/mFloat", fi);

    ASSERT_NE(fi, nullptr);
    ASSERT_NE(ptr, nullptr);
    EXPECT_NEAR(*(float*)ptr, 1.2f, 1e-4f);
}
