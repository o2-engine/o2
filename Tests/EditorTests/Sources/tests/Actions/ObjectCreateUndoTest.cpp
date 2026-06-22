#include "o2Editor/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Render/Sprite.h"
#include "o2/Scene/Components/EditorTestComponent.h"
#include "o2/Utils/Reflection/Type.h"

using namespace o2;

// Undo of "create object" must reset the ref field back to null. The action replays through
// FieldInfo::Deserialize with the captured before-value (a null ref serializes to an empty node).
TEST(ObjectCreateUndo, DeserializeNullResetsRefField)
{
    auto comp = mmake<EditorTestComponent>();
    auto& type = dynamic_cast<const ObjectType&>(comp->GetType());
    void* obj = type.DynamicCastFromIObject(dynamic_cast<IObject*>(comp.Get()));
    auto fi = type.GetField("mDrawable");
    ASSERT_NE(fi, nullptr);

    // before-value captured while mDrawable is null, the same way ObjectPtrProperty::StoreValues does it
    DataDocument before;
    before = (IObject*)comp->mDrawable.Get();

    comp->mDrawable = mmake<Sprite>();
    ASSERT_NE(comp->mDrawable, nullptr);

    fi->Deserialize(fi->GetValuePtr(obj), before);

    EXPECT_EQ(comp->mDrawable, nullptr) << "undo must clear the created object";
}
