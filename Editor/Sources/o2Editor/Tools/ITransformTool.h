#pragma once

#include "o2/Utils/Math/AABB.h"
#include "o2/Utils/Math/Quaternion.h"
#include "o2/Utils/Math/Vector3.h"
#include "o2Editor/Tools/SelectionTool.h"

using namespace o2;

namespace o2
{
    class Sprite;
    class SceneEditableObject;
}

namespace Editor
{
    // ---------------------
    // Editor transform tool
    // ---------------------
    class ITransformTool: public SelectionTool
    {
    public:
        Function<void()> onTransformBegin; // Called when transform begins
        Function<void()> onTransformEnd;   // Called when transform ends

    public:
        // Returns average world position of objects; actors contribute full 3D position, others their plane pivot
        static Vec3F GetSelectionCenter3D(const Vector<Ref<SceneEditableObject>>& objects);

        // Returns gizmos frame rotation, Unity-like: last selected actor orientation, world axes with Ctrl
        static Quat GetSelectionFrameRotation3D(const Vector<Ref<SceneEditableObject>>& objects);

        // Returns world axis aligned bounds of object: mesh components bound their mesh,
        // plain actors bound their oriented rect, other editables their plane basis; false when unresolvable
        static bool GetObjectBounds3D(const Ref<SceneEditableObject>& object, o2::AABB& bounds);

        // Returns bounds of object in the rotated frame: corners are transformed by the inverted
        // frame rotation before bounding, giving an oriented box when frameRotation is the object rotation
        static bool GetObjectBoundsInFrame3D(const Ref<SceneEditableObject>& object, const Quat& frameRotation,
                                             o2::AABB& bounds);

        // Returns combined world bounds of objects; false when empty
        static bool GetSelectionBounds3D(const Vector<Ref<SceneEditableObject>>& objects, o2::AABB& bounds);

        // Returns world corners of the object's 3D bounds source: mesh bounds box or the oriented rect
        static bool GetObjectWorldCorners3D(const Ref<SceneEditableObject>& object, Vector<Vec3F>& corners);

        // Casts a ray against the object's oriented 3D bounds (actor rotation frame);
        // returns hit distance in world units
        static bool RayIntersectsObject3D(const Ref<SceneEditableObject>& object, const Vec3F& rayOrigin,
                                          const Vec3F& rayDirection, float& distance);

        // Builds the screen rectangle of the projected 3D corners; projector maps world points to screen
        static bool GetObjectScreenRect3D(const Ref<SceneEditableObject>& object,
                                          const Function<Vec2F(const Vec3F&)>& projector, RectF& rect);

        IOBJECT(ITransformTool);
    };
}
// --- META ---

CLASS_BASES_META(Editor::ITransformTool)
{
    BASE_CLASS(Editor::SelectionTool);
}
END_META;
CLASS_FIELDS_META(Editor::ITransformTool)
{
    FIELD().PUBLIC().NAME(onTransformBegin);
    FIELD().PUBLIC().NAME(onTransformEnd);
}
END_META;
CLASS_METHODS_META(Editor::ITransformTool)
{

    FUNCTION().PUBLIC().SIGNATURE_STATIC(Vec3F, GetSelectionCenter3D, const Vector<Ref<SceneEditableObject>>&);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(Quat, GetSelectionFrameRotation3D, const Vector<Ref<SceneEditableObject>>&);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(bool, GetObjectBounds3D, const Ref<SceneEditableObject>&, o2::AABB&);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(bool, GetObjectBoundsInFrame3D, const Ref<SceneEditableObject>&, const Quat&, o2::AABB&);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(bool, GetSelectionBounds3D, const Vector<Ref<SceneEditableObject>>&, o2::AABB&);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(bool, GetObjectWorldCorners3D, const Ref<SceneEditableObject>&, Vector<Vec3F>&);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(bool, RayIntersectsObject3D, const Ref<SceneEditableObject>&, const Vec3F&, const Vec3F&, float&);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(bool, GetObjectScreenRect3D, const Ref<SceneEditableObject>&, const Function<Vec2F(const Vec3F&)>&, RectF&);
}
END_META;
// --- END META ---
