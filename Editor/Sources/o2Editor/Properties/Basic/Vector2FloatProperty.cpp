#include "o2Editor/stdafx.h"
#include "Vector2FloatProperty.h"

#include "o2Editor/Properties/Basic/FloatProperty.h"

namespace Editor
{
    Vec2FProperty::Vec2FProperty(RefCounter* refCounter):
        IPropertyField(refCounter)
    {}

    Vec2FProperty::Vec2FProperty(RefCounter* refCounter, const Vec2FProperty& other) :
        IPropertyField(refCounter, other)
    {
        InitializeControls();
    }

    Vec2FProperty& Vec2FProperty::operator=(const Vec2FProperty& other)
    {
        IPropertyField::operator=(other);
        InitializeControls();
        return *this;
    }

    void Vec2FProperty::InitializeControls()
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
    }

    void Vec2FProperty::StoreValues(Vector<DataDocument>& data) const
    {
        data.Clear();
        for (auto& ptr : mValuesProxies)
        {
            data.Add(DataDocument());
            data.Last() = GetProxy<Vec2F>(ptr.first);
        }
    }

    void Vec2FProperty::OnPropertyBeforeChange(const Ref<IPropertyField>& field, bool byUser)
    {
        if (mIsTargetProxiesProperties && byUser)
            BeginUserChanging();
    }

    void Vec2FProperty::OnPropertyChanged(const Ref<IPropertyField>& field, bool byUser)
    {
        onChanged(field, byUser);
    }

    void Vec2FProperty::OnPropertyChangeCompleted(const String& path, const Vector<DataDocument>& before, const Vector<DataDocument>& after)
    {
        if (mIsTargetProxiesProperties)
            EndUserChanging();
        else
            onChangeCompleted(mValuesPath + "/" + path, before, after);
    }
        
    void Vec2FProperty::SetValue(const Vec2F& value)
    {
        mXProperty->SetValue(value.x);
        mYProperty->SetValue(value.y);
    }

    void Vec2FProperty::SetValueX(float value)
    {
        mXProperty->SetValue(value);
    }

    void Vec2FProperty::SetValueY(float value)
    {
        mYProperty->SetValue(value);
    }

    void Vec2FProperty::SetUnknownValue(const Vec2F& defaultValue /*= Vec2F()*/)
    {
        mXProperty->SetUnknownValue(defaultValue.x);
        mYProperty->SetUnknownValue(defaultValue.y);
    }

    void Vec2FProperty::SetXUnknownValue(float defaultValue /*= 0.0f*/)
    {
        mXProperty->SetUnknownValue(defaultValue);
    }

    void Vec2FProperty::SetYUnknownValue(float defaultValue /*= 0.0f*/)
    {
        mYProperty->SetUnknownValue(defaultValue);
    }

    void Vec2FProperty::SetValueAndPrototypeProxy(const TargetsVec& targets)
    {
        mValuesProxies = targets;

        mIsTargetProxiesProperties = targets.IsEmpty() ? false : dynamic_cast<IPropertyValueProxy*>(targets[0].first.Get()) != nullptr;

        mXProperty->SetValueAndPrototypeProxy(targets.Convert<TargetPair>([](const TargetPair& x) {
            return TargetPair(mmake<XValueProxy>(x.first), x.second ? mmake<XValueProxy>(x.second) : nullptr); }));

        mYProperty->SetValueAndPrototypeProxy(targets.Convert<TargetPair>([](const TargetPair& x) {
            return TargetPair(mmake<YValueProxy>(x.first), x.second ? mmake<YValueProxy>(x.second) : nullptr); }));
    }

    void Vec2FProperty::Refresh(bool forcible /*= false*/)
    {
        if (mValuesProxies.IsEmpty())
            return;

        mXProperty->Refresh(forcible);
        mYProperty->Refresh(forcible);

        CheckRevertableState();
    }

    void Vec2FProperty::Revert()
    {
        if (mValuesProxies.IsEmpty())
            return;

        mXProperty->Refresh();
        mYProperty->Refresh();

        CheckRevertableState();
    }

    Vec2F Vec2FProperty::GetCommonValue() const
    {
        return Vec2F(mXProperty->GetCommonValue(), mYProperty->GetCommonValue());
    }

    bool Vec2FProperty::IsValuesDifferent() const
    {
        return mXProperty->IsValuesDifferent() || mYProperty->IsValuesDifferent();
    }

    const Ref<FloatProperty>& Vec2FProperty::GetXProperty() const
    {
        return mXProperty;
    }

    const Ref<FloatProperty>& Vec2FProperty::GetYProperty() const
    {
        return mYProperty;
    }

    const Type* Vec2FProperty::GetValueType() const
    {
        return GetValueTypeStatic();
    }

    const Type* Vec2FProperty::GetValueTypeStatic()
    {
        return &TypeOf(Vec2F);
    }

    Vec2FProperty::XValueProxy::XValueProxy(const Ref<IAbstractValueProxy>& proxy):
        mProxy(proxy)
    {}

    Vec2FProperty::XValueProxy::XValueProxy()
    {}

    void Vec2FProperty::XValueProxy::SetValue(const float& value)
    {
        Vec2F proxyValue;
        mProxy->GetValuePtr(&proxyValue);
        proxyValue.x = value;
        mProxy->SetValuePtr(&proxyValue);
    }

    float Vec2FProperty::XValueProxy::GetValue() const
    {
        Vec2F proxyValue;
        mProxy->GetValuePtr(&proxyValue);
        return proxyValue.x;
    }

    Vec2FProperty::YValueProxy::YValueProxy(const Ref<IAbstractValueProxy>& proxy):
        mProxy(proxy)
    {}

    Vec2FProperty::YValueProxy::YValueProxy()
    {}

    void Vec2FProperty::YValueProxy::SetValue(const float& value)
    {
        Vec2F proxyValue;
        mProxy->GetValuePtr(&proxyValue);
        proxyValue.y = value;
        mProxy->SetValuePtr(&proxyValue);
    }

    float Vec2FProperty::YValueProxy::GetValue() const
    {
        Vec2F proxyValue;
        mProxy->GetValuePtr(&proxyValue);
        return proxyValue.y;
    }
}

DECLARE_TEMPLATE_CLASS(Editor::TPropertyField<o2::Vec2F>);
DECLARE_TEMPLATE_CLASS(o2::LinkRef<Editor::Vec2FProperty>);
DECLARE_TEMPLATE_CLASS(o2::LinkRef<Editor::TPropertyField<o2::Vec2F>>);
// --- META ---

DECLARE_CLASS(Editor::Vec2FProperty, Editor__Vec2FProperty);
// --- END META ---
