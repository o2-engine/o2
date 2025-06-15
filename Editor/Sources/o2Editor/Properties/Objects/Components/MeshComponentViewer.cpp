#include "o2Editor/stdafx.h"
#include "MeshComponentViewer.h"

#include "o2Editor/Properties/Properties.h"
#include "o2Editor/Windows/SceneWindow/SceneEditScreen.h"

namespace Editor
{
    MeshComponentViewer::MeshComponentViewer()
    {
        mSplineTool = mmake<SplineTool>();
        mFrameTool = mmake<CustomFrameTool>();
        mTopologyTool = mmake<MeshTopologyTool>();
        mFrameTetxureLayer = mmake<SceneLayer>();
    }

    MeshComponentViewer::~MeshComponentViewer()
    {
        o2EditorSceneScreen.RemoveTool(mSplineTool);
        o2EditorSceneScreen.RemoveTool(mFrameTool);
        o2EditorSceneScreen.RemoveTool(mTopologyTool);
        o2EditorSceneScreen.RemoveEditorLayer(mFrameTetxureLayer);
    }

    MeshComponentViewer& MeshComponentViewer::operator=(const MeshComponentViewer& other)
    {
        TObjectPropertiesViewer<MeshComponent>::operator=(other);
        return *this;
    }

    void MeshComponentViewer::RebuildProperties(const Vector<Pair<IObject*, IObject*>>& targetObjets)
    {
        o2EditorProperties.BuildObjectProperties(mSpoiler, &TypeOf(MeshComponent), mPropertiesContext, "",
                                                 mOnChildFieldChangeCompleted, onChanged);

        mFitAndCenterButton = o2UI.CreateButton("Fit and centerize", THIS_FUNC(FitAndCenterize));

        mSpoiler->AddChild(mFitAndCenterButton);
    }

    void MeshComponentViewer::OnRefreshed(const Vector<Pair<IObject*, IObject*>>& targetObjets)
    {
        auto prevTargetObjects = mTypeTargetObjects;

        TObjectPropertiesViewer<MeshComponent>::OnRefreshed(targetObjets);

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

            mFrameTetxureLayer->viewer = Ref(this);

            // Topology tool
            auto mesh = mTypeTargetObjects[0].first;
            mTopologyTool->Setup([=]() { return mesh->GetExtraPoints(); },
                                 [=](int idx, Vec2F p) { mesh->SetExtraPoint(idx, p); mesh->GetActor()->OnChanged(); },
                                 [=]() { return mesh->GetActor()->transform->GetWorldNonSizedBasis(); },
                                 [=](Vec2F p) { mesh->AddExtraPoint(p); mesh->GetActor()->OnChanged(); },
                                 [=](int idx) { mesh->RemoveExtraPoint(idx); mesh->GetActor()->OnChanged(); });
        }
    }

    void MeshComponentViewer::OnPropertiesEnabled()
    {
        o2EditorSceneScreen.AddTool(mSplineTool);
        o2EditorSceneScreen.AddTool(mFrameTool);
        o2EditorSceneScreen.AddTool(mTopologyTool);

        mPrevSelectedTool = o2EditorSceneScreen.GetSelectedTool();
        o2EditorSceneScreen.SelectTool<SplineTool>();

        o2EditorSceneScreen.AddEditorLayer(mFrameTetxureLayer);
    }

    void MeshComponentViewer::OnPropertiesDisabled()
    {
        auto selectedTool = o2EditorSceneScreen.GetSelectedTool();
        if (selectedTool == mSplineTool || selectedTool == mFrameTool || selectedTool == mTopologyTool)
            o2EditorSceneScreen.SelectTool(mPrevSelectedTool.Lock());

        o2EditorSceneScreen.RemoveTool(mSplineTool);
        o2EditorSceneScreen.RemoveTool(mFrameTool);
        o2EditorSceneScreen.RemoveTool(mTopologyTool);

        o2EditorSceneScreen.RemoveEditorLayer(mFrameTetxureLayer);
    }

    void MeshComponentViewer::FitAndCenterize()
    {
        for (auto& target : mTypeTargetObjects)
        {
            auto component = target.first;
            auto texture = component->GetImage();
            auto size = texture->GetSize();

            component->SetMappingFrame(RectF(size*-0.5f, size*0.5f));

            mFrameTool->SetFrame(Basis(mTypeTargetObjects[0].first->GetMappingFrame()));
        }
    }

    void MeshComponentViewer::SceneLayer::DrawOverScene()
    {
        auto viewerRef = viewer.Lock();
        if (!viewerRef->mTypeTargetObjects.IsEmpty())
        {
            if (viewerRef->mFrameTool->isEnabled || viewerRef->mSplineTool->isEnabled)
            {
                auto obj = viewerRef->mTypeTargetObjects[0].first;

                textureSprite.SetImageAsset(obj->GetImage());
                textureSprite.SetBasis(Basis(obj->GetMappingFrame())
                                       *Basis::Translated(obj->GetActor()->transform->GetWorldPosition())
                                       *o2EditorSceneScreen.GetLocalToScreenTransform());
                textureSprite.SetTransparency(0.5f);
                textureSprite.Draw();
            }

            if (viewerRef->mTopologyTool->isEnabled || viewerRef->mSplineTool->isEnabled)
                DrawMeshWire();
        }
    }

    void MeshComponentViewer::SceneLayer::Update(float dt)
    {}

    int MeshComponentViewer::SceneLayer::GetOrder() const
    {
        return 0;
    }

    bool MeshComponentViewer::SceneLayer::IsEnabled() const
    {
        auto viewerRef = viewer.Lock();
        return viewerRef->mFrameTool->isEnabled || viewerRef->mSplineTool->isEnabled || viewerRef->mTopologyTool->isEnabled;
    }

    const String& MeshComponentViewer::SceneLayer::GetName() const
    {
        static String str("mesh texture overlay");
        return str;
    }

    const String& MeshComponentViewer::SceneLayer::GetIconName() const
    {
        return String::empty;
    }

    void MeshComponentViewer::SceneLayer::DrawMeshWire()
    {
        auto viewerRef = viewer.Lock();
        if (!viewerRef->mTypeTargetObjects.IsEmpty())
        {
            auto& mesh = viewerRef->mTypeTargetObjects[0].first->GetMesh();

            Color4 wireColor(0, 0, 0, 100);
            Vector<Vertex> verticies;
            for (UInt i = 0; i < mesh.polyCount; i++)
            {
                auto v = o2EditorSceneScreen.LocalToScreenPoint(mesh.vertices[mesh.indexes[i*3]]);
                auto v1 = o2EditorSceneScreen.LocalToScreenPoint(mesh.vertices[mesh.indexes[i*3 + 1]]);
                auto v2 = o2EditorSceneScreen.LocalToScreenPoint(mesh.vertices[mesh.indexes[i*3 + 2]]);

                verticies.Clear();
                verticies.Add(Vertex(v.x, v.y, 0.0f, wireColor.ARGB(), 0.0f, 0.0f));
                verticies.Add(Vertex(v1.x, v1.y, 0.0f, wireColor.ARGB(), 0.0f, 0.0f));
                verticies.Add(Vertex(v2.x, v2.y, 0.0f, wireColor.ARGB(), 0.0f, 0.0f));
                o2Render.DrawPolyLine(verticies.Data(), verticies.Count());
            }
        }
    }
}

DECLARE_TEMPLATE_CLASS(Editor::TObjectPropertiesViewer<MeshComponent>);
// --- META ---

DECLARE_CLASS(Editor::MeshComponentViewer, Editor__MeshComponentViewer);
// --- END META ---
