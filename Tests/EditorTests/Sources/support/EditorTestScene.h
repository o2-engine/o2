#pragma once

#include <initializer_list>

#include "o2/Scene/Actor.h"
#include "o2/Scene/ActorTransform.h"
#include "o2/Scene/Scene.h"
#include "o2/Utils/Editor/SceneEditableObject.h"
#include "o2/Utils/Math/Vector2.h"

namespace Editor::Tests
{
    // RAII guard: clears o2Scene on destruction so tests don't bleed state into each other
    class SceneCleanGuard
    {
    public:
        SceneCleanGuard() = default;
        ~SceneCleanGuard()
        {
            o2Scene.Clear(true);
            o2Scene.UpdateDestroyingEntities();
        }
    };

    // Pump one frame of scene bookkeeping (added entities, transforms)
    inline void TickScene()
    {
        o2Scene.UpdateAddedEntities();
        o2Scene.UpdateTransforms();
    }

    inline o2::Ref<o2::Actor> MakeActor()
    {
        return mmake<o2::Actor>(o2::ActorCreateMode::InScene);
    }

    inline o2::Ref<o2::Actor> MakeActor(const o2::Vec2F& pos)
    {
        auto a = MakeActor();
        a->transform->SetPosition(pos);
        TickScene();
        return a;
    }

    inline void SetActorPos(const o2::Ref<o2::Actor>& a, const o2::Vec2F& pos)
    {
        a->transform->SetPosition(pos);
        TickScene();
    }

    inline o2::Vector<o2::Ref<o2::SceneEditableObject>> AsEditable(
        std::initializer_list<o2::Ref<o2::Actor>> actors)
    {
        o2::Vector<o2::Ref<o2::SceneEditableObject>> v;
        for (auto& a : actors)
            v.Add(o2::DynamicCast<o2::SceneEditableObject>(a));
        return v;
    }

    inline bool NearV(const o2::Vec2F& a, const o2::Vec2F& b, float eps = 1e-3f)
    {
        return o2::Math::Abs(a.x - b.x) < eps && o2::Math::Abs(a.y - b.y) < eps;
    }
}
