#include "o2Editor/stdafx.h"
#include "RectangleIntProperty.h"

#include "o2Editor/Properties/Basic/IntegerProperty.h"

namespace Editor
{
    RectIProperty::RectIProperty(RefCounter* refCounter):
        IPropertyField(refCounter)
    {}

    RectIProperty::RectIProperty(RefCounter* refCounter, const RectIProperty& other) :
        IPropertyField(refCounter, other)
    {
        InitializeControls();
    }

    RectIProperty& RectIProperty::operator=(const RectIProperty& other)
    {
        IPropertyField::operator=(other);
        InitializeControls();
        return *this;
    }

    void RectIProperty::InitializeControls()
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

    void RectIProperty::SetValue(const RectI& value)
    {
        mLeftProperty->SetValue(value.left);
        mBottomProperty->SetValue(value.bottom);
        mRightProperty->SetValue(value.right);
        mTopProperty->SetValue(value.top);
    }

    void RectIProperty::SetValueLeft(int value)
    {
        mLeftProperty->SetValue(value);
    }

    void RectIProperty::SetValueRight(int value)
    {
        mRightProperty->SetValue(value);
    }

    void RectIProperty::SetValueTop(int value)
    {
        mTopProperty->SetValue(value);
    }

    void RectIProperty::SetValueBottom(int value)
    {
        mBottomProperty->SetValue(value);
    }

    void RectIProperty::SetUnknownValue(const RectI& defaultValue /*= RectI()*/)
    {
        mLeftProperty->SetUnknownValue(defaultValue.left);
        mRightProperty->SetUnknownValue(defaultValue.right);
        mTopProperty->SetUnknownValue(defaultValue.top);
        mBottomProperty->SetUnknownValue(defaultValue.bottom);
    }

    void RectIProperty::SetLeftUnknownValue(int defaultValue /*= 0*/)
    {
        mLeftProperty->SetUnknownValue(defaultValue);
    }

    void RectIProperty::SetRightUnknownValue(int defaultValue /*= 0*/)
    {
        mRightProperty->SetUnknownValue(defaultValue);
    }

    void RectIProperty::SetTopUnknownValue(int defaultValue /*= 0*/)
    {
        mTopProperty->SetUnknownValue(defaultValue);
    }

    void RectIProperty::SetBottomUnknownValue(int defaultValue /*= 0*/)
    {
        mBottomProperty->SetUnknownValue(defaultValue);
    }

    void RectIProperty::StoreValues(Vector<DataDocument>& data) const
    {
        data.Clear();
        for (auto& ptr : mValuesProxies)
        {
            data.Add(DataDocument());
            data.Last() = GetProxy<RectI>(ptr.first);
        }
    }

    void RectIProperty::OnPropertyBeforeChange(const Ref<IPropertyField>& field, bool byUser)
    {
        if (mIsTargetProxiesProperties && byUser)
            BeginUserChanging();
    }

    void RectIProperty::OnPropertyChanged(const Ref<IPropertyField>& field, bool byUser)
    {
        onChanged(field, byUser);
    }

    void RectIProperty::OnPropertyChangeCompleted(const String& path, const Vector<DataDocument>& before, const Vector<DataDocument>& after)
    {
        if (mIsTargetProxiesProperties)
            EndUserChanging();
        else
            onChangeCompleted(mValuesPath + "/" + path, before, after);
    }

    void RectIProperty::SetValueAndPrototypeProxy(const TargetsVec& targets)
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

    void RectIProperty::Refresh(bool forcible /*= false*/)
    {
        if (mValuesProxies.IsEmpty())
            return;

        mLeftProperty->Refresh(forcible);
        mRightProperty->Refresh(forcible);
        mTopProperty->Refresh(forcible);
        mBottomProperty->Refresh(forcible);

        CheckRevertableState();
    }

    RectI RectIProperty::GetCommonValue() const
    {
        return RectI(mLeftProperty->GetCommonValue(), mBottomProperty->GetCommonValue(),
                     mRightProperty->GetCommonValue(), mTopProperty->GetCommonValue());
    }

    bool RectIProperty::IsValuesDifferent() const
    {
        return mLeftProperty->IsValuesDifferent() || mRightProperty->IsValuesDifferent() ||
            mTopProperty->IsValuesDifferent() || mBottomProperty->IsValuesDifferent();
    }

    const Type* RectIProperty::GetValueType() const
    {
        return GetValueTypeStatic();
    }

    const Type* RectIProperty::GetValueTypeStatic()
    {
        return &TypeOf(RectI);
    }

    RectIProperty::LeftValueProxy::LeftValueProxy(const Ref<IAbstractValueProxy>& proxy) :mProxy(proxy)
    {}

    RectIProperty::LeftValueProxy::LeftValueProxy()
    {}

    void RectIProperty::LeftValueProxy::SetValue(const int& value)
    {
        RectI proxyValue;
        mProxy->GetValuePtr(&proxyValue);
        proxyValue.left = value;
        mProxy->SetValuePtr(&proxyValue);
    }

    int RectIProperty::LeftValueProxy::GetValue() const
    {
        RectI proxyValue;
        mProxy->GetValuePtr(&proxyValue);
        return proxyValue.left;
    }

    RectIProperty::RightValueProxy::RightValueProxy(const Ref<IAbstractValueProxy>& proxy) :mProxy(proxy)
    {}

    RectIProperty::RightValueProxy::RightValueProxy()
    {}

    void RectIProperty::RightValueProxy::SetValue(const int& value)
    {
        RectI proxyValue;
        mProxy->GetValuePtr(&proxyValue);
        proxyValue.right = value;
        mProxy->SetValuePtr(&proxyValue);
    }

    int RectIProperty::RightValueProxy::GetValue() const
    {
        RectI proxyValue;
        mProxy->GetValuePtr(&proxyValue);
        return proxyValue.right;
    }

    RectIProperty::TopValueProxy::TopValueProxy(const Ref<IAbstractValueProxy>& proxy) :mProxy(proxy)
    {}

    RectIProperty::TopValueProxy::TopValueProxy()
    {}

    void RectIProperty::TopValueProxy::SetValue(const int& value)
    {
        RectI proxyValue;
        mProxy->GetValuePtr(&proxyValue);
        proxyValue.top = value;
        mProxy->SetValuePtr(&proxyValue);
    }

    int RectIProperty::TopValueProxy::GetValue() const
    {
        RectI proxyValue;
        mProxy->GetValuePtr(&proxyValue);
        return proxyValue.top;
    }

    RectIProperty::BottomValueProxy::BottomValueProxy(const Ref<IAbstractValueProxy>& proxy) :mProxy(proxy)
    {}

    RectIProperty::BottomValueProxy::BottomValueProxy()
    {}

    void RectIProperty::BottomValueProxy::SetValue(const int& value)
    {
        RectI proxyValue;
        mProxy->GetValuePtr(&proxyValue);
        proxyValue.bottom = value;
        mProxy->SetValuePtr(&proxyValue);
    }

    int RectIProperty::BottomValueProxy::GetValue() const
    {
        RectI proxyValue;
        mProxy->GetValuePtr(&proxyValue);
        return proxyValue.bottom;
    }

}

DECLARE_TEMPLATE_CLASS(Editor::TPropertyField<o2::RectI>);
DECLARE_TEMPLATE_CLASS(o2::LinkRef<Editor::RectIProperty>);
DECLARE_TEMPLATE_CLASS(o2::LinkRef<Editor::TPropertyField<o2::RectI>>);
// --- META ---

DECLARE_CLASS(Editor::RectIProperty, Editor__RectIProperty);
// --- END META ---
