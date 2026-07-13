#include "o2Editor/stdafx.h"
#include "Vector3FloatProperty.h"

#include "o2Editor/Properties/Basic/FloatProperty.h"

namespace Editor
{
    Vec3FProperty::Vec3FProperty(RefCounter* refCounter):
        IPropertyField(refCounter)
    {}

    Vec3FProperty::Vec3FProperty(RefCounter* refCounter, const Vec3FProperty& other) :
        IPropertyField(refCounter, other)
    {
        InitializeControls();
    }

    Vec3FProperty& Vec3FProperty::operator=(const Vec3FProperty& other)
    {
        IPropertyField::operator=(other);
        InitializeControls();
        return *this;
    }

    void Vec3FProperty::InitializeControls()
    {
        mXProperty = GetChildByType<FloatProperty>("container/layout/properties/x");
        mXProperty->SetValuePath("x");
        mXProperty->SetValueChangeAppliedByAction(false);
        mXProperty->onBeforeChange = THIS_FUNC(OnPropertyBeforeChange);
        mXProperty->onChanged = THIS_FUNC(OnPropertyChanged);
        mXProperty->onChangeCompleted = THIS_FUNC(OnPropertyChangeCompleted);

        mYProperty = GetChildByType<FloatProperty>("container/layout/properties/y");
        mYProperty->SetValuePath("y");
        mYProperty->SetValueChangeAppliedByAction(false);
        mYProperty->onBeforeChange = THIS_FUNC(OnPropertyBeforeChange);
        mYProperty->onChanged = THIS_FUNC(OnPropertyChanged);
        mYProperty->onChangeCompleted = THIS_FUNC(OnPropertyChangeCompleted);

        mZProperty = GetChildByType<FloatProperty>("container/layout/properties/z");
        mZProperty->SetValuePath("z");
        mZProperty->SetValueChangeAppliedByAction(false);
        mZProperty->onBeforeChange = THIS_FUNC(OnPropertyBeforeChange);
        mZProperty->onChanged = THIS_FUNC(OnPropertyChanged);
        mZProperty->onChangeCompleted = THIS_FUNC(OnPropertyChangeCompleted);
    }

    void Vec3FProperty::StoreValues(Vector<DataDocument>& data) const
    {
        data.Clear();
        for (auto& ptr : mValuesProxies)
        {
            data.Add(DataDocument());
            data.Last() = GetProxy<Vec3F>(ptr.first);
        }
    }

    void Vec3FProperty::OnPropertyBeforeChange(const Ref<IPropertyField>& field, bool byUser)
    {
        if (mIsTargetProxiesProperties && byUser)
            BeginUserChanging();
    }

    void Vec3FProperty::OnPropertyChanged(const Ref<IPropertyField>& field, bool byUser)
    {
        onChanged(field, byUser);
    }

    void Vec3FProperty::OnPropertyChangeCompleted(const String& path, const Vector<DataDocument>& before, const Vector<DataDocument>& after)
    {
        if (mIsTargetProxiesProperties)
            EndUserChanging();
        else
            onChangeCompleted(mValuesPath + "/" + path, before, after);
    }

    void Vec3FProperty::SetValue(const Vec3F& value)
    {
        mXProperty->SetValue(value.x);
        mYProperty->SetValue(value.y);
        mZProperty->SetValue(value.z);
    }

    void Vec3FProperty::SetValueX(float value)
    {
        mXProperty->SetValue(value);
    }

    void Vec3FProperty::SetValueY(float value)
    {
        mYProperty->SetValue(value);
    }

    void Vec3FProperty::SetValueZ(float value)
    {
        mZProperty->SetValue(value);
    }

    void Vec3FProperty::SetUnknownValue(const Vec3F& defaultValue /*= Vec3F()*/)
    {
        mXProperty->SetUnknownValue(defaultValue.x);
        mYProperty->SetUnknownValue(defaultValue.y);
        mZProperty->SetUnknownValue(defaultValue.z);
    }

    void Vec3FProperty::SetXUnknownValue(float defaultValue /*= 0.0f*/)
    {
        mXProperty->SetUnknownValue(defaultValue);
    }

    void Vec3FProperty::SetYUnknownValue(float defaultValue /*= 0.0f*/)
    {
        mYProperty->SetUnknownValue(defaultValue);
    }

    void Vec3FProperty::SetZUnknownValue(float defaultValue /*= 0.0f*/)
    {
        mZProperty->SetUnknownValue(defaultValue);
    }

