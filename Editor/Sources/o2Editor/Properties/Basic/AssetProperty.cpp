#include "o2Editor/stdafx.h"
#include "AssetProperty.h"

#include "o2/Utils/StringUtils.h"
#include "o2Editor/Dialogs/System/OpenSaveDialog.h"
#include "o2Editor/Properties/ObjectViewer.h"
#include "o2Editor/Properties/Properties.h"
#include "o2Editor/Windows/AssetsWindow/AssetIcon.h"
#include "o2Editor/Windows/WindowsManager.h"

namespace Editor
{
    AssetProperty::AssetProperty(RefCounter* refCounter):
        TPropertyField<AssetRef<Asset>>(refCounter)
    {}

    AssetProperty::AssetProperty(RefCounter* refCounter, const AssetProperty& other):
        TPropertyField<AssetRef<Asset>>(refCounter, other)
    {
        InitializeControls();
    }

    const Type* AssetProperty::GetValueType() const
    {
        return GetValueTypeStatic();
    }

    const Type* AssetProperty::GetValueTypeStatic()
    {
        return &TypeOf(BaseAssetRef);
    }

    AssetProperty& AssetProperty::operator=(const AssetProperty& other)
    {
        TPropertyField::operator =(other);
        InitializeControls();
        return *this;
    }

    void AssetProperty::InitializeControls()
    {
        mSpoiler = GetChildByType<Spoiler>("spoiler");
        mHeaderContainer = mSpoiler->GetInternalWidgetByType<HorizontalLayout>("mainLayout/container/layout");
        mCaption = mSpoiler->GetInternalWidgetByType<Label>("mainLayout/propertyName");

        auto expandSpoilerBtn = mSpoiler->GetExpandButton();
        auto mainLayout = mSpoiler->GetInternalWidget("mainLayout");
        expandSpoilerBtn->isPointInside = [=](const Vec2F& p) {
            return expandSpoilerBtn->layout->IsPointInside(p) || mainLayout->layout->IsPointInside(p);
        };

        mBox = mSpoiler->GetInternalWidget("mainLayout/container/layout/box");
        if (mBox)
        {
            mBox->SetFocusable(true);
            mBox->onDraw += MakeFunction<CursorAreaEventsListener, void>(this, &DragDropArea::OnDrawn);

            mNameText = mBox->GetLayerDrawable<Text>("caption");
            if (mNameText)
                mNameText->text = "--";
        }

        mCreateInstanceBtn = mSpoiler->GetInternalWidgetByType<Button>("mainLayout/container/layout/create");
        if (mCreateInstanceBtn)
            mCreateInstanceBtn->onClick = THIS_FUNC(OnCreateInstancePressed);

        mEditBtn = mSpoiler->GetInternalWidgetByType<Button>("mainLayout/container/layout/edit");
        if (mEditBtn)
            mEditBtn->onClick = THIS_FUNC(OnEditPressed);

        auto saveInstanceBtn = mSpoiler->GetInternalWidgetByType<Button>("mainLayout/container/layout/save");
        if (saveInstanceBtn)
            saveInstanceBtn->onClick = THIS_FUNC(OnSaveInstancePressed);

        auto removeInstanceBtn = mSpoiler->GetInternalWidgetByType<Button>("mainLayout/container/layout/remove");
        if (removeInstanceBtn)
            removeInstanceBtn->onClick = THIS_FUNC(OnRemoveInstancePressed);
    }

    void AssetProperty::UpdateValueView()
    {
        if (mCreateInstanceBtn)
            mCreateInstanceBtn->SetEnabled(mAvailableToHaveInstance && !mValuesDifferent);

        bool allAreInstance = false;

        if (!mValuesDifferent)
        {
            if (!mCommonValue)
            {
                mNameText->text = "Null";
                if (mAssetType)
                    mNameText->text += WString(": " + GetSmartName(mAssetType->GetName()));

                mBox->layer["caption"]->transparency = 0.5f;

                if (mEditBtn)
                    mEditBtn->enabled = false;
            }
            else
            {
                UpdateAssetName();

                mBox->layer["caption"]->transparency = 1.0f;

				if (mEditBtn)
					mEditBtn->enabled = o2EditorWindows.GetAssetEditor(mAssetType) != nullptr;

                if (mAvailableToHaveInstance)
                {
                    auto instanceInfo = CollectInstanceTargets();
                    allAreInstance = instanceInfo.allAreInstance;

                    if (allAreInstance)
                    {
                        if (!mAssetObjectViewer)
                        {
                            mAssetObjectViewer = mmake<ObjectViewer>();
                            mAssetObjectViewer->SetParentContext(mParentContext.Lock());
                            mSpoiler->AddChild(mAssetObjectViewer);
                        }

                        mCreateInstanceBtn->SetEnabledForcible(false);
                        mAssetObjectViewer->SetEnabled(true);
                        mAssetObjectViewer->Refresh(instanceInfo.targets);
                    }
                    else if (mAssetObjectViewer)
                    {
                        mAssetObjectViewer->SetEnabled(false);
                    }
                }
            }
        }
        else
        {
            mNameText->text = "--";
            mBox->layer["caption"]->transparency = 1.0f;
        }

        SetState("instance", mAvailableToHaveInstance && allAreInstance && !mValuesDifferent);
    }

