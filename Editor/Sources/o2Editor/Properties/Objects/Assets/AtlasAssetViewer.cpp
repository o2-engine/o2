#include "o2Editor/stdafx.h"
#include "AtlasAssetViewer.h"

#include "o2/Assets/Types/AtlasAsset.h"
#include "o2/Scene/UI/UIManager.h"
#include "o2/Scene/UI/Widgets/Label.h"
#include "o2/Utils/Editor/EditorScope.h"
#include "o2Editor/UI/TexturePreview.h"

namespace Editor
{
    const Type* AtlasAssetViewer::GetViewingObjectType() const
    {
        return GetViewingObjectTypeStatic();
    }

    const Type* AtlasAssetViewer::GetViewingObjectTypeStatic()
    {
        return &TypeOf(AtlasAsset);
    }

    void AtlasAssetViewer::RebuildProperties(const Vector<Pair<IObject*, IObject*>>& targetObjets)
    {
        PushEditorScopeOnStack scope;

        mPagesLabel = o2UI.CreateLabel("Pages: 0");
        mPagesLabel->layout->minHeight = 20;
        mSpoiler->AddChild(mPagesLabel);

        mTexturePreview = mmake<TexturePreview>();
        *mTexturePreview->layout = WidgetLayout::HorStretch(VerAlign::Middle, 0, 0, 300);
        mTexturePreview->layout->minHeight = 300;

        mSpoiler->AddChild(mTexturePreview);
    }

    void AtlasAssetViewer::OnRefreshed(const Vector<Pair<IObject*, IObject*>>& targetObjets)
    {
        if (targetObjets.IsEmpty())
            return;

        // OnRefreshed is also called while the spoiler is collapsed, before the properties
        // (and these widgets) are built
        if (!mPagesLabel || !mTexturePreview)
            return;

        auto atlas = dynamic_cast<AtlasAsset*>(targetObjets.Last().first);
        if (!atlas)
            return;

        auto& pages = atlas->GetPages();
        mPagesLabel->SetText("Pages: " + (String)pages.Count() +
                             ", images: " + (String)atlas->GetImages().Count());

        // The first page is enough for a preview; the pages exist after the assets build
        if (!pages.IsEmpty() && pages[0].GetTexture())
        {
            mTexturePreview->SetTexture(pages[0].GetTexture());
            mTexturePreview->SetEnabled(true);
        }
        else
            mTexturePreview->SetEnabled(false);
    }
}
// --- META ---

DECLARE_CLASS(Editor::AtlasAssetViewer, Editor__AtlasAssetViewer);
// --- END META ---
