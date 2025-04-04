#include "o2Editor/stdafx.h"
#include "VectorProperty.h"

#include "o2/Scene/UI/UIManager.h"
#include "o2/Scene/UI/Widgets/Button.h"
#include "o2/Scene/UI/Widgets/Spoiler.h"
#include "o2/Utils/Editor/Attributes/InvokeOnChangeAttribute.h"
#include "o2/Utils/Editor/EditorScope.h"
#include "o2Editor/Core/Properties/Basic/IntegerProperty.h"
#include "o2Editor/Core/Properties/Properties.h"
#include "o2Editor/SceneWindow/SceneEditScreen.h"

namespace Editor
{
    VectorProperty::VectorProperty(RefCounter* refCounter):
        IPropertyField(refCounter)
    {
        InitializeControls();
    }

    VectorProperty::VectorProperty(RefCounter* refCounter, const VectorProperty& other):
        IPropertyField(refCounter, other)
    {
        InitializeControls();
    }

    VectorProperty& VectorProperty::operator=(const VectorProperty& other)
    {
        IPropertyField::operator=(other);
        InitializeControls();
        return *this;
    }

    void VectorProperty::OnPropertyEnabled()
    {
        for (auto& property : mValueProperties)
            property->SetPropertyEnabled(true);
    }

    void VectorProperty::OnPropertyDisabled()
    {
        for (auto& property : mValueProperties)
            property->SetPropertyEnabled(false);
    }

    void VectorProperty::InitializeControls()
    {
        // Spoiler
        mSpoiler = FindChildByType<Spoiler>(false);

        if (!mSpoiler)
        {
            mSpoiler = o2UI.CreateWidget<Spoiler>("expand with caption");
            AddChild(mSpoiler);
        }

        if (mSpoiler)
            mSpoiler->onExpand = THIS_FUNC(OnExpand);

        mSpoiler->borderTop = 5;

        mHeaderContainer = mmake<HorizontalLayout>();
        *mHeaderContainer->layout = WidgetLayout::HorStretch(VerAlign::Top, 100, 0, 20, 0);
        mHeaderContainer->layout->minHeight = 20;
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

        // Other
        expandHeight = true;
        expandWidth = true;
        fitByChildren = true;
    }

    VectorProperty::~VectorProperty()
    {}

    void VectorProperty::SetValueAndPrototypeProxy(const TargetsVec& targets)
    {
        for (auto& pair : mTargetObjects) {
            if (pair.first.isCreated)
                delete pair.first.data;

            if (pair.second.isCreated)
                delete pair.second.data;
        }

        mTargetObjects.Clear();

        for (auto& pair : targets)
            mTargetObjects.Add({ GetObjectFromProxy(pair.first), GetObjectFromProxy(pair.second) });

        Refresh();
    }

