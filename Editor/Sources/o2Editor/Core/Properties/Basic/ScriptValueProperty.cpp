#include "o2Editor/stdafx.h"
#include "ScriptValueProperty.h"

#include "o2/Scene/UI/UIManager.h"
#include "o2/Scene/UI/Widgets/Button.h"
#include "o2/Scene/UI/Widgets/Spoiler.h"
#include "o2/Utils/Editor/Attributes/NoHeaderAttribute.h"
#include "o2/Utils/Editor/EditorScope.h"
#include "o2Editor/Core/Properties/Basic/IntegerProperty.h"
#include "o2Editor/Core/Properties/IObjectPropertiesViewer.h"
#include "o2Editor/Core/Properties/Properties.h"

using namespace o2;

namespace Editor
{
    ScriptValueProperty::ScriptValueProperty(RefCounter* refCounter):
        IPropertyField(refCounter)
    {
        InitializeControls();
    }

    ScriptValueProperty::ScriptValueProperty(RefCounter* refCounter, const ScriptValueProperty& other) :
        IPropertyField(refCounter, other)
    {
        InitializeControls();
    }

    ScriptValueProperty& ScriptValueProperty::operator=(const ScriptValueProperty& other)
    {
        IPropertyField::operator=(other);
        InitializeControls();
        return *this;
    }

    void ScriptValueProperty::OnFreeProperty()
    {
    }

    void ScriptValueProperty::InitializeControls()
    {
        expandHeight = true;
        expandWidth = true;
        fitByChildren = true;

        mSpoiler = o2UI.CreateWidget<Spoiler>("expand with caption");
        AddChild(mSpoiler);

        if (mSpoiler)
            mSpoiler->onExpand = THIS_FUNC(OnExpand);

        mSpoiler->borderTop = 5;

        mHeaderContainer = mmake<HorizontalLayout>();
        *mHeaderContainer->layout = WidgetLayout::HorStretch(VerAlign::Top, 100, 0, 20, 0);
        mHeaderContainer->baseCorner = BaseCorner::Right;
        mHeaderContainer->expandHeight = false;
        mHeaderContainer->SetInternalParent(mSpoiler, false);

        // Add button
        mAddButton = mSpoiler->FindChildByTypeAndName<Button>("add button");
        if (!mAddButton)
        {
            mAddButton = o2UI.CreateWidget<Button>("add small");
            mAddButton->name = "add button";
            mAddButton->layout->maxWidth = 20;
            mAddButton->layout->minHeight = 20;
            mAddButton->onClick = THIS_FUNC(OnAddPressed);

            mHeaderContainer->AddChild(mAddButton);
        }

        // Count property
        mCountProperty = mSpoiler->FindChildByType<IntegerProperty>(false);
        if (!mCountProperty)
            mCountProperty = o2UI.CreateWidget<IntegerProperty>();

        if (mCountProperty)
        {
            mHeaderContainer->AddChild(mCountProperty);

            mCountProperty->layout->maxWidth = 40;
            mCountProperty->SetValue(0);
            mCountProperty->onChanged = THIS_FUNC(OnCountChanged);
        }
    }

    ScriptValueProperty::PropertiesList ScriptValueProperty::GetCommonProperties(
        const Vector<Pair<ScriptValue, ScriptValue>>& values, bool& isArray) const
    {
        PropertiesList res;

        for (auto& kv : values)
        {
            if (kv.first.IsObject())
            {
                isArray = false;

                kv.first.ForEachProperties(
                    [&](const ScriptValue& name, const ScriptValue& value)
                    {
                        auto nameStr = name.ToString();
                        if (nameStr[0] != '_')
                        {
                            Pair<Ref<IScriptValueProperty>, Ref<IScriptValueProperty>> elem;
                            elem.first = mmake<o2::ScriptValueProperty>(kv.first, name);
                            if (kv.second.IsObject())
                                elem.second = mmake<o2::ScriptValueProperty>(kv.second, name);

                            auto fnd = res.Find([&](auto& x) { return x.first == nameStr; });
                            if (fnd)
                                fnd->second.Add(elem);
                            else
                                res.Add({ nameStr, { elem } });
                        }

                        return true;
                    });
            }
            else if (kv.first.IsArray())
            {
                isArray = true;

                for (int i = 0; i < kv.first.GetLength(); i++)
                {
                    Pair<Ref<IScriptValueProperty>, Ref<IScriptValueProperty>> elem;
                    elem.first = mmake<ScriptValueArrayElement>(kv.first, i);
                    if (kv.second.IsObject())
                        elem.second = mmake<ScriptValueArrayElement>(kv.second, i);

                    auto nameStr = (String)i;
                    auto fnd = res.Find([&](auto& x) { return x.first == nameStr; });
                    if (fnd)
                        fnd->second.Add(elem);
                    else
                        res.Add({ nameStr, { elem } });
                }
            }
        }

        // Leave only common properties
        res.RemoveAll([&](const auto& item) { return item.second.Count() != values.Count(); });

        return res;
    }

