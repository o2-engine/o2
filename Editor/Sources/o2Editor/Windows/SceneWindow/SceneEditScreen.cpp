#include "o2Editor/stdafx.h"
#include "SceneEditScreen.h"

#include "o2/Events/ShortcutKeysListener.h"
#include "o2/Integration.h"
#include "o2/Physics/PhysicsWorld.h"
#include "o2/Render/Pipeline/RenderPipeline.h"
#include "o2/Render/Material.h"
#include "o2/Render/Mesh.h"
#include "o2/Render/Render.h"
#include "o2/Render/Sprite.h"
#include "o2/Scene/Actor.h"
#include "o2/Scene/CameraActor.h"
#include "o2/Scene/Component.h"
#include "o2/Scene/Scene.h"
#include "o2/Scene/SceneLayerRef.h"
#include "o2/Scene/UI/UIManager.h"
#include "o2/Scene/UI/Widgets/Tree.h"
#include "o2/Utils/Math/Math.h"
#include "o2Editor/Windows/AssetsWindow/AssetsIconsScroll.h"
#include "o2Editor/Actions/Select.h"
#include "o2Editor/EditorApplication.h"
#include "o2Editor/Tools/FrameTool.h"
#include "o2Editor/Tools/IEditorTool.h"
#include "o2Editor/Tools/MoveTool.h"
#include "o2Editor/Tools/RotateTool.h"
#include "o2Editor/Tools/ScaleTool.h"
#include "o2Editor/Tools/SelectionTool.h"
#include "o2Editor/ToolsPanel.h"
#include "o2Editor/UIRoot.h"
#include "o2Editor/Windows/WindowsManager.h"
#include "o2Editor/Windows/PropertiesWindow/PropertiesWindow.h"
#include "o2Editor/Windows/SceneWindow/SceneDragHandle.h"
#include "o2Editor/Windows/SceneWindow/SceneEditorLayer.h"
#include "o2Editor/Windows/TreeWindow/DrawOrderTree.h"
#include "o2Editor/Windows/TreeWindow/SceneHierarchyTree.h"
#include "o2Editor/Windows/TreeWindow/TreeWindow.h"
#include "o2Editor/Windows/WindowsManager.h"
#include "o2Editor/Windows/SceneWindow/SceneWindow.h"

DECLARE_SINGLETON(Editor::SceneEditScreen);

namespace Editor
{
    SceneEditScreen::SceneEditScreen(RefCounter* refCounter):
        Singleton<SceneEditScreen>(refCounter), ScrollView(refCounter)
    {
        InitializeTools();
        SelectTool<MoveTool>();

        mLeftTopWidgetsContainer = InitializeWidgetsContainer(BaseCorner::LeftTop);
        mRightTopWidgetsContainer = InitializeWidgetsContainer(BaseCorner::RightTop);
        mLeftBottomWidgetsContainer = InitializeWidgetsContainer(BaseCorner::LeftBottom);
        mRightBottomWidgetsContainer = InitializeWidgetsContainer(BaseCorner::RightBottom);
    }

    SceneEditScreen::SceneEditScreen(RefCounter* refCounter, const SceneEditScreen& other) :
        SceneEditScreen(refCounter)
    {}

    SceneEditScreen::~SceneEditScreen()
    {}

    void SceneEditScreen::Draw()
	{
        ScrollView::Draw();

 		if (mEnabledTool)
 			mEnabledTool->DrawScreen();
 
 		mEditorLayers.SortBy<int>([](auto& l) { return l->GetOrder(); });
 
 		for (auto& layer : mEditorLayers)
 		{
 			if (layer->IsEnabled() && IsLayerEnabled(layer->GetName()))
 				layer->DrawOverScene();
 		}

        DrawGizmos3D();
    }

    void SceneEditScreen::NeedRedraw()
    {
        mNeedRedraw = true;
    }

    // Free-function entry point for external integrations (hosted cocos scene in the
    // editor animates every frame): callable without pulling editor headers
    void RequestSceneViewRedraw()
    {
        if (SceneEditScreen::IsSingletonInitialzed())
            o2EditorSceneScreen.NeedRedraw();
    }

    // Returns the cursor position in scene-world coordinates when the cursor is over
    // the scene view; entry point for external integrations (cocos input forwarding)
    bool TryGetSceneCursorWorldPoint(Vec2F& worldPoint)
    {
        if (!SceneEditScreen::IsSingletonInitialzed())
            return false;

        Vec2F cursorPos = o2Input.GetCursorPos();
        if (!o2EditorSceneScreen.IsUnderPoint(cursorPos))
            return false;

        worldPoint = o2EditorSceneScreen.ScreenToScenePoint(cursorPos);
        return true;
    }

#undef DrawText

    void SceneEditScreen::Update(float dt)
    {
        Widget::Update(dt);

        if (m3DMode)
        {
            UpdateScreenSpaceCamera();
            UpdateFlyNavigation(dt);
        }
        else
            UpdateCamera(dt);

        o2Scene.CheckChangedObjects();

        for (auto& layer : mEditorLayers)
        {
            if (layer->IsEnabled() && IsLayerEnabled(layer->GetName()))
                layer->Update(dt);
        }

        for (auto& tool : mTools)
            tool->Update(dt);
    }

    bool SceneEditScreen::IsScrollable() const
    {
        return true;
    }

    Vec2F SceneEditScreen::ScreenToScenePoint(const Vec2F& point)
    {
        if (m3DMode)
        {
            Vec2F result;
            if (mView3D.ScreenToPlanePoint(ScreenToViewportPoint(point), GetViewportSize(), result))
                return result;

            return mView3D.target.XY();
        }

        return ScreenToLocalPoint(point);
    }

