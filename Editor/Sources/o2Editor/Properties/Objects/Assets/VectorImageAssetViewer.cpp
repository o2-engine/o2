#include "o2Editor/stdafx.h"
#include "VectorImageAssetViewer.h"

#include "o2/Assets/Types/VectorImageAsset.h"
#include "o2/Scene/UI/UIManager.h"
#include "o2/Utils/Editor/EditorScope.h"
#include "o2Editor/Properties/Basic/BorderIntProperty.h"
#include "o2Editor/UI/ImageSlicesEditorWidget.h"

namespace Editor
{
    const Type* VectorImageAssetViewer::GetViewingObjectType() const
    {
        return GetViewingObjectTypeStatic();
    }

    const Type* VectorImageAssetViewer::GetViewingObjectTypeStatic()
    {
        return &TypeOf(VectorImageAsset);
    }

    void VectorImageAssetViewer::RebuildProperties(const Vector<Pair<IObject*, IObject*>>& targetObjets)
    {
        PushEditorScopeOnStack scope;

        mSizeLabel = o2UI.CreateLabel("Size: 0x0");
        mSizeLabel->layout->minHeight = 20;
        mSpoiler->AddChild(mSizeLabel);

        mMeshLabel = o2UI.CreateLabel("");
        mMeshLabel->layout->minHeight = 20;
        mSpoiler->AddChild(mMeshLabel);

        mErrorLabel = o2UI.CreateLabel("");
        mErrorLabel->layout->minHeight = 20;
        mErrorLabel->color = Color4(200, 70, 50);
        mSpoiler->AddChild(mErrorLabel);

        mSlicesEditor = mmake<ImageSlicesEditorWidget>();
        mSpoiler->AddChild(mSlicesEditor);
    }

    void VectorImageAssetViewer::OnRefreshed(const Vector<Pair<IObject*, IObject*>>& targetObjets)
    {
        // Also called while the spoiler is collapsed, before the widgets are built
        if (targetObjets.IsEmpty() || !mSizeLabel)
            return;

        auto image = dynamic_cast<VectorImageAsset*>(targetObjets.Last().first);
        if (!image)
            return;

        Vec2F size = image->GetSize();
        mSizeLabel->SetText("Size: " + String(size.x) + "x" + String(size.y));

        String drawing = image->IsRasterized() ? "rasters in the atlas" : (image->IsMeshBaked() ? "baked mesh" :
            (image->IsMeshScalable() ? "mesh for any scale" : "mesh for each scale"));

        auto mesh = image->IsValid() ? image->GetMesh(Vec2F(1, 1)) : nullptr;
        mMeshLabel->SetText("Shapes: " + String((int)image->GetImage().shapes.Count()) + ", triangles at 1x: " +
                            String(mesh ? (int)mesh->mesh.GetTrianglesCount() : 0) + ", drawn by " + drawing);

        String problems = image->GetError();
        if (problems.IsEmpty() && !image->GetWarnings().IsEmpty())
            problems = String((int)image->GetWarnings().Count()) + " warnings: " + image->GetWarnings()[0];

        mErrorLabel->SetText(problems);
        mErrorLabel->enabled = !problems.IsEmpty();

        auto border = DynamicCast<BorderIProperty>(mPropertiesContext->properties[TypeOf(VectorImageAsset).GetField("sliceBorder")]);
        if (border)
            mSlicesEditor->Setup(AssetRef<VectorImageAsset>(image->GetUID()), border);
    }
}
// --- META ---

DECLARE_CLASS(Editor::VectorImageAssetViewer, Editor__VectorImageAssetViewer);
// --- END META ---
