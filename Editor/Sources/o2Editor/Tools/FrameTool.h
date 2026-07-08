#pragma once

#include "o2Editor/Tools/ITransformTool.h"
#include "o2Editor/Windows/SceneWindow/SceneDragHandle.h"
#include "o2Editor/Windows/SceneWindow/SceneDragHandle3D.h"
#include "o2/Utils/Math/AABB.h"
#include "o2/Utils/Math/Basis.h"

using namespace o2;

namespace o2
{
    class Sprite;
}

namespace Editor
{
    FORWARD_CLASS_REF(TransformAction);

    // ----------------------------------
    // Editor frame and pivot moving tool
    // ----------------------------------
    class FrameTool: public ITransformTool
    {
    public:
        // Default constructor
        FrameTool();

        FrameTool& operator=(const FrameTool& other) { return *this; }

        // Destructor
        ~FrameTool();

        IOBJECT(FrameTool);

    protected:
        // -------------------
        // Snap line draw info
        // -------------------
        struct SnapLine
        {
            Color4 color;
            Vec2F  begin;
            Vec2F  end;

        public:
            SnapLine() {}
            SnapLine(const Vec2F& begin, const Vec2F& end, const Color4& color):begin(begin), end(end), color(color) {}

            bool operator==(const SnapLine& other) const { return color == other.color && begin == other.begin && end == other.end; }
        };

    protected:
        const Color4 mFrameColor = Color4(44, 62, 80, 255);           // Objects frame color
        const Color4 mObjectColor = Color4(44, 62, 80, 150);          // Object color
        const Color4 mParentColor = Color4(44, 62, 80, 100);          // Object parent color
        const Color4 mAnchorsFrameColor = Color4(20, 100, 255, 255);  // Widgets anchors frame color
        const Color4 mSnapLinesColor = Color4(40, 255, 100, 255);     // Widgets anchors frame color

        const float mFrameMinimalSize = 0.001f;   // Minimal size of transforming frame

        Ref<SceneDragHandle> mLeftTopRotateHandle;      // Left top rotation handle
        Ref<SceneDragHandle> mLeftBottomRotateHandle;  // Left bottom rotation handle
        Ref<SceneDragHandle> mRightTopRotateHandle;      // Right top rotation handle
        Ref<SceneDragHandle> mRightBottomRotateHandle; // Right bottom rotation handle
        Ref<SceneDragHandle> mLeftTopHandle;              // Left top corner frame handle
        Ref<SceneDragHandle> mLeftHandle;              // Left corner frame handle
        Ref<SceneDragHandle> mLeftBottomHandle;          // Left bottom corner frame handle
        Ref<SceneDragHandle> mTopHandle;                  // Top corner frame handle
        Ref<SceneDragHandle> mBottomHandle;              // Bottom corner frame handle
        Ref<SceneDragHandle> mRightTopHandle;          // Right top corner frame handle
        Ref<SceneDragHandle> mRightHandle;              // Right corner frame handle
        Ref<SceneDragHandle> mRightBottomHandle;          // Right bottom corner frame handle
        Ref<SceneDragHandle> mPivotHandle;              // Frame or object pivot handle
                                                          
        Ref<SceneDragHandle> mAnchorsLeftTopHandle;       // Anchors Left top corner frame handle
        Ref<SceneDragHandle> mAnchorsLeftBottomHandle;  // Anchors Left bottom corner frame handle
        Ref<SceneDragHandle> mAnchorsRightTopHandle;       // Anchors Right top corner frame handle
        Ref<SceneDragHandle> mAnchorsRightBottomHandle; // Anchors Right bottom corner frame handle
        Ref<SceneDragHandle> mAnchorsCenter;            // Anchors center, enables when all anchors in one point and drags all of them
                                                          
        Basis mFrame; // Frame basis

        Basis mAnchorsFrame;                // Anchors frame basis
        bool  mAnchorsFrameEnabled = false; // Is selected some UI widgets and anchors frame enabled

        Vector<Ref<SceneDragHandle3D>> mCornerHandles3D; // 8 corner bracket handles of the 3D bounds frame

