#include "o2Editor/stdafx.h"
#include "BorderFloatProperty.h"

#include "o2Editor/Core/Properties/Basic/FloatProperty.h"

namespace Editor
{
    BorderFProperty::BorderFProperty(RefCounter* refCounter):
        IPropertyField(refCounter)
    {}

    BorderFProperty::BorderFProperty(RefCounter* refCounter, const BorderFProperty& other) :
        IPropertyField(refCounter, other)
    {
        InitializeControls();
    }

    BorderFProperty& BorderFProperty::operator=(const BorderFProperty& other)
    {
        IPropertyField::operator=(other);
        InitializeControls();
        return *this;
    }

    void BorderFProperty::InitializeControls()
    {
        mLeftProperty = GetChildByType<FloatProperty>("container/layout/properties/left");
        mLeftProperty->SetValuePath("left");
        mLeftProperty->onChanged = [&](const Ref<IPropertyField>& field) { onChanged(field); };
        mLeftProperty->onChangeCompleted = [&](const String& path, const Vector<DataDocument>& before, const Vector<DataDocument>& after)
        {
            onChangeCompleted(mValuesPath + "/" + path, before, after);
        };

        mBottomProperty = GetChildByType<FloatProperty>("container/layout/properties/bottom");
        mBottomProperty->SetValuePath("bottom");
        mBottomProperty->onChanged = [&](const Ref<IPropertyField>& field) { onChanged(field); };
        mBottomProperty->onChangeCompleted = [&](const String& path, const Vector<DataDocument>& before, const Vector<DataDocument>& after)
        {
            onChangeCompleted(mValuesPath + "/" + path, before, after);
        };

        mRightProperty = GetChildByType<FloatProperty>("container/layout/properties/right");
        mRightProperty->SetValuePath("right");
        mRightProperty->onChanged = [&](const Ref<IPropertyField>& field) { onChanged(field); };
        mRightProperty->onChangeCompleted = [&](const String& path, const Vector<DataDocument>& before, const Vector<DataDocument>& after)
        {
            onChangeCompleted(mValuesPath + "/" + path, before, after);
        };

        mTopProperty = GetChildByType<FloatProperty>("container/layout/properties/top");
        mTopProperty->SetValuePath("top");
        mTopProperty->onChanged = [&](const Ref<IPropertyField>& field) { onChanged(field); };
        mTopProperty->onChangeCompleted = [&](const String& path, const Vector<DataDocument>& before, const Vector<DataDocument>& after)
        {
            onChangeCompleted(mValuesPath + "/" + path, before, after);
        };
    }

    void BorderFProperty::SetValue(const BorderF& value)
    {
        mLeftProperty->SetValue(value.left);
        mBottomProperty->SetValue(value.bottom);
        mRightProperty->SetValue(value.right);
        mTopProperty->SetValue(value.top);
    }

    void BorderFProperty::SetValueLeft(float value)
    {
        mLeftProperty->SetValue(value);
    }

    void BorderFProperty::SetValueRight(float value)
    {
        mRightProperty->SetValue(value);
    }

    void BorderFProperty::SetValueTop(float value)
    {
        mTopProperty->SetValue(value);
    }

    void BorderFProperty::SetValueBottom(float value)
    {
        mBottomProperty->SetValue(value);
    }

    void BorderFProperty::SetUnknownValue(const BorderF& defaultValue /*= BorderF()*/)
    {
        mLeftProperty->SetUnknownValue(defaultValue.left);
        mRightProperty->SetUnknownValue(defaultValue.right);
        mTopProperty->SetUnknownValue(defaultValue.top);
        mBottomProperty->SetUnknownValue(defaultValue.bottom);
    }

    void BorderFProperty::SetLeftUnknownValue(float defaultValue /*= 0.0f*/)
    {
        mLeftProperty->SetUnknownValue(defaultValue);
    }

    void BorderFProperty::SetRightUnknownValue(float defaultValue /*= 0.0f*/)
    {
        mRightProperty->SetUnknownValue(defaultValue);
    }

    void BorderFProperty::SetTopUnknownValue(float defaultValue /*= 0.0f*/)
    {
        mTopProperty->SetUnknownValue(defaultValue);
    }

    void BorderFProperty::SetBottomUnknownValue(float defaultValue /*= 0.0f*/)
    {
        mBottomProperty->SetUnknownValue(defaultValue);
    }

