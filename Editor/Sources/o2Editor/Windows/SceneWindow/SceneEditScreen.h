#pragma once

#include "o2/Events/DrawableCursorEventsListener.h"
#include "o2/Events/KeyboardEventsListener.h"
#include "o2/Render/Camera.h"
#include "o2/Render/IDrawable.h"
#include "o2/Render/Material.h"
#include "o2/Render/Mesh.h"
#include "o2/Render/TextureRef.h"
#include "o2/Utils/Editor/DragAndDrop.h"
#include "o2/Utils/Singleton.h"
#include "o2Editor/UI/ScrollView.h"
#include "o2Editor/Windows/SceneWindow/SceneView3DState.h"

using namespace o2;

namespace o2
{
    class Sprite;
    class SceneEditableObject;
    class Component;
    class Tree;
}

// Editor scene screen accessor macros
#define o2EditorSceneScreen SceneEditScreen::Instance()

namespace Editor
{
    FORWARD_CLASS_REF(IEditTool);
    FORWARD_CLASS_REF(SceneDragHandle);
    FORWARD_CLASS_REF(SceneEditorLayer);
    FORWARD_CLASS_REF(SceneHierarchyTree);

    // --------------------
    // Scene editing screen
    // --------------------
    class SceneEditScreen : public Singleton<SceneEditScreen>, public ScrollView, public DragDropArea, public KeyboardEventsListener
    {
    public:
        Function<void(const Vector<Ref<SceneEditableObject>>&)> onSelectionChanged; // Actors selection change event

        Function<void(bool)> onView3DModeChanged; // 3D view mode toggle event

        // Default constructor
        SceneEditScreen(RefCounter* refCounter);

        // Copy-constructor
        SceneEditScreen(RefCounter* refCounter, const SceneEditScreen& other);

        // Destructor
        ~SceneEditScreen();

        // Draws widget
        void Draw() override;

        // Called when required to redraw content. Sets flag and redraws at next frame
        void NeedRedraw();

        // Updates drawables, states and widget
        void Update(float dt) override;

        // Returns is listener scrollable
        bool IsScrollable() const override;

        // Transforms point from screen space to scene space
        Vec2F ScreenToScenePoint(const Vec2F& point);

        // Transforms point from scene space to screen space
        Vec2F SceneToScreenPoint(const Vec2F& point);

        // Transforms point from screen space to scene space
        Vec2F ScreenToSceneVector(const Vec2F& point);

        // Transforms point from scene space to screen space
        Vec2F SceneToScreenVector(const Vec2F& point);

        // Enables or disables 3D view mode, keeping the view spatially continuous
        void SetView3DMode(bool enabled);

        // Returns is 3D view mode enabled
        bool IsView3DMode() const;

        // Returns 3D view state
        SceneView3DState& GetView3DState();

        // Returns z of the closest point on the vertical axis through plane anchor to the view ray, 3D mode only
        bool ScreenToZAxisPoint(const Vec2F& screenPoint, const Vec2F& planeAnchor, float& z);

        // Returns parameter of the closest point on the world line axisOrigin + param*axisDir to the view ray, 3D mode only
        bool ScreenToWorldAxisParam(const Vec2F& screenPoint, const Vec3F& axisOrigin, const Vec3F& axisDir, float& param);

        // Returns intersection of the view ray under screen point with an arbitrary world plane, 3D mode only
        bool ScreenToWorldPlanePoint(const Vec2F& screenPoint, const Vec3F& planeOrigin, const Vec3F& planeNormal, Vec3F& result);

        // Transforms world point with z to screen space, 3D mode only
        Vec2F World3DToScreenPoint(const Vec3F& worldPoint);

        // Returns view ray under screen point, 3D mode only
        bool ScreenToWorldRay(const Vec2F& screenPoint, Vec3F& origin, Vec3F& direction);

        // Returns is fly navigation active: right mouse held in 3D mode, WASD/QE moves camera
        bool IsFlyNavigation3D() const;

        // Sets fly navigation flag and suppresses all keyboard shortcuts while it is active
        void SetFlyNavigation3D(bool active);

        // Draws object selection
        void DrawObjectSelection(const Ref<SceneEditableObject>& object, const Color4& color);

        // Selects objects
        void SelectObjects(const Vector<Ref<SceneEditableObject>>& objects, bool additive = true);

