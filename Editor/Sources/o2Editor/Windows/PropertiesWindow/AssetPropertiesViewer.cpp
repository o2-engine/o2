#include "o2Editor/stdafx.h"
#include "AssetPropertiesViewer.h"

#include "o2Editor/Properties/ObjectViewer.h"

namespace Editor
{

    AssetPropertiesViewer::AssetPropertiesViewer()
    {
        mAssetHeader = mmake<Widget>();
        *mAssetHeader->layout = WidgetLayout::HorStretch(VerAlign::Top, 5, 5, 25, 5);

        mAssetNameLabel = o2UI.CreateLabel("asset path");
        *mAssetNameLabel->layout = WidgetLayout::HorStretch(VerAlign::Top, 0, 55, 20);
        mAssetNameLabel->horAlign = HorAlign::Left;
        mAssetHeader->AddChild(mAssetNameLabel);

        mSaveButton = o2UI.CreateButton("Save");
        *mSaveButton->layout = WidgetLayout::Based(BaseCorner::RightBottom, Vec2F(50, 17), Vec2F(0, 6));
        mSaveButton->onClick = THIS_FUNC(OnSavePressed);
        mAssetHeader->AddChild(mSaveButton);

        auto separatorImg = o2UI.CreateImage("ui/UI4_Separator.png");
        *separatorImg->layout = WidgetLayout::HorStretch(VerAlign::Bottom, -6, -15, 5, -4);
        mAssetHeader->AddChild(separatorImg);

        mContentWidget->AddChild(mAssetHeader, 0);
        *mViewer->layout = WidgetLayout::BothStretch(5, 0, 5, 35);
    }

    const Type* AssetPropertiesViewer::GetViewingObjectType() const
    {
        return &TypeOf(Asset);
    }

    void AssetPropertiesViewer::SetTargets(const Vector<IObject*>& targets)
    {
        DefaultPropertiesViewer::SetTargets(targets);

        if (targets.Count() == 1)
        {
            if (Asset* asset = dynamic_cast<Asset*>(targets[0]))
                mAssetNameLabel->text = asset->GetPath();
        }
        else
        {
            mAssetNameLabel->text = "Multiple assets selected";
        }
    }

    void AssetPropertiesViewer::OnSavePressed()
    {
        for (IObject* target : mTargets)
        {
            if (Asset* asset = dynamic_cast<Asset*>(target))
                asset->Save();
        }

        o2Assets.RebuildAssets();
    }

}
// --- META ---

DECLARE_CLASS(Editor::AssetPropertiesViewer, Editor__AssetPropertiesViewer);
// --- END META ---
