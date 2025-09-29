#include "o2Editor/stdafx.h"
#include "SkinningMeshComponentViewer.h"

#include "o2Editor/Properties/Properties.h"
#include "o2Editor/Tools/SkeletonTool.h"
#include "o2Editor/Windows/SceneWindow/SceneDragHandle.h"
#include "o2Editor/Windows/SceneWindow/SceneEditScreen.h"

namespace Editor
{
    SkinningMeshComponentViewer::SkinningMeshComponentViewer()
    {
        mSplineTool = mmake<SplineTool>();
        mFrameTool = mmake<CustomFrameTool>();
        mTopologyTool = mmake<MeshTopologyTool>();
        mSkeletonTool = mmake<SkeletonTool>();
        mFrameTetxureLayer = mmake<SkinningMeshEditorLayer>();
    }

    SkinningMeshComponentViewer::~SkinningMeshComponentViewer()
    {
        o2EditorSceneScreen.RemoveTool(mSplineTool);
        o2EditorSceneScreen.RemoveTool(mFrameTool);
        o2EditorSceneScreen.RemoveTool(mTopologyTool);
        o2EditorSceneScreen.RemoveEditorLayer(mFrameTetxureLayer);
    }

    SkinningMeshComponentViewer& SkinningMeshComponentViewer::operator=(const SkinningMeshComponentViewer& other)
    {
        TObjectPropertiesViewer<SkinningMeshComponent>::operator=(other);
        return *this;
    }

    void SkinningMeshComponentViewer::RebuildProperties(const Vector<Pair<IObject*, IObject*>>& targetObjets)
    {
        o2EditorProperties.BuildObjectProperties(mSpoiler, &TypeOf(SkinningMeshComponent), mPropertiesContext, "",
            mOnPropertyChangeCompleted, mOnPropertyChanged);

        mFitAndCenterButton = o2UI.CreateButton("Fit and centerize image", THIS_FUNC(FitAndCenterize));
        mEditSkeletonButton = o2UI.CreateButton("Edit skeleton", THIS_FUNC(OnEditSkeletonPressed));
        mReskinButton = o2UI.CreateButton("Reskin", THIS_FUNC(OnReskinPressed));

        mSpoiler->AddChild(mFitAndCenterButton);
        mSpoiler->AddChild(mEditSkeletonButton);
        mSpoiler->AddChild(mReskinButton);
    }

    void SkinningMeshComponentViewer::OnRefreshed(const Vector<Pair<IObject*, IObject*>>& targetObjets)
    {
        auto prevTargetObjects = mTypeTargetObjects;

        TObjectPropertiesViewer<SkinningMeshComponent>::OnRefreshed(targetObjets);

        if (!mTypeTargetObjects.IsEmpty() && prevTargetObjects != mTypeTargetObjects)
        {
            Function<Vec2F()> getOrigin = [&]() {
                return mTypeTargetObjects[0].first->GetActor()->transform->GetWorldNonSizedBasis().origin;
            };

            // Spline tool
            mSplineTool->SetSpline(mTypeTargetObjects[0].first->spline, getOrigin);
            mSplineTool->onChanged = [&]() { mTypeTargetObjects[0].first->GetActor()->OnChanged(); };

            // Frame tool
            mFrameTool->SetFrame(Basis(mTypeTargetObjects[0].first->GetMappingFrame()));
            mFrameTool->frameHandles->SetRotationEnabled(false);
            mFrameTool->getOrigin = getOrigin;
            mFrameTool->onChanged = [&](const Basis& b) {
                mTypeTargetObjects[0].first->SetMappingFrame(b.AABB());
                mTypeTargetObjects[0].first->GetActor()->OnChanged();
            };

            mFrameTetxureLayer->mesh = Ref(mTypeTargetObjects[0].first);

            // Topology tool
            auto mesh = mTypeTargetObjects[0].first;
            mTopologyTool->Setup([=]() { return mesh->GetExtraPoints(); },
                [=](int idx, Vec2F p) { mesh->SetExtraPoint(idx, p); mesh->GetActor()->OnChanged(); },
                [=]() { return mesh->GetActor()->transform->GetWorldNonSizedBasis(); },
                [=](Vec2F p) { mesh->AddExtraPoint(p); mesh->GetActor()->OnChanged(); },
                [=](int idx) { mesh->RemoveExtraPoint(idx); mesh->GetActor()->OnChanged(); });
        }
    }

