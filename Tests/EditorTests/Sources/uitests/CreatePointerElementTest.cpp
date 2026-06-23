#include "o2Editor/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Scene/Components/EditorTestComponent.h"
#include "o2/Scene/UI/UIManager.h"
#include "o2/Scene/UI/Widgets/VerticalLayout.h"
#include "o2/Utils/Reflection/Type.h"
#include "o2Editor/Properties/Basic/ObjectPtrProperty.h"
#include "o2Editor/Properties/IObjectPropertiesViewer.h"

using namespace o2;
using namespace Editor;

namespace
{
    class TestObjPtr : public ObjectPtrProperty
    {
    public:
        TestObjPtr(RefCounter* refCounter): ObjectPtrProperty(refCounter) {}
        using ObjectPtrProperty::CreateObject;
    };
}

// Creating an object in a raw-pointer vector element must keep it alive (the container owns it),
// not free it with a temporary Ref and leave a dangling pointer.
TEST(CreatePointerElementUI, CreateObjectInRawPointerVectorNoDangling)
{
    auto comp = mmake<EditorTestComponent>();
    auto& type = dynamic_cast<const ObjectType&>(comp->GetType());
    void* realObj = type.DynamicCastFromIObject(dynamic_cast<IObject*>(comp.Get()));

    auto fi = type.GetField("mTestInsideptrsVector");
    ASSERT_NE(fi, nullptr);
    auto vectorType = dynamic_cast<const VectorType*>(fi->GetType());
    ASSERT_NE(vectorType, nullptr);
    void* vectorData = fi->GetValuePtr(realObj);

    comp->mTestInsideptrsVector.Add(nullptr);
    auto elemProxy = vectorType->GetObjectVectorElementProxy(vectorData, 0);

    auto insideType = dynamic_cast<const ObjectType*>(&TypeOf(EditorTestComponent::TestInside));
    auto layout = o2UI.CreateWidget<VerticalLayout>();
    auto field = mmake<TestObjPtr>();
    layout->AddChild(field);
    field->SetBasicType(insideType);
    field->SetValueProxy(Vector<Ref<IAbstractValueProxy>>{ elemProxy });

    field->CreateObject(insideType);

    auto* elem = comp->mTestInsideptrsVector[0];
    ASSERT_NE(elem, nullptr);
    EXPECT_EQ(&dynamic_cast<IObject*>(elem)->GetType(), insideType) << "created element is dangling/freed";
}