        o2::AABB mFrame3D;                  // Selection bounds in frame space, 3D view mode
        Quat     mFrame3DRotation;          // Frame orientation: object local axes for single selection, world otherwise
        bool     mFrame3DValid = false;     // Is 3D frame computed for current selection
        bool     mToolEnabled = false;      // Is tool currently selected

        int   mDragCorner3D = -1;       // Dragged corner handle index, -1 when none
        int   mDragPlaneAxis3D = 2;     // Frame axis of the corner drag plane normal
        Quat  mDragFrameRotation3D;     // Frame orientation frozen at corner drag start
        Vec3F mDragAnchor3D;            // Fixed opposite corner during 3D drag, world space
        Vec3F mDragPlaneOrigin3D;       // Raycast plane origin during 3D corner drag, world space
        Vec3F mLastCornerPoint3D;       // Last applied corner offset from anchor, frame space

        bool mPivotHandleEnabled = false; // Is selected object supports pivot 

        Basis mBeginDraggingFrame;  // Frame before dragging any handle
        Vec2F mBeginDraggingOffset; // Offset at beginning dragging from frame origin to cursor

        bool mIsDragging = false;      // Is frame dragging
        bool mChangedFromThis = false; // Is objects changed from this, needs to break circular updating

        Ref<TransformAction> mTransformAction; // Current transform action. Creates when transform started

        Ref<TransformAction> mKeyboardAction;     // Coalesces consecutive arrow-key presses into one undo entry
        int                  mPressedArrowsCount = 0; // Number of arrow keys currently held down

        Vector<SnapLine> mSnapLines; // Immediate drawing lines, used for drawing snapping

    protected:
        // Returns toggle in menu panel icon name
        String GetPanelIcon() const override;

        // Returns shortcut keys for toggle
        ShortcutKeys GetShortcut() const override;

        // Draws tool
        void DrawScene() override;

        // Draws screen space bounds visualization in 3D view mode
        void DrawScreen() override;

        // Updates tool: places and enables 3D frame handles
        void Update(float dt) override;

        // Recomputes 3D bounds frame and updates corner bracket handles
        void Update3DHandles();

        // Returns world position of the 3D frame corner: bit 0 - x max, bit 1 - y max, bit 2 - z max
        Vec3F GetFrameCorner3D(int corner) const;

        // Called when 3D corner handle was pressed, stores the anchor corner and the drag plane
        void CornerHandle3DPressed(int corner);

        // Called when 3D corner handle was moved, scales two view-facing axes anchored at the opposite corner
        void OnCornerHandle3DMoved(int corner);

        // Called when 3D handle was released, resets drag state and completes the action
        void Handle3DReleased();

        // Builds a frame axis aligned scale step around anchor: x/y through the basis, z through positionZ and sizeZ
        void AppendScaleAroundAnchorStep3D(const Ref<TransformAction>& action, const Vec3F& anchor,
                                           const Quat& frameRotation, const Vec3F& scale);

        // Draws snapping lines
        void DrawSnapLines();

        // Called when tool was enabled
        void OnEnabled() override;

        // Called when tool was disabled
        void OnDisabled() override;

        // Called when scene objects was changed
        void OnSceneChanged(const Vector<Ref<SceneEditableObject>>& changedObjects) override;

        // Called when objects selection was changed
        void OnObjectsSelectionChanged(const Vector<Ref<SceneEditableObject>>& objects) override;

        // Called when key was pressed
        void OnKeyPressed(const Input::Key& key) override;

        // Called when key stay down during frame
        void OnKeyStayDown(const Input::Key& key) override;

        // Called when key was pressed
        void OnKeyReleased(const Input::Key& key) override;

        // Transforms top selected objects
        void TransformObjects(const Basis& transform);

        // Transforms top selected objects in a one-shot action
        void TransformObjectsWithAction(const Basis& transform);

        // Builds a transform step and Appends it to action
        void AppendTransformStep(const Ref<TransformAction>& action, const Basis& transform);

        // Builds a pivot step and Appends it to action
        void AppendPivotStep(const Ref<TransformAction>& action, const Vec2F& pivot);

        // Builds an anchors step (SetLayout + SetTransform pair) and Appends it to action
        void AppendAnchorsStep(const Ref<TransformAction>& action, const Basis& anchorsBasis);

        // Opens (or refcounts into) the keyboard-arrow batch action
        void BeginKeyboardAction();

