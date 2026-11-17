#include "o2Editor/stdafx.h"
#include "FontStyleAssetViewer.h"

#include "o2/Assets/Types/FontStyleAsset.h"
#include "o2/Assets/Types/VectorFontAsset.h"
#include "o2/Scene/UI/WidgetLayout.h"
#include "o2/Utils/Editor/EditorScope.h"
#include "o2Editor/UI/Preview/FontSampleView.h"

namespace Editor
{
    FontStyleAssetViewer::~FontStyleAssetViewer()
    {
        Unsubscribe();
    }

    const Type* FontStyleAssetViewer::GetViewingObjectType() const
    {
        return GetViewingObjectTypeStatic();
    }

    const Type* FontStyleAssetViewer::GetViewingObjectTypeStatic()
    {
        return &TypeOf(FontStyleAsset);
    }

    Ref<Font> FontStyleAssetViewer::GetSampleFont()
    {
        PushEditorScopeOnStack scope;

        AssetRef<VectorFontAsset> font("stdFont.ttf");
        return font ? font->GetFont() : Ref<Font>();
    }

    void FontStyleAssetViewer::RebuildProperties(const Vector<Pair<IObject*, IObject*>>& targetObjets)
    {
        PushEditorScopeOnStack scope;

        mSample = mmake<FontSampleView>();
        mSample->name = "font style sample";
        mSample->SetSizes({ 14, 24, 40 });
        mSample->SetSampleText("Aa Bb Cc 0123 Съешь");
        mSample->SetFont(GetSampleFont());
        mSpoiler->AddChild(mSample);
    }

    void FontStyleAssetViewer::OnRefreshed(const Vector<Pair<IObject*, IObject*>>& targetObjets)
    {
        if (targetObjets.IsEmpty() || !mSample)
            return;

        auto style = dynamic_cast<FontStyleAsset*>(targetObjets.Last().first);
        if (!style)
            return;

        if (mStyle.Lock().Get() != style)
        {
            Unsubscribe();

            Ref<FontStyleAsset> styleRef(style);
            mStyle = styleRef;
            style->onChanged += ObjFunctionPtr<FontStyleAssetViewer, void>(this, &FontStyleAssetViewer::OnStyleChanged);
            mSample->SetFontStyle(styleRef);
        }

        mSample->layout->minHeight = mSample->GetContentHeight();
    }

    void FontStyleAssetViewer::OnStyleChanged()
    {
        if (auto style = mStyle.Lock())
            mSample->SetFontStyle(style);
    }

    void FontStyleAssetViewer::Unsubscribe()
    {
        if (auto style = mStyle.Lock())
            style->onChanged -= ObjFunctionPtr<FontStyleAssetViewer, void>(this, &FontStyleAssetViewer::OnStyleChanged);

        mStyle = nullptr;
    }
}
// --- META ---

DECLARE_CLASS(Editor::FontStyleAssetViewer, Editor__FontStyleAssetViewer);
// --- END META ---