        // Selects object
        void SelectObject(const Ref<SceneEditableObject>& object, bool additive = true);

        // Selects all objects
        void SelectAllObjects();

        // Clears objects selection
        void ClearSelection();

        // Sets selection from ids, skipping unresolved ones, without recording an action
        void SelectObjectsByIdsWithoutAction(const Vector<SceneUID>& ids);

        // Returns left top widgets container, can be used for tools additional controls
        const Ref<HorizontalLayout>& GetLeftTopWidgetsContainer();

        // Returns right top widgets container, can be used for tools additional controls
        const Ref<HorizontalLayout>& GetRightTopWidgetsContainer();

        // Returns left bottom widgets container, can be used for tools additional controls
        const Ref<HorizontalLayout>& GetLeftBottomWidgetsContainer();

        // Returns right bottom widgets container, can be used for tools additional controls
        const Ref<HorizontalLayout>& GetRightBottomWidgetsContainer();

        // Adds editable layer
        void AddEditorLayer(const Ref<SceneEditorLayer>& layer);

        // Removes editable layer
        void RemoveEditorLayer(const Ref<SceneEditorLayer>& layer);

        // Sets layers with name enabled
        void SetLayerEnabled(const String& name, bool enabled);

        // Returns is layer enabled
        bool IsLayerEnabled(const String& name) const;

        // Selects tool with type
        template<typename _type>
        void SelectTool();

        // Selects tool
        void SelectTool(const Ref<IEditTool>& tool);

        // Returns selected tool
        const Ref<IEditTool>& GetSelectedTool() const;

        // Adds tool
        void AddTool(const Ref<IEditTool>& tool);

        // Removes tool
        void RemoveTool(const Ref<IEditTool>& tool);

        // Returns tool by type, or null if it doesn't exists
        template<typename _type>
        Ref<_type> GetTool();

        // Returns all registered tools
        const Vector<Ref<IEditTool>>& GetTools() const;

        // Returns selected objects array
        const Vector<Ref<SceneEditableObject>>& GetSelectedObjects() const;

        // Returns top selected objects in hierarchy
        const Vector<Ref<SceneEditableObject>>& GetTopSelectedObjects() const;

        // Returns color for single selected object
        const Color4& GetSingleObjectSelectionColor() const;

        // Return color for multiple selected objects
        const Color4& GetManyObjectsSelectionColor() const;

        // Called when scene was changed and needs to redraw
        void OnSceneChanged();

        // Returns true if point is in this object
        bool IsUnderPoint(const Vec2F& point) override;

        // Dynamic cast to RefCounterable via Singleton<SceneEditScreen>
        static Ref<RefCounterable> CastToRefCounterable(const Ref<SceneEditScreen>& ref);

        SERIALIZABLE(SceneEditScreen);
        CLONEABLE_REF(SceneEditScreen);

    protected:
        Color4 mSelectedObjectColor = Color4(220, 220, 220, 255);      // Selected object color
        Color4 mMultiSelectedObjectColor = Color4(220, 220, 220, 100); // Selected object color

        Color4 mSelection3DOutlineColor = Color4(10, 165, 150, 255); // Silhouette outline color of selected 3D objects

        TextureRef    mSelectionMaskTarget;     // Offscreen silhouette mask of selected 3D objects
        Ref<Material> mSelectionOutlineMaterial; // Outline composite material (Shaders/SelectionOutline)
        Ref<Mesh>     mSelectionOutlineQuad;     // Fullscreen quad for the outline composite
        float  mObjectMinimalSelectionSize = 10.0f;                    // Minimal object size on pixels

        Vector<Ref<SceneEditableObject>> mSelectedObjects;          // Current selected objects
        Vector<Ref<SceneEditableObject>> mTopSelectedObjects;       // Current selected objects most top in hierarchy
        bool                             mSelectedFromThis = false; // True if selection changed from this, needs to break recursive selection update

        Vector<Ref<IEditTool>> mTools;       // Available tools
        Ref<IEditTool>         mEnabledTool; // Current enabled tool

        Vector<Ref<SceneDragHandle>> mDragHandles; // Dragging handles array

        Vector<Ref<SceneEditorLayer>> mEditorLayers;        // List of editable layers
        Map<String, bool>             mEditorLayersEnabled; // Map of enabled or disabled layers by user