    void ScriptValueProperty::SetValueAndPrototypeProxy(const TargetsVec& targets)
    {
        mNeedUpdateProxies = targets != mValuesProxies;
        IPropertyField::SetValueAndPrototypeProxy(targets);
    }

    void ScriptValueProperty::Refresh(bool forcible /*= false*/)
    {
        mIsRefreshing = true;

        PushEditorScopeOnStack scope;

        // Get values from proxies
        Vector<Pair<ScriptValue, ScriptValue>> values =
            mValuesProxies.Convert<Pair<ScriptValue, ScriptValue>>(
                [](const Pair<Ref<IAbstractValueProxy>, Ref<IAbstractValueProxy>>& x)
                {
                    Pair<ScriptValue, ScriptValue> res;
                    x.first->GetValuePtr(&res.first);

                    if (x.second)
                        x.second->GetValuePtr(&res.second);

                    return res;
                }
        );

        // Get common properties
        auto commonProperties = GetCommonProperties(values, mIsArray);

        if (mIsArray && !values.IsEmpty())
        {
            int commonCount = values[0].first.GetLength();
            bool coundDifferents = false;

            for (int i = 1; i < values.Count(); i++)
            {
                int count = values[i].first.GetLength();
                if (count != commonCount)
                    coundDifferents = true;
            }

            if (mCountDifferents != coundDifferents || mCountOfElements != commonCount)
            {
                if (mCountDifferents)
                    mCountProperty->SetUnknownValue(commonCount);
                else
                    mCountProperty->SetValue(commonCount);
            }

            mCountOfElements = commonCount;
            mCountDifferents = coundDifferents;

            mHeaderContainer->SetEnabled(true);
        }
        else
            mHeaderContainer->SetEnabled(false);

        if (IsExpanded())
        {
            // Check that last built properties are same
            bool changedProperties = false;
            for (auto& kv : commonProperties)
            {
                if (!mPreviousBuiltTypes.ContainsKey(kv.first) ||
                    mPreviousBuiltTypes[kv.first] != kv.second[0].first->Get().GetValueType())
                {
                    changedProperties = true;
                    break;
                }
            }

            // Rebuild properties if needed
            if (changedProperties || forcible)
            {
                mPreviousBuiltTypes.Clear();
                for (auto& kv : mBuiltProperties)
                {
                    mSpoiler->RemoveChild(kv.second, false);
                    kv.second->RemoveLayer("drag");
                    kv.second->SetValueAndPrototypeProxy({});
                    o2EditorProperties.FreeProperty(kv.second);
                }

                mBuiltProperties.Clear();

                for (auto& kv : commonProperties)
                {
                    auto& name = kv.first;
                    auto value = kv.second[0].first->Get();
                    auto type = value.GetValueType();
                    mPreviousBuiltTypes[name] = type;

                    int idx = mIsArray ? (int)name : 0;

                    if (type == ScriptValue::ValueType::Number)
                        AddProperty(name, &TypeOf(float), idx);
                    else if (type == ScriptValue::ValueType::String)
                        AddProperty(name, &TypeOf(String), idx);
                    else if (type == ScriptValue::ValueType::Bool)
                        AddProperty(name, &TypeOf(bool), idx);
                    else if (type == ScriptValue::ValueType::Object || type == ScriptValue::ValueType::Array)
                    {
                        auto prototype = value.GetPrototype();
                        if (prototype == *ScriptValuePrototypes::GetVec2Prototype())
                            AddProperty(name, &TypeOf(Vec2F), idx);
                        else if (prototype == *ScriptValuePrototypes::GetRectPrototype())
                            AddProperty(name, &TypeOf(RectF), idx);
                        else if (prototype == *ScriptValuePrototypes::GetBorderPrototype())
                            AddProperty(name, &TypeOf(BorderF), idx);
                        else if (prototype == *ScriptValuePrototypes::GetColor4Prototype())
                            AddProperty(name, &TypeOf(Color4), idx);
                        else if (value.IsObjectContainer())
                            AddProperty(name, value.GetObjectContainerType(), idx);
                        else
                            AddProperty(name, &TypeOf(ScriptValue), idx);
                    }
                }

                mNeedUpdateProxies = true;
            }

            for (auto& kv : mBuiltProperties)
            {
                if (mNeedUpdateProxies)
                {
                    auto& name = kv.first;
                    auto prop = commonProperties.Find([&](auto& x) { return x.first == name; });
                    auto value = prop->second[0].first->Get();
                    auto type = value.GetValueType();
                    auto field = kv.second;

                    if (type == ScriptValue::ValueType::Number)
                        SetFieldProxies<float>(commonProperties, name, field);
                    else if (type == ScriptValue::ValueType::String)
                        SetFieldProxies<String>(commonProperties, name, field);
                    else if (type == ScriptValue::ValueType::Bool)
                        SetFieldProxies<bool>(commonProperties, name, field);
                    else if (type == ScriptValue::ValueType::Object || type == ScriptValue::ValueType::Array)
                    {
                        auto prototype = value.GetPrototype();
                        if (prototype == *ScriptValuePrototypes::GetVec2Prototype())
                            SetFieldProxies<Vec2F>(commonProperties, name, field);
                        else if (prototype == *ScriptValuePrototypes::GetRectPrototype())
                            SetFieldProxies<RectF>(commonProperties, name, field);
                        else if (prototype == *ScriptValuePrototypes::GetBorderPrototype())
                            SetFieldProxies<BorderF>(commonProperties, name, field);
                        else if (prototype == *ScriptValuePrototypes::GetColor4Prototype())
                            SetFieldProxies<Color4>(commonProperties, name, field);
                        else if (value.IsObjectContainer())
                        {
                            auto proxies = prop->second.Convert<Pair<Ref<IAbstractValueProxy>, Ref<IAbstractValueProxy>>>(
                                [](const Pair<Ref<IScriptValueProperty>, Ref<IScriptValueProperty>>& x)
                                {
                                    Pair<Ref<IAbstractValueProxy>, Ref<IAbstractValueProxy>> res;
                                    res.first = mmake<ScriptValueProxy>(x.first->Clone());
                                    if (x.second && x.second->Get().IsObject())
                                        res.second = mmake<ScriptValueProxy>(x.second->Clone());

                                    return res;
                                });

                            field->SetValueAndPrototypeProxy(proxies);
                        }
                        else
                            SetFieldProxies<ScriptValue>(commonProperties, name, field);
                    }
                }
                else
                    kv.second->Refresh();
            }

            mSpoiler->SetLayoutDirty();
        }

        mNeedUpdateProxies = false;
        mIsRefreshing = false;
    }