    Vec2F SceneEditScreen::SceneToScreenPoint(const Vec2F& point)
    {
        if (m3DMode)
            return ViewportToScreenPoint(mView3D.PlanePointToScreen(point, GetViewportSize()));

        return LocalToScreenPoint(point);
    }

    Vec2F SceneEditScreen::ScreenToSceneVector(const Vec2F& point)
    {
        if (m3DMode)
        {
            Vec2F center = layout->GetWorldRect().Center();
            return ScreenToScenePoint(center + point) - ScreenToScenePoint(center);
        }

        return point * mViewCamera.GetScale2D();
    }

    Vec2F SceneEditScreen::SceneToScreenVector(const Vec2F& point)
    {
        if (m3DMode)
        {
            Vec2F base = mView3D.target.XY();
            return SceneToScreenPoint(base + point) - SceneToScreenPoint(base);
        }

        return point / mViewCamera.GetScale2D();
    }

    void SceneEditScreen::SetView3DMode(bool enabled)
    {
        if (m3DMode == enabled)
            return;

        float halfFovTan = Math::Sin(mView3D.fov*0.5f)/Math::Cos(mView3D.fov*0.5f);
        float viewportHeight = GetViewportSize().y;

        if (enabled)
        {
            mView3D.target = Vec3F(mViewCamera.GetPosition2D(), 0.0f);
            mView3D.distance = Math::Clamp(viewportHeight*mViewCamera.GetScale().y/(2.0f*halfFovTan),
                                           SceneView3DState::minDistance, SceneView3DState::maxDistance);

            m3DMode = true;
            UpdateScreenSpaceCamera();
        }
        else
        {
            m3DMode = false;
            SetFlyNavigation3D(false);
            mAltOrbit3D = false;

            float scale = 2.0f*mView3D.distance*halfFovTan/viewportHeight;
            mViewCamera.SetPosition2D(mView3D.target.XY());
            mViewCamera.SetScale2D(Vec2F(scale, scale));
            mViewCameraTargetPos = mViewCamera.GetPosition2D();
            mViewCameraTargetScale = mViewCamera.GetScale2D();

            UpdateLocalScreenTransforms();
            OnCameraTransformChanged();
        }

        mNeedRedraw = true;

        onView3DModeChanged(m3DMode);
    }

    bool SceneEditScreen::IsView3DMode() const
    {
        return m3DMode;
    }

    SceneView3DState& SceneEditScreen::GetView3DState()
    {
        return mView3D;
    }

    SceneGizmos& SceneEditScreen::GetGizmos()
    {
        return mGizmos;
    }

    void SceneEditScreen::SetSelectionVisible(bool visible)
    {
        mSelectionVisible = visible;
    }

    bool SceneEditScreen::IsSelectionVisible() const
    {
        return mSelectionVisible;
    }

    bool SceneEditScreen::ScreenToZAxisPoint(const Vec2F& screenPoint, const Vec2F& planeAnchor, float& z)
    {
        if (!m3DMode)
            return false;

        return mView3D.ScreenToVerticalAxisZ(ScreenToViewportPoint(screenPoint), GetViewportSize(), planeAnchor, z);
    }

    bool SceneEditScreen::ScreenToWorldAxisParam(const Vec2F& screenPoint, const Vec3F& axisOrigin,
                                                 const Vec3F& axisDir, float& param)
    {
        if (!m3DMode)
            return false;

        return mView3D.ScreenToAxisParam(ScreenToViewportPoint(screenPoint), GetViewportSize(), axisOrigin, axisDir, param);
    }

    bool SceneEditScreen::ScreenToWorldPlanePoint(const Vec2F& screenPoint, const Vec3F& planeOrigin,
                                                  const Vec3F& planeNormal, Vec3F& result)
    {
        if (!m3DMode)
            return false;

        return mView3D.ScreenToPlanePoint3D(ScreenToViewportPoint(screenPoint), GetViewportSize(),
                                            planeOrigin, planeNormal, result);
    }

    Vec2F SceneEditScreen::World3DToScreenPoint(const Vec3F& worldPoint)
    {
        return ViewportToScreenPoint(mView3D.WorldToScreen(worldPoint, GetViewportSize()));
    }

    bool SceneEditScreen::ScreenToWorldRay(const Vec2F& screenPoint, Vec3F& origin, Vec3F& direction)
    {
        if (!m3DMode)
            return false;

        return mView3D.GetScreenRay(ScreenToViewportPoint(screenPoint), GetViewportSize(), origin, direction);
    }

    bool SceneEditScreen::IsFlyNavigation3D() const
    {
        return mFlyNavigation3D;
    }

    Vec2F SceneEditScreen::GetViewportSize() const
    {
        Vec2F size = layout->GetWorldRect().Size();
        return Vec2F(Math::Max(size.x, 1.0f), Math::Max(size.y, 1.0f));
    }

    Vec2F SceneEditScreen::ScreenToViewportPoint(const Vec2F& point) const
    {
        RectF rect = layout->GetWorldRect();
        return point - Vec2F(rect.left, rect.bottom);
    }

    Vec2F SceneEditScreen::ViewportToScreenPoint(const Vec2F& point) const
    {
        RectF rect = layout->GetWorldRect();
        return point + Vec2F(rect.left, rect.bottom);
    }

    void SceneEditScreen::UpdateScreenSpaceCamera()
    {
        Camera camera = mViewCamera;
        camera.SetPosition2D(layout->GetWorldRect().Center());
        camera.SetScale2D(Vec2F(1.0f, 1.0f));
        camera.SetAngle(0.0f);

        mViewCameraTargetPos = camera.GetPosition2D();
        mViewCameraTargetScale = camera.GetScale2D();

        if (camera != mViewCamera)
        {
            mViewCamera = camera;
            UpdateLocalScreenTransforms();
            OnCameraTransformChanged();
            mNeedRedraw = true;
        }
    }

