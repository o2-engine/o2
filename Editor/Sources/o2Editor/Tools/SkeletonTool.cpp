#include "o2Editor/stdafx.h"
#include "SkeletonTool.h"

#include "o2Editor/Actions/BoneTransform.h"
#include "o2Editor/Windows/SceneWindow/SceneEditScreen.h"
#include "o2Editor/Windows/SceneWindow/SceneDragHandle.h"
#include "o2Editor/Windows/SceneWindow/SceneWindow.h"

namespace Editor
{
    SkeletonTool::SkeletonTool(RefCounter* refCounter):
        IEditTool(refCounter)
    {
        sceneLayer->tool = Ref(this);
    }

    void SkeletonTool::AddSkeletonInstance(const Ref<SkinningMeshComponent>& skinningMeshComponent)
    {
        if (IsEditingSkeleton(skinningMeshComponent))
            return;

        auto newInstance = mmake<SkeletonInstance>();

        newInstance->skinningMeshComponent = skinningMeshComponent;

        for (auto& bones : skinningMeshComponent->GetBones())
        {
            auto boneHandle = mmake<BoneHandle>();
            boneHandle->boneComponent = bones.first.Lock();

            auto compTransform = skinningMeshComponent->GetActor()->transform;

            const float handleSize = 5;
            Vec2F drawableSize(bones.first.Lock()->length, handleSize);

            boneHandle->handle->position = compTransform->worldPosition;

            boneHandle->handle->GetRegularDrawable()->size = drawableSize;
            boneHandle->handle->GetHoverDrawable()->size = drawableSize;
            boneHandle->handle->GetPressedDrawable()->size = drawableSize;

            boneHandle->handle->GetRegularDrawable()->angle = compTransform->worldBasis.Get().GetAngle();
            boneHandle->handle->GetHoverDrawable()->angle = compTransform->worldBasis.Get().GetAngle();
            boneHandle->handle->GetPressedDrawable()->angle = compTransform->worldBasis.Get().GetAngle();

            newInstance->boneHandles.Add(boneHandle);
        }

        mSkeletons.Add(newInstance);
    }

    void SkeletonTool::RemoveSkeletonInstance(const Ref<SkinningMeshComponent>& skinningMeshComponent)
    {
        auto instance = mSkeletons.FindOrDefault([&](auto& x) { return x->skinningMeshComponent == skinningMeshComponent; });
        if (!instance)
            return;

        mSkeletons.Remove(instance);

        instance->boneHandles.Clear();
    }

    bool SkeletonTool::IsEditingSkeleton(const Ref<SkinningMeshComponent>& skinningMeshComponent) const
    {
        return mSkeletons.Contains([&](auto& x) { return x->skinningMeshComponent == skinningMeshComponent; });
    }

    const Vector<Ref<SkeletonTool::SkeletonInstance>>& SkeletonTool::GetEditingSkeletons() const
    {
        return mSkeletons;
    }

    void SkeletonTool::SceneLayer::DrawOverScene()
    {
    }

    void SkeletonTool::SceneLayer::Update(float dt)
    {
        for (auto& instance : tool.Lock()->mSkeletons)
            instance->UpdateHandlesPositions();
    }

    int SkeletonTool::SceneLayer::GetOrder() const
    {
        return 0;
    }

    bool SkeletonTool::SceneLayer::IsEnabled() const
    {
        return tool.Lock()->isEnabled;
    }

    const String& SkeletonTool::SceneLayer::GetName() const
    {
        static String res("Skeleton tool");
        return res;
    }

    const String& SkeletonTool::SceneLayer::GetIconName() const
    {
        return String::empty;
    }

    String SkeletonTool::SkeletonTool::GetPanelIcon() const
    {
        return "ui/UI4_bones_tool.png";
    }

    void SkeletonTool::SkeletonTool::OnEnabled()
    {
        o2EditorSceneScreen.AddEditorLayer(sceneLayer);
        isEnabled = true;
    }

    void SkeletonTool::SkeletonTool::OnDisabled()
    {
        o2EditorSceneScreen.RemoveEditorLayer(sceneLayer);
        isEnabled = false;
    }

