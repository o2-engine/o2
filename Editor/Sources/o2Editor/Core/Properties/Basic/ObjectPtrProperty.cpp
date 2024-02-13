#include "o2Editor/stdafx.h"
#include "ObjectPtrProperty.h"

#include "o2/Scene/UI/UIManager.h"
#include "o2/Scene/UI/Widgets/Button.h"
#include "o2/Scene/UI/Widgets/ContextMenu.h"
#include "o2/Scene/UI/Widgets/Label.h"
#include "o2/Scene/UI/Widgets/Spoiler.h"
#include "o2/Utils/Editor/Attributes/DefaultTypeAttribute.h"
#include "o2/Utils/Editor/Attributes/DontDeleteAttribute.h"
#include "o2/Utils/Editor/Attributes/NoHeaderAttribute.h"
#include "o2/Utils/Editor/EditorScope.h"
#include "o2Editor/Core/Properties/IObjectPropertiesViewer.h"
#include "o2Editor/Core/Properties/Properties.h"

using namespace o2;

namespace Editor
{
	ObjectPtrProperty::ObjectPtrProperty()
	{
		InitializeControls();
	}

	ObjectPtrProperty::ObjectPtrProperty(const ObjectPtrProperty& other):
		IPropertyField(other)
	{
		InitializeControls();
	}

	ObjectPtrProperty& ObjectPtrProperty::operator=(const ObjectPtrProperty& other)
	{
		IPropertyField::operator=(other);
		InitializeControls();
		return *this;
	}

	void ObjectPtrProperty::OnFreeProperty()
	{
		if (mObjectViewer)
		{
			RemoveChild(mObjectViewer->GetSpoiler(), false);
			o2EditorProperties.FreeObjectViewer(mObjectViewer);
		}

		mObjectViewer = nullptr;
	}

	void ObjectPtrProperty::InitializeControls()
	{
		PushEditorScopeOnStack scope;

		borderLeft = 11;

		mCaption = o2UI.CreateLabel("Caption");
		mCaption->SetHorAlign(HorAlign::Left);
		AddChild(mCaption);

		mHeaderContainer = mmake<HorizontalLayout>();
		*mHeaderContainer->layout = WidgetLayout::HorStretch(VerAlign::Top, 100, 0, 19, 0);
		mHeaderContainer->baseCorner = BaseCorner::Right;
		mHeaderContainer->expandHeight = false;
		mCaption->AddChildWidget(mHeaderContainer);

		mTypeCaption = o2UI.CreateLabel("nullptr");
		*mTypeCaption->layout = WidgetLayout(0, 1, 1, 0, 0, 0, 75, 1);
		mTypeCaption->horOverflow = Label::HorOverflow::Dots;
		mTypeCaption->horAlign = HorAlign::Left;
		mTypeCaption->verAlign = VerAlign::Bottom;
		mTypeCaption->height = 8;
		mTypeCaption->transparency = 0.6f;
		mHeaderContainer->AddInternalWidget(mTypeCaption);

		mCreateDeleteButton = o2UI.CreateButton("Create");
		mCreateDeleteButton->layout->maxWidth = 75;
		mCreateDeleteButton->layout->minHeight = 15;
		mCreateDeleteButton->layout->height = 15;
		mCreateDeleteButton->onClick = THIS_FUNC(OnCreateOrDeletePressed);
		mHeaderContainer->AddChild(mCreateDeleteButton);

		mCreateMenu = o2UI.CreateWidget<ContextMenu>();
		mCreateDeleteButton->AddChild(mCreateMenu);

		expandHeight = true;
		expandWidth = true;
		fitByChildren = true;
	}

	void ObjectPtrProperty::SetValueAndPrototypeProxy(const TargetsVec& targets)
	{
		mValuesProxies = targets;
		Refresh();
	}

