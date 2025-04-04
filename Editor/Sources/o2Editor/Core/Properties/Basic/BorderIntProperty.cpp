#include "o2Editor/stdafx.h"
#include "BorderIntProperty.h"

#include "o2Editor/Core/Properties/Basic/IntegerProperty.h"

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
        mLeftProperty->onChanged = [&](const Ref<IPropertyField>& field) { onChanged(field); };
        mLeftProperty->onChangeCompleted = [&](const String& path, const Vector<DataDocument>& before, const Vector<DataDocument>& after)
        {
            onChangeCompleted(mValuesPath + "/" + path, before, after);
        };

        mBottomProperty = GetChildByType<IntegerProperty>("container/layout/properties/bottom");
        mBottomProperty->SetValuePath("bottom");
        mBottomProperty->onChanged = [&](const Ref<IPropertyField>& field) { onChanged(field); };
        mBottomProperty->onChangeCompleted = [&](const String& path, const Vector<DataDocument>& before, const Vector<DataDocument>& after)
        {
            onChangeCompleted(mValuesPath + "/" + path, before, after);
        };

        mRightProperty = GetChildByType<IntegerProperty>("container/layout/properties/right");
        mRightProperty->SetValuePath("right");
        mRightProperty->onChanged = [&](const Ref<IPropertyField>& field) { onChanged(field); };
        mRightProperty->onChangeCompleted = [&](const String& path, const Vector<DataDocument>& before, const Vector<DataDocument>& after)
        {
            onChangeCompleted(mValuesPath + "/" + path, before, after);
        };

        mTopProperty = GetChildByType<IntegerProperty>("container/layout/properties/top");
        mTopProperty->SetValuePath("top");
        mTopProperty->onChanged = [&](const Ref<IPropertyField>& field) { onChanged(field); };
        mTopProperty->onChangeCompleted = [&](const String& path, const Vector<DataDocument>& before, const Vector<DataDocument>& after)
        {
            onChangeCompleted(mValuesPath + "/" + path, before, after);
        };
    }

    void BorderIProperty::SetValue(const BorderI& value)
    {
        mLeftProperty->SetValue(value.left);
        mBottomProperty->SetValue(value.bottom);
        mRightProperty->SetValue(value.right);
        mTopProperty->SetValue(value.top);
    }

    void BorderIProperty::SetValueLeft(int value)
    {
        mLeftProperty->SetValue(value);
    }

    void BorderIProperty::SetValueRight(int value)
    {
        mRightProperty->SetValue(value);
    }

    void BorderIProperty::SetValueTop(int value)
    {
        mTopProperty->SetValue(value);
    }

    void BorderIProperty::SetValueBottom(int value)
    {
        mBottomProperty->SetValue(value);
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

    void BorderIProperty::SetValueAndPrototypeProxy(const TargetsVec& targets)
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
