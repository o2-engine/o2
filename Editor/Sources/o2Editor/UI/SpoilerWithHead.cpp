#include "o2Editor/stdafx.h"
#include "SpoilerWithHead.h"

#include "o2/Assets/Assets.h"
#include "o2/Scene/UI/Widgets/Button.h"
#include "o2/Scene/UI/Widgets/Image.h"
#include "o2/Utils/Editor/EditorScope.h"
#include "o2Editor/UI/Style/EditorTheme.h"

namespace Editor
{

    SpoilerWithHead::SpoilerWithHead(RefCounter* refCounter) :
        Spoiler(refCounter)
    {}

    SpoilerWithHead::SpoilerWithHead(RefCounter* refCounter, const SpoilerWithHead& other) :
        Spoiler(refCounter, other)
    {
        InitializeControls();
        RetargetStatesAnimations();
    }

    SpoilerWithHead& SpoilerWithHead::operator=(const SpoilerWithHead& other)
    {
        Spoiler::operator=(other);
        InitializeControls();
        return *this;
    }

    const Ref<Image>& SpoilerWithHead::GetIcon() const
    {
        return mIcon;
    }

    void SpoilerWithHead::SetHeadIcon(const String& iconPath)
    {
        if (!mIcon || iconPath.IsEmpty())
            return;

        PushEditorScopeOnStack scope;

        String whiteTwin = iconPath.EndsWith(".svg") ? iconPath.SubStr(0, iconPath.Length() - 4) + "_white.svg" : String();
        mIcon->SetImageName(!whiteTwin.IsEmpty() && o2Assets.IsAssetExist(whiteTwin) ? whiteTwin : iconPath);

        if (auto drawable = mIcon->GetImageDrawable())
            drawable->SetColor(EditorTheme::Get().textOnHeader);
    }

    String SpoilerWithHead::GetCreateMenuCategory()
    {
        return "UI/Editor";
    }

    void SpoilerWithHead::InitializeControls()
    {
        mOptionsBtn = FindInternalWidgetByType<Button>("options");
        if (mOptionsBtn)
            mOptionsBtn->onClick = [&]() { onOptionsPressed(); };

        mSaveBtn = FindInternalWidgetByType<Button>("save");
        if (mSaveBtn)
            mSaveBtn->onClick = [&]() { onSavePressed(); };

        mIcon = FindInternalWidgetByType<Image>("icon");
    }
}

DECLARE_TEMPLATE_CLASS(o2::LinkRef<Editor::SpoilerWithHead>);
// --- META ---

DECLARE_CLASS(Editor::SpoilerWithHead, Editor__SpoilerWithHead);
// --- END META ---