	void ObjectPtrProperty::Refresh()
	{
		PushEditorScopeOnStack scope;

		mCurrentObjectType = nullptr;
		if (!mValuesProxies.IsEmpty())
		{
			auto object = GetProxy(mValuesProxies[0].first);
			if (object)
				mCurrentObjectType = dynamic_cast<const ObjectType*>(&object->GetType());
			else if (mFieldInfo)
			{
				if (auto defaultTypeAttr = mFieldInfo->GetAttribute<DefaultTypeAttribute>())
				{
					CreateObject(dynamic_cast<const ObjectType*>(defaultTypeAttr->defaultType));
					return;
				}
			}
		}

		const Type* prevObjectType = mObjectViewer ? mObjectViewer->GetViewingObjectType() : nullptr;
		if (mBuiltObjectType != prevObjectType || mCurrentObjectType != mBuiltObjectType)
		{
			if (mObjectViewer)
			{
				o2EditorProperties.FreeObjectViewer(mObjectViewer);
				mCaption->SetEnabledForcible(true);
				mCaption->AddChildWidget(mHeaderContainer);
				borderLeft = 11;
			}

			mObjectViewer = nullptr;
			mBuiltObjectType = mCurrentObjectType;

			if (mBuiltObjectType)
			{
				mObjectViewer = o2EditorProperties.CreateObjectViewer(mBuiltObjectType, mValuesPath, onChangeCompleted,
																	  onChanged);

				mObjectViewer->SetParentContext(mParentContext);
				mObjectViewer->SetHeaderEnabled(!mNoHeader);
				mObjectViewer->SetExpanded(mExpanded);
				AddChild(mObjectViewer->GetSpoiler());

				mCaption->SetEnabledForcible(false);
				mHeaderContainer->SetInternalParent(mObjectViewer->GetSpoiler());

				mTypeCaption->text = mCurrentObjectType->GetName();
				mTypeCaption->SetEnabledForcible(mAvailableMultipleTypes && !mNoHeader);

				mCreateDeleteButton->caption = "Delete";
				mCreateDeleteButton->enabledForcibly = !mDontDeleteEnabled;

				borderLeft = 0;

				UpdateViewerHeader();
			}
			else
			{
				mTypeCaption->SetEnabledForcible(true);
				mTypeCaption->text = "nullptr";
				mCreateDeleteButton->caption = "Create";
				mCreateDeleteButton->enabledForcibly = true;
			}
		}

		if (mObjectViewer)
		{
			mObjectViewer->Refresh(mValuesProxies.Convert<Pair<IObject*, IObject*>>(
				[&](const Pair<IAbstractValueProxy*, IAbstractValueProxy*>& x)
			{
				return Pair<IObject*, IObject*>(GetProxy(x.first), x.second ? GetProxy(x.second) : nullptr);
			}));
		}

		SetEnabled((mObjectViewer && !mObjectViewer->IsEmpty()) || !mNoHeader);
	}

	const Type* ObjectPtrProperty::GetValueType() const
	{
		return GetValueTypeStatic();
	}

	const Type* ObjectPtrProperty::GetValueTypeStatic()
	{
		return &TypeOf(IObject*);
	}

	void ObjectPtrProperty::SetFieldInfo(const FieldInfo* fieldInfo)
	{
		IPropertyField::SetFieldInfo(fieldInfo);

		if (fieldInfo)
		{
			mExpanded = fieldInfo->HasAttribute<ExpandedByDefaultAttribute>();
			mNoHeader = fieldInfo->HasAttribute<NoHeaderAttribute>();
			mDontDeleteEnabled = fieldInfo->HasAttribute<DontDeleteAttribute>();
		}
	}

	void ObjectPtrProperty::SetCaption(const WString& text)
	{
		mCaption->SetText(text);

		UpdateViewerHeader();
	}

	WString ObjectPtrProperty::GetCaption() const
	{
		return mCaption->GetText();
	}

	Button* ObjectPtrProperty::GetRemoveButton()
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

	void ObjectPtrProperty::SetBasicType(const ObjectType* type)
	{
		mBasicObjectType = type;
		mAvailableMultipleTypes = !mBasicObjectType->GetDerivedTypes().IsEmpty();
	}

	void ObjectPtrProperty::Expand()
	{
		SetExpanded(true);
	}

	void ObjectPtrProperty::Collapse()
	{
		SetExpanded(false);
	}

	void ObjectPtrProperty::SetExpanded(bool expanded)
	{
		mExpanded = expanded;

		if (mObjectViewer)
			mObjectViewer->GetSpoiler()->SetExpanded(expanded);
	}

	bool ObjectPtrProperty::IsExpanded() const
	{
		if (mObjectViewer)
			return mObjectViewer->GetSpoiler()->IsExpanded();

		return mExpanded;
	}

	void ObjectPtrProperty::OnCreateOrDeletePressed()
	{
		PushEditorScopeOnStack scope;

		bool hasObject = !mValuesProxies.IsEmpty() && GetProxy(mValuesProxies[0].first) != nullptr;
		if (hasObject)
		{
			for (auto& targetObj : mValuesProxies)
			{
				IObject* object = GetProxy(targetObj.first);

				if (object != nullptr)
				{
					delete object;
					SetProxy(targetObj.first, nullptr);
				}
			}

			Refresh();

			if (mObjectViewer)
			{
				mObjectViewer->GetSpoiler()->SetLayoutDirty();
				mObjectViewer->GetSpoiler()->Collapse();
			}
		}
		else
		{
			if (!mContextInitialized)
			{
				mCreateMenu->RemoveAllItems();

				auto availableTypes = mBasicObjectType->GetDerivedTypes();
				availableTypes.Insert(mBasicObjectType, 0);

				mImmediateCreateObject = availableTypes.Count() == 1;

				mCreateMenu->AddItems(availableTypes.Convert<ContextMenu::Item*>([&](const Type* type)
				{
					return mnew ContextMenu::Item(type->GetName(), [=]() { CreateObject(dynamic_cast<const ObjectType*>(type)); });
				}));

				mContextInitialized = true;
			}

			if (mImmediateCreateObject)
				CreateObject(mBasicObjectType);
			else
				mCreateMenu->Show();
		}
	}

