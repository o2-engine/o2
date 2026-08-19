#include "o2Editor/stdafx.h"
#include "SceneGizmos.h"

#include "o2/Render/Gizmos.h"
#include "o2/Scene/Actor.h"
#include "o2/Scene/Component.h"
#include "o2/Scene/Scene.h"
#include "o2/Utils/Reflection/Type.h"

namespace Editor
{
    void SceneGizmos::SetEnabled(bool enabled)
    {
        mEnabled = enabled;
    }

    bool SceneGizmos::IsEnabled() const
    {
        return mEnabled;
    }

    void SceneGizmos::SetTypeEnabled(const Type* type, bool enabled)
    {
        mTypesEnabled[type] = enabled;
    }

    bool SceneGizmos::IsTypeEnabled(const Type* type) const
    {
        auto found = mTypesEnabled.find(type);
        return found == mTypesEnabled.end() || found->second;
    }

    const Vector<const Type*>& SceneGizmos::GetGizmosTypes() const
    {
        return mGizmosTypes;
    }

    void SceneGizmos::UpdateGizmosTypes()
    {
        for (auto& [actorPtr, actorRef] : o2Scene.GetAllActors())
        {
            auto actor = actorRef.Lock();
            if (!actor)
                continue;

            if (IsGizmosDrawer(actor->GetType()))
                RegisterGizmosType(&actor->GetType());

            for (auto& component : actor->GetComponents())
            {
                if (IsGizmosDrawer(component->GetType()))
                    RegisterGizmosType(&component->GetType());
            }
        }
    }

    void SceneGizmos::Draw(const Vector<Ref<SceneEditableObject>>& objects,
                           const Function<Vec2F(const Vec3F&)>& projection, const Vec3F& clipPlaneOrigin /*= Vec3F()*/,
                           const Vec3F& clipPlaneNormal /*= Vec3F()*/)
    {
        if (!mEnabled || objects.IsEmpty())
            return;

        o2Gizmos.SetProjection(projection, clipPlaneOrigin, clipPlaneNormal);

        mDrawnActors.Clear();

        for (auto& object : objects)
        {
            if (auto actor = DynamicCast<Actor>(object))
                DrawActorGizmos(actor);
        }

        o2Gizmos.ResetProjection();
    }

    void SceneGizmos::DrawActorGizmos(const Ref<Actor>& actor)
    {
        if (!actor->IsEnabledInHierarchy() || mDrawnActors.ContainsKey(actor.Get()))
            return;

        mDrawnActors.Add(actor.Get(), true);

        if (IsTypeEnabled(&actor->GetType()))
            actor->DrawGizmos();

        for (auto& component : actor->GetComponents())
        {
            if (component->IsEnabledInHierarchy() && IsTypeEnabled(&component->GetType()))
                component->DrawGizmos();
        }

        // The selection covers the whole subtree of the selected object, same as its outline does
        for (auto& child : actor->GetChildren())
            DrawActorGizmos(child);
    }

    bool SceneGizmos::IsGizmosDrawer(const Type& type)
    {
        if (auto cached = mGizmosDrawers.find(&type); cached != mGizmosDrawers.end())
            return cached->second;

        bool isDrawer = false;

        // Actor and Component declare the empty hook itself, only overrides below them count
        if (&type != &TypeOf(Actor) && &type != &TypeOf(Component))
        {
            isDrawer = type.GetFunctions().Contains([](const FunctionInfo* func) {
                return func->GetName() == "OnDrawGizmos";
            });

            if (!isDrawer)
                isDrawer = type.GetBaseTypes().Contains([&](const Type::BaseType& base) { return IsGizmosDrawer(*base.type); });
        }

        mGizmosDrawers[&type] = isDrawer;

        return isDrawer;
    }

    void SceneGizmos::RegisterGizmosType(const Type* type)
    {
        if (mGizmosTypes.Contains(type))
            return;

        mGizmosTypes.Add(type);
        mGizmosTypes.SortBy<String>([](const Type* x) { return x->GetName(); });

        onGizmosTypesChanged();
    }
}
