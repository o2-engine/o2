#include "o2Editor/stdafx.h"
#include <gtest/gtest.h>

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

// @DONT_DELETE must hide the Delete button on a Ref<Sprite> property even when it has no header.
TEST(DontDeleteRefPropertyUI, NoHeaderRefPropertyHidesDeleteButton)
{
    auto comp = mmake<EditorTestComponent>();
    auto& type = dynamic_cast<const ObjectType&>(comp->GetType());
    void* realObj = type.DynamicCastFromIObject(dynamic_cast<IObject*>(comp.Get()));

    auto context = mmake<PropertiesContext>();
    auto layout = o2UI.CreateWidget<VerticalLayout>();
    auto field = o2EditorProperties.BuildField(layout, type, "spritePropPtr", "", context);
    ASSERT_NE(field, nullptr);

    auto fi = type.GetField("spritePropPtr");
    auto proxy = fi->GetType()->GetValueProxy(fi->GetValuePtr(realObj));
    field->SetValueProxy(Vector<Ref<IAbstractValueProxy>>{ proxy });
    field->Refresh();

    auto btn = DynamicCast<Button>(FindByNameDeep(field, "createOrRemove"));
    ASSERT_NE(btn, nullptr) << "create/remove button not found";
    EXPECT_EQ((String)btn->GetCaption(), String("Delete")) << "object is non-null, so it's the Delete state";
    EXPECT_FALSE(btn->IsEnabled()) << "@DONT_DELETE must hide the Delete button even without a header";
}