    void SceneEditScreen::SetFlyNavigation3D(bool active)
    {
        if (mFlyNavigation3D == active)
            return;

        mFlyNavigation3D = active;

        // Fly navigation owns the keyboard: WASD/QE must not trigger tool shortcuts
        if (ShortcutKeysListenersManager::IsSingletonInitialzed())
            ShortcutKeysListenersManager::SetSuppressed(active);
    }

    void SceneEditScreen::UpdateFlyNavigation(float dt)
    {
        if (!mFlyNavigation3D)
            return;

        if (!o2Input.IsRightMouseDown())
        {
            SetFlyNavigation3D(false);
            return;
        }

        // X11 backend reports letter keys as lower case keysyms
        auto isKeyDown = [](char upper) { return o2Input.IsKeyDown(upper) || o2Input.IsKeyDown(upper - 'A' + 'a'); };

        Vec3F direction;
        if (isKeyDown('W')) direction.z += 1.0f;
        if (isKeyDown('S')) direction.z -= 1.0f;
        if (isKeyDown('D')) direction.x += 1.0f;
        if (isKeyDown('A')) direction.x -= 1.0f;
        if (isKeyDown('E')) direction.y += 1.0f;
        if (isKeyDown('Q')) direction.y -= 1.0f;

        if (direction.Length() < FLT_EPSILON)
            return;

        float speed = mView3D.distance*(o2Input.IsKeyDown(VK_SHIFT) ? 3.0f : 1.0f);
        mView3D.Fly(direction.Normalized()*speed*dt);
        mViewCameraMoved = true;
        mNeedRedraw = true;
    }

    Input::Cursor SceneEditScreen::ToSceneCursor(const Input::Cursor& cursor)
    {
        if (!m3DMode)
            return cursor;

        Input::Cursor result = cursor;
        result.position = ScreenToScenePoint(cursor.position);
        result.delta = result.position - ScreenToScenePoint(cursor.position - cursor.delta);
        return result;
    }

    void SceneEditScreen::InitializeTools()
    {
        mTools.Add(mmake<SelectionTool>());
        mTools.Add(mmake<MoveTool>());
        mTools.Add(mmake<RotateTool>());
        mTools.Add(mmake<ScaleTool>());
        mTools.Add(mmake<FrameTool>());
    }

    Ref<HorizontalLayout> SceneEditScreen::InitializeWidgetsContainer(BaseCorner baseCorner)
    {
        auto controlsWidget = mmake<HorizontalLayout>();
        *controlsWidget->layout = WidgetLayout::BothStretch();
        controlsWidget->baseCorner = baseCorner;
        controlsWidget->spacing = 5;
        controlsWidget->expandHeight = false;
        controlsWidget->expandWidth = false;
        controlsWidget->border = BorderF(5, 5, 5, 5);
        controlsWidget->layout->pivot2D = Vec2F(1, 1);
        AddInternalWidget(controlsWidget);

        return controlsWidget;
    }

    bool SceneEditScreen::IsHandleWorking(const Input::Cursor& cursor) const
    {
        return false;
    }

    void SceneEditScreen::OnObjectsSelectedFromThis()
    {
        mSelectedFromThis = true;

        if (TreeWindow::IsSingletonInitialzed())
            o2EditorTree.SetSelectedObjects(mSelectedObjects);

        if (mEnabledTool)
            mEnabledTool->OnObjectsSelectionChanged(mSelectedObjects);

        onSelectionChanged(mSelectedObjects);

        if (PropertiesWindow::IsSingletonInitialzed())
            o2EditorPropertiesWindow.SetTargets(mSelectedObjects.Convert<IObject*>([](auto x) { return (IObject*)x.Get(); }));
    }

    void SceneEditScreen::RedrawContent()
    {
        // The scene pass draws scene layers content, which may contain this widget itself
        // (editor UI in tests, nested views), so the nested redraw is cut off
        if (mIsRedrawingContent)
            return;

        mIsRedrawingContent = true;

        // In 3D mode the whole scene pass is drawn with the perspective camera in plane coordinates,
        // then the screen-space camera is restored for drag handles overlay
        Camera screenCamera;
        if (m3DMode)
        {
            screenCamera = o2Render.GetCamera();

            Camera viewCamera = mView3D.BuildCamera();
            o2Render.SetCamera(viewCamera);

            Draw3DGrid();
            DrawObjects3D(viewCamera);
        }
        else
        {
            DrawGrid();
            DrawObjects();
        }

        o2Debug.Draw(false);
        DrawSelection();
        DrawGizmos2D();

        for (auto& layer : mEditorLayers)
        {
            if (layer->IsEnabled() && IsLayerEnabled(layer->GetName()))
                layer->DrawScene();
        }

        if (mEnabledTool)
        {
            mEnabledTool->DrawScene();
            mEnabledTool->mNeedRedraw = false;
		}

        if (m3DMode)
            o2Render.SetCamera(screenCamera);

		for (auto& handle : mDragHandles)
			handle->Draw();

        mIsRedrawingContent = false;
    }

