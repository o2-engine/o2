#pragma once

#include "o2/Utils/Function/Function.h"
#include "o2/Utils/Math/Vector2.h"
#include "o2/Utils/Math/Vector3.h"
#include "o2/Utils/Types/Containers/Map.h"
#include "o2/Utils/Types/Containers/Vector.h"

using namespace o2;

namespace o2
{
    class Type;

    FORWARD_CLASS_REF(Actor);
    FORWARD_CLASS_REF(SceneEditableObject);
}

namespace Editor
{
    // ---------------------------------------------------------------------------------------------
    // Scene gizmos drawer. Walks the selected objects with their children and calls their gizmos
    // drawing, keeping visibility settings: common switch and per type switches. Types list is
    // collected from all scene objects which override gizmos drawing, on demand
    // ---------------------------------------------------------------------------------------------
    class SceneGizmos
    {
    public:
        Function<void()> onGizmosTypesChanged; // Called when new gizmos drawing type was met on scene

    public:
        // Sets are gizmos drawing at all
        void SetEnabled(bool enabled);

        // Returns are gizmos drawing at all
        bool IsEnabled() const;

        // Sets are gizmos of objects with type drawing
        void SetTypeEnabled(const Type* type, bool enabled);

        // Returns are gizmos of objects with type drawing
        bool IsTypeEnabled(const Type* type) const;

        // Returns types of scene objects which draw gizmos, sorted by name
        const Vector<const Type*>& GetGizmosTypes() const;

        // Collects types of scene objects which draw gizmos
        void UpdateGizmosTypes();

        // Draws gizmos of the given objects and their children; projection maps world point into
        // current drawing space, clip plane cuts geometry the projection can't map, zero normal
        // means no clipping
        void Draw(const Vector<Ref<SceneEditableObject>>& objects, const Function<Vec2F(const Vec3F&)>& projection,
                  const Vec3F& clipPlaneOrigin = Vec3F(), const Vec3F& clipPlaneNormal = Vec3F());

        // Returns is type overriding gizmos drawing
        bool IsGizmosDrawer(const Type& type);

    private:
        bool mEnabled = true; // Is gizmos drawing enabled at all

        Map<const Type*, bool> mTypesEnabled; // Per type drawing switches, missing type means enabled

        Vector<const Type*> mGizmosTypes; // Types of scene objects which draw gizmos, sorted by name

        Map<const Type*, bool> mGizmosDrawers; // Cache of types overriding gizmos drawing

        Map<const Actor*, bool> mDrawnActors; // Actors drawn in the current pass, so overlapping
                                             // selections (a parent and its child) don't draw twice

    private:
        // Adds type into gizmos types list, keeping it sorted by name
        void RegisterGizmosType(const Type* type);

        // Draws gizmos of the actor and its components, then of its children
        void DrawActorGizmos(const Ref<Actor>& actor);
    };
}
