#include "o2Editor/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Scene/Actor.h"
#include "o2/Scene/UI/UIManager.h"
#include "o2/Scene/UI/Widget.h"
#include "o2/Scene/UI/Widgets/Spoiler.h"
#include "o2/Utils/Reflection/Type.h"
#include "o2Editor/Properties/IObjectPropertiesViewer.h"
#include "o2Editor/Properties/Properties.h"

using namespace o2;
using namespace Editor;

// Инспектор строит пары «объект + объект прототипа» по типу поля. Если объект
// прототипа не кастится к типу-владельцу поля (несовпавший или битый линк),
// прокси прототипа должен просто отсутствовать, а не строиться вокруг нулевого
// указателя — раньше это роняло редактор при выделении такой ноды
TEST(PrototypeMismatchedLink, ViewerSurvivesWrongPrototypeType)
{
    auto widget = mmake<Widget>(ActorCreateMode::NotInScene);
    widget->SetName("Instance");

    // «прототип» другого типа: к Widget не кастится
    auto plainPrototype = mmake<Actor>(ActorCreateMode::NotInScene);
    plainPrototype->SetName("WrongProto");

    bool wasPrivate = o2EditorProperties.IsPrivateFieldsVisible();
    o2EditorProperties.SetPrivateFieldsVisible(true);

    auto viewer = o2EditorProperties.CreateObjectViewer(&TypeOf(Widget), "");
    ASSERT_TRUE(viewer);

    auto spoiler = o2UI.CreateWidget<Spoiler>();
    viewer->CheckCreateSpoiler(spoiler);
    viewer->SetHeaderEnabled(false); // как в инспекторе актора: поля строятся сразу

    Vector<Pair<IObject*, IObject*>> targets;
    targets.Add(Pair<IObject*, IObject*>(dynamic_cast<IObject*>(widget.Get()),
                                         dynamic_cast<IObject*>(plainPrototype.Get())));

    // до фикса здесь падало в Widget::GetTransparency на нулевом this
    viewer->Refresh(targets);
    viewer->Refresh(targets);

    o2EditorProperties.FreeObjectViewer(viewer);
    o2EditorProperties.SetPrivateFieldsVisible(wasPrivate);
}