    void SceneEditScreen::Draw3DGrid()
    {
        Vec3F cameraPos = mView3D.GetCameraPosition();
        float referenceSize = Math::Max(mView3D.distance, Math::Abs(cameraPos.z))*0.5f;

        float minCellSize = 0.000001f;
        float maxCellSize = 1000000.0f;
        float cellSize = minCellSize;
        while (cellSize < maxCellSize)
        {
            float next = cellSize*10.0f;
            if (referenceSize > cellSize && referenceSize <= next)
                break;

            cellSize = next;
        }

        // Grid follows the camera ground point and reaches far enough to look infinite from any view
        const int halfCells = 120;
        float extent = cellSize*(float)halfCells;

        Vec2F center = cameraPos.XY();
        Vec2F gridOrigin(Math::Round(center.x/cellSize)*cellSize,
                         Math::Round(center.y/cellSize)*cellSize);

        float tenCellsSize = cellSize*10.0f;
        Color4 cellColorSmoothed = Math::Lerp(mGridColor, mBackColor, 0.7f);

        for (int i = -halfCells; i <= halfCells; i++)
        {
            float d = (float)i*cellSize;
            float xv = gridOrigin.x + d;
            float yv = gridOrigin.y + d;

            float rdx = Math::Abs(xv/tenCellsSize - Math::Floor(xv/tenCellsSize));
            float rdy = Math::Abs(yv/tenCellsSize - Math::Floor(yv/tenCellsSize));
            bool xTen = rdx < 0.05f || rdx > 0.95f;
            bool yTen = rdy < 0.05f || rdy > 0.95f;

            if (verGridEnabled)
            {
                o2Render.DrawLine(Vec2F(gridOrigin.x - extent, yv), Vec2F(gridOrigin.x + extent, yv),
                                  yTen ? mGridColor : cellColorSmoothed);
            }

            if (horGridEnabled)
            {
                o2Render.DrawLine(Vec2F(xv, gridOrigin.y - extent), Vec2F(xv, gridOrigin.y + extent),
                                  xTen ? mGridColor : cellColorSmoothed);
            }
        }
    }

    void SceneEditScreen::DrawObjects()
    {
        if (TreeWindow::IsSingletonInitialzed() && o2EditorTree.GetSceneTree()->IsEditorWatching())
        {
            static bool drawing = false;
            if (drawing)
                return;

            drawing = true;
            //EditorUIRoot.GetRootWidget()->Draw();
            drawing = false;

            mNeedRedraw = true;
            return;
        }

        DrawScenePipeline();
    }

    void SceneEditScreen::DrawObjects3D(const Camera& viewCamera)
    {
        if (TreeWindow::IsSingletonInitialzed() && o2EditorTree.GetSceneTree()->IsEditorWatching())
        {
            mNeedRedraw = true;
            return;
        }

        DrawScenePipeline();
    }

    void SceneEditScreen::SetStableCameraMode(bool stable)
    {
        if (mStableCameraMode == stable)
            return;

        mStableCameraMode = stable;
        NeedRedraw();
        onStableCameraModeChanged(stable);
    }

    bool SceneEditScreen::IsStableCameraMode() const
    {
        return mStableCameraMode;
    }

    const Ref<RenderPipeline>& SceneEditScreen::GetEditRenderPipeline() const
    {
        return mEditPipeline;
    }

    Ref<RenderPipeline> SceneEditScreen::ResolveScenePipeline() const
    {
        if (!mStableCameraMode && !o2Scene.GetCameras().IsEmpty())
        {
            if (auto sceneCamera = o2Scene.GetCameras()[0].Lock())
            {
                if (auto pipeline = sceneCamera->GetRenderPipeline())
                    return pipeline;
            }
        }

        return CameraActor::GetDefaultRenderPipeline();
    }

    void SceneEditScreen::DrawScenePipeline()
    {
        o2Scene.BeginDrawingScene();

        Ref<RenderPipeline> sourcePipeline = ResolveScenePipeline();

        // Run our own clone so the edit view's deferred pass state doesn't collide with the Game
        // window, which executes the scene camera's pipeline instance in the same frame. Re-clone
        // only when the source pipeline changes (scene reload, camera/pipeline edit).
        if (mEditPipelineSource != sourcePipeline.Get())
        {
            mEditPipeline = sourcePipeline->CloneAsRef<RenderPipeline>();
            mEditPipelineSource = sourcePipeline.Get();
        }

        Ref<RenderPipeline> pipeline = mEditPipeline ? mEditPipeline : sourcePipeline;

        RenderPassContext context;
        context.camera = o2Render.GetCamera();
        context.fillBackground = false;

        for (auto& layer : o2Scene.GetLayers())
        {
            if (layer->IsVisible())
                context.layers.Add(layer);
        }

        pipeline->Execute(context);

        o2Scene.EndDrawingScene();

        o2Physics.DrawDebug();
    }

    void SceneEditScreen::DrawSelection()
    {
        if (!mSelectionVisible)
            return;

        if (m3DMode)
        {
            DrawSelectionOutline(SceneDrawableCategory::Scene3D);
            return;
        }

        DrawSelectionOutline(SceneDrawableCategory::Scene2D);

        if (mSelectedObjects.Count() == 1)
        {
            DrawObjectSelection(mSelectedObjects[0], mSelectedObjectColor);
        }
        else
        {
            for (auto& object : mSelectedObjects)
                DrawObjectSelection(object, mMultiSelectedObjectColor);
        }
    }

    void SceneEditScreen::CollectDrawableComponents(const Ref<Actor>& actor, SceneDrawableCategory category,
                                                    Vector<Ref<Component>>& components)
    {
        if (!actor || !actor->IsEnabledInHierarchy())
            return;

        for (auto& component : actor->GetComponents())
        {
            if (component->GetSceneDrawableCategory() == category && component->IsEnabledInHierarchy() &&
                !components.Contains(component))
            {
                components.Add(component);
            }
        }

        for (auto& child : actor->GetChildren())
            CollectDrawableComponents(child, category, components);
    }

