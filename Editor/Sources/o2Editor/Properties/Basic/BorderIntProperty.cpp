#include "o2Editor/stdafx.h"
#include "BorderIntProperty.h"

#include "o2Editor/Properties/Basic/IntegerProperty.h"

namespace Editor
{
    BorderIProperty::BorderIProperty(RefCounter* refCounter):
        IPropertyField(refCounter)
    {}

    BorderIProperty::BorderIProperty(RefCounter* refCounter, const BorderIProperty& other) :
        IPropertyField(refCounter, other)
    {
        InitializeControls();
    }

    BorderIProperty& BorderIProperty::operator=(const BorderIProperty& other)
    {
        IPropertyField::operator=(other);
        InitializeControls();
        return *this;
    }

    void BorderIProperty::InitializeControls()
    {
        mLeftProperty = GetChildByType<IntegerProperty>("container/layout/properties/left");
        mLeftProperty->SetValuePath("left");
        mLeftProperty->onBeforeChange = THIS_FUNC(OnPropertyBeforeChange);
        mLeftProperty->onChanged = THIS_FUNC(OnPropertyChanged);
        mLeftProperty->onChangeCompleted = THIS_FUNC(OnPropertyChangeCompleted);

        mBottomProperty = GetChildByType<IntegerProperty>("container/layout/properties/bottom");
        mBottomProperty->SetValuePath("bottom");
        mBottomProperty->onBeforeChange = THIS_FUNC(OnPropertyBeforeChange);
        mBottomProperty->onChanged = THIS_FUNC(OnPropertyChanged);
        mBottomProperty->onChangeCompleted = THIS_FUNC(OnPropertyChangeCompleted);

        mRightProperty = GetChildByType<IntegerProperty>("container/layout/properties/right");
        mRightProperty->SetValuePath("right");
        mRightProperty->onBeforeChange = THIS_FUNC(OnPropertyBeforeChange);
        mRightProperty->onChanged = THIS_FUNC(OnPropertyChanged);
        mRightProperty->onChangeCompleted = THIS_FUNC(OnPropertyChangeCompleted);

        mTopProperty = GetChildByType<IntegerProperty>("container/layout/properties/top");
        mTopProperty->SetValuePath("top");
        mTopProperty->onBeforeChange = THIS_FUNC(OnPropertyBeforeChange);
        mTopProperty->onChanged = THIS_FUNC(OnPropertyChanged);
        mTopProperty->onChangeCompleted = THIS_FUNC(OnPropertyChangeCompleted);
    }

    void BorderIProperty::SetValue(const BorderI& value)
    {
        mLeftProperty->SetValue(value.left, true);
        mBottomProperty->SetValue(value.bottom, true);
        mRightProperty->SetValue(value.right, true);
        mTopProperty->SetValue(value.top, true);
    }

    void BorderIProperty::SetValueLeft(int value)
    {
        mLeftProperty->SetValue(value, true);
    }

    void BorderIProperty::SetValueRight(int value)
    {
        mRightProperty->SetValue(value, true);
    }

    void BorderIProperty::SetValueTop(int value)
    {
        mTopProperty->SetValue(value, true);
    }

    void BorderIProperty::SetValueBottom(int value)
    {
        mBottomProperty->SetValue(value, true);
    }

    void BorderIProperty::SetUnknownValue(const BorderI& defaultValue /*= BorderI()*/)
    {
        mLeftProperty->SetUnknownValue(defaultValue.left);
        mRightProperty->SetUnknownValue(defaultValue.right);
        mTopProperty->SetUnknownValue(defaultValue.top);
        mBottomProperty->SetUnknownValue(defaultValue.bottom);
    }

    void BorderIProperty::SetLeftUnknownValue(int defaultValue /*= 0*/)
    {
        mLeftProperty->SetUnknownValue(defaultValue);
    }

    void BorderIProperty::SetRightUnknownValue(int defaultValue /*= 0*/)
    {
        mRightProperty->SetUnknownValue(defaultValue);
    }

    void BorderIProperty::SetTopUnknownValue(int defaultValue /*= 0*/)
    {
        mTopProperty->SetUnknownValue(defaultValue);
    }

    void BorderIProperty::SetBottomUnknownValue(int defaultValue /*= 0*/)
    {
        mBottomProperty->SetUnknownValue(defaultValue);
    }

    void BorderIProperty::StoreValues(Vector<DataDocument>& data) const
    {
        data.Clear();
        for (auto& ptr : mValuesProxies)
        {
            data.Add(DataDocument());
            data.Last() = GetProxy<BorderI>(ptr.first);
        }
    }

    void BorderIProperty::OnPropertyBeforeChange(const Ref<IPropertyField>& field, bool byUser)
    {
        if (mIsTargetProxiesProperties && byUser)
            BeginUserChanging();
    }

    void BorderIProperty::OnPropertyChanged(const Ref<IPropertyField>& field, bool byUser)
    {
        onChanged(field, byUser);
    }

    void BorderIProperty::OnPropertyChangeCompleted(const String& path, const Vector<DataDocument>& before, const Vector<DataDocument>& after)
    {
        if (mIsTargetProxiesProperties)
            EndUserChanging();
        else
            onChangeCompleted(mValuesPath + "/" + path, before, after);
    }

