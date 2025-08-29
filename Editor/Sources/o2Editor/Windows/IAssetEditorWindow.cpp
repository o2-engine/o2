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

	void IAssetEditorWindow::EditAsset(const AssetRef<Asset>& asset, const Ref<Component>& component,
                                       const Ref<IAssetEditablePreview>& preview)
	{
		if (mEditingAsset)
			OnCompletedEditingAsset();

		if (mEditingComponent)
			OnCompletedEditingComponent();

		SetComponentPreview(false);

		AssetRef<Asset> editingAsset = asset ? asset : CreateAssetInstance();

		mEditingComponent = component;
		mEditingAsset = editingAsset;
		mIsEditingAssetInstance = editingAsset.IsInstance();
		mEditingAssetEditablePreview = preview ? preview : DynamicCast<IAssetEditablePreview>(component);

		if (mEditingAsset)
		{
			OnStartEditingAsset();
			mEditingAssetInstanceCache = mEditingAsset;
		}

		if (mEditingComponent)
			OnStartEditingComponent();

		SetComponentPreview(true);

		UpdateWindowTitle();

		if (mWindow)
			mWindow->Focus();

		mEditingAssetProperty = nullptr;
	}

	void IAssetEditorWindow::EditAsset(const Ref<AssetProperty>& assetProperty, const Ref<Component>& component,
                                       const Ref<IAssetEditablePreview>& preview)
	{
		EditAsset(assetProperty->GetCommonValue(), component, preview);
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
			if (auto editablePreview = mEditingAssetEditablePreview.Lock())
			{
				editablePreview->BeginPreview();
				OnAssetEditablePreviewEnabled();
			}

			OnComponentPreviewEnabled();
		}
		else
		{
			if (auto editablePreview = mEditingAssetEditablePreview.Lock())
			{
				OnAssetEditablePreviewDisabled();
				editablePreview->EndPreview();
			}

			OnComponentPreviewDisabled();
		}
	}

	void IAssetEditorWindow::CreateNewAsset()
	{
		auto newAsset = CreateAssetInstance();
		SetComponentAndPropertyAsset(newAsset);

		UpdateWindowTitle();
	}

	AssetRef<Asset> IAssetEditorWindow::GetEditingAsset() const
	{
		return mEditingAsset.Lock();
	}

	void IAssetEditorWindow::SaveEditingAsset()
	{
		if (!mEditingAsset)
			return;

		auto editingAsset = mEditingAsset.Lock();
		if (!editingAsset || !editingAsset->IsDirty())
			return;

		if (editingAsset->GetPath().IsEmpty())
			MenuSaveAsAsset();
		else
		{
			editingAsset->Save();
			OnAssetSaved();
			o2Assets.RebuildAssets();
		}

		UpdateWindowTitle();
	}

	void IAssetEditorWindow::RevertEditingAsset()
	{
		if (!mEditingAsset)
			return;

		if (mIsEditingAssetInstance)
		{
			Ref<Asset> asset;
			mEditingAssetInstanceCache.Get(asset);
			mEditingAsset = asset;
		}
		else
			mEditingAsset.Lock()->Reload(); 

		SetComponentAndPropertyAsset(mEditingAsset.Lock());
	}

	void IAssetEditorWindow::MenuCreateNewAsset()
	{
		CheckDirtyAssetAndExecute([this]() { CreateNewAsset(); });
	}

	void IAssetEditorWindow::MenuOpenAsset()
	{
		CheckDirtyAssetAndExecute([this]() {
			Map<String, String> extensionMap = CreateFileExtensionMap();
			String fileName = GetOpenFileNameDialog("Open Asset", extensionMap);
			if (!fileName.IsEmpty())
			{
				String relativePath = o2FileSystem.GetPathRelativeToPath(fileName, ::GetAssetsPath());
				if (auto asset = o2Assets.GetAssetRef(relativePath))
					SetComponentAndPropertyAsset(asset);

				UpdateWindowTitle();
			}
		});
	}

	void IAssetEditorWindow::MenuSaveAsset()
	{
		SaveEditingAsset();
	}

	void IAssetEditorWindow::MenuSaveAsAsset()
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

			if (auto asset = mEditingAsset.Lock())
			{
				asset->SetPath(relativePath);
				asset->Save();

				SetComponentAndPropertyAsset(asset);

				OnAssetSaved();
				o2Assets.RebuildAssets();

				UpdateWindowTitle();
			}
		}
	}

	void IAssetEditorWindow::MenuRevertAsset()
	{
		if (!mEditingAsset || !mEditingAsset.Lock()->IsDirty())
			return;

		YesNoCancelDlg::ShowYesNo( "Revert changes to asset?", [this]() { RevertEditingAsset(); });
	}

	void IAssetEditorWindow::OnAssetChanged()
	{
		if (auto asset = mEditingAsset.Lock())
			asset->SetDirty(true);
	}

	void IAssetEditorWindow::Initialize()
	{
		if (IsCreateNewAssetAtStartupEnabled())
			CreateNewAsset();
	}

	void IAssetEditorWindow::Update(float dt)
	{
		CheckAssetAlive();

		mSaveAssetButton->interactable = mEditingAsset && mEditingAsset.Lock()->IsDirty();
	}

	Ref<RefCounterable> IAssetEditorWindow::CastToRefCounterable(const Ref<IAssetEditorWindow>& ref)
	{
		return DynamicCast<IEditorWindow>(ref);
	}

	void IAssetEditorWindow::UpdateWindowTitle()
	{
		if (!mWindow)
			return;

		String title = GetWindowTitle();
		if (auto asset = mEditingAsset.Lock())
		{
			String assetName;
			if (asset->GetPath().IsEmpty())
				assetName = mIsEditingAssetInstance ? "Instance" : "Unnamed";
			else
				assetName = o2FileSystem.GetFileNameWithoutExtension(asset->GetPath());

			title += " - " + assetName;
		}

		mWindow->caption = title;
	}

	void IAssetEditorWindow::CheckAssetAlive()
	{
		if (mPrevEditingAssetAlive && !mEditingAsset)
			EditAsset(nullptr);

		mPrevEditingAssetAlive = mEditingAsset != nullptr;
	}

	void IAssetEditorWindow::InitializeWindow()
	{
		mWindow->SetViewLayout(Layout::BothStretch(-1, 0, 0, 18));

		mUpPanel = mmake<HorizontalLayout>();
		mUpPanel->name = "up panel";
		*mUpPanel->layout = WidgetLayout::HorStretch(VerAlign::Top, 0, 0, 20, 0);
		mUpPanel->baseCorner = BaseCorner::Left;
		mUpPanel->expandHeight = true;
		mUpPanel->expandWidth = true;
		mUpPanel->AddLayer("back", mmake<Sprite>("ui/UI4_small_panel_back.png"), Layout::BothStretch(-5, -5, -4, -5));
		mWindow->AddChild(mUpPanel);

		mButtonsPanel = mmake<HorizontalLayout>();
		mButtonsPanel->name = "buttons panel";
		*mButtonsPanel->layout = WidgetLayout::HorStretch(VerAlign::Top, 0, 0, 20, 0);
		mButtonsPanel->baseCorner = BaseCorner::Left;
		mButtonsPanel->expandHeight = true;
		mButtonsPanel->expandWidth = false;
		mUpPanel->AddChild(mButtonsPanel);

		mPreviewToggle = o2UI.CreateWidget<Toggle>("menu preview");
		mPreviewToggle->onToggle = THIS_FUNC(OnMenuPreviewToggle);
		mButtonsPanel->AddChild(mPreviewToggle);

		mNewAssetButton = o2UI.CreateWidget<Button>("menu new asset");
		mNewAssetButton->name = "new asset button";
		mNewAssetButton->onClick += THIS_FUNC(MenuCreateNewAsset);
		mButtonsPanel->AddChild(mNewAssetButton);

		mOpenAssetButton = o2UI.CreateWidget<Button>("menu open asset");
		mOpenAssetButton->name = "open asset button";
		mOpenAssetButton->onClick += THIS_FUNC(MenuOpenAsset);
		mButtonsPanel->AddChild(mOpenAssetButton);

		mSaveAssetButton = o2UI.CreateWidget<Button>("menu save asset");
		mSaveAssetButton->name = "save button";
		mSaveAssetButton->onClick += THIS_FUNC(MenuSaveAsset);
		mButtonsPanel->AddChild(mSaveAssetButton);

		mSaveAsAssetButton = o2UI.CreateWidget<Button>("menu save as asset");
		mSaveAsAssetButton->name = "save as button";
		mSaveAsAssetButton->onClick += THIS_FUNC(MenuSaveAsAsset);
		mButtonsPanel->AddChild(mSaveAsAssetButton);

		mRevertAssetButton = o2UI.CreateWidget<Button>("menu revert asset");
		mRevertAssetButton->name = "revert button";
		mRevertAssetButton->onClick += THIS_FUNC(MenuRevertAsset);
		mButtonsPanel->AddChild(mRevertAssetButton);

		mUndoActionListener = mmake<FunctionalShortcutKeysListener>();
		mUndoActionListener->onShortcutPressed = [this]() { UndoAction(); };
		mUndoActionListener->SetShortcut(ShortcutKeys('Z', true));

		mRedoActionListener = mmake<FunctionalShortcutKeysListener>();
		mRedoActionListener->onShortcutPressed = [this]() { RedoAction(); };
		mRedoActionListener->SetShortcut(ShortcutKeys('Y', true));
	}

	String IAssetEditorWindow::GetWindowTitle() const
	{
		return "Asset Editor";
	}

	void IAssetEditorWindow::OnFocused()
	{
		mUndoActionListener->SetMaxPriority();
		mRedoActionListener->SetMaxPriority();
	}

	void IAssetEditorWindow::OnUnfocused()
	{}

	bool IAssetEditorWindow::IsComponentPreviewAvailable() const
	{
		return mEditingComponent != nullptr;
	}

	bool IAssetEditorWindow::IsCreateNewAssetAtStartupEnabled() const
	{
		return true;
	}

    void IAssetEditorWindow::SetComponentAndPropertyAsset(const AssetRef<Asset> &asset)
    {
		if (auto assetProperty = mEditingAssetProperty.Lock())
			assetProperty->SetValue(asset);

		if (mEditingComponent)
			ComponentSetAsset(asset);

		auto tmpAssetProperty = mEditingAssetProperty;
		EditAsset(asset, mEditingComponent.Lock());
		mEditingAssetProperty = tmpAssetProperty;
    }

    AssetRef<Asset> IAssetEditorWindow::CreateAssetInstance()
    {
		AssetRef result(DynamicCast<Asset>(GetAssetType().CreateSampleRef()));
		result->SetPath("Unnamed");
		return result;
	}

	void IAssetEditorWindow::OnMenuPreviewToggle(bool preview)
	{
		SetComponentPreview(preview);
	}

	void IAssetEditorWindow::CheckDirtyAssetAndExecute(const Function<void()>& callback)
	{
		if (!mEditingAsset || !mEditingAsset.Lock()->IsDirty())
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
}
// --- META ---

DECLARE_CLASS(Editor::IAssetEditorWindow, Editor__IAssetEditorWindow);
// --- END META ---
