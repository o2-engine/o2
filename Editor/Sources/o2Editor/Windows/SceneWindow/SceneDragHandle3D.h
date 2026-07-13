#pragma once

#include "o2/Utils/Math/Mesh3DPrimitives.h"
#include "o2/Utils/Math/AABB.h"
#include "o2/Utils/Math/Quaternion.h"
#include "o2/Utils/Math/Vector3.h"
#include "o2Editor/Windows/SceneWindow/SceneDragHandle.h"

using namespace o2;

namespace o2
{
    class Mesh;
}

namespace Editor
{
    // ----------------------------------------------------------------------------------------
    // Volumetric scene drag handle for 3D view mode. Lives in world space as real 3D geometry:
    // the tool draws it inside the scene pass via DrawGeometry (perspective camera, on top of
    // scene content), while cursor picking casts the view ray against the world bounds
    // ----------------------------------------------------------------------------------------
    class SceneDragHandle3D : public SceneDragHandle
    {
    public:
        // Default constructor
        SceneDragHandle3D(RefCounter* refCounter);

        // Copy-constructor
        SceneDragHandle3D(RefCounter* refCounter, const SceneDragHandle3D& other);

        // Destructor
        ~SceneDragHandle3D();

        // Sets local space geometry, drawn transformed by pose and view scale
        void SetGeometry(const Mesh3DData& data);

        // Sets world position and orientation
        void SetPose(const Vec3F& position, const Quat& rotation);

        // Returns world position
        Vec3F GetPosition3D() const;

        // Returns world orientation
        Quat GetRotation3D() const;

        // Sets base color; hover and pressed colors are derived
        void SetColor(const Color4& color);

        // Sets all color states explicitly
        void SetColors(const Color4& regular, const Color4& hover, const Color4& pressed);

        // Sets constant screen size factor: world scale = factor * distance from camera to handle
        void SetScreenSizeFactor(float factor);

        // Returns current world scale of local geometry
        float GetWorldScale() const;

        // Sets lambert shading of the drawn geometry
        void SetShaded(bool shaded);

        // Sets extra picking bounds padding in local geometry units, eases picking of thin geometry
        void SetPickPadding(float padding);

        // Clears analytic pick shapes, falling back to local bounds picking
        void ClearPickShapes();

        // Adds finite cylinder pick shape in local geometry units; pick padding expands the radius
        void AddPickCylinder(const Vec3F& start, const Vec3F& end, float radius);

        // Adds parallelogram pick shape in local geometry units; pick padding expands the edges
        void AddPickQuad(const Vec3F& corner, const Vec3F& edgeU, const Vec3F& edgeV);

        // Adds box pick shape in local geometry units; pick padding expands the box
        void AddPickBox(const o2::AABB& box);

        // Registers cursor events listening; geometry is drawn in the scene pass by DrawGeometry
        void Draw() override;

        // Draws geometry with the current camera; must be called inside the 3D scene pass
        void DrawGeometry();

        // Returns true when the view ray under point hits the handle bounds and no other
        // enabled 3D handle is hit nearer
        bool IsUnderPoint(const Vec2F& point) override;

        // Returns world axis aligned bounds of posed geometry
        o2::AABB GetWorldBounds() const;

        // Casts the view ray under screen point against the handle bounds
        bool GetRayHitDistance(const Vec2F& screenPoint, float& distance) const;

        // Returns Unity-style gizmo color of world axis: 0 - X red, 1 - Y green, 2 - Z blue
        static Color4 GetAxisColor(int axis);

        SERIALIZABLE(SceneDragHandle3D);
        CLONEABLE_REF(SceneDragHandle3D);

    public:
        // ------------------------------------------------------
        // Analytic pick shape in local geometry units
        // ------------------------------------------------------
        struct PickShape
        {
            enum class Type { Cylinder, Quad, Box };

            Type type = Type::Box;

            Vec3F pointA;         // Cylinder start; quad corner; box min
            Vec3F pointB;         // Cylinder end; quad edge U; box max
            Vec3F pointC;         // Quad edge V
            float radius = 0.0f;  // Cylinder radius

            bool operator==(const PickShape& other) const
            {
                return type == other.type && pointA == other.pointA && pointB == other.pointB &&
                    pointC == other.pointC && Math::Equals(radius, other.radius);
            }
        };

    protected:
        Mesh3DData mGeometry;    // Local space geometry
        o2::AABB   mLocalBounds; // Local geometry bounds

        Vector<PickShape> mPickShapes; // Analytic pick shapes; empty - local bounds picking

        Ref<Mesh> mMesh; // Drawable mesh, refilled on draw

