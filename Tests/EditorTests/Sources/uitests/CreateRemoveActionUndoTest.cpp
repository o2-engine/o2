#include "o2Editor/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Render/Sprite.h"
#include "o2/Scene/Actor.h"
#include "o2/Scene/Components/EditorTestComponent.h"
#include "o2/Scene/Scene.h"
#include "o2/Scene/UI/UIManager.h"
#include "o2/Scene/UI/Widgets/Button.h"
#include "o2/Scene/UI/Widgets/VerticalLayout.h"
#include "o2/Render/IRectDrawable.h"
#include "o2/Utils/Reflection/Type.h"
#include "o2Editor/Actions/PropertyChange.h"
#include "o2Editor/Properties/Basic/ObjectPtrProperty.h"
#include "o2Editor/Properties/IObjectPropertiesViewer.h"
#include "o2Editor/Properties/Properties.h"
#include "o2Editor/Properties/PropertiesContext.h"
#include "support/EditorTestScene.h"

using namespace o2;
using namespace Editor;
using namespace Editor::Tests;

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

    void ExpectButton(const Ref<ObjectPtrProperty>& field, const String& caption, const String& stage)
    {
        auto btn = DynamicCast<Button>(FindByNameDeep(field, "createOrRemove"));
        ASSERT_NE(btn, nullptr) << "create/remove button missing at: " << stage.Data();
        EXPECT_EQ((String)btn->GetCaption(), caption) << "at: " << stage.Data();
        EXPECT_TRUE(btn->IsEnabled()) << "button hidden at: " << stage.Data();
        if (caption == "Create")
        {
            auto header = FindByNameDeep(field, "header");
            ASSERT_NE(header, nullptr) << "at: " << stage.Data();
            EXPECT_TRUE(header->IsDrawingDepthInheritedFromParent())
                << "header draws at wrong depth (invisible) at: " << stage.Data();
        }
    }

    class TestObjectPtrProperty : public ObjectPtrProperty
    {
    public:
        TestObjectPtrProperty(RefCounter* refCounter): ObjectPtrProperty(refCounter) {}
        using ObjectPtrProperty::CreateObject;
    };
}

// Remove an object through the real action (PropertyChangeAction), then undo/redo, checking the button stays correct.
TEST(CreateRemoveActionUndoUI, RemoveThroughActionThenUndo)
{
    SceneCleanGuard guard;
    auto actor = MakeActor();
    auto comp = mmake<EditorTestComponent>();
    actor->AddComponent(comp);
    comp->mDrawable = mmake<Sprite>();
    TickScene();

    auto& type = dynamic_cast<const ObjectType&>(comp->GetType());
    void* realObj = type.DynamicCastFromIObject(dynamic_cast<IObject*>(comp.Get()));
    auto fi = type.GetField("mDrawable");
    const String fullPath = "component/" + String(type.GetName()) + "/mDrawable";

    auto context = mmake<PropertiesContext>();
    auto layout = o2UI.CreateWidget<VerticalLayout>();
    auto field = DynamicCast<ObjectPtrProperty>(
        o2EditorProperties.BuildField(layout, type, "mDrawable", "", context));
    ASSERT_NE(field, nullptr);

    Ref<PropertyChangeAction> action;
    field->onChangeCompleted = [&](const String&, const Vector<DataDocument>& before, const Vector<DataDocument>& after)
    {
        action = mmake<PropertyChangeAction>(AsEditable({ actor }), fullPath, before, after);
        action->Redo();
    };

    auto rebind = [&]
    {
        field->SetValueProxy(Vector<Ref<IAbstractValueProxy>>{ fi->GetType()->GetValueProxy(fi->GetValuePtrStrong(realObj)) });
        field->Refresh();
    };

    rebind();
    ExpectButton(field, "Delete", "object present");

    // remove through the widget's button -> fires onChangeCompleted -> builds + redoes the action
    DynamicCast<Button>(FindByNameDeep(field, "createOrRemove"))->onClick();
    ASSERT_NE(action, nullptr) << "removal did not produce an action";
    EXPECT_EQ(comp->mDrawable, nullptr) << "removal must clear the field";
    ExpectButton(field, "Create", "after remove through action");

    // undo the removal -> object restored; property window refreshes the field
    action->Undo();
    EXPECT_NE(comp->mDrawable, nullptr) << "undo must restore the object";
    rebind();
    ExpectButton(field, "Delete", "after undo of remove");

    // redo
    action->Redo();
    EXPECT_EQ(comp->mDrawable, nullptr);
    rebind();
    ExpectButton(field, "Create", "after redo of remove");
}

