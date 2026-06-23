#include "o2Editor/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Assets/Types/AnimationAsset.h"
#include "o2/Scene/Components/EditorTestComponent.h"
#include "o2/Scene/UI/UIManager.h"
#include "o2/Scene/UI/Widgets/Button.h"
#include "o2/Scene/UI/Widgets/VerticalLayout.h"
#include "o2Editor/Properties/Properties.h"
#include "o2Editor/Properties/PropertiesContext.h"

using namespace o2;
using namespace Editor;

namespace
{
    Ref<Widget> FindByNameDeep(const Ref<Widget>& w, const String& name)
    {
        if (w->GetName() == name)
            return w;
        if (auto r = w->FindInternalWidget(name))
            return r;
        for (auto& c : w->GetChildWidgets())
            if (auto r = FindByNameDeep(c, name))
                return r;
        return nullptr;
    }
}

// Creating an asset instance must go through the action system (fire onChangeCompleted), like every other mutation.
TEST(AssetPropertyActionUI, CreateInstanceFiresChangeCompleted)
{
    auto comp = mmake<EditorTestComponent>();
    auto& type = dynamic_cast<const ObjectType&>(comp->GetType());
    void* realObj = type.DynamicCastFromIObject(dynamic_cast<IObject*>(comp.Get()));
    auto fi = type.GetField("mAnimationAsset");
    ASSERT_NE(fi, nullptr);

    auto context = mmake<PropertiesContext>();
    auto layout = o2UI.CreateWidget<VerticalLayout>();
    auto field = o2EditorProperties.BuildField(layout, type, "mAnimationAsset", "", context);
    ASSERT_NE(field, nullptr);

    int completed = 0;
    field->onChangeCompleted = [&](const String&, const Vector<DataDocument>&, const Vector<DataDocument>&) { completed++; };
    field->SetValueProxy(Vector<Ref<IAbstractValueProxy>>{ fi->GetType()->GetValueProxy(fi->GetValuePtrStrong(realObj)) });

    ASSERT_FALSE(comp->mAnimationAsset.IsInstance());

    auto createBtn = DynamicCast<Button>(FindByNameDeep(field, "create"));
    ASSERT_NE(createBtn, nullptr) << "create-instance button not found";
    createBtn->onClick();

    EXPECT_TRUE(comp->mAnimationAsset.IsInstance()) << "instance must be created";
    EXPECT_EQ(completed, 1) << "create instance must fire one onChangeCompleted (an undoable action)";
}
