#include "o2/stdafx.h"
#include <gtest/gtest.h>

#include "Scene/SceneTestHelpers.h"
#include "o2/Assets/Types/ActorAsset.h"
#include "o2/Scene/Components/ImageComponent.h"
#include "o2/Scene/Scene.h"
#include "o2/Scene/UI/Widget.h"
#include "o2/Scene/UI/WidgetLayout.h"

using namespace o2;

namespace
{
    // Виджет с рисуемым компонентом на потомке — так выглядит окно, в которое
    // добавили эффект: эмиттер частиц или картинку вешают на виджет-контейнер
    Ref<Actor> BuildWidgetWithDrawableComponent()
    {
        auto prevMode = Actor::GetDefaultCreationMode();
        Actor::SetDefaultCreationMode(ActorCreateMode::NotInScene);

        auto root = mmake<Widget>();
        root->SetName("Window");

        auto child = mmake<Widget>();
        child->SetName("Effect");
        root->AddChild(child);
        child->layout->anchorMin = Vec2F(0.5f, 0.5f);
        child->layout->anchorMax = Vec2F(0.5f, 0.5f);
        child->layout->offsetMin = Vec2F(-10, -10);
        child->layout->offsetMax = Vec2F(10, 10);
        child->AddComponent(mmake<ImageComponent>());

        Actor::SetDefaultCreationMode(prevMode);
        return root;
    }
}

// Клонирование виджета с рисуемым компонентом падало: копирующий конструктор Actor
// добавлял компонент и сразу дёргал его OnTransformUpdated, когда часть Widget ещё
// не сконструирована, и WidgetLayout читал неинициализированную память
TEST(WidgetDrawableComponentClone, CloneKeepsComponentAlive)
{
    SceneCleanGuard sceneGuard;

    auto source = BuildWidgetWithDrawableComponent();
    auto clone = source->CloneAsRef<Actor>();

    ASSERT_TRUE(clone);
    auto child = clone->GetChild("Effect");
    ASSERT_TRUE(child);
    EXPECT_TRUE(child->GetComponent<ImageComponent>());
}

TEST(WidgetDrawableComponentClone, PrototypeInstantiatesIntoScene)
{
    SceneCleanGuard sceneGuard;

    auto asset = mmake<ActorAsset>(BuildWidgetWithDrawableComponent());

    auto prevMode = Actor::GetDefaultCreationMode();
    Actor::SetDefaultCreationMode(ActorCreateMode::InScene);

    auto instance = asset->Instantiate();
    o2Scene.UpdateAddedEntities();

    ASSERT_TRUE(instance);
    EXPECT_TRUE(instance->IsOnScene());

    auto child = instance->GetChild("Effect");
    ASSERT_TRUE(child);

    auto image = child->GetComponent<ImageComponent>();
    ASSERT_TRUE(image);

    // после инициализации компонент обязан видеть актуальный трансформ владельца
    child->transform->SetPosition2D(Vec2F(40, 25));
    child->transform->Update();
    EXPECT_TRUE(image->GetActor());

    Actor::SetDefaultCreationMode(prevMode);
}