        // Appends a translation step to the open keyboard-arrow batch action
        void AppendKeyboardStep(const Vec2F& delta);

        // Decrements the keyboard-arrow batch refcount; commits the action when no arrow is held
        void EndKeyboardAction();

        // Transforms top selected objects anchors
        void TransformAnchorsObjects(const Basis& transform);

        // Updates selection frame and handles
        void UpdateSelectionFrame();

        // Called when cursor pressed on this
        void OnCursorPressed(const Input::Cursor& cursor) override;

        // Called when cursor released (only when cursor pressed this at previous time)
        void OnCursorReleased(const Input::Cursor& cursor) override;

        // Called when cursor pressing was broken (when scrolled scroll area or some other)
        void OnCursorPressBreak(const Input::Cursor& cursor) override;

        // Called when cursor stay down during frame
        void OnCursorStillDown(const Input::Cursor& cursor) override;

        // Left top handle moved
        void OnLeftTopHandle(const Vec2F& position);

        // Left handle moved
        void OnLeftHandle(const Vec2F& position);

        // Left bottom handle moved
        void OnLeftBottomHandle(const Vec2F& position);

        // Top handle moved
        void OnTopHandle(const Vec2F& position);

        // Bottom handle moved
        void OnBottomHandle(const Vec2F& position);

        // Right top handle moved
        void OnRightTopHandle(const Vec2F& position);

        // Right handle moved
        void OnRightHandle(const Vec2F& position);

        // Right bottom handle moved
        void OnRightBottomHandle(const Vec2F& position);

        // Left top anchor handle moved
        void OnAnchorLeftTopHandle(const Vec2F& position);

        // Left bottom anchor handle moved
        void OnAnchorLeftBottomHandle(const Vec2F& position);

        // Right top anchor handle moved
        void OnAnchorRightTopHandle(const Vec2F& position);

        // Right bottom anchor handle moved
        void OnAnchorRightBottomHandle(const Vec2F& position);

        // Center anchor handle moved
        void OnCenterAnchorHandle(const Vec2F& position);

        // Pivot handle moved
        void OnPivotHandle(const Vec2F& position);

        // Left top rotation handle moved
        void OnLeftTopRotateHandle(const Vec2F& position);

        // Left bottom rotation handle moved
        void OnLeftBottomRotateHandle(const Vec2F& position);

        // Right top rotation handle moved
        void OnRightTopRotateHandle(const Vec2F& position);

        // Right bottom rotation handle moved
        void OnRightBottomRotateHandle(const Vec2F& position);

        // Rotate handle moved
        void OnRotateHandle(const Vec2F& position, Vec2F lastHandleCoords);

        // Sets all handles enable
        void SetHandlesEnable(bool enable);

        // Updates handles position and angle
        void UpdateHandlesTransform();

        // Called when some handle was pressed, stores before transformations
        void HandlePressed();

        // Called when handle was released, completes transformation action
        void HandleReleased();

        // Return transformed basis when Left top handle moved
        Basis GetLeftTopHandleTransformed(const Vec2F& position);

        // Return transformed basis when Left handle moved
        Basis GetLeftHandleTransformed(const Vec2F& position);

        // Return transformed basis when Left bottom handle moved
        Basis GetLeftBottomHandleTransformed(const Vec2F& position);

        // Return transformed basis when Top handle moved
        Basis GetTopHandleTransformed(const Vec2F& position);

        // Return transformed basis when Bottom handle moved
        Basis GetBottomHandleTransformed(const Vec2F& position);

        // Return transformed basis when Right top handle moved
        Basis GetRightTopHandleTransformed(const Vec2F& position);

        // Return transformed basis when Right handle moved
        Basis GetRightHandleTransformed(const Vec2F& position);

        // Return transformed basis when Right bottom handle moved
        Basis GetRightBottomHandleTransformed(const Vec2F& position);

        // Return transformed anchor basis when Left top handle moved
        Basis GetLeftTopAnchorHandleTransformed(const Vec2F& position);

        // Return transformed anchor basis when Left bottom handle moved
        Basis GetLeftBottomAnchorHandleTransformed(const Vec2F& position);

