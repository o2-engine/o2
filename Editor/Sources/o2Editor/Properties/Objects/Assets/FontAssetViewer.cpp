#include "o2Editor/stdafx.h"
#include "FontAssetViewer.h"

#include "o2/Assets/Types/FontAsset.h"
#include "o2/Scene/UI/UIManager.h"
#include "o2/Scene/UI/WidgetLayout.h"
#include "o2/Scene/UI/Widgets/HorizontalLayout.h"
#include "o2/Utils/Editor/EditorScope.h"
#include "o2Editor/UI/Preview/FontSampleView.h"
#include "o2Editor/Windows/AssetPreviewWindow/AssetPreviewWindow.h"

namespace Editor
{
    const Type* FontAssetViewer::GetViewingObjectType() const
    {
        return GetViewingObjectTypeStatic();
    }

    const Type* FontAssetViewer::GetViewingObjectTypeStatic()
    {
        return &TypeOf(FontAsset);
    }

    void FontAssetViewer::RebuildProperties(const Vector<Pair<IObject*, IObject*>>& targetObjets)
    {
        PushEditorScopeOnStack scope;

        mSample = mmake<FontSampleView>();
        mSample->name = "font sample";
        mSample->SetSizes({ 12, 16, 24 });
        mSpoiler->AddChild(mSample);

        auto getAsset = [this]() {
            auto asset = mAsset.Lock();
            return asset ? AssetRef<Asset>(asset) : AssetRef<Asset>();
        };

        auto buttons = o2UI.CreateHorLayout();
        buttons->name = "buttons";
        buttons->spacing = 4.0f;
        buttons->expandWidth = true;
        buttons->layout->minHeight = 22.0f;
        buttons->AddChild(AssetPreviewWindow::CreateOpenButton(getAsset));
        buttons->AddChild(AssetPreviewWindow::CreateOpenButton(getAsset, "Open atlas", "Atlas"));
        mSpoiler->AddChild(buttons);
    }

    void FontAssetViewer::OnRefreshed(const Vector<Pair<IObject*, IObject*>>& targetObjets)
    {
        if (targetObjets.IsEmpty() || !mSample)
            return;

        auto font = dynamic_cast<FontAsset*>(targetObjets.Last().first);
        if (!font)
            return;

        if (mAsset.Lock().Get() != font)
            mSample->SetFont(font->GetFont());

        mAsset = WeakRef<Asset>(font);
        mSample->layout->minHeight = mSample->GetContentHeight();
    }
}
// --- META ---

DECLARE_CLASS(Editor::FontAssetViewer, Editor__FontAssetViewer);
// --- END META ---
