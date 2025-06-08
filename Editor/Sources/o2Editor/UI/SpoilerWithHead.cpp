#include "o2Editor/stdafx.h"
#include "SpoilerWithHead.h"

#include "o2/Scene/UI/Widgets/Button.h"

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
