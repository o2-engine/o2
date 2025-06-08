#include "o2Editor/stdafx.h"
#include "TagProperty.h"

#include "o2/Scene/UI/UIManager.h"
#include "o2/Scene/UI/WidgetLayout.h"
#include "o2/Scene/UI/Widgets/EditBox.h"

namespace Editor
{
    TagsProperty::TagsProperty(RefCounter* refCounter):
        TPropertyField<TagGroup>(refCounter)
    {}

    TagsProperty::TagsProperty(RefCounter* refCounter, const TagsProperty& other) :
        TPropertyField<TagGroup>(refCounter, other)
    {
        InitializeControls();
    }

    TagsProperty& TagsProperty::operator=(const TagsProperty& other)
    {
        TPropertyField<TagGroup>::operator=(other);
        InitializeControls();
        return *this;
    }

    void TagsProperty::InitializeControls()
    {
        mEditBox = FindChildByType<EditBox>();
        if (mEditBox)
        {
            mTagsContext = FindChildByType<ContextMenu>();
            if (!mTagsContext)
            {
                mTagsContext = o2UI.CreateWidget<ContextMenu>();
                AddChild(mTagsContext);
            }

            mTagsContext->SetMaxItemsVisible(10);
            mEditBox->AddChild(mTagsContext);

            mEditBox->onFocused = [&]() { UpdateContextData(""); };
            mEditBox->onChangeCompleted = THIS_FUNC(OnEditBoxChangeCompleted);
            mEditBox->onChanged = THIS_FUNC(OnEditBoxChanged);
            mEditBox->text = "--";
        }
    }

    void TagsProperty::UpdateValueView()
    {}

    void TagsProperty::UpdateContextData(const WString& filter)
    {
        if (mPushingTag)
            return;

        mTagsContext->RemoveAllItems();
        mTagsContext->Show(mEditBox->layout->GetWorldLeftBottom());

        for (auto& tag : o2Scene.GetTags())
        {
            if (filter.IsEmpty() || tag->GetName().CountOf(filter) > 0)
                mTagsContext->AddItem(tag->GetName(), [=]() { PushTag(tag->GetName()); });
        }
    }

    void TagsProperty::SetCommonValue(const TagGroup& value)
    {
        mCommonValue = value;
        mValuesDifferent = false;

        WString res;
        for (auto& tag : mCommonValue.GetTagsNames())
            res += tag + " ";

        mPushingTag = true;
        mEditBox->text = res;
        mPushingTag = false;

        OnValueChanged();
    }

    void TagsProperty::OnEditBoxChanged(const WString& text)
    {
        if (mPushingTag || !mEditBox->IsFocused())
            return;

        WString lastTagText;

        int spacePos = text.FindLast(" ");
        if (spacePos < 0)
            lastTagText = text;
        else
            lastTagText = text.SubStr(spacePos + 1);

        UpdateContextData(lastTagText);
    }

    void TagsProperty::OnEditBoxChangeCompleted(const WString& text)
    {
        SetTags(text);
    }

    void TagsProperty::SetTags(const WString &text)
    {
        auto tagsNames = text.Split(" ");

        TagGroup tagsValue;

        for (auto& tagName : tagsNames)
        {
            if (!tagName.IsEmpty())
                tagsValue.AddTag(tagName);
        }

        SetValueByUser(tagsValue);
    }

    void TagsProperty::PushTag(String name)
    {
        String editText = (WString)mEditBox->text;

        int spaceIdx = editText.FindLast(" ");
        if (spaceIdx >= 0)
            editText = editText.SubStr(0, spaceIdx);
        else
            editText = "";

        if (!editText.IsEmpty())
            editText += " ";

        editText += name;

        SetTags(editText);
    }
}

DECLARE_TEMPLATE_CLASS(Editor::TPropertyField<o2::TagGroup>);
DECLARE_TEMPLATE_CLASS(o2::LinkRef<Editor::TagsProperty>);
DECLARE_TEMPLATE_CLASS(o2::LinkRef<Editor::TPropertyField<o2::TagGroup>>);
// --- META ---

DECLARE_CLASS(Editor::TagsProperty, Editor__TagsProperty);
// --- END META ---
