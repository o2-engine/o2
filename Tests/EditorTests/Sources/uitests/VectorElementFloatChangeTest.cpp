#include "o2Editor/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Scene/Actor.h"
#include "o2/Scene/Components/EditorTestComponent.h"
#include "o2/Scene/UI/UIManager.h"
#include "o2/Scene/UI/Widgets/EditBox.h"
#include "o2/Scene/UI/Widgets/Spoiler.h"
#include "o2/Scene/UI/Widgets/VerticalLayout.h"
#include "o2Editor/Actions/PropertyChange.h"
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
        if (w->GetName() == name) return w;
        if (auto r = w->FindInternalWidget(name)) return r;
        for (auto& c : w->GetChildWidgets())
            if (auto r = FindByNameDeep(c, name)) return r;
        return nullptr;
    }

    Ref<EditBox> FindEditBoxDeep(const Ref<Widget>& w)
    {
        if (auto e = DynamicCast<EditBox>(w)) return e;
        for (auto& c : w->GetChildWidgets())
            if (auto r = FindEditBoxDeep(c)) return r;
        for (auto& c : w->GetChildren())
            if (auto cw = DynamicCast<Widget>(c))
                if (auto r = FindEditBoxDeep(cw)) return r;
        return nullptr;
    }
}

TEST(VectorElementFloatChangeUI, ChangeNestedFloatDoesNotCrash)
{
    SceneCleanGuard guard;
    auto actor = MakeActor();
    auto comp = mmake<EditorTestComponent>();
    actor->AddComponent(comp);
    comp->mTestInsideVector.Add(EditorTestComponent::TestInside());
    TickScene();

    auto& type = dynamic_cast<const ObjectType&>(comp->GetType());
    const String prefix = "component/" + String(type.GetName()) + "/";

    auto context = mmake<PropertiesContext>();
    auto layout = o2UI.CreateWidget<VerticalLayout>();

    Ref<PropertyChangeAction> action;
    auto field = o2EditorProperties.BuildField(layout, type, "mTestInsideVector", "", context,
        [&](const String& path, const Vector<DataDocument>& before, const Vector<DataDocument>& after) {
            action = mmake<PropertyChangeAction>(AsEditable({ actor }), prefix + path, before, after);
            action->Redo();
        });
    ASSERT_NE(field, nullptr);

    Vector<Pair<IObject*, IObject*>> targets{ { dynamic_cast<IObject*>(comp.Get()), nullptr } };
    context->Set(targets);

    std::function<void(const Ref<Widget>&)> expandAll = [&](const Ref<Widget>& w) {
        if (auto sp = DynamicCast<Spoiler>(w)) sp->SetExpanded(true, true);
        for (auto& c : w->GetChildWidgets()) expandAll(c);
    };
    for (int i = 0; i < 6; i++) { expandAll(field); context->Set(targets); }
    for (int i = 0; i < 6; i++) expandAll(field);

    auto floatField = FindByNameDeep(field, "mFloat : float");
    ASSERT_NE(floatField, nullptr) << "nested mFloat property not found";
    auto editBox = FindEditBoxDeep(floatField);
    ASSERT_NE(editBox, nullptr) << "float edit box not found";

    editBox->onChangeCompleted("5");

    EXPECT_NEAR(comp->mTestInsideVector[0].mFloat, 5.0f, 1e-4f);
}
