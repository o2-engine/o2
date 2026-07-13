#include "o2Editor/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Scene/UI/UIManager.h"
#include "o2/Scene/UI/Widgets/EditBox.h"
#include "o2/Utils/Math/Vector3.h"
#include "o2Editor/Properties/Basic/FloatProperty.h"
#include "o2Editor/Properties/Basic/Vector3FloatProperty.h"
#include "o2Editor/Properties/Properties.h"

using namespace o2;
using namespace Editor;

TEST(Vec3FPropertyUI, RegisteredForVec3FType)
{
    EXPECT_EQ(o2EditorProperties.GetFieldPropertyType(&TypeOf(Vec3F)), &TypeOf(Vec3FProperty));
    EXPECT_EQ(Vec3FProperty::GetValueTypeStatic(), &TypeOf(Vec3F));
}

TEST(Vec3FPropertyUI, RefreshShowsTargetValueInSubFields)
{
    auto field = DynamicCast<Vec3FProperty>(o2EditorProperties.CreateRegularField(&TypeOf(Vec3F), "test"));
    ASSERT_NE(field, nullptr);

    Vec3F target(1.0f, 2.0f, 3.0f);
    field->SetValuePointers<Vec3F>({ &target });
    field->Refresh();

    EXPECT_FLOAT_EQ(field->GetXProperty()->GetCommonValue(), 1.0f);
    EXPECT_FLOAT_EQ(field->GetYProperty()->GetCommonValue(), 2.0f);
    EXPECT_FLOAT_EQ(field->GetZProperty()->GetCommonValue(), 3.0f);
    EXPECT_FALSE(field->IsValuesDifferent());
}

TEST(Vec3FPropertyUI, SubFieldEditWritesBackToTarget)
{
    auto field = DynamicCast<Vec3FProperty>(o2EditorProperties.CreateRegularField(&TypeOf(Vec3F), "test"));
    ASSERT_NE(field, nullptr);

    Vec3F target(1.0f, 2.0f, 3.0f);
    field->SetValuePointers<Vec3F>({ &target });
    field->Refresh();

    field->GetZProperty()->SetValue(5.0f);
    EXPECT_FLOAT_EQ(target.z, 5.0f);
    EXPECT_FLOAT_EQ(target.x, 1.0f);
    EXPECT_FLOAT_EQ(target.y, 2.0f);

    field->SetValue(Vec3F(7.0f, 8.0f, 9.0f));
    EXPECT_FLOAT_EQ(target.x, 7.0f);
    EXPECT_FLOAT_EQ(target.y, 8.0f);
    EXPECT_FLOAT_EQ(target.z, 9.0f);
}

TEST(Vec3FPropertyUI, UserEditFiresChangeCompletedWithComponentPath)
{
    auto field = DynamicCast<Vec3FProperty>(o2EditorProperties.CreateRegularField(&TypeOf(Vec3F), "test"));
    ASSERT_NE(field, nullptr);

    Vec3F target(1.0f, 2.0f, 3.0f);
    field->SetValuePointers<Vec3F>({ &target });
    field->SetValuePath("value");
    field->Refresh();

    int completed = 0;
    String path;
    field->onChangeCompleted = [&](const String& p, const Vector<DataDocument>&, const Vector<DataDocument>&) {
        completed++;
        path = p;
    };

    field->GetZProperty()->GetEditBox()->onChangeCompleted("10");

    EXPECT_EQ(completed, 1);
    EXPECT_EQ(path, String("value/z"));
    EXPECT_FLOAT_EQ(target.z, 10.0f);
}
