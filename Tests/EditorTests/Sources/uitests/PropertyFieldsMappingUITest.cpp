#include "o2Editor/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Scene/Actor.h"
#include "o2/Scene/Component.h"
#include "o2/Utils/Math/AABB.h"
#include "o2/Utils/Math/Basis3D.h"
#include "o2/Utils/Math/Matrix4.h"
#include "o2/Utils/Math/Quaternion.h"
#include "o2/Utils/Math/Vector3.h"
#include "o2Editor/Properties/Properties.h"

using namespace o2;
using namespace Editor;

TEST(PropertyFieldsMapping, New3DTypesAreRealReflectedTypes)
{
    EXPECT_NE(&TypeOf(Vec3F), Type::Dummy::type);
    EXPECT_NE(&TypeOf(Vec3I), Type::Dummy::type);
    EXPECT_NE(&TypeOf(Quat), Type::Dummy::type);
    EXPECT_NE(&TypeOf(Mat4), Type::Dummy::type);
    EXPECT_NE(&TypeOf(Basis3D), Type::Dummy::type);
    EXPECT_NE(&TypeOf(AABB), Type::Dummy::type);
    EXPECT_EQ(TypeOf(Vec3F).GetName(), String("o2::Vec3F"));
    EXPECT_EQ(TypeOf(Basis3D).GetName(), String("o2::Basis3D"));
    EXPECT_EQ(TypeOf(AABB).GetName(), String("o2::AABB"));
}

TEST(PropertyFieldsMapping, NoFieldEditorMappedToUnknownType)
{
    EXPECT_EQ(o2EditorProperties.GetFieldPropertyType(Type::Dummy::type), nullptr);
}

TEST(PropertyFieldsMapping, ServiceActorFieldsHaveNoEditor)
{
    for (auto& fieldName : { "id", "children", "components", "enabledInHierarchy", "lockedInHierarchy" })
    {
        auto field = TypeOf(Actor).GetField(fieldName);
        ASSERT_TRUE(field) << fieldName;
        EXPECT_EQ(o2EditorProperties.GetFieldPropertyType(field->GetType()), nullptr) << fieldName;
    }

    auto actorField = TypeOf(Component).GetField("actor");
    ASSERT_TRUE(actorField);
    EXPECT_EQ(o2EditorProperties.GetFieldPropertyType(actorField->GetType()), nullptr);
}
