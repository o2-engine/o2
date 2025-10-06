#include "o2Editor/stdafx.h"
#include "RectangleFloatProperty.h"

#include "o2Editor/Properties/Basic/FloatProperty.h"

namespace Editor
{
    RectFProperty::RectFProperty(RefCounter* refCounter):
        IPropertyField(refCounter)
    {}

    RectFProperty::RectFProperty(RefCounter* refCounter, const RectFProperty& other) :
        IPropertyField(refCounter, other)
    {
        InitializeControls();
    }

    RectFProperty& RectFProperty::operator=(const RectFProperty& other)
    {
        IPropertyField::operator=(other);
        InitializeControls();
        return *this;
    }

    void RectFProperty::InitializeControls()
    {
        mLeftProperty = GetChildByType<FloatProperty>("container/layout/properties/left");
        mLeftProperty->SetValuePath("left");
        mLeftProperty->onBeforeChange = THIS_FUNC(OnPropertyBeforeChange);
        mLeftProperty->onChanged = THIS_FUNC(OnPropertyChanged);
        mLeftProperty->onChangeCompleted = THIS_FUNC(OnPropertyChangeCompleted);

        mBottomProperty = GetChildByType<FloatProperty>("container/layout/properties/bottom");
        mBottomProperty->SetValuePath("bottom");
        mBottomProperty->onBeforeChange = THIS_FUNC(OnPropertyBeforeChange);
        mBottomProperty->onChanged = THIS_FUNC(OnPropertyChanged);
        mBottomProperty->onChangeCompleted = THIS_FUNC(OnPropertyChangeCompleted);

        mRightProperty = GetChildByType<FloatProperty>("container/layout/properties/right");
        mRightProperty->SetValuePath("right");
        mRightProperty->onBeforeChange = THIS_FUNC(OnPropertyBeforeChange);
        mRightProperty->onChanged = THIS_FUNC(OnPropertyChanged);
        mRightProperty->onChangeCompleted = THIS_FUNC(OnPropertyChangeCompleted);

        mTopProperty = GetChildByType<FloatProperty>("container/layout/properties/top");
        mTopProperty->SetValuePath("top");
        mTopProperty->onBeforeChange = THIS_FUNC(OnPropertyBeforeChange);
        mTopProperty->onChanged = THIS_FUNC(OnPropertyChanged);
        mTopProperty->onChangeCompleted = THIS_FUNC(OnPropertyChangeCompleted);
    }

    void RectFProperty::SetValue(const RectF& value)
    {
        mLeftProperty->SetValue(value.left);
        mBottomProperty->SetValue(value.bottom);
        mRightProperty->SetValue(value.right);
        mTopProperty->SetValue(value.top);
    }

    void RectFProperty::SetValueLeft(float value)
    {
        mLeftProperty->SetValue(value);
    }

    void RectFProperty::SetValueRight(float value)
    {
        mRightProperty->SetValue(value);
    }

    void RectFProperty::SetValueTop(float value)
    {
        mTopProperty->SetValue(value);
    }

    void RectFProperty::SetValueBottom(float value)
    {
        mBottomProperty->SetValue(value);
    }

    void RectFProperty::SetUnknownValue(const RectF& defaultValue /*= RectF()*/)
    {
        mLeftProperty->SetUnknownValue(defaultValue.left);
        mRightProperty->SetUnknownValue(defaultValue.right);
        mTopProperty->SetUnknownValue(defaultValue.top);
        mBottomProperty->SetUnknownValue(defaultValue.bottom);
    }

    void RectFProperty::SetLeftUnknownValue(float defaultValue /*= 0.0f*/)
    {
        mLeftProperty->SetUnknownValue(defaultValue);
    }

    void RectFProperty::SetRightUnknownValue(float defaultValue /*= 0.0f*/)
    {
        mRightProperty->SetUnknownValue(defaultValue);
    }

    void RectFProperty::SetTopUnknownValue(float defaultValue /*= 0.0f*/)
    {
        mTopProperty->SetUnknownValue(defaultValue);
    }

    void RectFProperty::SetBottomUnknownValue(float defaultValue /*= 0.0f*/)
    {
        mBottomProperty->SetUnknownValue(defaultValue);
    }

    void RectFProperty::StoreValues(Vector<DataDocument>& data) const
    {
        data.Clear();
        for (auto& ptr : mValuesProxies)
        {
            data.Add(DataDocument());
            data.Last() = GetProxy<RectF>(ptr.first);
        }
    }

    void RectFProperty::OnPropertyBeforeChange(const Ref<IPropertyField>& field, bool byUser)
    {
        if (mIsTargetProxiesProperties && byUser)
            BeginUserChanging();
    }