    void Vec3FProperty::SetValueAndPrototypeProxy(const TargetsVec& targets)
    {
        mValuesProxies = targets;

        mIsTargetProxiesProperties = targets.IsEmpty() ? false : dynamic_cast<IPropertyValueProxy*>(targets[0].first.Get()) != nullptr;

        mXProperty->SetValueAndPrototypeProxy(targets.Convert<TargetPair>([](const TargetPair& x) {
            return TargetPair(mmake<XValueProxy>(x.first), x.second ? mmake<XValueProxy>(x.second) : nullptr); }));

        mYProperty->SetValueAndPrototypeProxy(targets.Convert<TargetPair>([](const TargetPair& x) {
            return TargetPair(mmake<YValueProxy>(x.first), x.second ? mmake<YValueProxy>(x.second) : nullptr); }));

        mZProperty->SetValueAndPrototypeProxy(targets.Convert<TargetPair>([](const TargetPair& x) {
            return TargetPair(mmake<ZValueProxy>(x.first), x.second ? mmake<ZValueProxy>(x.second) : nullptr); }));
    }

    void Vec3FProperty::Refresh(bool forcible /*= false*/)
    {
        if (mValuesProxies.IsEmpty())
            return;

        mXProperty->Refresh(forcible);
        mYProperty->Refresh(forcible);
        mZProperty->Refresh(forcible);

        CheckRevertableState();
    }

    void Vec3FProperty::Revert()
    {
        if (mValuesProxies.IsEmpty())
            return;

        mXProperty->Refresh();
        mYProperty->Refresh();
        mZProperty->Refresh();

        CheckRevertableState();
    }

    Vec3F Vec3FProperty::GetCommonValue() const
    {
        return Vec3F(mXProperty->GetCommonValue(), mYProperty->GetCommonValue(), mZProperty->GetCommonValue());
    }

    bool Vec3FProperty::IsValuesDifferent() const
    {
        return mXProperty->IsValuesDifferent() || mYProperty->IsValuesDifferent() || mZProperty->IsValuesDifferent();
    }

    const Ref<FloatProperty>& Vec3FProperty::GetXProperty() const
    {
        return mXProperty;
    }

    const Ref<FloatProperty>& Vec3FProperty::GetYProperty() const
    {
        return mYProperty;
    }

    const Ref<FloatProperty>& Vec3FProperty::GetZProperty() const
    {
        return mZProperty;
    }

    const Type* Vec3FProperty::GetValueType() const
    {
        return GetValueTypeStatic();
    }

    const Type* Vec3FProperty::GetValueTypeStatic()
    {
        return &TypeOf(Vec3F);
    }

    Vec3FProperty::XValueProxy::XValueProxy(const Ref<IAbstractValueProxy>& proxy):
        mProxy(proxy)
    {}

    Vec3FProperty::XValueProxy::XValueProxy()
    {}

    void Vec3FProperty::XValueProxy::SetValue(const float& value)
    {
        Vec3F proxyValue;
        mProxy->GetValuePtr(&proxyValue);
        proxyValue.x = value;
        mProxy->SetValuePtr(&proxyValue);
    }

    float Vec3FProperty::XValueProxy::GetValue() const
    {
        Vec3F proxyValue;
        mProxy->GetValuePtr(&proxyValue);
        return proxyValue.x;
    }

    Vec3FProperty::YValueProxy::YValueProxy(const Ref<IAbstractValueProxy>& proxy):
        mProxy(proxy)
    {}

    Vec3FProperty::YValueProxy::YValueProxy()
    {}

    void Vec3FProperty::YValueProxy::SetValue(const float& value)
    {
        Vec3F proxyValue;
        mProxy->GetValuePtr(&proxyValue);
        proxyValue.y = value;
        mProxy->SetValuePtr(&proxyValue);
    }

    float Vec3FProperty::YValueProxy::GetValue() const
    {
        Vec3F proxyValue;
        mProxy->GetValuePtr(&proxyValue);
        return proxyValue.y;
    }

    Vec3FProperty::ZValueProxy::ZValueProxy(const Ref<IAbstractValueProxy>& proxy):
        mProxy(proxy)
    {}

    Vec3FProperty::ZValueProxy::ZValueProxy()
    {}

    void Vec3FProperty::ZValueProxy::SetValue(const float& value)
    {
        Vec3F proxyValue;
        mProxy->GetValuePtr(&proxyValue);
        proxyValue.z = value;
        mProxy->SetValuePtr(&proxyValue);
    }

    float Vec3FProperty::ZValueProxy::GetValue() const
    {
        Vec3F proxyValue;
        mProxy->GetValuePtr(&proxyValue);
        return proxyValue.z;
    }
}

DECLARE_TEMPLATE_CLASS(Editor::TPropertyField<o2::Vec3F>);
DECLARE_TEMPLATE_CLASS(o2::LinkRef<Editor::Vec3FProperty>);
DECLARE_TEMPLATE_CLASS(o2::LinkRef<Editor::TPropertyField<o2::Vec3F>>);
// --- META ---

DECLARE_CLASS(Editor::Vec3FProperty, Editor__Vec3FProperty);
// --- END META ---
