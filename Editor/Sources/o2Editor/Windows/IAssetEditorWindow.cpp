#include "o2Editor/stdafx.h"
#include "IAssetEditorWindow.h"

#include "o2/Assets/Assets.h"
#include "o2/Render/Sprite.h"
#include "o2/Scene/UI/UIManager.h"
#include "o2/Scene/UI/WidgetLayout.h"
#include "o2/Utils/FileSystem/FileSystem.h"
#include "o2Editor/Dialogs/System/OpenSaveDialog.h"
#include "o2Editor/Dialogs/YesNoCancelDlg.h"

namespace Editor
{
	IAssetEditorWindow::IAssetEditorWindow() :
		IAssetEditorWindow(nullptr)
	{
	}

	IAssetEditorWindow::IAssetEditorWindow(RefCounter* refCounter) :
		IEditorWindow(refCounter)
	{
		InitializeWindow();
	}

	IAssetEditorWindow::IAssetEditorWindow(const IAssetEditorWindow& other) :
		IAssetEditorWindow(nullptr, other)
	{
	}

	IAssetEditorWindow::IAssetEditorWindow(RefCounter* refCounter, const IAssetEditorWindow& other) :
		IEditorWindow(refCounter, other), mEditingAsset(other.mEditingAsset)
	{
		InitializeWindow();
	}

	IAssetEditorWindow::~IAssetEditorWindow()
	{
	}

	const Type& IAssetEditorWindow::GetAssetType() const
	{
		return TypeOf(Asset);
	}

	void IAssetEditorWindow::EditAsset(const AssetRef<Asset>& asset)
	{
		EditAsset(asset, nullptr);
	}

	void IAssetEditorWindow::EditAsset(const AssetRef<Asset>& asset, const Ref<Component>& component)
	{
		if (mEditingAsset)
			OnCompletedEditingAsset();

		if (mEditingComponent)
		{
			if (mPreviewEnabled)
			{
				OnComponentPreviewDisabled();
				mPreviewEnabled = false;
			}

			OnCompletedEditingComponent();
		}

		mEditingComponent = component;
		mEditingAsset = asset ? asset : CreateAssetInstance();
		mEditingAssetEditablePreview = DynamicCast<IAssetEditablePreview>(mEditingComponent);

		if (mEditingAsset)
		{
			OnStartEditingAsset();
			mEditingAssetInstanceCache = mEditingAsset;
		}

		if (mEditingComponent)
			OnStartEditingComponent();

		SetComponentPreview(true);

		if (mWindow)
			mWindow->Focus();

		mEditingAssetProperty = nullptr;
	}

	void IAssetEditorWindow::EditAsset(const Ref<AssetProperty>& assetProperty, const Ref<Component>& component)
	{
		EditAsset(assetProperty->GetCommonValue(), component);
		mEditingAssetProperty = assetProperty;
	}

	void IAssetEditorWindow::SetComponentPreview(bool enable)
	{
		if (!IsComponentPreviewAvailable())
		{
			mPreviewToggle->enabled = false;
			return;
		}

		mPreviewToggle->enabled = true;
		mPreviewToggle->value = enable;
		mPreviewEnabled = enable;

		if (enable)
		{
			if (mEditingAssetEditablePreview)
				mEditingAssetEditablePreview->BeginPreview();

			OnComponentPreviewEnabled();
		}
		else
		{
			if (mEditingAssetEditablePreview)
				mEditingAssetEditablePreview->EndPreview();

			OnComponentPreviewDisabled();
		}
	}

	void IAssetEditorWindow::CreateNewAsset()
	{
		auto newAsset = CreateAssetInstance();
		SetComponentAndPropertyAsset(newAsset);
	}

	const AssetRef<Asset>& IAssetEditorWindow::GetEditingAsset() const
	{
		return mEditingAsset;
	}

	void IAssetEditorWindow::SaveEditingAsset()
	{
		if (!mEditingAsset || !mEditingAsset->IsDirty())
			return;

		if (mEditingAsset->GetPath().IsEmpty())
			OnSaveAsAssetPressed();
		else
		{
			mEditingAsset->Save();
			OnAssetSaved();
			o2Assets.RebuildAssets();
		}
	}

	void IAssetEditorWindow::RevertEditingAsset()
	{
		if (!mEditingAsset)
			return;

		if (mEditingAsset.IsInstance())
			mEditingAsset = mEditingAssetInstanceCache;
		else
			mEditingAsset->Reload();

		SetComponentAndPropertyAsset(mEditingAsset);
	}

	void IAssetEditorWindow::OnAssetChanged()
	{
		if (mEditingAsset)
			mEditingAsset->SetDirty(true);
	}

	void IAssetEditorWindow::Initialize()
	{
		CreateNewAsset();
	}

	void IAssetEditorWindow::Update(float dt)
	{
		mSaveAssetButton->interactable = mEditingAsset && mEditingAsset->IsDirty();
	}