    void SceneEditScreen::DrawSelectionOutline(SceneDrawableCategory category)
    {
        Vector<Ref<Component>> components;
        for (auto& object : mSelectedObjects)
        {
            if (auto actor = DynamicCast<Actor>(object))
                CollectDrawableComponents(actor, category, components);
        }

        if (components.IsEmpty())
            return;

        Vec2I targetSize = o2Render.GetCurrentResolution();
        if (targetSize.x < 1 || targetSize.y < 1)
            return;

        if (!mSelectionMaskTarget || (Vec2I)mSelectionMaskTarget->GetSize() != targetSize)
            mSelectionMaskTarget = TextureRef(targetSize, TextureFormat::R8G8B8A8, Texture::Usage::RenderTarget);

        if (!mSelectionOutlineMaterial)
        {
            mSelectionOutlineMaterial = Material::CreateFromBuiltinShaders("SelectionOutline");
            if (mSelectionOutlineMaterial)
            {
                mSelectionOutlineMaterial->AddParam(mmake<ShaderParamVec2>("u_texelSize", Vec2F()));
                mSelectionOutlineMaterial->Build();
            }
        }

        if (!mSelectionOutlineMaterial || !mSelectionOutlineMaterial->IsReady())
            return;

        if (!mSelectionOutlineQuad)
            mSelectionOutlineQuad = mmake<Mesh>(TextureRef(), 4, 2);

        // Silhouette mask: selected content rendered into an offscreen target, alpha marks coverage
        o2Render.PushRenderTargets({ mSelectionMaskTarget });
        o2Render.Clear(Color4(0.0f, 0.0f, 0.0f, 0.0f));

        for (auto& component : components)
            component->Draw();

        o2Render.PopRenderTargets();

        DynamicCast<ShaderParamVec2>(mSelectionOutlineMaterial->GetShaderParam("u_texelSize"))
            ->SetValue(Vec2F(1.0f/(float)targetSize.x, 1.0f/(float)targetSize.y));
        mSelectionOutlineMaterial->InvalidateHash();

        Camera sceneCamera = o2Render.GetCamera();
        o2Render.SetCamera(Camera());

        Vec2F halfSize = (Vec2F)o2Render.GetCurrentResolution()*0.5f;
        ULong colorValue = mSelectionOutlineColor.ABGR();

        mSelectionOutlineQuad->SetMaterial(mSelectionOutlineMaterial);
        mSelectionOutlineQuad->SetTexture(mSelectionMaskTarget);

        // The mask target is rendered y-flipped like any render target, the quad samples it back upright
        Vertex* vertices = mSelectionOutlineQuad->GetVertices<Vertex>();
        vertices[0] = Vertex(-halfSize.x, halfSize.y, 0.0f, colorValue, 0.0f, 1.0f);
        vertices[1] = Vertex(halfSize.x, halfSize.y, 0.0f, colorValue, 1.0f, 1.0f);
        vertices[2] = Vertex(halfSize.x, -halfSize.y, 0.0f, colorValue, 1.0f, 0.0f);
        vertices[3] = Vertex(-halfSize.x, -halfSize.y, 0.0f, colorValue, 0.0f, 0.0f);

        VertexIndex* indexes = mSelectionOutlineQuad->GetIndexes();
        indexes[0] = 0; indexes[1] = 1; indexes[2] = 2;
        indexes[3] = 0; indexes[4] = 2; indexes[5] = 3;

        mSelectionOutlineQuad->vertexCount = 4;
        mSelectionOutlineQuad->polyCount = 2;

        mSelectionOutlineQuad->Draw();

        o2Render.SetCamera(sceneCamera);
    }

    void SceneEditScreen::DrawGizmos2D()
    {
        if (m3DMode)
            return;

        mGizmos.Draw(mSelectedObjects, [](const Vec3F& worldPoint) { return Vec2F(worldPoint.x, worldPoint.y); });
    }

    void SceneEditScreen::DrawGizmos3D()
    {
        if (!m3DMode)
            return;

        Vec3F clipPlaneOrigin, clipPlaneNormal;
        mView3D.GetNearClipPlane(clipPlaneOrigin, clipPlaneNormal);

        // The view-projection is built once for the whole gizmos pass: a wireframe scene projects
        // thousands of points, and rebuilding the camera matrices per point dominates the frame
        Vec2F viewportSize = GetViewportSize();
        Mat4 viewProjection = mView3D.GetViewProjection(viewportSize);

        mGizmos.Draw(mSelectedObjects,
                     [&](const Vec3F& worldPoint) {
                         return ViewportToScreenPoint(mView3D.WorldToScreen(worldPoint, viewportSize, viewProjection));
                     },
                     clipPlaneOrigin, clipPlaneNormal);
    }


    void SceneEditScreen::DrawObjectSelection(const Ref<SceneEditableObject>& object, const Color4& color)
    {
        o2Render.DrawAABasis(object->GetTransform(), color, color, color);
        // 
        //     auto bs = actor->transform->GetWorldNonSizedBasis();
        //     o2Render.DrawLine(bs.offs, bs.offs + bs.xv*100.0f);
        //     o2Render.DrawLine(bs.offs, bs.offs + bs.yv*100.0f);
    }

    void SceneEditScreen::SelectObjects(const Vector<Ref<SceneEditableObject>>& objects, bool additive /*= true*/)
    {
        auto prevSelectedObjects = mSelectedObjects;

        SelectObjectsWithoutAction(objects, additive);

        if (mSelectedObjects != prevSelectedObjects)
        {
            auto selectionAction = mmake<SelectAction>(mSelectedObjects, prevSelectedObjects);
            o2EditorSceneWindow.DoneAction(selectionAction);
        }
    }

    void SceneEditScreen::SelectObject(const Ref<SceneEditableObject>& actor, bool additive /*= true*/)
    {
        auto prevSelectedObjects = mSelectedObjects;

        SelectObjectWithoutAction(actor, additive);

        if (mSelectedObjects != prevSelectedObjects)
        {
            auto selectionAction = mmake<SelectAction>(mSelectedObjects, prevSelectedObjects);
            o2EditorSceneWindow.DoneAction(selectionAction);
        }
    }