    void VectorProperty::Refresh(bool forcible /*= false*/)
    {
        PushEditorScopeOnStack scope;

        if (mTargetObjects.IsEmpty())
            return;
        
        mIsRefreshing = true;

        if (IsExpanded())
        {
            for (auto& pair : mTargetObjects)
            {
                pair.first.Refresh();
                pair.second.Refresh();
            }
        }

        auto lastDifferent = mCountDifferents;

        mCountOfElements = mVectorType->GetObjectVectorSize(mTargetObjects[0].first.data);
        mCountDifferents = false;

        for (auto& target : mTargetObjects)
        {
            int targetCount = mVectorType->GetObjectVectorSize(target.first.data);
            if (targetCount != mCountOfElements)
            {
                mCountDifferents = true;
                break;
            }
        }

        if (mCountDifferents)
        {
            if (!lastDifferent)
            {
                mCountProperty->SetUnknownValue();

                if (IsExpanded())
                {
                    for (auto& prop : mValueProperties)
                    {
                        mSpoiler->RemoveChild(prop, false);
                        FreeValueProperty(prop);
                    }

                    mValueProperties.Clear();

                    mAddButton->Hide(true);

                    onChanged(Ref(this));
                    o2EditorSceneScreen.OnSceneChanged();
                }
            }
        }
        else if (lastDifferent || mValueProperties.Count() != mCountOfElements || forcible)
        {
            mCountProperty->SetValue(mCountOfElements);

            if (IsExpanded())
            {
                int i = 0;
                for (; i < mCountOfElements; i++)
                {
                    auto itemTargetValues = mTargetObjects.Convert<Pair<Ref<IAbstractValueProxy>, Ref<IAbstractValueProxy>>>(
                        [&](const Pair<TargetObjectData, TargetObjectData>& x)
                    {
                        return Pair<Ref<IAbstractValueProxy>, Ref<IAbstractValueProxy>>(
                            mVectorType->GetObjectVectorElementProxy(x.first.data, i),
                            x.second.data ? mVectorType->GetObjectVectorElementProxy(x.second.data, i) : nullptr);
                    });

                    Ref<IPropertyField> propertyDef;
                    if (i < mValueProperties.Count())
                        propertyDef = mValueProperties[i];
                    else
                    {
                        propertyDef = GetFreeValueProperty();
                        mValueProperties.Add(propertyDef);
                    }

                    mSpoiler->AddChild(propertyDef);
                    propertyDef->SetCaption(GetElementCaption(i, itemTargetValues));
                    propertyDef->SetValueAndPrototypeProxy(itemTargetValues);
                    propertyDef->SetValuePath((String)i);
                    propertyDef->GetRemoveButton()->onClick = [=]() { Remove(i); };

                    propertyDef->onChangeCompleted =
                        [&](const String& path, const Vector<DataDocument>& before, const Vector<DataDocument>& after)
                    {
                        OnPropertyChanged(mValuesPath + "/" + path, before, after);
                    };
                }

                for (; i < mValueProperties.Count(); i++)
                {
                    mSpoiler->RemoveChild(mValueProperties[i]);
                    FreeValueProperty(mValueProperties[i]);
                }

                mValueProperties.Resize(mCountOfElements);

                mAddButton->Show(true);

                mSpoiler->SetLayoutDirty();

                onChanged(Ref(this));
                o2EditorSceneScreen.OnSceneChanged();
            }
        }
        else
        {
            if (IsExpanded())
            {
                for (int i = 0; i < mCountOfElements; i++)
                {
                    auto itemTargetValues = mTargetObjects.Convert<Pair<Ref<IAbstractValueProxy>, Ref<IAbstractValueProxy>>>(
                        [&](const Pair<TargetObjectData, TargetObjectData>& x)
                    {
                        return Pair<Ref<IAbstractValueProxy>, Ref<IAbstractValueProxy>>(
                            mVectorType->GetObjectVectorElementProxy(x.first.data, i),
                            x.second.data ? mVectorType->GetObjectVectorElementProxy(x.second.data, i) : nullptr);
                    });

                    auto propertyDef = mValueProperties[i];
                    propertyDef->SetValueAndPrototypeProxy(itemTargetValues);
                    propertyDef->SetCaption(GetElementCaption(i, itemTargetValues));
                }
            }
        }

        if (!mHeaderEnabled)
        {
			mHeaderContainer->SetInternalParent(nullptr);
			mHeaderContainer->SetDrawingDepthInheritFromParent(true);
            mSpoiler->AddChild(mHeaderContainer);
        }

        mIsRefreshing = false;
    }

    const Type* VectorProperty::GetValueType() const
    {
        return mVectorType;
    }

    void VectorProperty::SpecializeType(const Type* type)
    {
        mVectorType = nullptr;

        if (type->GetUsage() == Type::Usage::Vector)
            mVectorType = dynamic_cast<const VectorType*>(type);
        else if (type->GetUsage() == Type::Usage::Property) 
        {
            auto propertyType = dynamic_cast<const PropertyType*>(type);

            if (propertyType->GetValueType()->GetUsage() == Type::Usage::Vector)
                mVectorType = dynamic_cast<const VectorType*>(propertyType->GetValueType());
        }
    }

    void VectorProperty::SetFieldInfo(const FieldInfo* fieldInfo)
    {
        IPropertyField::SetFieldInfo(fieldInfo);

        if (fieldInfo)
        {
            if (fieldInfo->HasAttribute<ExpandedByDefaultAttribute>())
                mSpoiler->Expand();
        }
    }

    const Type* VectorProperty::GetSpecializedType() const
    {
        return mVectorType;
    }

    void VectorProperty::SetCaption(const WString& text)
    {
        mSpoiler->SetCaption(text);

        auto spoilerCaptionLayer = mSpoiler->GetLayerDrawable<Text>("caption");
        if (spoilerCaptionLayer)
        {
            Vec2F captionSize = Text::GetTextSize(text, spoilerCaptionLayer->GetFont(), spoilerCaptionLayer->GetFontHeight());
            *mHeaderContainer->layout = WidgetLayout::HorStretch(VerAlign::Top, captionSize.x + 20.0f, 0, 17, 0);
            mHeaderContainer->layout->minHeight = 17;
        }
    }

