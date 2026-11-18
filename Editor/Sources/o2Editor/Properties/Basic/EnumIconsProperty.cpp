#include "o2Editor/stdafx.h"
#include "EnumIconsProperty.h"

namespace Editor
{
    EnumIconsProperty::EnumIconsProperty(RefCounter* refCounter):
        EnumProperty(refCounter)
    {
        SetValueChangeAppliedByAction(true);
    }

    EnumIconsProperty::EnumIconsProperty(RefCounter* refCounter, const EnumIconsProperty& other):
        EnumProperty(refCounter, other)
    {
        InitializeSegments();
        SetOptions(other.mOptions);
    }

    EnumIconsProperty& EnumIconsProperty::operator=(const EnumIconsProperty& other)
    {
        EnumProperty::operator=(other);
        InitializeSegments();
        SetOptions(other.mOptions);
        return *this;
    }

    void EnumIconsProperty::InitializeSegments()
    {
        mSegments = FindChildByType<SegmentedIcons>("segments");
        if (!mSegments)
            return;

        mSegments->SetMode(SegmentedIcons::Mode::Choice);
        mSegments->onClicked = [this](int index)
        {
            if (index >= 0 && index < mOptions.Count())
                SetValueByUserAndComplete(mOptions[index].value);

            UpdateValueView();
        };
    }

    void EnumIconsProperty::SetOptions(const Vector<EnumIconOption>& options)
    {
        mOptions = options;

        if (!mSegments)
            return;

        mSegments->SetSegments(mOptions.Convert<String>([](const EnumIconOption& x) { return x.icon; }));
        layout->minWidth = mSegments->layout->minWidth;
        layout->maxWidth = mSegments->layout->maxWidth;
        UpdateValueView();
        mSegments->SetSelected(mSegments->GetSelected(), false);
    }

    const Vector<EnumIconOption>& EnumIconsProperty::GetOptions() const
    {
        return mOptions;
    }

    int EnumIconsProperty::GetOptionIndex(int value) const
    {
        return mOptions.IndexOf([&](const EnumIconOption& x) { return x.value == value; });
    }

    const Ref<SegmentedIcons>& EnumIconsProperty::GetSegments() const
    {
        return mSegments;
    }

    void EnumIconsProperty::UpdateValueView()
    {
        if (!mSegments)
            return;

        bool known = !mValuesDifferent && !mValuesProxies.IsEmpty();
        mSegments->SetSelected(known ? GetOptionIndex(mCommonValue) : -1);
    }
}

DECLARE_TEMPLATE_CLASS(o2::LinkRef<Editor::EnumIconsProperty>);
// --- META ---

DECLARE_CLASS(Editor::EnumIconsProperty, Editor__EnumIconsProperty);
// --- END META ---