	void AssetProperty::UpdateAssetName()
	{
		auto name = o2FileSystem.GetFileNameWithoutExtension(
			o2FileSystem.GetPathWithoutDirectories(mCommonValue->GetPath()));

		mNameText->text = name;
	}

	AssetProperty::InstanceTargetsInfo AssetProperty::CollectInstanceTargets() const
	{
		InstanceTargetsInfo info;

		for (auto& proxy : mValuesProxies)
		{
			auto assetRef = GetProxy(proxy.first);
			if (assetRef.IsInstance())
			{
				info.targets.Add(assetRef.GetAssetBase());
				info.allAreInstance = true;
			}
			else
			{
				info.allAreInstance = false;
				break;
			}
		}

		return info;
	}

	bool AssetProperty::IsAlwaysRefresh() const
    {
        return false;
    }

    void AssetProperty::SetAssetId(const UID& id)
    {
        mCommonValue = id == UID::empty ? AssetRef<Asset>() : AssetRef<Asset>(id);

        for (auto& ptr : mValuesProxies)
            SetProxy(ptr.first, mCommonValue);

        SetCommonAssetId(id);
    }

    void AssetProperty::SetAssetType(const Type* assetType)
    {
        mAssetType = assetType;
        mAvailableToHaveInstance = assetType->InvokeStatic<bool>("IsReferenceCanOwnInstance");
    }

    void AssetProperty::SetFieldInfo(const FieldInfo* fieldInfo)
    {
        IPropertyField::SetFieldInfo(fieldInfo);
    }

    void AssetProperty::SetCaption(const WString& text)
    {
        if (mCaption)
            mCaption->SetText(text);
    }

    WString AssetProperty::GetCaption() const
    {
        if (mCaption)
            return mCaption->GetText();

        return WString();
    }

    Ref<Button> AssetProperty::GetRemoveButton()
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

    void AssetProperty::Refresh(bool forcible)
    {
        TPropertyField::Refresh(forcible);

        if (mAvailableToHaveInstance && mAssetObjectViewer)
        {
            auto instanceInfo = CollectInstanceTargets();
            if (instanceInfo.allAreInstance)
                mAssetObjectViewer->Refresh(instanceInfo.targets);
        }
    }

    void AssetProperty::SetCommonAssetId(const UID& id)
    {
        mCommonValue = id == UID::empty ? AssetRef<Asset>() : AssetRef<Asset>(id);
        mValuesDifferent = false;

        UpdateValueView();
        CheckRevertableState();
        OnValueChanged(false);
    }

    void AssetProperty::SetAssetIdByUser(const UID& id)
    {
        StoreValues(mBeforeChangeValues);
        SetAssetId(id);
        CheckValueChangeCompleted();
    }

    void AssetProperty::OnCreateInstancePressed()
	{
		for (auto& proxy : mValuesProxies)
		{
			auto proxyType = dynamic_cast<const ObjectType*>(&proxy.first->GetType());
			auto assetRef = proxyType->CreateSample();
			proxy.first->GetValuePtr(assetRef);

			if (auto* refPtr = dynamic_cast<BaseAssetRef*>(proxyType->DynamicCastToIObject(assetRef)))
				refPtr->CreateInstance();

			proxy.first->SetValuePtr(assetRef);

			proxyType->DestroySample(assetRef);
		}

        Refresh(true);
    }

    void AssetProperty::OnEditPressed()
    {
        if (mCommonValue && !mValuesDifferent)
        {
            if (auto editor = o2EditorWindows.GetAssetEditor(mAssetType))
            {
				Ref<Component> component;
                if (auto parentContext = mParentContext.Lock())
                    component = Ref(parentContext->FindOnStack<Component>());

                Ref<IAssetEditablePreview> preview;
                if (auto parentContext = mParentContext.Lock())
                    preview = Ref(parentContext->FindOnStack<IAssetEditablePreview>());

				editor->EditAsset(Ref(this), component, preview);
			}
        }
    }

    void AssetProperty::OnRemoveInstancePressed()
    {
        mCommonValue.RemoveInstance();
		SetValueByUserAndComplete(mCommonValue);
		mSpoiler->Collapse();
    }

    void AssetProperty::OnSaveInstancePressed()
    {
        String assetTypeName = GetSmartName(mAssetType->GetName());
        Vector<String> extesions = mAssetType->InvokeStatic<Vector<String>>("GetFileExtensions");
        auto extension = !extesions.IsEmpty() ? extesions[0] : String("");
        String defaultPath = ::GetAssetsPath();

        String path = GetSaveFileNameDialog("Save asset", { { assetTypeName, "*." + extension } }, defaultPath);
        if (path.IsEmpty()) {
            return;
        }

		String relativePath = o2FileSystem.GetPathRelativeToPath(path, ::GetAssetsPath());

        auto asset = GetProxy(mValuesProxies[0].first);
        asset.SaveInstance(relativePath + "." + extension);
        o2Assets.RebuildAssets();

        SetValueByUserAndComplete(asset);
    }