    WString VectorProperty::GetCaption() const
    {
        return mSpoiler->GetCaption();
    }

    Ref<Button> VectorProperty::GetRemoveButton()
    {
        if (!mRemoveBtn)
        {
            mRemoveBtn = o2UI.CreateWidget<Button>("remove small");
            mRemoveBtn->layout->maxWidth = 20;
            mRemoveBtn->layout->minHeight = 20;
            mHeaderContainer->AddChild(mRemoveBtn, 0);
        }

        return mRemoveBtn;
    }

    void VectorProperty::Expand()
    {
        SetExpanded(true);
    }

    void VectorProperty::Collapse()
    {
        SetExpanded(false);
    }

    void VectorProperty::SetExpanded(bool expanded)
    {
        mSpoiler->SetExpanded(expanded);
    }

    bool VectorProperty::IsExpanded() const
    {
        return mSpoiler->IsExpanded() || !mHeaderEnabled;
    }

	void VectorProperty::SetHeaderEnabled(bool enabled)
	{
		mHeaderEnabled = enabled;

		if (mHeaderEnabled)
		{
			mSpoiler->SetHeadHeight(20);
            mSpoiler->GetLayerDrawable<Text>("caption")->enabled = true;
            mSpoiler->GetInternalWidget("expand")->enabledForcibly = true;
            mSpoiler->borderTop = 2;
            mHeaderContainer->SetInternalParent(mSpoiler);
		}
		else
		{
            mSpoiler->SetHeadHeight(0);
            mSpoiler->GetLayerDrawable<Text>("caption")->enabled = false;
            mSpoiler->GetInternalWidget("expand")->enabledForcibly = false;
            mSpoiler->borderTop = 0;
            mSpoiler->Expand();
            mHeaderContainer->SetInternalParent(nullptr);
		}
	}

	bool VectorProperty::IsHeaderEnabled() const
	{
        return mHeaderEnabled;
	}

	void VectorProperty::SetCaptionIndexesEnabled(bool enabled)
	{
        mCaptionIndexesEnabled = enabled;
	}

	bool VectorProperty::IsCaptionIndexesEnabled() const
	{
        return mCaptionIndexesEnabled;
	}

	void VectorProperty::SetCountEditBoxEnabled(bool enabled)
	{
        mCountEditBoxEnabled = enabled;
        mCountProperty->SetEnabled(enabled);
	}

	bool VectorProperty::IsCountEditBoxEnabled() const
	{
        return mCountEditBoxEnabled;
	}

	void* VectorProperty::GetProxyValuePointer(const Ref<IAbstractValueProxy>& proxy) const
    {
        auto variableProxy = DynamicCast<IPointerValueProxy>(proxy);
        if (variableProxy)
            return variableProxy->GetValueVoidPointer();

        return nullptr;
    }

    Ref<VectorProperty::IPropertyField> VectorProperty::GetFreeValueProperty()
    {
        if (!mValuePropertiesPool.IsEmpty())
            return mValuePropertiesPool.PopBack();

        auto res = o2EditorProperties.CreateFieldProperty(mVectorType->GetElementType(), "Element", 
                                                          onChangeCompleted, onChanged);

        if (mParentContext)
            res->SetParentContext(mParentContext.Lock());

        res->AddLayer("drag", mmake<Sprite>("ui/UI4_drag_handle.png"), Layout::Based(BaseCorner::LeftTop, Vec2F(20, 20), Vec2F(-18, 0)));

        if (res)
            res->SetFieldInfo(mFieldInfo);

        return res;
    }

    void VectorProperty::FreeValueProperty(const Ref<IPropertyField>& def)
    {
        mValuePropertiesPool.Add(def);
    }

    void VectorProperty::OnCountChanged(const Ref<IPropertyField>& def)
    {
        if (mIsRefreshing)
            return;

        Resize(mCountProperty->GetCommonValue());
    }

    void VectorProperty::OnAddPressed()
    {
        Expand();
        Resize(mCountOfElements + 1);
    }

    void VectorProperty::OnExpand()
    {
        Refresh();
    }

    VectorProperty::TargetObjectData VectorProperty::GetObjectFromProxy(const Ref<IAbstractValueProxy>& proxy)
    {
        TargetObjectData res;

        if (!proxy)
            return res;

        res.proxy = proxy;

        if (auto pointerProxy = DynamicCast<IPointerValueProxy>(proxy)) 
        {
            res.data = pointerProxy->GetValueVoidPointer();
            res.isCreated = false;
        }
        else
        {
            void* sample = proxy->GetType().CreateSample();
            proxy->GetValuePtr(sample);

            res.data = sample;
            res.isCreated = true;
        }

        return res;
    }

