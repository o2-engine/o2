#pragma once

#include "o2/Utils/Math/Mesh3DPrimitives.h"
#include "o2/Utils/Editor/SceneEditableObject.h"
#include "o2/Utils/Math/Basis.h"
#include "o2/Utils/Math/Vector3.h"
#include "o2Editor/Tools/ITransformTool.h"
#include "o2Editor/Windows/SceneWindow/SceneDragHandle.h"

using namespace o2;

namespace o2
{
    class Mesh;
    class SceneEditableObject;
    class Text;
}

namespace Editor
{
    FORWARD_CLASS_REF(TransformAction);

    // -------------------
    // Rotate objects tool
    // -------------------
    class RotateTool: public ITransformTool
    {
    public:
        float angleSnapStep = 15.0f; // Rotation angle step in degree

    public:
        // Default constructor
        RotateTool();

        // Copy-operator
        RotateTool& operator=(const RotateTool& other) { return *this; }

        // Destructor
        ~RotateTool();

        IOBJECT(RotateTool);

    protected:
        const float  mRotateRingInsideRadius = 60;                            // Rotate ring inside radius in pixels
        const float  mRotateRingOutsideRadius = 100;                        // Rotate ring outside radius in pixels
        const int    mRotateRingSegs = 50;                                    // Rotate ring segments
        const Color4 mRotateRingsColor = Color4(220, 220, 220, 255);        // Rotate ring border color
        const Color4 mRotateRingsFillColor = Color4(220, 220, 220, 50);        // Rotate ring color 1
        const Color4 mRotateRingsFillColor2 = Color4(220, 220, 220, 100);   // Rotate ring color 2
        const Color4 mRotateMeshClockwiseColor = Color4(211, 87, 40, 100);  // Rotate angle clockwise rotation color
        const Color4 mRotateMeshCClockwiseColor = Color4(87, 211, 40, 100); // Rotate angle counter clockwise rotation color
                         
        Ref<Mesh> mRotateRingFillMesh; // Rotate ring mesh
        Ref<Mesh> mAngleMesh;          // Rotation angle mesh
        Vec2F     mScenePivot;           // Rotation pivot in scene space

        Ref<SceneDragHandle> mPivotDragHandle; // Pivot drag handle

        float mPressAngle;                   // Angle at cursor pressing
        float mCurrentRotateAngle;           // Current rotation angle
        bool  mRingPressed = false;           // Is rotate ring was pressed
        float mSnapAngleAccumulated = 0.0f; // Snapping angle accumulated

        bool  mToolEnabled = false;             // Is tool currently selected
        Vec3F mPivot3D;                         // Rotation pivot in world space, 3D view mode only
        Quat  mRingFrame3D;                     // Rings orientation frame: selection local axes, world with Ctrl
        int   mPressedRing3D = -1;              // Axis of the pressed 3D ring: 0 - X, 1 - Y, 2 - Z, -1 when none
        float mLastRingAngle3D = 0.0f;          // Last cursor angle around the pressed ring axis
        float mPressRingAngle3D = 0.0f;         // Cursor angle around the ring axis at press
        float mAccumulatedRingAngle3D = 0.0f;   // Total rotation angle applied during the current 3D ring drag
        Vec3F mDragRingAxis3D;                  // World rotation axis of the pressed ring, frozen at press
        Vec3F mDragRingU3D;                     // Angle measuring plane basis of the pressed ring, frozen at press
        Vec3F mDragRingV3D;                     // Angle measuring plane basis of the pressed ring, frozen at press
        int   mHoveredRing3D = -1;              // Ring under cursor when not dragging, for hover redraw
        Vector<Vector<Vec2F>> mRingPoints3D;    // Screen space polylines of the three rings, updated each draw

        const float mRing3DWidth = 0.12f; // Flat ring band width, fraction of the ring radius