	void ObjectPtrProperty::CreateObject(const ObjectType* type)
	{
		PushEditorScopeOnStack scope;

		StoreValues(mBeforeChangeValues);
		for (auto& targetObj : mValuesProxies)
		{
			if (GetProxy(targetObj.first) == nullptr)
				SetProxy(targetObj.first, type->DynamicCastToIObject(type->CreateSample()));
		}

		CheckValueChangeCompleted();

		Refresh();

		if (mObjectViewer)
			mObjectViewer->GetSpoiler()->SetLayoutDirty();
	}

	void ObjectPtrProperty::StoreValues(Vector<DataDocument>& data) const
	{
		data.Clear();
		for (auto& targetObj : mValuesProxies)
		{
			data.Add(DataDocument());
			data.Last() = GetProxy(targetObj.first);
		}
	}

	class DummyObj : public RefCounterable
	{

	};

	IObject* ObjectPtrProperty::GetProxy(IAbstractValueProxy* proxy) const
	{
		const Type& proxyType = proxy->GetType();
		if (proxyType.GetUsage() == Type::Usage::Pointer)
		{
			const Type& baseType = *dynamic_cast<const PointerType&>(proxyType).GetBaseType();
			if (baseType.IsBasedOn(TypeOf(IObject)))
			{
				const ObjectType& objectType = dynamic_cast<const ObjectType&>(baseType);

				void* valuePtr;
				proxy->GetValuePtr(&valuePtr);

				return objectType.DynamicCastToIObject(valuePtr);
			}
		}
		else if (proxyType.GetUsage() == Type::Usage::Reference)
        {
            const Type& baseType = *dynamic_cast<const ReferenceType&>(proxyType).GetBaseType();
            if (baseType.IsBasedOn(TypeOf(IObject)))
            {
                const ObjectType& objectType = dynamic_cast<const ObjectType&>(baseType);
				const ReferenceType& refType = dynamic_cast<const ReferenceType&>(proxyType);

                void* valuePtr = proxyType.CreateSample();
                proxy->GetValuePtr(valuePtr);

                IObject* res = objectType.DynamicCastToIObject(refType.GetObjectRawPtr(valuePtr));

				proxyType.DestroySample(valuePtr);

				return res;
            }
		}

		return nullptr;
	}

	void ObjectPtrProperty::SetProxy(IAbstractValueProxy* proxy, IObject* object)
	{
        const Type& proxyType = proxy->GetType();
        if (proxyType.GetUsage() == Type::Usage::Pointer)
        {
            const Type& baseType = *dynamic_cast<const PointerType&>(proxyType).GetBaseType();
            if (baseType.IsBasedOn(TypeOf(IObject)))
            {
                const ObjectType& objectType = dynamic_cast<const ObjectType&>(baseType);

                void* valuePtr = objectType.DynamicCastFromIObject(object);
                proxy->SetValuePtr(&valuePtr);
            }
        }
        else if (proxyType.GetUsage() == Type::Usage::Reference)
        {
            const Type& baseType = *dynamic_cast<const ReferenceType&>(proxyType).GetBaseType();
            if (baseType.IsBasedOn(TypeOf(IObject)))
            {
                const ObjectType& objectType = dynamic_cast<const ObjectType&>(baseType);
                const ReferenceType& refType = dynamic_cast<const ReferenceType&>(proxyType);

                void* valuePtr = objectType.DynamicCastFromIObject(object);
				
                proxy->SetValuePtr(refType.CreateSample(valuePtr));
            }
        }
	}

	void ObjectPtrProperty::UpdateViewerHeader()
	{
		if (mObjectViewer)
			mObjectViewer->SetCaption(mCaption->GetText());

		Text* spoilerCaptionLayer = !mObjectViewer ?
			mCaption->GetLayerDrawableByType<Text>() :
			mObjectViewer->GetSpoiler()->GetLayerDrawable<Text>("caption");

		if (spoilerCaptionLayer)
		{
			Vec2F captionSize = Text::GetTextSize(mCaption->GetText(), spoilerCaptionLayer->GetFont(), spoilerCaptionLayer->GetFontHeight());
			*mHeaderContainer->layout = WidgetLayout::HorStretch(VerAlign::Top, captionSize.x + 20.0f, 0, 17, 0);
		}
	}

}
// --- META ---

DECLARE_CLASS(Editor::ObjectPtrProperty, Editor__ObjectPtrProperty);
// --- END META ---