	String VectorProperty::GetElementCaption(int idx, const Vector<Pair<Ref<IAbstractValueProxy>, Ref<IAbstractValueProxy>>>& targets)
	{
		String caption = mCaptionIndexesEnabled ? (String)"# " + (String)idx : String::empty;

		if (!targets.IsEmpty())
		{
			auto& targetValue = targets[0].first;
			auto& type = targetValue->GetType();
			void* object = type.CreateSample();
			targetValue->GetValuePtr(object);
			String name = TryGetObjectName(object, type);
			type.DestroySample(object);

            if (!name.IsEmpty())
            {
                if (mCaptionIndexesEnabled)
                    caption += " ";

                caption += name;
            }
		}

        return caption;
	}

	String VectorProperty::TryGetObjectName(void* object, const Type& type) const
	{
        static Vector<String> searchNames = { "name", "id", "mName", "mId", "_name", "_id" };

		if (!object)
			return String::empty;

        if (type.GetUsage() == Type::Usage::Reference)
        {
            if (auto referenceType = dynamic_cast<const ReferenceType*>(&type))
                return TryGetObjectName(referenceType->GetObjectRawPtr(object), *referenceType->GetBaseType());
        }

        for (auto& fieldInfo : type.GetFields())
        {
            if (searchNames.Contains(fieldInfo.GetName()) && fieldInfo.GetType() == &TypeOf(String))
            {
                String value = fieldInfo.GetValue<String>(object);
                return value;
            }
        }

        for (auto& base : type.GetBaseTypes())
        {
            void* baseObject = base.dynamicCastUpFunc(object);
            String value = TryGetObjectName(baseObject, *base.type);
            if (!value.IsEmpty())
                return value;
        }

        return String::empty;
	}

	void VectorProperty::OnPropertyChanged(const String& path, const Vector<DataDocument>& before,
                                           const Vector<DataDocument>& after)
    {
        for (auto& pair : mTargetObjects)
            pair.first.SetValue();

        onChangeCompleted(path, before, after);
    }

    void VectorProperty::Resize(int newCount)
    {
        newCount = Math::Max(0, newCount);

        Vector<DataDocument> prevValues, newValues;
        auto elementFieldInfo = mVectorType->GetElementFieldInfo();

        for (auto& obj : mTargetObjects)
        {
            prevValues.Add(DataDocument());
            prevValues.Last()["Size"].Set(mVectorType->GetObjectVectorSize(obj.first.data));
            DataValue& elementsData = prevValues.Last()["Elements"];

            int lastCount = mVectorType->GetObjectVectorSize(obj.first.data);
            for (int i = newCount; i < lastCount; i++)
            {
                elementFieldInfo->Serialize(mVectorType->GetObjectVectorElementPtr(obj.first.data, i),
                                            elementsData.AddMember("Element" + (String)i));
            }

            newValues.Add(DataDocument());
            newValues.Last()["Size"].Set(newCount);

            mVectorType->SetObjectVectorSize(obj.first.data, newCount);
        }

        Refresh();

        if (prevValues != newValues)
            onChangeCompleted(mValuesPath + "/count", prevValues, newValues);

        onChanged(Ref(this));
        o2EditorSceneScreen.OnSceneChanged();
    }

    void VectorProperty::Remove(int idx)
    {
        Vector<DataDocument> prevValues, newValues;

        for (auto& obj : mTargetObjects)
        {
            prevValues.Add(DataDocument());
            mVectorType->Serialize(obj.first.data, prevValues.Last());

            mVectorType->RemoveObjectVectorElement(obj.first.data, idx);

            newValues.Add(DataDocument());
            mVectorType->Serialize(obj.first.data, newValues.Last());
        }

        Refresh();

        if (prevValues != newValues)
            onChangeCompleted(mValuesPath, prevValues, newValues);

        onChanged(Ref(this));
        o2EditorSceneScreen.OnSceneChanged();
    }

    void VectorProperty::TargetObjectData::Refresh()
    {
        if (!isCreated)
            return;

        proxy->GetValuePtr(data);
    }

    void VectorProperty::TargetObjectData::SetValue()
    {
        if (!isCreated)
            return;

        proxy->SetValuePtr(data);
    }
}

DECLARE_TEMPLATE_CLASS(o2::LinkRef<Editor::VectorProperty>);
// --- META ---

DECLARE_CLASS(Editor::VectorProperty, Editor__VectorProperty);
// --- END META ---