    void BorderIProperty::SetValueAndPrototypeProxy(const TargetsVec& targets)
    {
        mValuesProxies = targets;

        mIsTargetProxiesProperties = targets.IsEmpty() ? false : dynamic_cast<IPropertyValueProxy*>(targets[0].first.Get()) != nullptr;

        mLeftProperty->SetValueAndPrototypeProxy(targets.Convert<TargetPair>([](const TargetPair& x) {
            return TargetPair(mmake<LeftValueProxy>(x.first), x.second ? mmake<LeftValueProxy>(x.second) : nullptr); }));

        mRightProperty->SetValueAndPrototypeProxy(targets.Convert<TargetPair>([](const TargetPair& x) {
            return TargetPair(mmake<RightValueProxy>(x.first), x.second ? mmake<RightValueProxy>(x.second) : nullptr); }));

        mTopProperty->SetValueAndPrototypeProxy(targets.Convert<TargetPair>([](const TargetPair& x) {
            return TargetPair(mmake<TopValueProxy>(x.first), x.second ? mmake<TopValueProxy>(x.second) : nullptr); }));

        mBottomProperty->SetValueAndPrototypeProxy(targets.Convert<TargetPair>([](const TargetPair& x) {
            return TargetPair(mmake<BottomValueProxy>(x.first), x.second ? mmake<BottomValueProxy>(x.second) : nullptr); }));
    }

    void BorderIProperty::Refresh(bool forcible /*= false*/)
    {
        if (mValuesProxies.IsEmpty())
            return;

        mLeftProperty->Refresh(forcible);
        mRightProperty->Refresh(forcible);
        mTopProperty->Refresh(forcible);
        mBottomProperty->Refresh(forcible);

        CheckRevertableState();
    }

    BorderI BorderIProperty::GetCommonValue() const
    {
        return BorderI(mLeftProperty->GetCommonValue(), mBottomProperty->GetCommonValue(),
                       mRightProperty->GetCommonValue(), mTopProperty->GetCommonValue());
    }

    bool BorderIProperty::IsValuesDifferent() const
    {
        return mLeftProperty->IsValuesDifferent() || mRightProperty->IsValuesDifferent() ||
            mTopProperty->IsValuesDifferent() || mBottomProperty->IsValuesDifferent();
    }

    const Type* BorderIProperty::GetValueType() const
    {
        return GetValueTypeStatic();
    }

    const Type* BorderIProperty::GetValueTypeStatic()
    {
        return &TypeOf(BorderI);
    }

    BorderIProperty::LeftValueProxy::LeftValueProxy(const Ref<IAbstractValueProxy>& proxy) :mProxy(proxy)
    {}

    BorderIProperty::LeftValueProxy::LeftValueProxy()
    {}

    void BorderIProperty::LeftValueProxy::SetValue(const int& value)
    {
        BorderI proxyValue;
        mProxy->GetValuePtr(&proxyValue);
        proxyValue.left = value;
        mProxy->SetValuePtr(&proxyValue);
    }

    int BorderIProperty::LeftValueProxy::GetValue() const
    {
        BorderI proxyValue;
        mProxy->GetValuePtr(&proxyValue);
        return proxyValue.left;
    }

    BorderIProperty::RightValueProxy::RightValueProxy(const Ref<IAbstractValueProxy>& proxy) :mProxy(proxy)
    {}

    BorderIProperty::RightValueProxy::RightValueProxy()
    {}

    void BorderIProperty::RightValueProxy::SetValue(const int& value)
    {
        BorderI proxyValue;
        mProxy->GetValuePtr(&proxyValue);
        proxyValue.right = value;
        mProxy->SetValuePtr(&proxyValue);
    }

    int BorderIProperty::RightValueProxy::GetValue() const
    {
        BorderI proxyValue;
        mProxy->GetValuePtr(&proxyValue);
        return proxyValue.right;
    }

    BorderIProperty::TopValueProxy::TopValueProxy(const Ref<IAbstractValueProxy>& proxy) :mProxy(proxy)
    {}

    BorderIProperty::TopValueProxy::TopValueProxy()
    {}

    void BorderIProperty::TopValueProxy::SetValue(const int& value)
    {
        BorderI proxyValue;
        mProxy->GetValuePtr(&proxyValue);
        proxyValue.top = value;
        mProxy->SetValuePtr(&proxyValue);
    }

    int BorderIProperty::TopValueProxy::GetValue() const
    {
        BorderI proxyValue;
        mProxy->GetValuePtr(&proxyValue);
        return proxyValue.top;
    }

    BorderIProperty::BottomValueProxy::BottomValueProxy(const Ref<IAbstractValueProxy>& proxy) :mProxy(proxy)
    {}

    BorderIProperty::BottomValueProxy::BottomValueProxy()
    {}

    void BorderIProperty::BottomValueProxy::SetValue(const int& value)
    {
        BorderI proxyValue;
        mProxy->GetValuePtr(&proxyValue);
        proxyValue.bottom = value;
        mProxy->SetValuePtr(&proxyValue);
    }

    int BorderIProperty::BottomValueProxy::GetValue() const
    {
        BorderI proxyValue;
        mProxy->GetValuePtr(&proxyValue);
        return proxyValue.bottom;
    }

}

DECLARE_TEMPLATE_CLASS(Editor::TPropertyField<o2::BorderI>);
DECLARE_TEMPLATE_CLASS(o2::LinkRef<Editor::BorderIProperty>);
DECLARE_TEMPLATE_CLASS(o2::LinkRef<Editor::TPropertyField<o2::BorderI>>);
// --- META ---

DECLARE_CLASS(Editor::BorderIProperty, Editor__BorderIProperty);
// --- END META ---
