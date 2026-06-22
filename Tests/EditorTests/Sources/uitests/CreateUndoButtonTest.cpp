#include "o2Editor/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Render/Sprite.h"
#include "o2/Scene/Components/EditorTestComponent.h"
#include "o2/Scene/UI/UIManager.h"
#include "o2/Scene/UI/Widgets/Button.h"
#include "o2/Scene/UI/Widgets/VerticalLayout.h"
#include "o2Editor/Properties/Basic/ObjectPtrProperty.h"
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

namespace
{
    void ExpectButton(const Ref<ObjectPtrProperty>& field, const String& caption, const String& stage)
    {
        auto btn = DynamicCast<Button>(FindByNameDeep(field, "createOrRemove"));
        ASSERT_NE(btn, nullptr) << "create/remove button missing at: " << stage.Data();
        EXPECT_EQ((String)btn->GetCaption(), caption) << "at: " << stage.Data();
        EXPECT_TRUE(btn->IsEnabled()) << "button hidden at: " << stage.Data();
    }
}

// Create an object then undo (clear the field + refresh), repeatedly: the Create button must stay present.
TEST(CreateUndoButtonUI, RefreshAfterExternalClearRestoresCreateButton)
{
    auto comp = mmake<EditorTestComponent>();
    auto& type = dynamic_cast<const ObjectType&>(comp->GetType());
    void* realObj = type.DynamicCastFromIObject(dynamic_cast<IObject*>(comp.Get()));

    auto context = mmake<PropertiesContext>();
    auto layout = o2UI.CreateWidget<VerticalLayout>();
    auto field = DynamicCast<ObjectPtrProperty>(
        o2EditorProperties.BuildField(layout, type, "mDrawable", "", context));
    ASSERT_NE(field, nullptr);

    auto fi = type.GetField("mDrawable");
    auto proxy = fi->GetType()->GetValueProxy(fi->GetValuePtr(realObj));
    field->SetValueProxy(Vector<Ref<IAbstractValueProxy>>{ proxy });
    field->Refresh();
    ExpectButton(field, "Create", "initial null");

    for (int cycle = 0; cycle < 3; cycle++)
    {
        comp->mDrawable = mmake<Sprite>();
        field->Refresh();
        ExpectButton(field, "Delete", "after create");

        comp->mDrawable = nullptr;
        field->Refresh();
        ExpectButton(field, "Create", "after undo");
    }
}

// Reproduces the editor flow where the property is pooled (OnFreeProperty) after holding a viewer, then reused:
// the create/remove button must survive.
TEST(CreateUndoButtonUI, ReusedAfterFreeKeepsCreateButton)
{
    auto comp = mmake<EditorTestComponent>();
    auto& type = dynamic_cast<const ObjectType&>(comp->GetType());
    void* realObj = type.DynamicCastFromIObject(dynamic_cast<IObject*>(comp.Get()));
    auto fi = type.GetField("mDrawable");

    auto context = mmake<PropertiesContext>();
    auto layout = o2UI.CreateWidget<VerticalLayout>();
    auto field = DynamicCast<ObjectPtrProperty>(
        o2EditorProperties.BuildField(layout, type, "mDrawable", "", context));
    ASSERT_NE(field, nullptr);

    // object present -> viewer built, header reparented into the spoiler
    comp->mDrawable = mmake<Sprite>();
    field->SetValueProxy(Vector<Ref<IAbstractValueProxy>>{ fi->GetType()->GetValueProxy(fi->GetValuePtr(realObj)) });
    field->Refresh();
    ExpectButton(field, "Delete", "after create");

    // pooled by the properties manager
    o2EditorProperties.FreeProperty(field);

    // undo cleared the field; the pooled property is reused for the same field
    comp->mDrawable = nullptr;
    field->SetFieldInfo(fi);
    field->SetValueProxy(Vector<Ref<IAbstractValueProxy>>{ fi->GetType()->GetValueProxy(fi->GetValuePtr(realObj)) });
    field->Refresh();

    ExpectButton(field, "Create", "after free + reuse");
}
