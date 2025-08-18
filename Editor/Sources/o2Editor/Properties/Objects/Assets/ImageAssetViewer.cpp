#include "o2Editor/stdafx.h"
#include "ImageAssetViewer.h"

#include "o2/Assets/Types/ImageAsset.h"
#include "o2/Scene/UI/UIManager.h"
#include "o2/Scene/UI/Widgets/Button.h"
#include "o2/Scene/UI/Widgets/VerticalLayout.h"
#include "o2/Utils/Editor/DragHandle.h"
#include "o2Editor/Windows/AnimationWindow/AnimationWindow.h"
#include "o2/Utils/Editor/EditorScope.h"
#include "o2Editor/Properties/Basic/AssetProperty.h"
#include "o2Editor/Properties/Basic/BorderIntProperty.h"
#include "o2Editor/Properties/Properties.h"
#include "o2Editor/UI/ImageSlicesEditorWidget.h"
#include "o2Editor/Windows/SceneWindow/SceneEditScreen.h"

namespace Editor
{
    const Type* ImageAssetViewer::GetViewingObjectType() const
    {
        return GetViewingObjectTypeStatic();
    }

    const Type* ImageAssetViewer::GetViewingObjectTypeStatic()
    {
        return &TypeOf(ImageAsset);
    }

    void ImageAssetViewer::RebuildProperties(const Vector<Pair<IObject*, IObject*>>& targetObjets)
    {
        PushEditorScopeOnStack scope;

        mSizeLabel = o2UI.CreateLabel("Size: 0x0");
        mSizeLabel->layout->minHeight = 20;
        mSpoiler->AddChild(mSizeLabel);

        mSlicesEditor = mmake<ImageSlicesEditorWidget>();
        mSpoiler->AddChild(mSlicesEditor);

        mAtlasProperty = DynamicCast<AssetProperty>(o2EditorProperties.CreateRegularField(
            &TypeOf(AssetRef<AtlasAsset>), "Atlas", mOnChildFieldChangeCompleted, onChanged));

        mSpoiler->AddChild(mAtlasProperty);
    }

    void ImageAssetViewer::OnRefreshed(const Vector<Pair<IObject*, IObject*>>& targetObjets)
    {
        if (!targetObjets.IsEmpty())
        {
            auto image = dynamic_cast<ImageAsset*>(targetObjets.Last().first);
            mSizeLabel->SetText("Size: " + String(image->GetSize().x) + "x" + String(image->GetSize().y));

            mSlicesEditor->Setup(AssetRef<ImageAsset>(dynamic_cast<ImageAsset*>(targetObjets.Last().first)->GetUID()),
                                 DynamicCast<BorderIProperty>(mPropertiesContext->properties[TypeOf(ImageAsset).GetField("sliceBorder")]));
        }

        mAtlasProxies.Clear();
        for (auto& targets : targetObjets)
        {
            ImageAsset* image = dynamic_cast<ImageAsset*>(targets.first);
            auto proxy = mmake<FunctionalValueProxy<AssetRef<AtlasAsset>>>(
                [image](const AssetRef<AtlasAsset>& value) { image->SetAtlas(value ? value->GetUID() : UID::empty); },
                [image]() { return AssetRef<AtlasAsset>(image->GetAtlasUID()); }
            );

            mAtlasProxies.Add(proxy);
        }

        mAtlasProperty->SetValueProxy(mAtlasProxies);
    }

}
// --- META ---

DECLARE_CLASS(Editor::ImageAssetViewer, Editor__ImageAssetViewer);
// --- END META ---