        Vector<Ref<Mesh>> mRingMeshes3D;      // Flat annulus meshes of the three rings
        Mesh3DData        mRingGeometry3D;    // Unit flat ring geometry shared by the rings
        Ref<Mesh>         mAngleSectorMesh3D; // Screen space swept angle sector mesh, 3D view mode only
        Ref<Text>         mAngleText3D;       // Rotation degrees text near cursor, 3D view mode only

        Ref<TransformAction> mTransformAction; // Current transform action. Creates when transform started

    public:
        // Returns toggle in menu panel icon name
        String GetPanelIcon() const override;

        // Returns shortcut keys for toggle
        ShortcutKeys GetShortcut() const override;

        // Updates tool
        void Update(float dt) override;

        // Draws tool scene pass: volumetric rings in 3D mode
        void DrawScene() override;

        // Draws tool
        void DrawScreen() override;

        // Draws swept angle sector and degrees text while dragging a 3D ring
        void DrawAngle3D();

        // Called when tool was enabled
        void OnEnabled() override;

        // Called when tool was disabled
        void OnDisabled() override;

        // Called when scene objects was changed
        void OnSceneChanged(const Vector<Ref<SceneEditableObject>>& changedObjects) override;

        // Called when objects selection was changed
        void OnObjectsSelectionChanged(const Vector<Ref<SceneEditableObject>>& objects) override;

        // Updates ring and angle meshes
        void UpdateMeshes();

        // Calculates rotate pivot in objects center
        void CalcPivotByObjectsCenter();

        // Called when pivot handle moved
        void OnPivotDragHandleMoved(const Vec2F& position);

        // Returns is point inside rotate ring (point and pivot in screen pixels)
        bool IsPointInRotateRing(const Vec2F& screenPoint) const;

        // Pure ring-pick math: true when screenPoint lies between innerRadius and outerRadius
        // around screenPivot. Both args must be in the same coordinate space.
        static bool IsScreenPointInRing(const Vec2F& screenPivot, const Vec2F& screenPoint,
                                        float innerRadius, float outerRadius);

        // Intersects the ray with the ring plane and tests the flat annulus band with tolerance;
        // false when the ray is nearly parallel to the plane (edge-on) or the hit is outside the band
        static bool IsRayHitInRingBand(const Vec3F& rayOrigin, const Vec3F& rayDirection,
                                       const Vec3F& pivot, const Vec3F& planeNormal,
                                       float innerRadius, float outerRadius, float tolerance,
                                       float& hitDistance);

        // Rebuilds screen space polylines of the three world axis rings around the selection center
        void UpdateRings3D();

        // Returns nearest ring axis to the screen point within the pick threshold, -1 when none
        int PickRing3D(const Vec2F& screenPoint) const;

        // Returns cursor angle around the frame-rotated ring axis; false on miss or edge-on ring
        bool GetCursorRingAngle3D(int axis, const Vec2F& screenPoint, float& angle) const;

        // Returns cursor angle in the explicit rotation plane basis; false on miss or edge-on plane
        bool GetCursorAngleOnPlane(const Vec3F& normal, const Vec3F& u, const Vec3F& v,
                                   const Vec2F& screenPoint, float& angle) const;

        // Builds a world axis rotation step over the current rotation and Appends it to action
        void AppendRotateEulerStep(const Ref<TransformAction>& action, const Vec3F& worldAxis, float angleDelta);

        // Called when cursor pressed on this
        void OnCursorPressed(const Input::Cursor& cursor) override;

        // Called when cursor released (only when cursor pressed this at previous time)
        void OnCursorReleased(const Input::Cursor& cursor) override;

        // Called when cursor pressing was broken (when scrolled scroll area or some other)
        void OnCursorPressBreak(const Input::Cursor& cursor) override;

        // Called when cursor stay down during frame
        void OnCursorStillDown(const Input::Cursor& cursor) override;

        // Called when key was pressed
        void OnKeyPressed(const Input::Key& key) override;