        Ref<HorizontalLayout> mLeftTopWidgetsContainer;     // Additional controls widgets container at left top
        Ref<HorizontalLayout> mRightTopWidgetsContainer;    // Additional controls widgets container at right top
        Ref<HorizontalLayout> mLeftBottomWidgetsContainer;  // Additional controls widgets container at left bottom
        Ref<HorizontalLayout> mRightBottomWidgetsContainer; // Additional controls widgets container at right bottom

        bool             m3DMode = false; // Is 3D view mode enabled
        SceneView3DState mView3D;         // Orbit camera state for 3D view mode

        bool mFlyNavigation3D = false; // Is fly navigation active: right mouse held in 3D mode, WASD/QE moves camera
        bool mAltOrbit3D = false;      // Is alt+left mouse orbit navigation active in 3D mode

    protected:
        // Initializes tools
        void InitializeTools();

        // Creates and configures widgets container with specified base corner
        Ref<HorizontalLayout> InitializeWidgetsContainer(BaseCorner baseCorner);

        // Returns true if some handle hovered or pressed by cursor
        bool IsHandleWorking(const Input::Cursor& cursor) const;

        // Called when cursor pressed on this
        void OnCursorPressed(const Input::Cursor& cursor) override;

        // Called when cursor released (only when cursor pressed this at previous time)
        void OnCursorReleased(const Input::Cursor& cursor) override;

        // Called when cursor pressing was broken (when scrolled scroll area or some other)
        void OnCursorPressBreak(const Input::Cursor& cursor) override;

        // Called when cursor stay down during frame
        void OnCursorStillDown(const Input::Cursor& cursor) override;

        // Called when cursor moved on this (or moved outside when this was pressed)
        void OnCursorMoved(const Input::Cursor& cursor) override;

        // Called when cursor enters this object
        void OnCursorEnter(const Input::Cursor& cursor) override;

        // Called when cursor exits this object
        void OnCursorExit(const Input::Cursor& cursor) override;

        // Called when right mouse button was pressed on this
        void OnCursorRightMousePressed(const Input::Cursor& cursor) override;

        // Called when right mouse button stay down on this
        void OnCursorRightMouseStayDown(const Input::Cursor& cursor) override;

        // Called when right mouse button was released (only when right mouse button pressed this at previous time)
        void OnCursorRightMouseReleased(const Input::Cursor& cursor) override;

        // Called when middle mouse button was pressed on this
        void OnCursorMiddleMousePressed(const Input::Cursor& cursor) override;

        // Called when middle mouse button stay down on this
        void OnCursorMiddleMouseStayDown(const Input::Cursor& cursor) override;

        // Called when middle mouse button was released (only when middle mouse button pressed this at previous time)
        void OnCursorMiddleMouseReleased(const Input::Cursor& cursor) override;

        // Called when scrolling
        void OnScrolled(float scroll) override;

        // Called when key was pressed
        void OnKeyPressed(const Input::Key& key) override;

        // Called when key was released
        void OnKeyReleased(const Input::Key& key) override;

        // Called when key stay down during frame
        void OnKeyStayDown(const Input::Key& key) override;

        // Called when changed selected objects from this
        void OnObjectsSelectedFromThis();

        // Redraws scene texture
        void RedrawContent() override;

        // Draws grid on the z=0 plane in 3D mode
        void Draw3DGrid();

        // Updates view camera to cover the widget rect 1:1 in 3D mode, so cursor events stay in screen space
        void UpdateScreenSpaceCamera();

        // Moves 3D view camera by WASD/QE keys while fly navigation is active
        void UpdateFlyNavigation(float dt);

        // Returns viewport size for 3D projection
        Vec2F GetViewportSize() const;

        // Converts screen point to viewport point (left-bottom origin, y up)
        Vec2F ScreenToViewportPoint(const Vec2F& point) const;

        // Converts viewport point to screen point
        Vec2F ViewportToScreenPoint(const Vec2F& point) const;

        // Converts cursor position and delta to scene space in 3D mode; returns cursor unchanged in 2D
        Input::Cursor ToSceneCursor(const Input::Cursor& cursor);

        // Draws objects drawables components
        void DrawObjects();

        // Draws scene objects through the scene camera render pipeline with the editor view camera (3D mode)
        void DrawObjects3D(const Camera& viewCamera);

        // Draws selection on objects
        void DrawSelection();

        // Draws silhouette outline of selected 3D objects through an offscreen mask (3D mode)
        void DrawSelection3DOutline();

