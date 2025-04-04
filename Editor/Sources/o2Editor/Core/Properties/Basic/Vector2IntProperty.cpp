#include "o2Editor/stdafx.h"
#include "Vector2IntProperty.h"

#include "o2Editor/Core/Properties/Basic/IntegerProperty.h"

namespace Editor
{
    Vec2IProperty::Vec2IProperty(RefCounter* refCounter):
        IPropertyField(refCounter)
    {}

    Vec2IProperty::Vec2IProperty(RefCounter* refCounter, const Vec2IProperty& other):
        IPropertyField(refCounter, other)
    {
        InitializeControls();
    }

    Editor::Vec2IProperty& Vec2IProperty::operator=(const Vec2IProperty& other)
    {
        IPropertyField::operator=(other);
        InitializeControls();
        return *this;
    }

    void Vec2IProperty::InitializeControls()
    {
        mXProperty = GetChildByType<IntegerProperty>("container/layout/properties/x");
        mXProperty->SetValuePath("x");
        mXProperty->onChanged = [&](const Ref<IPropertyField>& field) { onChanged(field); };
        mXProperty->onChangeCompleted = [&](const String& path, const Vector<DataDocument>& before, const Vector<DataDocument>& after)
        {
            onChangeCompleted(mValuesPath + "/" + path, before, after);
        };

        mYProperty = GetChildByType<IntegerProperty>("container/layout/properties/y");
        mYProperty->SetValuePath("y");
        mYProperty->onChanged = [&](const Ref<IPropertyField>& field) { onChanged(field); };
        mYProperty->onChangeCompleted = [&](const String& path, const Vector<DataDocument>& before, const Vector<DataDocument>& after)
        {
            onChangeCompleted(mValuesPath + "/" + path, before, after);
        };
    }

    void Vec2IProperty::SetValue(const Vec2I& value)
    {
        mXProperty->SetValue(value.x);
        mYProperty->SetValue(value.y);
    }

    void Vec2IProperty::SetValueX(int value)
    {
        mXProperty->SetValue(value);
    }

    void Vec2IProperty::SetValueY(int value)
    {
        mYProperty->SetValue(value);
    }

    void Vec2IProperty::SetUnknownValue(const Vec2I& defaultValue /*= Vec2I()*/)
    {
        mXProperty->SetUnknownValue(defaultValue.x);
        mYProperty->SetUnknownValue(defaultValue.y);
    }

    void Vec2IProperty::SetXUnknownValue(int defaultValue /*= 0.0f*/)
    {
        mXProperty->SetUnknownValue(defaultValue);
    }

    void Vec2IProperty::SetYUnknownValue(int defaultValue /*= 0.0f*/)
    {
        mYProperty->SetUnknownValue(defaultValue);
    }

    void Vec2IProperty::SetValueAndPrototypeProxy(const TargetsVec& targets)
    {
        mValuesProxies = targets;

        mXProperty->SetValueAndPrototypeProxy(targets.Convert<TargetPair>([](const TargetPair& x) {
            return TargetPair(mmake<XValueProxy>(x.first), x.second ? mmake<XValueProxy>(x.second) : nullptr); }));

        mYProperty->SetValueAndPrototypeProxy(targets.Convert<TargetPair>([](const TargetPair& x) {
            return TargetPair(mmake<YValueProxy>(x.first), x.second ? mmake<YValueProxy>(x.second) : nullptr); }));
    }

    void Vec2IProperty::Refresh(bool forcible /*= false*/)
    {
        if (mValuesProxies.IsEmpty())
            return;

        mXProperty->Refresh(forcible);
        mXProperty->Refresh(forcible);

        CheckRevertableState();
    }

    void Vec2IProperty::Revert()
    {
        if (mValuesProxies.IsEmpty())
            return;

        mXProperty->Refresh();
        mXProperty->Refresh();

        CheckRevertableState();
    }

    Vec2I Vec2IProperty::GetCommonValue() const
    {
        return Vec2I(mXProperty->GetCommonValue(), mYProperty->GetCommonValue());
    }

    bool Vec2IProperty::IsValuesDifferent() const
    {
        return mXProperty->IsValuesDifferent() || mYProperty->IsValuesDifferent();
    }

    const Type* Vec2IProperty::GetValueType() const
    {
        return GetValueTypeStatic();
    }

    const Type* Vec2IProperty::GetValueTypeStatic()
    {
        return &TypeOf(Vec2I);
    }

    Vec2IProperty::XValueProxy::XValueProxy(const Ref<IAbstractValueProxy>& proxy):
        mProxy(proxy)
    {}

    Vec2IProperty::XValueProxy::XValueProxy()
    {}

    void Vec2IProperty::XValueProxy::SetValue(const int& value)
    {
        Vec2I proxyValue;
        mProxy->GetValuePtr(&proxyValue);
        proxyValue.x = value;
        mProxy->SetValuePtr(&proxyValue);
    }

    int Vec2IProperty::XValueProxy::GetValue() const
    {
        Vec2I proxyValue;
        mProxy->GetValuePtr(&proxyValue);
        return proxyValue.x;
    }

    Vec2IProperty::YValueProxy::YValueProxy(const Ref<IAbstractValueProxy>& proxy):
        mProxy(proxy)
    {}

    Vec2IProperty::YValueProxy::YValueProxy()
    {}

    void Vec2IProperty::YValueProxy::SetValue(const int& value)
    {
        Vec2I proxyValue;
        mProxy->GetValuePtr(&proxyValue);
        proxyValue.y = value;
        mProxy->SetValuePtr(&proxyValue);
    }

    int Vec2IProperty::YValueProxy::GetValue() const
    {
        Vec2I proxyValue;
        mProxy->GetValuePtr(&proxyValue);
        return proxyValue.y;
    }
}

DECLARE_TEMPLATE_CLASS(Editor::TPropertyField<o2::Vec2I>);
DECLARE_TEMPLATE_CLASS(o2::LinkRef<Editor::Vec2IProperty>);
DECLARE_TEMPLATE_CLASS(o2::LinkRef<Editor::TPropertyField<o2::Vec2I>>);
// --- META ---

DECLARE_CLASS(Editor::Vec2IProperty, Editor__Vec2IProperty);
// --- END META ---