// Create an object through the real action (CreateObject), then undo: the Create button must return.
TEST(CreateRemoveActionUndoUI, CreateThroughActionThenUndo)
{
    SceneCleanGuard guard;
    auto actor = MakeActor();
    auto comp = mmake<EditorTestComponent>();
    actor->AddComponent(comp);
    comp->mDrawable = nullptr;
    TickScene();

    auto& type = dynamic_cast<const ObjectType&>(comp->GetType());
    void* realObj = type.DynamicCastFromIObject(dynamic_cast<IObject*>(comp.Get()));
    auto fi = type.GetField("mDrawable");
    auto baseType = dynamic_cast<const ObjectType*>(dynamic_cast<const ReferenceType*>(fi->GetType())->GetBaseType());
    const String fullPath = "component/" + String(type.GetName()) + "/mDrawable";

    auto layout = o2UI.CreateWidget<VerticalLayout>();
    auto field = mmake<TestObjectPtrProperty>();
    layout->AddChild(field);
    field->SetCaption("mDrawable");
    field->SetBasicType(baseType);
    field->SetFieldInfo(fi);

    Ref<PropertyChangeAction> action;
    field->onChangeCompleted = [&](const String&, const Vector<DataDocument>& before, const Vector<DataDocument>& after)
    {
        action = mmake<PropertyChangeAction>(AsEditable({ actor }), fullPath, before, after);
        action->Redo();
    };

    auto rebind = [&]
    {
        field->SetValueProxy(Vector<Ref<IAbstractValueProxy>>{ fi->GetType()->GetValueProxy(fi->GetValuePtrStrong(realObj)) });
        field->Refresh();
    };

    rebind();
    ExpectButton(field, "Create", "object null");

    field->CreateObject(dynamic_cast<const ObjectType*>(&TypeOf(Sprite)));
    ASSERT_NE(action, nullptr) << "create did not produce an action";
    EXPECT_NE(comp->mDrawable, nullptr) << "create must set the field";
    ExpectButton(field, "Delete", "after create through action");

    action->Undo();
    EXPECT_EQ(comp->mDrawable, nullptr) << "undo must clear the created object";
    field->Refresh();
    ExpectButton(field, "Create", "after undo of create");
}

// Same create -> undo, but refreshed through the real component PropertiesContext (full nesting).
TEST(CreateRemoveActionUndoUI, CreateUndoThroughComponentContext)
{
    SceneCleanGuard guard;
    auto actor = MakeActor();
    auto comp = mmake<EditorTestComponent>();
    actor->AddComponent(comp);
    comp->mDrawable = nullptr;
    TickScene();

    auto& type = dynamic_cast<const ObjectType&>(comp->GetType());
    const String fullPath = "component/" + String(type.GetName()) + "/mDrawable";

    auto context = mmake<PropertiesContext>();
    auto layout = o2UI.CreateWidget<VerticalLayout>();
    o2EditorProperties.BuildObjectProperties(layout, &type, context, "",
        IPropertyField::OnChangeCompletedFunc::empty, IPropertyField::OnChangedFunc::empty);

    Vector<Pair<IObject*, IObject*>> targets{ { dynamic_cast<IObject*>(comp.Get()), nullptr } };
    context->Set(targets);

    Ref<ObjectPtrProperty> field;
    for (auto& kv : context->properties)
        if (kv.first->GetName() == "mDrawable")
            field = DynamicCast<ObjectPtrProperty>(kv.second);
    ASSERT_NE(field, nullptr);
    ExpectButton(field, "Create", "object null");

    comp->mDrawable = mmake<Sprite>();
    DataDocument before;
    DataDocument after; after = dynamic_cast<IObject*>(comp->mDrawable.Get());
    auto action = mmake<PropertyChangeAction>(AsEditable({ actor }), fullPath,
                                              Vector<DataDocument>{ before }, Vector<DataDocument>{ after });
    context->Set(targets);
    ExpectButton(field, "Delete", "after create");

    action->Undo();
    EXPECT_EQ(comp->mDrawable, nullptr);
    context->Set(targets);
    ExpectButton(field, "Create", "after undo of create");
}