        // Binds to scene tree selection window
        void BindSceneTree();

        // Called when scene tree selection changed
        void OnTreeSelectionChanged(Vector<Ref<SceneEditableObject>> selectedObjects);

        // Updates top selected objects
        void UpdateTopSelectedObjects();

        // Called when objects was changed
        void OnSceneChanged(Vector<Ref<SceneEditableObject>> objects);

        // Clears objects selection
        void ClearSelectionWithoutAction(bool sendSelectedMessage = true);

        // Selects objects
        void SelectObjectsWithoutAction(Vector<Ref<SceneEditableObject>> objects, bool additive = true);

        // Selects object
        void SelectObjectWithoutAction(const Ref<SceneEditableObject>& object, bool additive = true);

        // Called when some selectable listeners was dropped to this
        void OnDropped(const Ref<ISelectableDragableObjectsGroup>& group) override;

        // Called when some drag listeners was entered to this area
        void OnDragEnter(const Ref<ISelectableDragableObjectsGroup>& group) override;

        // Called when some drag listeners was dragged above this area
        void OnDraggedAbove(const Ref<ISelectableDragableObjectsGroup>& group) override;

        // Called when some drag listeners was exited from this area
        void OnDragExit(const Ref<ISelectableDragableObjectsGroup>& group) override;

        // Returns that this has transparent input
        bool IsInputTransparent() const override;

        REF_COUNTERABLE_IMPL(ScrollView);

        friend class DeleteAction;
        friend class SelectAction;
        friend class SelectionTool;
        friend class SceneDragHandle;
        friend class SceneWindow;
        friend class TreeWindow;
        friend class CreateAction;
        friend class DefaultActionsUIBridge;
    };
}

#include "o2Editor/Tools/IEditorTool.h"

namespace Editor
{
    template<typename _type>
    void SceneEditScreen::SelectTool()
    {
        SelectTool(mTools.FindOrDefault([&](auto x) { return x->GetType() == TypeOf(_type); }));
    }

    template<typename _type>
    Ref<_type> SceneEditScreen::GetTool()
    {
        for (auto& tool : mTools) 
        {
            if (auto typedTool = DynamicCast<_type>(tool))
                return typedTool;
        }

        return nullptr;
    }

}
// --- META ---