    void SceneEditScreen::SelectAllObjects()
    {
        auto prevSelectedObjects = mSelectedObjects;

        mSelectedObjects.Clear();
        for (auto& [ptr, weak] : o2Scene.GetAllActors())
        {
            auto a = weak.Lock();
            if (a && !a->IsLockedInHierarchy())
                mSelectedObjects.Add(DynamicCast<SceneEditableObject>(a));
        }

        mNeedRedraw = true;
        OnObjectsSelectedFromThis();

        if (mSelectedObjects != prevSelectedObjects)
        {
            auto selectionAction = mmake<SelectAction>(mSelectedObjects, prevSelectedObjects);
            o2EditorSceneWindow.DoneAction(selectionAction);
        }
    }

    void SceneEditScreen::ClearSelection()
    {
        auto prevSelectedObjects = mSelectedObjects;

        ClearSelectionWithoutAction();

        if (mSelectedObjects != prevSelectedObjects)
        {
            auto selectionAction = mmake<SelectAction>(mSelectedObjects, prevSelectedObjects);
            o2EditorSceneWindow.DoneAction(selectionAction);
        }
    }

    void SceneEditScreen::SelectObjectsByIdsWithoutAction(const Vector<SceneUID>& ids)
    {
        mSelectedObjects.Clear();
        for (auto id : ids)
        {
            if (auto obj = o2Scene.GetEditableObjectByID(id))
                mSelectedObjects.Add(obj);
        }

        UpdateTopSelectedObjects();
        OnObjectsSelectedFromThis();
        mNeedRedraw = true;
    }

    const Ref<HorizontalLayout>& SceneEditScreen::GetLeftTopWidgetsContainer()
    {
        return mLeftTopWidgetsContainer;
    }

    const Ref<HorizontalLayout>& SceneEditScreen::GetRightTopWidgetsContainer()
    {
        return mRightTopWidgetsContainer;
    }

    const Ref<HorizontalLayout>& SceneEditScreen::GetLeftBottomWidgetsContainer()
    {
        return mLeftBottomWidgetsContainer;
    }

    const Ref<HorizontalLayout>& SceneEditScreen::GetRightBottomWidgetsContainer()
    {
        return mRightBottomWidgetsContainer;
    }

    void SceneEditScreen::AddEditorLayer(const Ref<SceneEditorLayer>& layer)
    {
        mEditorLayers.Add(layer);
    }

    void SceneEditScreen::RemoveEditorLayer(const Ref<SceneEditorLayer>& layer)
    {
        mEditorLayers.Remove(layer);
    }

    void SceneEditScreen::SetLayerEnabled(const String& name, bool enabled)
    {
        mEditorLayersEnabled[name] = enabled;
    }

    bool SceneEditScreen::IsLayerEnabled(const String& name) const
    {
        bool enabled = true;
        mEditorLayersEnabled.TryGetValue(name, enabled);
        return enabled;
    }

    void SceneEditScreen::SelectTool(const Ref<IEditTool>& tool)
    {
        if (tool == mEnabledTool)
            return;

        if (mEnabledTool)
            mEnabledTool->OnDisabled();

        mEnabledTool = tool;
        if (auto toggle = mEnabledTool->GetPanelToggle())
            toggle->SetValue(true);

        if (mEnabledTool)
            mEnabledTool->OnEnabled();
    }

    const Ref<IEditTool>& SceneEditScreen::GetSelectedTool() const
    {
        return mEnabledTool;
    }

    void SceneEditScreen::AddTool(const Ref<IEditTool>& tool)
    {
        mTools.Add(tool);
        o2EditorTools.AddToolToggle(tool->GetPanelToggle());
    }

    void SceneEditScreen::RemoveTool(const Ref<IEditTool>& tool)
    {
        if (tool == mEnabledTool)
        {
            if (!mTools.IsEmpty())
                SelectTool(mTools[0]);
            else
                SelectTool(nullptr);
        }

        mTools.Remove(tool);
        o2EditorTools.RemoveToolToggle(tool->GetPanelToggle());
    }

    const Vector<Ref<IEditTool>>& SceneEditScreen::GetTools() const
    {
        return mTools;
    }

    const Vector<Ref<SceneEditableObject>>& SceneEditScreen::GetSelectedObjects() const
    {
        return mSelectedObjects;
    }

    const Vector<Ref<SceneEditableObject>>& SceneEditScreen::GetTopSelectedObjects() const
    {
        return mTopSelectedObjects;
    }

    const Color4& SceneEditScreen::GetSingleObjectSelectionColor() const
    {
        return mSelectedObjectColor;
    }

    const Color4& SceneEditScreen::GetManyObjectsSelectionColor() const
    {
        return mMultiSelectedObjectColor;
    }

    bool SceneEditScreen::IsUnderPoint(const Vec2F& point)
    {
        return true;
    }

    Ref<RefCounterable> SceneEditScreen::CastToRefCounterable(const Ref<SceneEditScreen>& ref)
    {
        return DynamicCast<Singleton<SceneEditScreen>>(ref);
    }

    void SceneEditScreen::BindSceneTree()
    {
        o2EditorTree.GetSceneTree()->onObjectsSelectionChanged += THIS_FUNC(OnTreeSelectionChanged);
        o2EditorTree.GetDrawOrderTree()->onObjectsSelectionChanged += THIS_FUNC(OnTreeSelectionChanged);

        o2Scene.onObjectsChanged += Function<void(Vector<Ref<SceneEditableObject>>)>(this, &SceneEditScreen::OnSceneChanged);
    }