	void IAssetEditorWindow::InitializeWindow()
	{
		mWindow->SetViewLayout(Layout::BothStretch(-1, 0, 0, 18));

		mUpPanel = mmake<HorizontalLayout>();
		mUpPanel->name = "up panel";
		*mUpPanel->layout = WidgetLayout::HorStretch(VerAlign::Top, 0, 0, 20, 0);
		mUpPanel->baseCorner = BaseCorner::Left;
		mUpPanel->expandHeight = true;
		mUpPanel->expandWidth = false;
		mUpPanel->AddLayer("back", mmake<Sprite>("ui/UI4_small_panel_back.png"), Layout::BothStretch(-5, -4, -4, -5));
		mWindow->AddChild(mUpPanel);

		mPreviewToggle = o2UI.CreateWidget<Toggle>("menu preview");
		mPreviewToggle->onToggle = THIS_FUNC(OnMenuPreviewToggle);
		mUpPanel->AddChild(mPreviewToggle);

		mNewAssetButton = o2UI.CreateWidget<Button>("menu new asset");
		mNewAssetButton->name = "new asset button";
		mNewAssetButton->onClick += THIS_FUNC(OnNewAssetPressed);
		mUpPanel->AddChild(mNewAssetButton);

		mOpenAssetButton = o2UI.CreateWidget<Button>("menu open asset");
		mOpenAssetButton->name = "open asset button";
		mOpenAssetButton->onClick += THIS_FUNC(OnOpenAssetPressed);
		mUpPanel->AddChild(mOpenAssetButton);

		mSaveAssetButton = o2UI.CreateWidget<Button>("menu save asset");
		mSaveAssetButton->name = "save button";
		mSaveAssetButton->onClick += THIS_FUNC(SaveEditingAsset);
		mUpPanel->AddChild(mSaveAssetButton);

		mSaveAsAssetButton = o2UI.CreateWidget<Button>("menu save as asset");
		mSaveAsAssetButton->name = "save as button";
		mSaveAsAssetButton->onClick += THIS_FUNC(OnSaveAsAssetPressed);
		mUpPanel->AddChild(mSaveAsAssetButton);

		mRevertAssetButton = o2UI.CreateWidget<Button>("menu revert asset");
		mRevertAssetButton->name = "revert button";
		mRevertAssetButton->onClick += THIS_FUNC(OnRevertAssetPressed);
		mUpPanel->AddChild(mRevertAssetButton);
	}

	bool IAssetEditorWindow::IsComponentPreviewAvailable() const
	{
		return mEditingComponent != nullptr;
	}

    void IAssetEditorWindow::SetComponentAndPropertyAsset(const AssetRef<Asset> &asset)
    {
		if (mEditingAssetProperty)
			mEditingAssetProperty->SetValue(asset);

		if (mEditingComponent)
			ComponentSetAsset(asset);

		auto tmpAssetProperty = mEditingAssetProperty;
		EditAsset(asset, mEditingComponent);
		mEditingAssetProperty = tmpAssetProperty;
    }

    AssetRef<Asset> IAssetEditorWindow::CreateAssetInstance()
    {
		return AssetRef(DynamicCast<Asset>(GetAssetType().CreateSampleRef()));
	}

	void IAssetEditorWindow::OnMenuPreviewToggle(bool preview)
	{
		SetComponentPreview(preview);
	}

	void IAssetEditorWindow::CheckDirtyAssetAndExecute(const Function<void()>& callback)
	{
		if (!mEditingAsset || !mEditingAsset->IsDirty())
		{
			callback();
			return;
		}

		YesNoCancelDlg::ShowYesNoCancel("Asset has been modified. Save changes?",
										[this, callback]() { SaveEditingAsset(); callback(); },
										callback);
	}

	Map<String, String> IAssetEditorWindow::CreateFileExtensionMap() const
	{
		auto& assetType = GetAssetType();
		auto extensions = assetType.InvokeStatic<Vector<String>>("GetFileExtensions");

		Map<String, String> extensionMap;
		for (const auto& ext : extensions)
			extensionMap[assetType.GetName() + " files"] = "*." + ext;

		return extensionMap;
	}

	void IAssetEditorWindow::OnNewAssetPressed()
	{
		CheckDirtyAssetAndExecute([this]() { CreateNewAsset(); });
	}

	void IAssetEditorWindow::OnOpenAssetPressed()
	{
		CheckDirtyAssetAndExecute([this]() {
			Map<String, String> extensionMap = CreateFileExtensionMap();
			String fileName = GetOpenFileNameDialog("Open Asset", extensionMap);
			if (!fileName.IsEmpty())
			{
				String relativePath = o2FileSystem.GetPathRelativeToPath(fileName, ::GetAssetsPath());
				if (auto asset = o2Assets.GetAssetRef(relativePath))
					SetComponentAndPropertyAsset(asset);
			}
		});
	}

	void IAssetEditorWindow::OnSaveAsAssetPressed()
	{
		Map<String, String> extensionMap = CreateFileExtensionMap();
		String defaultPath = ::GetAssetsPath();
		String fileName = GetSaveFileNameDialog("Save Asset As", extensionMap, defaultPath);

		if (!fileName.IsEmpty() && mEditingAsset)
		{
			String relativePath = o2FileSystem.GetPathRelativeToPath(fileName, defaultPath);

			auto extensions = GetAssetType().InvokeStatic<Vector<String>>("GetFileExtensions");
			if (!extensions.IsEmpty() && !relativePath.EndsWith(extensions[0]))
				relativePath += "." + extensions[0];

			mEditingAsset.SaveInstance(relativePath);
			SetComponentAndPropertyAsset(mEditingAsset);
			OnAssetSaved();
			o2Assets.RebuildAssets();
		}
	}

	void IAssetEditorWindow::OnRevertAssetPressed()
	{
		if (!mEditingAsset || !mEditingAsset->IsDirty())
			return;

		YesNoCancelDlg::ShowYesNo( "Revert changes to asset?", [this]() { RevertEditingAsset(); });
	}
}
// --- META ---

DECLARE_CLASS(Editor::IAssetEditorWindow, Editor__IAssetEditorWindow);
// --- END META ---