CLASS_BASES_META(Editor::SceneEditScreen)
{
    BASE_CLASS(o2::Singleton<SceneEditScreen>);
    BASE_CLASS(Editor::ScrollView);
    BASE_CLASS(o2::DragDropArea);
    BASE_CLASS(o2::KeyboardEventsListener);
}
END_META;
CLASS_FIELDS_META(Editor::SceneEditScreen)
{
    FIELD().PUBLIC().NAME(onSelectionChanged);
    FIELD().PUBLIC().NAME(onView3DModeChanged);
    FIELD().PROTECTED().DEFAULT_VALUE(Color4(220, 220, 220, 255)).NAME(mSelectedObjectColor);
    FIELD().PROTECTED().DEFAULT_VALUE(Color4(220, 220, 220, 100)).NAME(mMultiSelectedObjectColor);
    FIELD().PROTECTED().DEFAULT_VALUE(Color4(10, 165, 150, 255)).NAME(mSelection3DOutlineColor);
    FIELD().PROTECTED().NAME(mSelectionMaskTarget);
    FIELD().PROTECTED().NAME(mSelectionOutlineMaterial);
    FIELD().PROTECTED().NAME(mSelectionOutlineQuad);
    FIELD().PROTECTED().DEFAULT_VALUE(10.0f).NAME(mObjectMinimalSelectionSize);
    FIELD().PROTECTED().NAME(mSelectedObjects);
    FIELD().PROTECTED().NAME(mTopSelectedObjects);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mSelectedFromThis);
    FIELD().PROTECTED().NAME(mTools);
    FIELD().PROTECTED().NAME(mEnabledTool);
    FIELD().PROTECTED().NAME(mDragHandles);
    FIELD().PROTECTED().NAME(mEditorLayers);
    FIELD().PROTECTED().NAME(mEditorLayersEnabled);
    FIELD().PROTECTED().NAME(mLeftTopWidgetsContainer);
    FIELD().PROTECTED().NAME(mRightTopWidgetsContainer);
    FIELD().PROTECTED().NAME(mLeftBottomWidgetsContainer);
    FIELD().PROTECTED().NAME(mRightBottomWidgetsContainer);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(m3DMode);
    FIELD().PROTECTED().NAME(mView3D);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mFlyNavigation3D);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mAltOrbit3D);
}
END_META;
CLASS_METHODS_META(Editor::SceneEditScreen)
{

    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*);
    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*, const SceneEditScreen&);
    FUNCTION().PUBLIC().SIGNATURE(void, Draw);
    FUNCTION().PUBLIC().SIGNATURE(void, NeedRedraw);
    FUNCTION().PUBLIC().SIGNATURE(void, Update, float);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsScrollable);
    FUNCTION().PUBLIC().SIGNATURE(Vec2F, ScreenToScenePoint, const Vec2F&);
    FUNCTION().PUBLIC().SIGNATURE(Vec2F, SceneToScreenPoint, const Vec2F&);
    FUNCTION().PUBLIC().SIGNATURE(Vec2F, ScreenToSceneVector, const Vec2F&);
    FUNCTION().PUBLIC().SIGNATURE(Vec2F, SceneToScreenVector, const Vec2F&);
    FUNCTION().PUBLIC().SIGNATURE(void, SetView3DMode, bool);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsView3DMode);
    FUNCTION().PUBLIC().SIGNATURE(SceneView3DState&, GetView3DState);
    FUNCTION().PUBLIC().SIGNATURE(bool, ScreenToZAxisPoint, const Vec2F&, const Vec2F&, float&);
    FUNCTION().PUBLIC().SIGNATURE(bool, ScreenToWorldAxisParam, const Vec2F&, const Vec3F&, const Vec3F&, float&);
    FUNCTION().PUBLIC().SIGNATURE(bool, ScreenToWorldPlanePoint, const Vec2F&, const Vec3F&, const Vec3F&, Vec3F&);
    FUNCTION().PUBLIC().SIGNATURE(Vec2F, World3DToScreenPoint, const Vec3F&);
    FUNCTION().PUBLIC().SIGNATURE(bool, ScreenToWorldRay, const Vec2F&, Vec3F&, Vec3F&);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsFlyNavigation3D);
    FUNCTION().PUBLIC().SIGNATURE(void, SetFlyNavigation3D, bool);
    FUNCTION().PUBLIC().SIGNATURE(void, DrawObjectSelection, const Ref<SceneEditableObject>&, const Color4&);
    FUNCTION().PUBLIC().SIGNATURE(void, SelectObjects, const Vector<Ref<SceneEditableObject>>&, bool);
    FUNCTION().PUBLIC().SIGNATURE(void, SelectObject, const Ref<SceneEditableObject>&, bool);
    FUNCTION().PUBLIC().SIGNATURE(void, SelectAllObjects);
    FUNCTION().PUBLIC().SIGNATURE(void, ClearSelection);
    FUNCTION().PUBLIC().SIGNATURE(void, SelectObjectsByIdsWithoutAction, const Vector<SceneUID>&);
    FUNCTION().PUBLIC().SIGNATURE(const Ref<HorizontalLayout>&, GetLeftTopWidgetsContainer);
    FUNCTION().PUBLIC().SIGNATURE(const Ref<HorizontalLayout>&, GetRightTopWidgetsContainer);
    FUNCTION().PUBLIC().SIGNATURE(const Ref<HorizontalLayout>&, GetLeftBottomWidgetsContainer);
    FUNCTION().PUBLIC().SIGNATURE(const Ref<HorizontalLayout>&, GetRightBottomWidgetsContainer);
    FUNCTION().PUBLIC().SIGNATURE(void, AddEditorLayer, const Ref<SceneEditorLayer>&);
    FUNCTION().PUBLIC().SIGNATURE(void, RemoveEditorLayer, const Ref<SceneEditorLayer>&);
    FUNCTION().PUBLIC().SIGNATURE(void, SetLayerEnabled, const String&, bool);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsLayerEnabled, const String&);
    FUNCTION().PUBLIC().SIGNATURE(void, SelectTool, const Ref<IEditTool>&);
    FUNCTION().PUBLIC().SIGNATURE(const Ref<IEditTool>&, GetSelectedTool);
    FUNCTION().PUBLIC().SIGNATURE(void, AddTool, const Ref<IEditTool>&);
    FUNCTION().PUBLIC().SIGNATURE(void, RemoveTool, const Ref<IEditTool>&);
    FUNCTION().PUBLIC().SIGNATURE(const Vector<Ref<IEditTool>>&, GetTools);
    FUNCTION().PUBLIC().SIGNATURE(const Vector<Ref<SceneEditableObject>>&, GetSelectedObjects);
    FUNCTION().PUBLIC().SIGNATURE(const Vector<Ref<SceneEditableObject>>&, GetTopSelectedObjects);
    FUNCTION().PUBLIC().SIGNATURE(const Color4&, GetSingleObjectSelectionColor);
    FUNCTION().PUBLIC().SIGNATURE(const Color4&, GetManyObjectsSelectionColor);
    FUNCTION().PUBLIC().SIGNATURE(void, OnSceneChanged);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsUnderPoint, const Vec2F&);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(Ref<RefCounterable>, CastToRefCounterable, const Ref<SceneEditScreen>&);
    FUNCTION().PROTECTED().SIGNATURE(void, InitializeTools);
    FUNCTION().PROTECTED().SIGNATURE(Ref<HorizontalLayout>, InitializeWidgetsContainer, BaseCorner);
    FUNCTION().PROTECTED().SIGNATURE(bool, IsHandleWorking, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCursorPressed, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCursorReleased, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCursorPressBreak, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCursorStillDown, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCursorMoved, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCursorEnter, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCursorExit, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCursorRightMousePressed, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCursorRightMouseStayDown, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCursorRightMouseReleased, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCursorMiddleMousePressed, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCursorMiddleMouseStayDown, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCursorMiddleMouseReleased, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnScrolled, float);
    FUNCTION().PROTECTED().SIGNATURE(void, OnKeyPressed, const Input::Key&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnKeyReleased, const Input::Key&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnKeyStayDown, const Input::Key&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnObjectsSelectedFromThis);
    FUNCTION().PROTECTED().SIGNATURE(void, RedrawContent);
    FUNCTION().PROTECTED().SIGNATURE(void, Draw3DGrid);
    FUNCTION().PROTECTED().SIGNATURE(void, UpdateScreenSpaceCamera);
    FUNCTION().PROTECTED().SIGNATURE(void, UpdateFlyNavigation, float);
    FUNCTION().PROTECTED().SIGNATURE(Vec2F, GetViewportSize);
    FUNCTION().PROTECTED().SIGNATURE(Vec2F, ScreenToViewportPoint, const Vec2F&);
    FUNCTION().PROTECTED().SIGNATURE(Vec2F, ViewportToScreenPoint, const Vec2F&);
    FUNCTION().PROTECTED().SIGNATURE(Input::Cursor, ToSceneCursor, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, DrawObjects);
    FUNCTION().PROTECTED().SIGNATURE(void, DrawObjects3D, const Camera&);
    FUNCTION().PROTECTED().SIGNATURE(void, DrawSelection);
    FUNCTION().PROTECTED().SIGNATURE(void, DrawSelection3DOutline);
    FUNCTION().PROTECTED().SIGNATURE(void, BindSceneTree);
    FUNCTION().PROTECTED().SIGNATURE(void, OnTreeSelectionChanged, Vector<Ref<SceneEditableObject>>);
    FUNCTION().PROTECTED().SIGNATURE(void, UpdateTopSelectedObjects);
    FUNCTION().PROTECTED().SIGNATURE(void, OnSceneChanged, Vector<Ref<SceneEditableObject>>);
    FUNCTION().PROTECTED().SIGNATURE(void, ClearSelectionWithoutAction, bool);
    FUNCTION().PROTECTED().SIGNATURE(void, SelectObjectsWithoutAction, Vector<Ref<SceneEditableObject>>, bool);
    FUNCTION().PROTECTED().SIGNATURE(void, SelectObjectWithoutAction, const Ref<SceneEditableObject>&, bool);
    FUNCTION().PROTECTED().SIGNATURE(void, OnDropped, const Ref<ISelectableDragableObjectsGroup>&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnDragEnter, const Ref<ISelectableDragableObjectsGroup>&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnDraggedAbove, const Ref<ISelectableDragableObjectsGroup>&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnDragExit, const Ref<ISelectableDragableObjectsGroup>&);
    FUNCTION().PROTECTED().SIGNATURE(bool, IsInputTransparent);
}
END_META;
// --- END META ---