    void SceneEditScreen::OnTreeSelectionChanged(Vector<Ref<SceneEditableObject>> selectedObjects)
    {
        if (mSelectedFromThis)
        {
            mSelectedFromThis = false;
            return;
        }

        auto prevSelectedObjects = mSelectedObjects;

        mSelectedObjects = selectedObjects;
        mNeedRedraw = true;

        UpdateTopSelectedObjects();

        if (mEnabledTool)
            mEnabledTool->OnObjectsSelectionChanged(mSelectedObjects);

        auto selectedIObjects = mSelectedObjects.Convert<IObject*>([](auto& x) { return dynamic_cast<IObject*>(x.Get()); });

        if (mSelectedObjects != prevSelectedObjects ||
            selectedIObjects != o2EditorPropertiesWindow.GetTargets())
        {
            auto selectionAction = mmake<SelectAction>(mSelectedObjects, prevSelectedObjects);
            if (auto sceneWindow = o2EditorWindows.GetWindow<SceneWindow>())
                sceneWindow->DoneAction(selectionAction);

            onSelectionChanged(mSelectedObjects);
            o2EditorPropertiesWindow.SetTargets(selectedIObjects);
        }
    }

    void SceneEditScreen::UpdateTopSelectedObjects()
    {
        mTopSelectedObjects.Clear();
        for (auto& object : mSelectedObjects)
        {
            bool processing = true;

            auto parent = object->GetEditableParent();
            while (parent)
            {
                if (mSelectedObjects.Contains([&](auto x) { return parent == x; }))
                {
                    processing = false;
                    break;
                }

                parent = parent->GetEditableParent();
            }

            if (processing)
                mTopSelectedObjects.Add(object);
        }
    }

    void SceneEditScreen::OnSceneChanged(Vector<Ref<SceneEditableObject>> actors)
    {
        mNeedRedraw = true;

        if (mEnabledTool)
            mEnabledTool->OnSceneChanged(actors);
    }

    void SceneEditScreen::OnSceneChanged()
    {
        mNeedRedraw = true;
    }

    void SceneEditScreen::ClearSelectionWithoutAction(bool sendSelectedMessage /*= true*/)
    {
        mSelectedObjects.Clear();
        mTopSelectedObjects.Clear();
        mNeedRedraw = true;

        if (sendSelectedMessage)
            OnObjectsSelectedFromThis();
    }

    void SceneEditScreen::SelectObjectsWithoutAction(Vector<Ref<SceneEditableObject>> objects, bool additive /*= true*/)
    {
        if (!additive)
            mSelectedObjects.Clear();

        mSelectedObjects.Add(objects);
        mNeedRedraw = true;

        UpdateTopSelectedObjects();
        OnObjectsSelectedFromThis();
    }

    void SceneEditScreen::SelectObjectWithoutAction(const Ref<SceneEditableObject>& object, bool additive /*= true*/)
    {
        if (!additive)
            mSelectedObjects.Clear();

        mSelectedObjects.Add(object);
        mNeedRedraw = true;

        UpdateTopSelectedObjects();
        OnObjectsSelectedFromThis();
    }

    void SceneEditScreen::OnDropped(const Ref<ISelectableDragableObjectsGroup>& group)
    {
        auto assetsScroll = DynamicCast<AssetsIconsScrollArea>(group);
        if (!assetsScroll)
            return;

        assetsScroll->RegObjectsCreationAction();

        o2UI.FocusWidget(o2EditorTree.GetSceneTree());
        o2EditorTree.GetSceneTree()->SetSelectedObjects(assetsScroll->mInstantiatedSceneDragObjects);

        assetsScroll->mInstantiatedSceneDragObjects.Clear();

        o2Application.SetCursor(CursorType::Arrow);
    }

    void SceneEditScreen::OnDragEnter(const Ref<ISelectableDragableObjectsGroup>& group)
    {
        auto assetsScroll = DynamicCast<AssetsIconsScrollArea>(group);
        if (!assetsScroll)
            return;

        assetsScroll->InstantiateDraggingAssets();
        if (assetsScroll->mInstantiatedSceneDragObjects.Count() > 0)
            o2Application.SetCursor(CursorType::Hand);
    }

    void SceneEditScreen::OnDraggedAbove(const Ref<ISelectableDragableObjectsGroup>& group)
    {
        auto assetsScroll = DynamicCast<AssetsIconsScrollArea>(group);
        if (!assetsScroll)
            return;

        for (auto& object : assetsScroll->mInstantiatedSceneDragObjects)
        {
            object->UpdateTransform();
            Basis transform = object->GetTransform();
            transform.origin = ScreenToScenePoint(o2Input.cursorPos) - (transform.xv + transform.yv) * 0.5f;
            object->SetTransform(transform);
        }
    }

    void SceneEditScreen::OnDragExit(const Ref<ISelectableDragableObjectsGroup>& group)
    {
        auto assetsScroll = DynamicCast<AssetsIconsScrollArea>(group);
        if (!assetsScroll)
            return;

        assetsScroll->ClearInstantiatedDraggingAssets();
        o2Application.SetCursor(CursorType::Arrow);

        mNeedRedraw = true;
    }

    bool SceneEditScreen::IsInputTransparent() const
    {
        return true;
    }

    void SceneEditScreen::OnScrolled(float scroll)
    {
        if (m3DMode)
        {
            mView3D.Zoom(1.0f - scroll*mViewCameraScaleSence);
            mNeedRedraw = true;
        }
        else
            ScrollView::OnScrolled(scroll);

        if (mEnabledTool)
            mEnabledTool->OnScrolled(scroll);
    }

    void SceneEditScreen::OnKeyPressed(const Input::Key& key)
    {
        // Fly navigation owns the keyboard: WASD/QE must not reach mode toggles and tool handlers
        if (mFlyNavigation3D)
            return;

        if (key == '3' && (!SceneWindow::IsSingletonInitialzed() || o2EditorSceneWindow.IsFocused()))
            SetView3DMode(!m3DMode);

        if (mEnabledTool)
            mEnabledTool->OnKeyPressed(key);
    }