    void SkinningMeshComponentViewer::OnPropertiesEnabled()
    {
        o2EditorSceneScreen.AddTool(mSplineTool);
        o2EditorSceneScreen.AddTool(mFrameTool);
        o2EditorSceneScreen.AddTool(mTopologyTool);

        o2EditorSceneScreen.SelectTool<SplineTool>();

        o2EditorSceneScreen.AddEditorLayer(mFrameTetxureLayer);

        mSkeletonTool = o2EditorSceneScreen.GetTool<SkeletonTool>();
        if (mSkeletonTool && !mTypeTargetObjects.IsEmpty() && mTypeTargetObjects[0].first)
            mEditingSkeleton = mSkeletonTool->IsEditingSkeleton(Ref(mTypeTargetObjects[0].first));

        UpdateEditSkeletonButtonCaption();
    }

    void SkinningMeshComponentViewer::OnPropertiesDisabled()
    {
        o2EditorSceneScreen.RemoveTool(mSplineTool);
        o2EditorSceneScreen.RemoveTool(mFrameTool);
        o2EditorSceneScreen.RemoveTool(mTopologyTool);

        o2EditorSceneScreen.RemoveEditorLayer(mFrameTetxureLayer);

        mSplineTool->Reset();
        mFrameTool->Reset();
        mTopologyTool->Reset();
    }

    void SkinningMeshComponentViewer::FitAndCenterize()
    {
        for (auto& target : mTypeTargetObjects)
        {
            auto component = target.first;
            auto texture = component->GetImage();
            auto size = texture->GetSize();

            component->SetMappingFrame(RectF(size * -0.5f, size * 0.5f));

            mFrameTool->SetFrame(Basis(mTypeTargetObjects[0].first->GetMappingFrame()));
        }
    }

    void SkinningMeshComponentViewer::OnEditSkeletonPressed()
    {
        if (mTypeTargetObjects.IsEmpty() || !mTypeTargetObjects[0].first)
            return;

        mEditingSkeleton = !mEditingSkeleton;

        auto component = Ref(mTypeTargetObjects[0].first);

        if (mEditingSkeleton)
        {
            mSkeletonTool = o2EditorSceneScreen.GetTool<SkeletonTool>();
            if (!mSkeletonTool)
            {
                mSkeletonTool = mmake<SkeletonTool>();
                o2EditorSceneScreen.AddTool(mSkeletonTool);
            }

            mSkeletonTool->AddSkeletonInstance(component);

            o2EditorSceneScreen.SelectTool<SkeletonTool>();
        }
        else
        {
            if (mSkeletonTool)
            {
                mSkeletonTool->RemoveSkeletonInstance(component);

                if (mSkeletonTool->GetEditingSkeletons().Count() == 0)
                {
                    o2EditorSceneScreen.RemoveTool(mSkeletonTool);
                    mSkeletonTool = nullptr;
                }
            }
        }

        UpdateEditSkeletonButtonCaption();
    }

    void SkinningMeshComponentViewer::OnReskinPressed()
    {
        for (auto& targetPair : mTypeTargetObjects)
        {
            targetPair.first->NeedUpdateBones();
            targetPair.first->OnUpdate(0.0f);
        }
    }

    void SkinningMeshComponentViewer::UpdateEditSkeletonButtonCaption()
    {
        mEditSkeletonButton->caption = mEditingSkeleton ? "Stop editing skeleton" : "Edit skeleton";
    }
}

DECLARE_TEMPLATE_CLASS(Editor::TObjectPropertiesViewer<SkinningMeshComponent>);
// --- META ---

DECLARE_CLASS(Editor::SkinningMeshComponentViewer, Editor__SkinningMeshComponentViewer);
// --- END META ---