    void BorderFProperty::SetValueAndPrototypeProxy(const TargetsVec& targets)
    {
        mValuesProxies = targets;

        mLeftProperty->SetValueAndPrototypeProxy(targets.Convert<TargetPair>([](const TargetPair& x) {
            return TargetPair(mmake<LeftValueProxy>(x.first), x.second ? mmake<LeftValueProxy>(x.second) : nullptr); }));

        mRightProperty->SetValueAndPrototypeProxy(targets.Convert<TargetPair>([](const TargetPair& x) {
            return TargetPair(mmake<RightValueProxy>(x.first), x.second ? mmake<RightValueProxy>(x.second) : nullptr); }));

        mTopProperty->SetValueAndPrototypeProxy(targets.Convert<TargetPair>([](const TargetPair& x) {
            return TargetPair(mmake<TopValueProxy>(x.first), x.second ? mmake<TopValueProxy>(x.second) : nullptr); }));

        mBottomProperty->SetValueAndPrototypeProxy(targets.Convert<TargetPair>([](const TargetPair& x) {
            return TargetPair(mmake<BottomValueProxy>(x.first), x.second ? mmake<BottomValueProxy>(x.second) : nullptr); }));
    }

    void BorderFProperty::Refresh(bool forcible /*= false*/)
    {
        if (mValuesProxies.IsEmpty())
            return;

        mLeftProperty->Refresh(forcible);
        mRightProperty->Refresh(forcible);
        mTopProperty->Refresh(forcible);
        mBottomProperty->Refresh(forcible);

        CheckRevertableState();
    }

    BorderF BorderFProperty::GetCommonValue() const
    {
        return BorderF(mLeftProperty->GetCommonValue(), mBottomProperty->GetCommonValue(),
                       mRightProperty->GetCommonValue(), mTopProperty->GetCommonValue());
    }

    bool BorderFProperty::IsValuesDifferent() const
    {
        return mLeftProperty->IsValuesDifferent() || mRightProperty->IsValuesDifferent() ||
            mTopProperty->IsValuesDifferent() || mBottomProperty->IsValuesDifferent();
    }

    const Type* BorderFProperty::GetValueType() const
    {
        return GetValueTypeStatic();
    }

    const Type* BorderFProperty::GetValueTypeStatic()
    {
        return &TypeOf(BorderF);
    }

    BorderFProperty::LeftValueProxy::LeftValueProxy(const Ref<IAbstractValueProxy>& proxy) :mProxy(proxy)
    {}

    BorderFProperty::LeftValueProxy::LeftValueProxy()
    {}

    void BorderFProperty::LeftValueProxy::SetValue(const float& value)
    {
        BorderF proxyValue;
        mProxy->GetValuePtr(&proxyValue);
        proxyValue.left = value;
        mProxy->SetValuePtr(&proxyValue);
    }

    float BorderFProperty::LeftValueProxy::GetValue() const
    {
        BorderF proxyValue;
        mProxy->GetValuePtr(&proxyValue);
        return proxyValue.left;
    }

    BorderFProperty::RightValueProxy::RightValueProxy(const Ref<IAbstractValueProxy>& proxy) :mProxy(proxy)
    {}

    BorderFProperty::RightValueProxy::RightValueProxy()
    {}

    void BorderFProperty::RightValueProxy::SetValue(const float& value)
    {
        BorderF proxyValue;
        mProxy->GetValuePtr(&proxyValue);
        proxyValue.right = value;
        mProxy->SetValuePtr(&proxyValue);
    }

    float BorderFProperty::RightValueProxy::GetValue() const
    {
        BorderF proxyValue;
        mProxy->GetValuePtr(&proxyValue);
        return proxyValue.right;
    }

    BorderFProperty::TopValueProxy::TopValueProxy(const Ref<IAbstractValueProxy>& proxy) :mProxy(proxy)
    {}

    BorderFProperty::TopValueProxy::TopValueProxy()
    {}

    void BorderFProperty::TopValueProxy::SetValue(const float& value)
    {
        BorderF proxyValue;
        mProxy->GetValuePtr(&proxyValue);
        proxyValue.top = value;
        mProxy->SetValuePtr(&proxyValue);
    }

    float BorderFProperty::TopValueProxy::GetValue() const
    {
        BorderF proxyValue;
        mProxy->GetValuePtr(&proxyValue);
        return proxyValue.top;
    }

    BorderFProperty::BottomValueProxy::BottomValueProxy(const Ref<IAbstractValueProxy>& proxy) :mProxy(proxy)
    {}

    BorderFProperty::BottomValueProxy::BottomValueProxy()
    {}

    void BorderFProperty::BottomValueProxy::SetValue(const float& value)
    {
        BorderF proxyValue;
        mProxy->GetValuePtr(&proxyValue);
        proxyValue.bottom = value;
        mProxy->SetValuePtr(&proxyValue);
    }

    float BorderFProperty::BottomValueProxy::GetValue() const
    {
        BorderF proxyValue;
        mProxy->GetValuePtr(&proxyValue);
        return proxyValue.bottom;
    }

}

DECLARE_TEMPLATE_CLASS(Editor::TPropertyField<o2::BorderF>);
DECLARE_TEMPLATE_CLASS(o2::LinkRef<Editor::BorderFProperty>);
DECLARE_TEMPLATE_CLASS(o2::LinkRef<Editor::TPropertyField<o2::BorderF>>);
// --- META ---

DECLARE_CLASS(Editor::BorderFProperty, Editor__BorderFProperty);
// --- END META ---