    void SceneEditScreen::OnKeyReleased(const Input::Key& key)
    {
        // Releases pass through even while flying, so tools can close held-key actions
        if (mEnabledTool)
            mEnabledTool->OnKeyReleased(key);
    }

    void SceneEditScreen::OnKeyStayDown(const Input::Key& key)
    {
        if (mFlyNavigation3D)
            return;

        if (mEnabledTool)
            mEnabledTool->OnKeyStayDown(key);
    }

    void SceneEditScreen::OnCursorPressed(const Input::Cursor& cursor)
    {
        if (mParentWidget)
			mParentWidget.Lock()->Focus();

        o2EditorTree.OnSceneFocused();

        if (m3DMode && o2Input.IsKeyDown(VK_MENU))
        {
            mAltOrbit3D = true;
            return;
        }

        if (mEnabledTool && !IsHandleWorking(cursor))
            mEnabledTool->OnCursorPressed(ToSceneCursor(cursor));
    }

    void SceneEditScreen::OnCursorReleased(const Input::Cursor& cursor)
    {
        if (mAltOrbit3D)
        {
            mAltOrbit3D = false;
            return;
        }

        if (mEnabledTool && !IsHandleWorking(cursor))
            mEnabledTool->OnCursorReleased(ToSceneCursor(cursor));
    }

    void SceneEditScreen::OnCursorPressBreak(const Input::Cursor& cursor)
    {
        if (mAltOrbit3D)
        {
            mAltOrbit3D = false;
            return;
        }

        if (mEnabledTool && !IsHandleWorking(cursor))
            mEnabledTool->OnCursorPressBreak(ToSceneCursor(cursor));
    }

    void SceneEditScreen::OnCursorStillDown(const Input::Cursor& cursor)
    {
        if (mAltOrbit3D)
        {
            if (cursor.delta.Length() > Math::Epsilon)
            {
                mView3D.Orbit(Vec2F(-cursor.delta.x, cursor.delta.y)*0.005f);
                mViewCameraMoved = true;
                mNeedRedraw = true;
            }
            return;
        }

        if (mEnabledTool && !IsHandleWorking(cursor))
            mEnabledTool->OnCursorStillDown(ToSceneCursor(cursor));
    }

    void SceneEditScreen::OnCursorMoved(const Input::Cursor& cursor)
    {
        if (mEnabledTool && !IsHandleWorking(cursor))
            mEnabledTool->OnCursorMoved(ToSceneCursor(cursor));
    }

    void SceneEditScreen::OnCursorEnter(const Input::Cursor& cursor)
    {
        if (mEnabledTool && !IsHandleWorking(cursor))
            mEnabledTool->OnCursorEnter(ToSceneCursor(cursor));
    }

    void SceneEditScreen::OnCursorExit(const Input::Cursor& cursor)
    {
        if (mEnabledTool && !IsHandleWorking(cursor))
            mEnabledTool->OnCursorExit(ToSceneCursor(cursor));
    }

    void SceneEditScreen::OnCursorRightMousePressed(const Input::Cursor& cursor)
	{
		if (mParentWidget)
			mParentWidget.Lock()->Focus();

		o2EditorTree.OnSceneFocused();

        if (m3DMode)
            SetFlyNavigation3D(true);

        if (mEnabledTool && !IsHandleWorking(cursor))
            mEnabledTool->OnCursorRightMousePressed(ToSceneCursor(cursor));

        ScrollView::OnCursorRightMousePressed(cursor);
    }

    void SceneEditScreen::OnCursorRightMouseStayDown(const Input::Cursor& cursor)
    {
        if (m3DMode)
        {
            if (cursor.delta.Length() > Math::Epsilon)
            {
                mView3D.Look(Vec2F(-cursor.delta.x, cursor.delta.y)*0.005f);
                mViewCameraMoved = true;
                mNeedRedraw = true;
            }
        }
        else
            ScrollView::OnCursorRightMouseStayDown(cursor);

        if (mEnabledTool)
            mEnabledTool->OnCursorRightMouseStayDown(ToSceneCursor(cursor));
    }

    void SceneEditScreen::OnCursorRightMouseReleased(const Input::Cursor& cursor)
    {
        SetFlyNavigation3D(false);

        if (mEnabledTool)
            mEnabledTool->OnCursorRightMouseReleased(ToSceneCursor(cursor));

        ScrollView::OnCursorRightMouseReleased(cursor);
    }

    void SceneEditScreen::OnCursorMiddleMousePressed(const Input::Cursor& cursor)
    {
        if (mEnabledTool)
            mEnabledTool->OnCursorMiddleMousePressed(ToSceneCursor(cursor));
    }

    void SceneEditScreen::OnCursorMiddleMouseStayDown(const Input::Cursor& cursor)
    {
        if (m3DMode && cursor.delta.Length() > Math::Epsilon)
        {
            mView3D.Pan(cursor.delta, GetViewportSize());
            mNeedRedraw = true;
        }

        if (mEnabledTool)
            mEnabledTool->OnCursorMiddleMouseStayDown(ToSceneCursor(cursor));
    }

    void SceneEditScreen::OnCursorMiddleMouseReleased(const Input::Cursor& cursor)
    {
        if (mEnabledTool)
            mEnabledTool->OnCursorMiddleMouseReleased(ToSceneCursor(cursor));
    }
}

DECLARE_TEMPLATE_CLASS(o2::LinkRef<Editor::SceneEditScreen>);
// --- META ---

DECLARE_CLASS(Editor::SceneEditScreen, Editor__SceneEditScreen);
// --- END META ---