    void AssetProperty::OnTypeSpecialized(const Type& type)
    {
        SetAssetType(type.InvokeStatic<const Type*>("GetAssetTypeStatic"));
    }

    AssetRef<Asset> AssetProperty::GetProxy(const Ref<IAbstractValueProxy>& proxy) const
    {
        auto proxyType = dynamic_cast<const ObjectType*>(&proxy->GetType());
        auto proxySample = proxyType->CreateSample();
        proxy->GetValuePtr(proxySample);

        auto objectSample = proxyType->DynamicCastToIObject(proxySample);
        BaseAssetRef* assetSample = dynamic_cast<BaseAssetRef*>(objectSample);

        AssetRef<Asset> res = AssetRef<Asset>(*assetSample);
        delete assetSample;

        return res;
    }

    void AssetProperty::SetProxy(const Ref<IAbstractValueProxy>& proxy, const AssetRef<Asset>& value)
    {
        auto proxyType = dynamic_cast<const ObjectType*>(&proxy->GetType());
        auto proxySample = proxyType->CreateSample();
        auto objectSample = proxyType->DynamicCastToIObject(proxySample);
        auto assetSample = dynamic_cast<BaseAssetRef*>(objectSample);
        auto assetPtr = const_cast<Asset*>(value.Get());

        if (value.IsInstance())
            assetSample->SetInstance(assetPtr);
        else
            assetSample->SetAssetBase(assetPtr);

        proxy->SetValuePtr(proxySample);

        delete assetSample;
    }

    void AssetProperty::OnCursorPressed(const Input::Cursor& cursor)
    {
        o2UI.FocusWidget(mBox);

        if (mCommonValue)
            o2EditorAssets.ShowAssetIcon(mCommonValue->GetPath());
    }

    void AssetProperty::OnCursorExit(const Input::Cursor& cursor)
    {
        mBox->SetState("select", false);
    }

    void AssetProperty::OnCursorEnter(const Input::Cursor& cursor)
    {
        mBox->SetState("select", true);
    }

    void AssetProperty::OnKeyPressed(const Input::Key& key)
    {
        if (mBox && mBox->IsFocused() && (key == VK_DELETE || key == VK_BACK))
            SetAssetIdByUser(UID::empty);
    }

    bool AssetProperty::IsUnderPoint(const Vec2F& point)
    {
        return mBox->IsUnderPoint(point) && mBox->transparency > 0.1f;
    }

    void AssetProperty::OnDragExit(const Ref<ISelectableDragableObjectsGroup>& group)
    {
        o2Application.SetCursor(CursorType::Arrow);
        mBox->SetState("focused", false);
    }

	void AssetProperty::OnUpdate(float dt)
	{
		const float checkAssetNameDelay = 0.3f;

        if (!mValuesDifferent && mCommonValue)
        {
            mCheckAssetNameChangeTime -= dt;
            if (mCheckAssetNameChangeTime < 0.0f)
            {
                UpdateAssetName();
                mCheckAssetNameChangeTime = checkAssetNameDelay;
            }
        }
	}

	void AssetProperty::OnDragEnter(const Ref<ISelectableDragableObjectsGroup>& group)
    {
        auto assetIconsScroll = DynamicCast<AssetsIconsScrollArea>(group);
        if (!assetIconsScroll)
            return;

        if (mAssetType)
        {
            auto lastSelectedAsset = assetIconsScroll->GetSelectedAssets().Last();
            if (!lastSelectedAsset->meta->GetAssetType()->IsBasedOn(*mAssetType))
                return;
        }

        o2Application.SetCursor(CursorType::Hand);
        mBox->SetState("focused", true);
    }

    void AssetProperty::OnDropped(const Ref<ISelectableDragableObjectsGroup>& group)
    {
        auto assetIconsScroll = DynamicCast<AssetsIconsScrollArea>(group);
        if (!assetIconsScroll)
            return;

        auto lastSelectedAssetIcon = DynamicCast<AssetIcon>(assetIconsScroll->GetDraggingObject());
        if (!lastSelectedAssetIcon ||
            (mAssetType && !lastSelectedAssetIcon->GetAssetInfo()->meta->GetAssetType()->IsBasedOn(*mAssetType)))
        {
            return;
        }

        SetAssetIdByUser(lastSelectedAssetIcon->GetAssetInfo()->meta->ID());

        o2Application.SetCursor(CursorType::Arrow);
        mBox->Focus();
    }
}

DECLARE_TEMPLATE_CLASS(Editor::TPropertyField<AssetRef<Asset>>);
DECLARE_TEMPLATE_CLASS(o2::LinkRef<Editor::AssetProperty>);
DECLARE_TEMPLATE_CLASS(o2::LinkRef<Editor::TPropertyField<AssetRef<Asset>>>);
// --- META ---

DECLARE_CLASS(Editor::AssetProperty, Editor__AssetProperty);
// --- END META ---