    void ScriptValueProperty::AddProperty(const String& name, const Type* type, int idx)
    {
        auto prop = o2EditorProperties.CreateFieldProperty(type, name);
        if (mIsArray)
        {
            prop->GetRemoveButton()->onClick = [=]() { Remove(idx); };
            prop->AddLayer("drag", mmake<Sprite>("ui/UI4_drag_handle.png"),
                           Layout::Based(BaseCorner::LeftTop, Vec2F(20, 20), Vec2F(-18, 0)));
        }

        mSpoiler->AddChild(prop);
        mBuiltProperties.Add(name, prop);
    }

    void ScriptValueProperty::OnCountChanged(const Ref<IPropertyField>& def)
    {
        if (mIsRefreshing)
            return;

        Resize(mCountProperty->GetCommonValue());
    }

    void ScriptValueProperty::OnExpand()
    {
        Refresh();
    }

    void ScriptValueProperty::Resize(int newCount)
    {
        if (mIsRefreshing)
            return;

        for (auto& p : mValuesProxies)
        {
            ScriptValue first;
            p.first->GetValuePtr(&first);

            int length = first.GetLength();
            if (length > newCount)
            {
                for (int i = 0; i < length - newCount; i++)
                    first.RemoveElement(first.GetLength() - 1);
            }
            else
            {
                for (int i = 0; i < newCount - length; i++)
                    OnAddPressed();
            }
        }

        Refresh();
    }