        // Return transformed anchor basis when Right top handle moved
        Basis GetRightTopAnchorHandleTransformed(const Vec2F& position);

        // Return transformed anchor basis when Right bottom handle moved
        Basis GetRightBottomAnchorHandleTransformed(const Vec2F& position);

        // Return transformed anchor basis when center anchor moved
        Basis GetAnchorsCenterHandleTransformed(const Vec2F& position);

        // Checks handle position snapping to center and corners of frame
        Vec2F CheckFrameSnapping(const Vec2F& point, const Basis& frame);

        // Checks pivot handle position snapping to center and corners
        Vec2F CheckPivotSnapping(const Vec2F& point);

        // Checks anchor handle position snapping to center and corners of parent
        Vec2F CheckAnchorCenterSnapping(const Vec2F& point);

        // Checks anchor handle position snapping to center and corners of parent
        Vec2F CheckAnchorLeftTopSnapping(const Vec2F& point);

        // Checks anchor handle position snapping to center and corners of parent
        Vec2F CheckAnchorLeftBottomSnapping(const Vec2F& point);

        // Checks anchor handle position snapping to center and corners of parent
        Vec2F CheckAnchorRightTopSnapping(const Vec2F& point);

        // Checks anchor handle position snapping to center and corners of parent
        Vec2F CheckAnchorRightBottomSnapping(const Vec2F& point);

        // Checks top handle position snapping to other objects
        Vec2F CheckTopSnapping(const Vec2F& point);

        // Checks bottom handle position snapping to other objects
        Vec2F CheckBottomSnapping(const Vec2F& point);

        // Checks left handle position snapping to other objects
        Vec2F CheckLeftSnapping(const Vec2F& point);

        // Checks right handle position snapping to other objects
        Vec2F CheckRightSnapping(const Vec2F& point);

        // Checks left top handle position snapping to other objects
        Vec2F CheckLeftTopSnapping(const Vec2F& point);

        // Checks left bottom handle position snapping to other objects
        Vec2F CheckLeftBottomSnapping(const Vec2F& point);

        // Checks right top handle position snapping to other objects
        Vec2F CheckRightTopSnapping(const Vec2F& point);

        // Checks right bottom handle position snapping to other objects
        Vec2F CheckRightBottomSnapping(const Vec2F& point);

        // Checks is point in area of top handle
        bool IsPointInTopHandle(const Vec2F& point);

        // Checks is point in area of left handle
        bool IsPointInLeftHandle(const Vec2F& point);

        // Checks is point in area of right handle
        bool IsPointInRightHandle(const Vec2F& point);

        // Checks is point in area of bottom handle
        bool IsPointInBottomHandle(const Vec2F& point);

        // Checks is point in area of center anchors handle
        bool IsPointInAnchorsCenterHandle(const Vec2F& point);

        // Checks is all anchor handles in same point and enables center anchors handle
        void CheckAnchorsCenterEnabled();

        // Returns objects' transforms 
        Vector<Basis> GetObjectsTransforms(Vector<Ref<SceneEditableObject>> objects) const;

        // Returns all objects' transforms for snapping and including anchors frame when enabled
        Vector<Basis> GetSnapBasisesForAllObjects() const;

        // Returns object's parent snap basis - world rect or children rect
        Basis GetObjectParentAnchorSnapBasis(const Ref<SceneEditableObject>& object);

        // Calculates snapping offset for point by parallels lines, offset is on normal
        Vec2F CalculateSnapOffset(const Vec2F& point, const Basis& frame, 
                                  const Vector<Vec2F>& xLines, const Vec2F& xNormal,
                                  const Vector<Vec2F>& yLines, const Vec2F& yNormal,
                                  Vector<Basis> basises = Vector<Basis>());
    };
}
// --- META ---