    void RectFProperty::OnPropertyChanged(const Ref<IPropertyField>& field, bool byUser)
    {
        onChanged(field, byUser);
    }

    void RectFProperty::OnPropertyChangeCompleted(const String& path, const Vector<DataDocument>& before, const Vector<DataDocument>& after)
    {
        if (mIsTargetProxiesProperties)
            EndUserChanging();
        else
            onChangeCompleted(mValuesPath + "/" + path, before, after);
    }

    void RectFProperty::SetValueAndPrototypeProxy(const TargetsVec& targets)
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

    void RectFProperty::Refresh(bool forcible /*= false*/)
    {
        if (mValuesProxies.IsEmpty())
            return;

        mLeftProperty->Refresh(forcible);
        mRightProperty->Refresh(forcible);
        mTopProperty->Refresh(forcible);
        mBottomProperty->Refresh(forcible);

        CheckRevertableState();
    }

    RectF RectFProperty::GetCommonValue() const
    {
        return RectF(mLeftProperty->GetCommonValue(), mBottomProperty->GetCommonValue(),
                     mRightProperty->GetCommonValue(), mTopProperty->GetCommonValue());
    }

    bool RectFProperty::IsValuesDifferent() const
    {
        return mLeftProperty->IsValuesDifferent() || mRightProperty->IsValuesDifferent() ||
            mTopProperty->IsValuesDifferent() || mBottomProperty->IsValuesDifferent();
    }

    const Type* RectFProperty::GetValueType() const
    {
        return GetValueTypeStatic();
    }

    const Type* RectFProperty::GetValueTypeStatic()
    {
        return &TypeOf(RectF);
    }

    RectFProperty::LeftValueProxy::LeftValueProxy(const Ref<IAbstractValueProxy>& proxy) :mProxy(proxy)
    {}

    RectFProperty::LeftValueProxy::LeftValueProxy()
    {}

    void RectFProperty::LeftValueProxy::SetValue(const float& value)
    {
        RectF proxyValue;
        mProxy->GetValuePtr(&proxyValue);
        proxyValue.left = value;
        mProxy->SetValuePtr(&proxyValue);
    }

    float RectFProperty::LeftValueProxy::GetValue() const
    {
        RectF proxyValue;
        mProxy->GetValuePtr(&proxyValue);
        return proxyValue.left;
    }

    RectFProperty::RightValueProxy::RightValueProxy(const Ref<IAbstractValueProxy>& proxy) :mProxy(proxy)
    {}

    RectFProperty::RightValueProxy::RightValueProxy()
    {}

    void RectFProperty::RightValueProxy::SetValue(const float& value)
    {
        RectF proxyValue;
        mProxy->GetValuePtr(&proxyValue);
        proxyValue.right = value;
        mProxy->SetValuePtr(&proxyValue);
    }

    float RectFProperty::RightValueProxy::GetValue() const
    {
        RectF proxyValue;
        mProxy->GetValuePtr(&proxyValue);
        return proxyValue.right;
    }

    RectFProperty::TopValueProxy::TopValueProxy(const Ref<IAbstractValueProxy>& proxy) :mProxy(proxy)
    {}

    RectFProperty::TopValueProxy::TopValueProxy()
    {}

    void RectFProperty::TopValueProxy::SetValue(const float& value)
    {
        RectF proxyValue;
        mProxy->GetValuePtr(&proxyValue);
        proxyValue.top = value;
        mProxy->SetValuePtr(&proxyValue);
    }

    float RectFProperty::TopValueProxy::GetValue() const
    {
        RectF proxyValue;
        mProxy->GetValuePtr(&proxyValue);
        return proxyValue.top;
    }

    RectFProperty::BottomValueProxy::BottomValueProxy(const Ref<IAbstractValueProxy>& proxy) :mProxy(proxy)
    {}

    RectFProperty::BottomValueProxy::BottomValueProxy()
    {}

    void RectFProperty::BottomValueProxy::SetValue(const float& value)
    {
        RectF proxyValue;
        mProxy->GetValuePtr(&proxyValue);
        proxyValue.bottom = value;
        mProxy->SetValuePtr(&proxyValue);
    }

    float RectFProperty::BottomValueProxy::GetValue() const
    {
        RectF proxyValue;
        mProxy->GetValuePtr(&proxyValue);
        return proxyValue.bottom;
    }
}

DECLARE_TEMPLATE_CLASS(Editor::TPropertyField<o2::RectF>);
DECLARE_TEMPLATE_CLASS(o2::LinkRef<Editor::RectFProperty>);
DECLARE_TEMPLATE_CLASS(o2::LinkRef<Editor::TPropertyField<o2::RectF>>);
// --- META ---

DECLARE_CLASS(Editor::RectFProperty, Editor__RectFProperty);
// --- END META ---