    void ScriptValueProperty::Remove(int idx)
    {
        for (auto& p : mValuesProxies)
        {
            ScriptValue first;
            p.first->GetValuePtr(&first);
            first.RemoveElement(idx);
        }

        Refresh();
    }

    void ScriptValueProperty::OnAddPressed()
    {
        for (auto& p : mValuesProxies)
        {
            ScriptValue first;
            p.first->GetValuePtr(&first);
            if (first.GetLength() > 0)
                first.AddElement(first.GetElement(first.GetLength() - 1).Copy());
            else
                first.AddElement(ScriptValue(0));
        }

        Refresh();
    }

    const Type* ScriptValueProperty::GetValueType() const
    {
        return GetValueTypeStatic();
    }

    const Type* ScriptValueProperty::GetValueTypeStatic()
    {
        return &TypeOf(ScriptValue);
    }

    void ScriptValueProperty::SetFieldInfo(const FieldInfo* fieldInfo)
    {
        IPropertyField::SetFieldInfo(fieldInfo);

        if (fieldInfo)
        {
            mExpanded = fieldInfo->HasAttribute<ExpandedByDefaultAttribute>();
            SetHeaderEnabled(!fieldInfo->HasAttribute<NoHeaderAttribute>());
        }
    }

    void ScriptValueProperty::SetCaption(const WString& text)
    {
        mSpoiler->SetCaption(text);;
    }

    WString ScriptValueProperty::GetCaption() const
    {
        return mSpoiler->GetCaption();
    }

    Ref<Button> ScriptValueProperty::GetRemoveButton()
    {
        if (!mRemoveBtn)
        {
            mRemoveBtn = o2UI.CreateWidget<Button>("remove small");
            *mRemoveBtn->layout = WidgetLayout::Based(BaseCorner::RightTop, Vec2F(20, 20), Vec2F(2, 0));
            AddInternalWidget(mRemoveBtn);
        }

        return mRemoveBtn;
    }

    void ScriptValueProperty::Expand()
    {
        SetExpanded(true);
    }

    void ScriptValueProperty::Collapse()
    {
        SetExpanded(false);
    }

    void ScriptValueProperty::SetExpanded(bool expanded)
    {
        mExpanded = expanded;
        mSpoiler->SetExpanded(expanded);
    }

    bool ScriptValueProperty::IsExpanded() const
    {
        return mSpoiler->IsExpanded();
    }

    void ScriptValueProperty::SetHeaderEnabled(bool enabled)
    {
        mHeaderEnabled = enabled;

        if (mHeaderEnabled)
        {
            mSpoiler->SetHeadHeight(20);
            mSpoiler->GetLayerDrawable<Text>("caption")->enabled = true;
            mSpoiler->GetInternalWidget("expand")->enabledForcibly = true;
            mSpoiler->borderLeft = 10;
            mSpoiler->borderTop = 2;
        }
        else
        {
            mSpoiler->SetHeadHeight(0);
            mSpoiler->GetLayerDrawable<Text>("caption")->enabled = false;
            mSpoiler->GetInternalWidget("expand")->enabledForcibly = false;
            mSpoiler->borderLeft = 0;
            mSpoiler->borderTop = 0;
            mSpoiler->Expand();
        }
    }

    bool ScriptValueProperty::IsHeaderEnabled() const
    {
        return mHeaderEnabled;
    }

    void ScriptValueProperty::OnPropertyChanged(const String& path, const Vector<DataDocument>& before,
                                                const Vector<DataDocument>& after)
    {
        onChangeCompleted(path, before, after);
    }
}

DECLARE_TEMPLATE_CLASS(o2::LinkRef<Editor::ScriptValueProperty>);
// --- META ---

DECLARE_CLASS(Editor::ScriptValueProperty, Editor__ScriptValueProperty);
// --- END META ---