        // Called when key stay down during frame
        void OnKeyStayDown(const Input::Key& key) override;

        // Rotates objects on angle
        void RotateObjects(float angleDelta);

        // Rotates objects on angle separated
        void RotateObjectsSeparated(float angleDelta);

        // Rotates objects on angle in a one-shot action
        void RotateObjectsWithAction(float angleDelta);

        // Rotates objects on angle separated in a one-shot action
        void RotateObjectsSeparatedWithAction(float angleDelta);

        // Builds a rotation step and Appends it to action
        void AppendRotateStep(const Ref<TransformAction>& action, float angleDelta, bool separated);
    };
}
// --- META ---

CLASS_BASES_META(Editor::RotateTool)
{
    BASE_CLASS(Editor::ITransformTool);
}
END_META;
CLASS_FIELDS_META(Editor::RotateTool)
{
    FIELD().PUBLIC().DEFAULT_VALUE(15.0f).NAME(angleSnapStep);
    FIELD().PROTECTED().DEFAULT_VALUE(60).NAME(mRotateRingInsideRadius);
    FIELD().PROTECTED().DEFAULT_VALUE(100).NAME(mRotateRingOutsideRadius);
    FIELD().PROTECTED().DEFAULT_VALUE(50).NAME(mRotateRingSegs);
    FIELD().PROTECTED().DEFAULT_VALUE(Color4(220, 220, 220, 255)).NAME(mRotateRingsColor);
    FIELD().PROTECTED().DEFAULT_VALUE(Color4(220, 220, 220, 50)).NAME(mRotateRingsFillColor);
    FIELD().PROTECTED().DEFAULT_VALUE(Color4(220, 220, 220, 100)).NAME(mRotateRingsFillColor2);
    FIELD().PROTECTED().DEFAULT_VALUE(Color4(211, 87, 40, 100)).NAME(mRotateMeshClockwiseColor);
    FIELD().PROTECTED().DEFAULT_VALUE(Color4(87, 211, 40, 100)).NAME(mRotateMeshCClockwiseColor);
    FIELD().PROTECTED().NAME(mRotateRingFillMesh);
    FIELD().PROTECTED().NAME(mAngleMesh);
    FIELD().PROTECTED().NAME(mScenePivot);
    FIELD().PROTECTED().NAME(mPivotDragHandle);
    FIELD().PROTECTED().NAME(mPressAngle);
    FIELD().PROTECTED().NAME(mCurrentRotateAngle);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mRingPressed);
    FIELD().PROTECTED().DEFAULT_VALUE(0.0f).NAME(mSnapAngleAccumulated);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mToolEnabled);
    FIELD().PROTECTED().NAME(mPivot3D);
    FIELD().PROTECTED().NAME(mRingFrame3D);
    FIELD().PROTECTED().DEFAULT_VALUE(-1).NAME(mPressedRing3D);
    FIELD().PROTECTED().DEFAULT_VALUE(0.0f).NAME(mLastRingAngle3D);
    FIELD().PROTECTED().DEFAULT_VALUE(0.0f).NAME(mPressRingAngle3D);
    FIELD().PROTECTED().DEFAULT_VALUE(0.0f).NAME(mAccumulatedRingAngle3D);
    FIELD().PROTECTED().NAME(mDragRingAxis3D);
    FIELD().PROTECTED().NAME(mDragRingU3D);
    FIELD().PROTECTED().NAME(mDragRingV3D);
    FIELD().PROTECTED().DEFAULT_VALUE(-1).NAME(mHoveredRing3D);
    FIELD().PROTECTED().NAME(mRingPoints3D);
    FIELD().PROTECTED().DEFAULT_VALUE(0.12f).NAME(mRing3DWidth);
    FIELD().PROTECTED().NAME(mRingMeshes3D);
    FIELD().PROTECTED().NAME(mRingGeometry3D);
    FIELD().PROTECTED().NAME(mAngleSectorMesh3D);
    FIELD().PROTECTED().NAME(mAngleText3D);
    FIELD().PROTECTED().NAME(mTransformAction);
}
END_META;
CLASS_METHODS_META(Editor::RotateTool)
{

    FUNCTION().PUBLIC().CONSTRUCTOR();
    FUNCTION().PUBLIC().SIGNATURE(String, GetPanelIcon);
    FUNCTION().PUBLIC().SIGNATURE(ShortcutKeys, GetShortcut);
    FUNCTION().PUBLIC().SIGNATURE(void, Update, float);
    FUNCTION().PUBLIC().SIGNATURE(void, DrawScene);
    FUNCTION().PUBLIC().SIGNATURE(void, DrawScreen);
    FUNCTION().PUBLIC().SIGNATURE(void, DrawAngle3D);
    FUNCTION().PUBLIC().SIGNATURE(void, OnEnabled);
    FUNCTION().PUBLIC().SIGNATURE(void, OnDisabled);
    FUNCTION().PUBLIC().SIGNATURE(void, OnSceneChanged, const Vector<Ref<SceneEditableObject>>&);
    FUNCTION().PUBLIC().SIGNATURE(void, OnObjectsSelectionChanged, const Vector<Ref<SceneEditableObject>>&);
    FUNCTION().PUBLIC().SIGNATURE(void, UpdateMeshes);
    FUNCTION().PUBLIC().SIGNATURE(void, CalcPivotByObjectsCenter);
    FUNCTION().PUBLIC().SIGNATURE(void, OnPivotDragHandleMoved, const Vec2F&);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsPointInRotateRing, const Vec2F&);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(bool, IsScreenPointInRing, const Vec2F&, const Vec2F&, float, float);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(bool, IsRayHitInRingBand, const Vec3F&, const Vec3F&, const Vec3F&, const Vec3F&, float, float, float, float&);
    FUNCTION().PUBLIC().SIGNATURE(void, UpdateRings3D);
    FUNCTION().PUBLIC().SIGNATURE(int, PickRing3D, const Vec2F&);
    FUNCTION().PUBLIC().SIGNATURE(bool, GetCursorRingAngle3D, int, const Vec2F&, float&);
    FUNCTION().PUBLIC().SIGNATURE(bool, GetCursorAngleOnPlane, const Vec3F&, const Vec3F&, const Vec3F&, const Vec2F&, float&);
    FUNCTION().PUBLIC().SIGNATURE(void, AppendRotateEulerStep, const Ref<TransformAction>&, const Vec3F&, float);
    FUNCTION().PUBLIC().SIGNATURE(void, OnCursorPressed, const Input::Cursor&);
    FUNCTION().PUBLIC().SIGNATURE(void, OnCursorReleased, const Input::Cursor&);
    FUNCTION().PUBLIC().SIGNATURE(void, OnCursorPressBreak, const Input::Cursor&);
    FUNCTION().PUBLIC().SIGNATURE(void, OnCursorStillDown, const Input::Cursor&);
    FUNCTION().PUBLIC().SIGNATURE(void, OnKeyPressed, const Input::Key&);
    FUNCTION().PUBLIC().SIGNATURE(void, OnKeyStayDown, const Input::Key&);
    FUNCTION().PUBLIC().SIGNATURE(void, RotateObjects, float);
    FUNCTION().PUBLIC().SIGNATURE(void, RotateObjectsSeparated, float);
    FUNCTION().PUBLIC().SIGNATURE(void, RotateObjectsWithAction, float);
    FUNCTION().PUBLIC().SIGNATURE(void, RotateObjectsSeparatedWithAction, float);
    FUNCTION().PUBLIC().SIGNATURE(void, AppendRotateStep, const Ref<TransformAction>&, float, bool);
}
END_META;
// --- END META ---
