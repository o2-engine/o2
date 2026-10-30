#include "o2Editor/stdafx.h"
#include "ModelAssetViewers.h"

#include "o2/Assets/Types/Mesh3DAsset.h"
#include "o2/Assets/Types/SkinnedModelAsset.h"
#include "o2/Scene/UI/UIManager.h"
#include "o2/Scene/UI/WidgetLayout.h"
#include "o2/Scene/UI/Widgets/Label.h"
#include "o2/Utils/Editor/EditorScope.h"
#include "o2Editor/UI/Preview/ModelView.h"
#include "o2Editor/Windows/AssetPreviewWindow/AssetPreviewWindow.h"

namespace Editor
{
    const Type* Mesh3DAssetViewer::GetViewingObjectType() const
    {
        return GetViewingObjectTypeStatic();
    }

    const Type* Mesh3DAssetViewer::GetViewingObjectTypeStatic()
    {
        return &TypeOf(Mesh3DAsset);
    }

    void Mesh3DAssetViewer::RebuildProperties(const Vector<Pair<IObject*, IObject*>>& targetObjets)
    {
        PushEditorScopeOnStack scope;

        mInfoLabel = o2UI.CreateLabel("");
        mInfoLabel->name = "model info";
        mInfoLabel->layout->minHeight = 20;
        mInfoLabel->horAlign = HorAlign::Left;
        mSpoiler->AddChild(mInfoLabel);

        mModelView = mmake<ModelView>();
        mModelView->name = "model view";
        mModelView->layout->minHeight = 220;
        mSpoiler->AddChild(mModelView);

        mSpoiler->AddChild(AssetPreviewWindow::CreateOpenButton([this]() {
            auto asset = mAsset.Lock();
            return asset ? AssetRef<Asset>(asset) : AssetRef<Asset>();
        }));
    }

    void Mesh3DAssetViewer::OnRefreshed(const Vector<Pair<IObject*, IObject*>>& targetObjets)
    {
        if (targetObjets.IsEmpty() || !mModelView)
            return;

        auto mesh = dynamic_cast<Mesh3DAsset*>(targetObjets.Last().first);
        if (!mesh)
            return;

        // The properties refresh their viewers all the time: the view keeps its camera until the mesh changes
        if (mAsset.Lock().Get() != mesh)
            mModelView->SetMesh(mesh->GetMeshData());

        mAsset = WeakRef<Asset>(mesh);
        mInfoLabel->SetText((String)(int)mModelView->GetVerticesCount() + " vertices, " +
                            (String)(int)mModelView->GetTrianglesCount() + " triangles");
    }

    const Type* SkinnedModelAssetViewer::GetViewingObjectType() const
    {
        return GetViewingObjectTypeStatic();
    }

    const Type* SkinnedModelAssetViewer::GetViewingObjectTypeStatic()
    {
        return &TypeOf(SkinnedModelAsset);
    }

    void SkinnedModelAssetViewer::OnRefreshed(const Vector<Pair<IObject*, IObject*>>& targetObjets)
    {
        if (targetObjets.IsEmpty() || !mModelView)
            return;

        auto model = dynamic_cast<SkinnedModelAsset*>(targetObjets.Last().first);
        if (!model)
            return;

        if (mAsset.Lock().Get() != model)
            mModelView->SetSkinnedModel(AssetRef<SkinnedModelAsset>(Ref<SkinnedModelAsset>(model)));

        mAsset = WeakRef<Asset>(model);

        auto& data = model->GetModelData();
        String clips;
        for (auto& clip : data.animations)
            clips += (clips.IsEmpty() ? String(": ") : String(", ")) + clip.name;

        mInfoLabel->SetText((String)(int)mModelView->GetVerticesCount() + " vertices, " +
                            (String)(int)mModelView->GetTrianglesCount() + " triangles, " + (String)data.joints.Count() +
                            " joints, " + (String)data.animations.Count() + " animations" + clips);
    }
}
// --- META ---

DECLARE_CLASS(Editor::Mesh3DAssetViewer, Editor__Mesh3DAssetViewer);

DECLARE_CLASS(Editor::SkinnedModelAssetViewer, Editor__SkinnedModelAssetViewer);
// --- END META ---
