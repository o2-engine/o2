#include "o2/stdafx.h"
#include <gtest/gtest.h>

#include "Scene/SceneTestHelpers.h"
#include "o2/Animation/AnimationClip.h"
#include "o2/Animation/Tracks/AnimationTrack.h"
#include "o2/Assets/Types/ActorAsset.h"
#include "o2/Render/Sprite.h"
#include "o2/Scene/Scene.h"
#include "o2/Scene/UI/WidgetLayer.h"
#include "o2/Scene/UI/WidgetLayout.h"
#include "o2/Scene/UI/Widgets/Button.h"

using namespace o2;

namespace
{
    // Виджет в духе игровых прототипов: кнопка со слоями и вложенным виджетом
    Ref<Actor> BuildWidgetTree()
    {
        auto prevMode = Actor::GetDefaultCreationMode();
        Actor::SetDefaultCreationMode(ActorCreateMode::NotInScene);

        auto button = mmake<Button>();
        button->SetName("Proto");

        auto back = mmake<Sprite>();
        back->SetColor(Color4(200, 100, 50));
        button->AddLayer("back", back, Layout::BothStretch());

        auto inner = mmake<Widget>();
        inner->SetName("Inner");
        button->AddChild(inner);
        inner->layout->anchorMin = Vec2F(0.5f, 0.5f);
        inner->layout->anchorMax = Vec2F(0.5f, 0.5f);
        inner->layout->offsetMin = Vec2F(-10, -10);
        inner->layout->offsetMax = Vec2F(10, 10);

        // стейт вдавливания с мультитрек-клипом — как у игровых кнопок
        auto dim = mmake<Sprite>();
        dim->SetColor(Color4(20, 20, 40));
        auto pressedLayer = button->AddLayer("pressed", dim, Layout::BothStretch());
        pressedLayer->SetEnabled(true);

        auto clip = mmake<AnimationClip>();
        *clip->AddTrack<float>("layer/pressed/transparency") = AnimationTrack<float>::EaseInOut(0.0f, 0.35f, 0.06f);
        *clip->AddTrack<Vec2F>("layout/offsetMin") = AnimationTrack<Vec2F>::EaseInOut(Vec2F(-32, -32), Vec2F(-32, -36), 0.06f);
        button->AddState("pressed", clip);

        button->layout->anchorMin = Vec2F(0.5f, 0.5f);
        button->layout->anchorMax = Vec2F(0.5f, 0.5f);
        button->layout->offsetMin = Vec2F(-32, -32);
        button->layout->offsetMax = Vec2F(32, 32);

        Actor::SetDefaultCreationMode(prevMode);
        return button;
    }

    void ExpectInstanceAlive(const Ref<Actor>& instance, const char* stage)
    {
        ASSERT_TRUE(instance) << stage;

        auto button = DynamicCast<Button>(instance);
        ASSERT_TRUE(button) << stage;
        EXPECT_TRUE(button->GetLayer("back")) << stage;
        EXPECT_TRUE(button->GetChild("Inner")) << stage;
        EXPECT_TRUE(button->IsEnabled()) << stage;
        EXPECT_TRUE(button->IsOnScene()) << stage;
    }
}

// Прототип-виджет должен инстанцироваться в сцену повторно: после очистки сцены
// клон из того же ActorAsset обязан быть полноценным (раньше давал пустой инстанс)
TEST(WidgetPrototypeReinstantiate, SecondSceneInstanceIsAlive)
{
    SceneCleanGuard sceneGuard;

    auto asset = mmake<ActorAsset>(BuildWidgetTree());

    auto prevMode = Actor::GetDefaultCreationMode();
    Actor::SetDefaultCreationMode(ActorCreateMode::InScene);

    // первая сцена
    auto first = asset->Instantiate();
    o2Scene.UpdateAddedEntities();
    ExpectInstanceAlive(first, "first scene");

    // пересоздание сцены, как между тестами или уровнями
    first = nullptr;
    o2Scene.Clear(true);
    o2Scene.UpdateDestroyingEntities();

    auto second = asset->Instantiate();
    o2Scene.UpdateAddedEntities();
    ExpectInstanceAlive(second, "second scene");

    Actor::SetDefaultCreationMode(prevMode);
}