        Vec3F mPosition3D;              // World position
        Quat  mRotation3D;              // World orientation
        float mScreenSizeFactor = 0.1f; // World scale per unit of camera distance
        bool  mShaded = true;           // Is geometry lambert shaded
        float mPickPadding = 0.0f;      // Extra picking bounds padding in local geometry units

        Color4 mRegularColor = Color4(220, 220, 220, 255); // Base color
        Color4 mHoverColor = Color4(255, 255, 255, 255);   // Hovered color
        Color4 mPressedColor = Color4(255, 220, 80, 255);  // Pressed color

    protected:
        // Returns color for current interaction state
        Color4 GetCurrentColor() const;

        // Called when cursor enters this object, requests scene redraw for the hover state
        void OnCursorEnter(const Input::Cursor& cursor) override;

        // Called when cursor exits this object, requests scene redraw for the hover state
        void OnCursorExit(const Input::Cursor& cursor) override;

        friend class SceneEditScreen;
    };
}
// --- META ---

PRE_ENUM_META(Editor::SceneDragHandle3D::PickShape::Type);

CLASS_BASES_META(Editor::SceneDragHandle3D)
{
    BASE_CLASS(Editor::SceneDragHandle);
}
END_META;
CLASS_FIELDS_META(Editor::SceneDragHandle3D)
{
    FIELD().PROTECTED().NAME(mGeometry);
    FIELD().PROTECTED().NAME(mLocalBounds);
    FIELD().PROTECTED().NAME(mPickShapes);
    FIELD().PROTECTED().NAME(mMesh);
    FIELD().PROTECTED().NAME(mPosition3D);
    FIELD().PROTECTED().NAME(mRotation3D);
    FIELD().PROTECTED().DEFAULT_VALUE(0.1f).NAME(mScreenSizeFactor);
    FIELD().PROTECTED().DEFAULT_VALUE(true).NAME(mShaded);
    FIELD().PROTECTED().DEFAULT_VALUE(0.0f).NAME(mPickPadding);
    FIELD().PROTECTED().DEFAULT_VALUE(Color4(220, 220, 220, 255)).NAME(mRegularColor);
    FIELD().PROTECTED().DEFAULT_VALUE(Color4(255, 255, 255, 255)).NAME(mHoverColor);
    FIELD().PROTECTED().DEFAULT_VALUE(Color4(255, 220, 80, 255)).NAME(mPressedColor);
}
END_META;
CLASS_METHODS_META(Editor::SceneDragHandle3D)
{

    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*);
    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*, const SceneDragHandle3D&);
    FUNCTION().PUBLIC().SIGNATURE(void, SetGeometry, const Mesh3DData&);
    FUNCTION().PUBLIC().SIGNATURE(void, SetPose, const Vec3F&, const Quat&);
    FUNCTION().PUBLIC().SIGNATURE(Vec3F, GetPosition3D);
    FUNCTION().PUBLIC().SIGNATURE(Quat, GetRotation3D);
    FUNCTION().PUBLIC().SIGNATURE(void, SetColor, const Color4&);
    FUNCTION().PUBLIC().SIGNATURE(void, SetColors, const Color4&, const Color4&, const Color4&);
    FUNCTION().PUBLIC().SIGNATURE(void, SetScreenSizeFactor, float);
    FUNCTION().PUBLIC().SIGNATURE(float, GetWorldScale);
    FUNCTION().PUBLIC().SIGNATURE(void, SetShaded, bool);
    FUNCTION().PUBLIC().SIGNATURE(void, SetPickPadding, float);
    FUNCTION().PUBLIC().SIGNATURE(void, ClearPickShapes);
    FUNCTION().PUBLIC().SIGNATURE(void, AddPickCylinder, const Vec3F&, const Vec3F&, float);
    FUNCTION().PUBLIC().SIGNATURE(void, AddPickQuad, const Vec3F&, const Vec3F&, const Vec3F&);
    FUNCTION().PUBLIC().SIGNATURE(void, AddPickBox, const o2::AABB&);
    FUNCTION().PUBLIC().SIGNATURE(void, Draw);
    FUNCTION().PUBLIC().SIGNATURE(void, DrawGeometry);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsUnderPoint, const Vec2F&);
    FUNCTION().PUBLIC().SIGNATURE(o2::AABB, GetWorldBounds);
    FUNCTION().PUBLIC().SIGNATURE(bool, GetRayHitDistance, const Vec2F&, float&);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(Color4, GetAxisColor, int);
    FUNCTION().PROTECTED().SIGNATURE(Color4, GetCurrentColor);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCursorEnter, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCursorExit, const Input::Cursor&);
}
END_META;
// --- END META ---