CLASS_BASES_META(Editor::FrameTool)
{
    BASE_CLASS(Editor::ITransformTool);
}
END_META;
CLASS_FIELDS_META(Editor::FrameTool)
{
    FIELD().PROTECTED().DEFAULT_VALUE(Color4(44, 62, 80, 255)).NAME(mFrameColor);
    FIELD().PROTECTED().DEFAULT_VALUE(Color4(44, 62, 80, 150)).NAME(mObjectColor);
    FIELD().PROTECTED().DEFAULT_VALUE(Color4(44, 62, 80, 100)).NAME(mParentColor);
    FIELD().PROTECTED().DEFAULT_VALUE(Color4(20, 100, 255, 255)).NAME(mAnchorsFrameColor);
    FIELD().PROTECTED().DEFAULT_VALUE(Color4(40, 255, 100, 255)).NAME(mSnapLinesColor);
    FIELD().PROTECTED().DEFAULT_VALUE(0.001f).NAME(mFrameMinimalSize);
    FIELD().PROTECTED().NAME(mLeftTopRotateHandle);
    FIELD().PROTECTED().NAME(mLeftBottomRotateHandle);
    FIELD().PROTECTED().NAME(mRightTopRotateHandle);
    FIELD().PROTECTED().NAME(mRightBottomRotateHandle);
    FIELD().PROTECTED().NAME(mLeftTopHandle);
    FIELD().PROTECTED().NAME(mLeftHandle);
    FIELD().PROTECTED().NAME(mLeftBottomHandle);
    FIELD().PROTECTED().NAME(mTopHandle);
    FIELD().PROTECTED().NAME(mBottomHandle);
    FIELD().PROTECTED().NAME(mRightTopHandle);
    FIELD().PROTECTED().NAME(mRightHandle);
    FIELD().PROTECTED().NAME(mRightBottomHandle);
    FIELD().PROTECTED().NAME(mPivotHandle);
    FIELD().PROTECTED().NAME(mAnchorsLeftTopHandle);
    FIELD().PROTECTED().NAME(mAnchorsLeftBottomHandle);
    FIELD().PROTECTED().NAME(mAnchorsRightTopHandle);
    FIELD().PROTECTED().NAME(mAnchorsRightBottomHandle);
    FIELD().PROTECTED().NAME(mAnchorsCenter);
    FIELD().PROTECTED().NAME(mFrame);
    FIELD().PROTECTED().NAME(mAnchorsFrame);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mAnchorsFrameEnabled);
    FIELD().PROTECTED().NAME(mCornerHandles3D);
    FIELD().PROTECTED().NAME(mFrame3D);
    FIELD().PROTECTED().NAME(mFrame3DRotation);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mFrame3DValid);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mToolEnabled);
    FIELD().PROTECTED().DEFAULT_VALUE(-1).NAME(mDragCorner3D);
    FIELD().PROTECTED().DEFAULT_VALUE(2).NAME(mDragPlaneAxis3D);
    FIELD().PROTECTED().NAME(mDragFrameRotation3D);
    FIELD().PROTECTED().NAME(mDragAnchor3D);
    FIELD().PROTECTED().NAME(mDragPlaneOrigin3D);
    FIELD().PROTECTED().NAME(mLastCornerPoint3D);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mPivotHandleEnabled);
    FIELD().PROTECTED().NAME(mBeginDraggingFrame);
    FIELD().PROTECTED().NAME(mBeginDraggingOffset);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mIsDragging);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mChangedFromThis);
    FIELD().PROTECTED().NAME(mTransformAction);
    FIELD().PROTECTED().NAME(mKeyboardAction);
    FIELD().PROTECTED().DEFAULT_VALUE(0).NAME(mPressedArrowsCount);
    FIELD().PROTECTED().NAME(mSnapLines);
}
END_META;
CLASS_METHODS_META(Editor::FrameTool)
{

    FUNCTION().PUBLIC().CONSTRUCTOR();
    FUNCTION().PROTECTED().SIGNATURE(String, GetPanelIcon);
    FUNCTION().PROTECTED().SIGNATURE(ShortcutKeys, GetShortcut);
    FUNCTION().PROTECTED().SIGNATURE(void, DrawScene);
    FUNCTION().PROTECTED().SIGNATURE(void, DrawScreen);
    FUNCTION().PROTECTED().SIGNATURE(void, Update, float);
    FUNCTION().PROTECTED().SIGNATURE(void, Update3DHandles);
    FUNCTION().PROTECTED().SIGNATURE(Vec3F, GetFrameCorner3D, int);
    FUNCTION().PROTECTED().SIGNATURE(void, CornerHandle3DPressed, int);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCornerHandle3DMoved, int);
    FUNCTION().PROTECTED().SIGNATURE(void, Handle3DReleased);
    FUNCTION().PROTECTED().SIGNATURE(void, AppendScaleAroundAnchorStep3D, const Ref<TransformAction>&, const Vec3F&, const Quat&, const Vec3F&);
    FUNCTION().PROTECTED().SIGNATURE(void, DrawSnapLines);
    FUNCTION().PROTECTED().SIGNATURE(void, OnEnabled);
    FUNCTION().PROTECTED().SIGNATURE(void, OnDisabled);
    FUNCTION().PROTECTED().SIGNATURE(void, OnSceneChanged, const Vector<Ref<SceneEditableObject>>&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnObjectsSelectionChanged, const Vector<Ref<SceneEditableObject>>&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnKeyPressed, const Input::Key&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnKeyStayDown, const Input::Key&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnKeyReleased, const Input::Key&);
    FUNCTION().PROTECTED().SIGNATURE(void, TransformObjects, const Basis&);
    FUNCTION().PROTECTED().SIGNATURE(void, TransformObjectsWithAction, const Basis&);
    FUNCTION().PROTECTED().SIGNATURE(void, AppendTransformStep, const Ref<TransformAction>&, const Basis&);
    FUNCTION().PROTECTED().SIGNATURE(void, AppendPivotStep, const Ref<TransformAction>&, const Vec2F&);
    FUNCTION().PROTECTED().SIGNATURE(void, AppendAnchorsStep, const Ref<TransformAction>&, const Basis&);
    FUNCTION().PROTECTED().SIGNATURE(void, BeginKeyboardAction);
    FUNCTION().PROTECTED().SIGNATURE(void, AppendKeyboardStep, const Vec2F&);
    FUNCTION().PROTECTED().SIGNATURE(void, EndKeyboardAction);
    FUNCTION().PROTECTED().SIGNATURE(void, TransformAnchorsObjects, const Basis&);
    FUNCTION().PROTECTED().SIGNATURE(void, UpdateSelectionFrame);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCursorPressed, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCursorReleased, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCursorPressBreak, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCursorStillDown, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnLeftTopHandle, const Vec2F&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnLeftHandle, const Vec2F&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnLeftBottomHandle, const Vec2F&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnTopHandle, const Vec2F&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnBottomHandle, const Vec2F&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnRightTopHandle, const Vec2F&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnRightHandle, const Vec2F&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnRightBottomHandle, const Vec2F&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnAnchorLeftTopHandle, const Vec2F&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnAnchorLeftBottomHandle, const Vec2F&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnAnchorRightTopHandle, const Vec2F&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnAnchorRightBottomHandle, const Vec2F&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCenterAnchorHandle, const Vec2F&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnPivotHandle, const Vec2F&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnLeftTopRotateHandle, const Vec2F&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnLeftBottomRotateHandle, const Vec2F&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnRightTopRotateHandle, const Vec2F&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnRightBottomRotateHandle, const Vec2F&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnRotateHandle, const Vec2F&, Vec2F);
    FUNCTION().PROTECTED().SIGNATURE(void, SetHandlesEnable, bool);
    FUNCTION().PROTECTED().SIGNATURE(void, UpdateHandlesTransform);
    FUNCTION().PROTECTED().SIGNATURE(void, HandlePressed);
    FUNCTION().PROTECTED().SIGNATURE(void, HandleReleased);
    FUNCTION().PROTECTED().SIGNATURE(Basis, GetLeftTopHandleTransformed, const Vec2F&);
    FUNCTION().PROTECTED().SIGNATURE(Basis, GetLeftHandleTransformed, const Vec2F&);
    FUNCTION().PROTECTED().SIGNATURE(Basis, GetLeftBottomHandleTransformed, const Vec2F&);
    FUNCTION().PROTECTED().SIGNATURE(Basis, GetTopHandleTransformed, const Vec2F&);
    FUNCTION().PROTECTED().SIGNATURE(Basis, GetBottomHandleTransformed, const Vec2F&);
    FUNCTION().PROTECTED().SIGNATURE(Basis, GetRightTopHandleTransformed, const Vec2F&);
    FUNCTION().PROTECTED().SIGNATURE(Basis, GetRightHandleTransformed, const Vec2F&);
    FUNCTION().PROTECTED().SIGNATURE(Basis, GetRightBottomHandleTransformed, const Vec2F&);
    FUNCTION().PROTECTED().SIGNATURE(Basis, GetLeftTopAnchorHandleTransformed, const Vec2F&);
    FUNCTION().PROTECTED().SIGNATURE(Basis, GetLeftBottomAnchorHandleTransformed, const Vec2F&);
    FUNCTION().PROTECTED().SIGNATURE(Basis, GetRightTopAnchorHandleTransformed, const Vec2F&);
    FUNCTION().PROTECTED().SIGNATURE(Basis, GetRightBottomAnchorHandleTransformed, const Vec2F&);
    FUNCTION().PROTECTED().SIGNATURE(Basis, GetAnchorsCenterHandleTransformed, const Vec2F&);
    FUNCTION().PROTECTED().SIGNATURE(Vec2F, CheckFrameSnapping, const Vec2F&, const Basis&);
    FUNCTION().PROTECTED().SIGNATURE(Vec2F, CheckPivotSnapping, const Vec2F&);
    FUNCTION().PROTECTED().SIGNATURE(Vec2F, CheckAnchorCenterSnapping, const Vec2F&);
    FUNCTION().PROTECTED().SIGNATURE(Vec2F, CheckAnchorLeftTopSnapping, const Vec2F&);
    FUNCTION().PROTECTED().SIGNATURE(Vec2F, CheckAnchorLeftBottomSnapping, const Vec2F&);
    FUNCTION().PROTECTED().SIGNATURE(Vec2F, CheckAnchorRightTopSnapping, const Vec2F&);
    FUNCTION().PROTECTED().SIGNATURE(Vec2F, CheckAnchorRightBottomSnapping, const Vec2F&);
    FUNCTION().PROTECTED().SIGNATURE(Vec2F, CheckTopSnapping, const Vec2F&);
    FUNCTION().PROTECTED().SIGNATURE(Vec2F, CheckBottomSnapping, const Vec2F&);
    FUNCTION().PROTECTED().SIGNATURE(Vec2F, CheckLeftSnapping, const Vec2F&);
    FUNCTION().PROTECTED().SIGNATURE(Vec2F, CheckRightSnapping, const Vec2F&);
    FUNCTION().PROTECTED().SIGNATURE(Vec2F, CheckLeftTopSnapping, const Vec2F&);
    FUNCTION().PROTECTED().SIGNATURE(Vec2F, CheckLeftBottomSnapping, const Vec2F&);
    FUNCTION().PROTECTED().SIGNATURE(Vec2F, CheckRightTopSnapping, const Vec2F&);
    FUNCTION().PROTECTED().SIGNATURE(Vec2F, CheckRightBottomSnapping, const Vec2F&);
    FUNCTION().PROTECTED().SIGNATURE(bool, IsPointInTopHandle, const Vec2F&);
    FUNCTION().PROTECTED().SIGNATURE(bool, IsPointInLeftHandle, const Vec2F&);
    FUNCTION().PROTECTED().SIGNATURE(bool, IsPointInRightHandle, const Vec2F&);
    FUNCTION().PROTECTED().SIGNATURE(bool, IsPointInBottomHandle, const Vec2F&);
    FUNCTION().PROTECTED().SIGNATURE(bool, IsPointInAnchorsCenterHandle, const Vec2F&);
    FUNCTION().PROTECTED().SIGNATURE(void, CheckAnchorsCenterEnabled);
    FUNCTION().PROTECTED().SIGNATURE(Vector<Basis>, GetObjectsTransforms, Vector<Ref<SceneEditableObject>>);
    FUNCTION().PROTECTED().SIGNATURE(Vector<Basis>, GetSnapBasisesForAllObjects);
    FUNCTION().PROTECTED().SIGNATURE(Basis, GetObjectParentAnchorSnapBasis, const Ref<SceneEditableObject>&);
    FUNCTION().PROTECTED().SIGNATURE(Vec2F, CalculateSnapOffset, const Vec2F&, const Basis&, const Vector<Vec2F>&, const Vec2F&, const Vector<Vec2F>&, const Vec2F&, Vector<Basis>);
}
END_META;
// --- END META ---