    SkeletonTool::BoneHandle::BoneHandle()
    {
        Color4 handleRegularColor = Color4(126, 149, 160);
        Color4 handleHoverColor = Color4(147+30, 166+30, 174+30);
        Color4 handlePressedColor = Color4(44, 62, 80);

        auto getHandleVertices = [=](const Basis& transform, const Color4& color) {
            Vec2F scale = transform.GetScale();
            float handleTmpSize = 1.0f/scale.x/o2EditorSceneScreen.GetCamera().GetScaleX();

            Vector<Vec2F> points = { Vec2F(0, 0.5f), Vec2F(handleTmpSize, 0),
                Vec2F(0.99f, 0.4f), Vec2F(1.0f, 0.5f), Vec2F(0.99f, 0.6f),
                Vec2F(handleTmpSize, 1), Vec2F(0, 0.5f) };

            Vector<Vertex> localVertices; localVertices.Resize(points.Count());
            for (int i = 0; i < points.Count(); i++)
                localVertices[i].Set(points[i]*transform, color.ABGR(), 0, 0);

            return localVertices;
        };

        auto drawHandleRegular = [=](const Basis& transform, const Color4& color) {
            Color4 resultColor = handleRegularColor; resultColor.a = color.a;
            auto vertices = getHandleVertices(transform, resultColor);
            VertexIndex indexes[] = { 0, 1, 2,  0, 2, 3,  0, 3, 4,  0, 4, 5 };

            o2Render.DrawAAPolyLine(vertices.Data(), vertices.Count(), 3.0f);
            o2Render.DrawBuffer(PrimitiveType::Polygon, vertices.Data(), vertices.Count(), indexes, 4, o2Render.GetDefaultMaterial(), TextureRef::Null());
            //o2Render.DrawAABasis(transform);
        };

        auto drawHandleHover = [=](const Basis& transform, const Color4& color) {
            Color4 resultColor = handleHoverColor; resultColor.a = color.a;
            auto vertices = getHandleVertices(transform, resultColor);
            o2Render.DrawAAPolyLine(vertices.Data(), vertices.Count(), 3.0f);
        };

        auto drawHandlePressed = [=](const Basis& transform, const Color4& color) {
            Color4 resultColor = handlePressedColor; resultColor.a = color.a;
            auto vertices = getHandleVertices(transform, resultColor);
            o2Render.DrawAAPolyLine(vertices.Data(), vertices.Count(), 3.0f);
        };

        handle = mmake<SceneDragHandle>(mmake<FunctionalRectDrawable>(drawHandleRegular),
                                        mmake<FunctionalRectDrawable>(drawHandleHover),
                                        mmake<FunctionalRectDrawable>(drawHandlePressed));

        handle->GetRegularDrawable()->pivot = Vec2F(0, 0.5f);
        handle->GetHoverDrawable()->pivot = Vec2F(0, 0.5f);
        handle->GetPressedDrawable()->pivot = Vec2F(0, 0.5f);

        handle->onPressed = [&]() { OnHandlePressed(); };
        handle->onChangedPos = [&](const Vec2F& p) { OnHandleChangedPos(p); };
        handle->onReleased = [&]() { OnHandleReleased(); };
    }

    void SkeletonTool::BoneHandle::OnHandlePressed()
    {
        auto actor = boneComponent->GetActor();
        pressedTransform = actor->transform->worldBasis;
        mAction = mmake<BoneTransformAction>(actor);
    }

    void SkeletonTool::BoneHandle::OnHandleReleased()
    {
        if (!mAction)
            return;

        mAction->Completed();
        o2EditorSceneWindow.DoneAction(mAction);
        mAction = nullptr;
    }

    void SkeletonTool::BoneHandle::AppendBoneStep(const Basis& worldBasis, const Vec2F& worldPosition)
    {
        if (!mAction)
            return;

        auto actor = boneComponent->GetActor();
        auto step = mmake<BoneTransformAction>(actor);
        step->doneWorldBasis = worldBasis;
        step->doneWorldPosition = worldPosition;
        step->doneCaptured = true;

        mAction->Append(step);
    }

    SkeletonTool::BoneHandle::~BoneHandle()
    {}

    void SkeletonTool::BoneHandle::UpdatePosition()
    {
        auto compTransform = boneComponent->GetActor()->transform;
        float angle = compTransform->GetWorldBasis().GetAngle();
        handle->position = compTransform->worldPosition;

        handle->GetRegularDrawable()->angle = angle;
        handle->GetHoverDrawable()->angle = angle;
        handle->GetPressedDrawable()->angle = angle;
    }

    void SkeletonTool::BoneHandle::OnHandleChangedPos(const Vec2F& pos)
    {
        Vec2F origin = handle->GetDraggingBeginPosition();
        Vec2F pressedPos = origin - handle->GetDraggingOffset();
        Vec2F currentPos = handle->position - handle->GetDraggingOffset();
        Vec2F pressedDir = pressedPos - origin;
        Vec2F currentDir = currentPos - origin;
        float angle = pressedDir.Angle(currentDir);

        auto compTransform = boneComponent->GetActor()->transform;
        auto worldPos = compTransform->GetWorldPosition();
        Basis transform = Basis::Translated(worldPos*-1.0f)
            *Basis::Rotated(angle)
            *Basis::Translated(worldPos);

        AppendBoneStep(pressedTransform*transform, worldPos);

        UpdatePosition();
    }

    void SkeletonTool::SkeletonInstance::UpdateHandlesPositions()
    {
        for (auto& handle : boneHandles)
            handle->UpdatePosition();
    }

}
// --- META ---

DECLARE_CLASS(Editor::SkeletonTool, Editor__SkeletonTool);
// --- END META ---
