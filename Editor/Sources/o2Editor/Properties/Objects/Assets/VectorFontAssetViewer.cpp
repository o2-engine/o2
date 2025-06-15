#include "o2Editor/stdafx.h"
#include "VectorFontAssetViewer.h"

#include "o2/Utils/Editor/EditorScope.h"
#include "o2Editor/UI/TexturePreview.h"

namespace Editor
{
    const Type* VectorFontAssetViewer::GetViewingObjectType() const
    {
        return GetViewingObjectTypeStatic();
    }

    const Type* VectorFontAssetViewer::GetViewingObjectTypeStatic()
    {
        return &TypeOf(VectorFontAsset);
    }

    void VectorFontAssetViewer::RebuildProperties(const Vector<Pair<IObject*, IObject*>>& targetObjets)
    {
        PushEditorScopeOnStack scope;

        mTexturePreview = mmake<TexturePreview>();
        *mTexturePreview->layout = WidgetLayout::HorStretch(VerAlign::Middle, 0, 0, 300);
        mTexturePreview->layout->minHeight = 300;

        mSpoiler->AddChild(mTexturePreview);
    }

    void VectorFontAssetViewer::OnRefreshed(const Vector<Pair<IObject*, IObject*>>& targetObjets)
    {
        if (!targetObjets.IsEmpty())
        {
            auto fontAsset = dynamic_cast<VectorFontAsset*>(targetObjets[0].first);
            auto font = fontAsset->GetFont();
            if (font)
            {
                mTexturePreview->SetTexture(font->GetTexture(), font->GetTextureSrcRect());
                mTexturePreview->SetEnabled(true);
            }
            else
                mTexturePreview->SetEnabled(false);
        }
    }

}
// --- META ---

DECLARE_CLASS(Editor::VectorFontAssetViewer, Editor__VectorFontAssetViewer);
// --- END META ---
