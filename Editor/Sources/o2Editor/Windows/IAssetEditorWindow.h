#pragma once

#include "IEditorWindow.h"

#include "o2/Assets/Asset.h"
#include "o2/Assets/AssetRef.h"
#include "o2Editor/Properties/Basic/AssetProperty.h"
#include "o2/Scene/UI/Widget.h"
#include "o2/Utils/Editor/AssetEditablePreview.h"
#include "o2/Utils/Function/Function.h"

using namespace o2;

namespace Editor
{
	// ------------------------------------------------
	// Asset editor window interface for editing assets
	// Base interface for all asset editor windows
	// ------------------------------------------------
    class IAssetEditorWindow: public IEditorWindow
    {
    public:
        // Default constructor
        IAssetEditorWindow();

        // Constructor with ref counter
        explicit IAssetEditorWindow(RefCounter* refCounter);

        // Copy-constructor
        IAssetEditorWindow(const IAssetEditorWindow& other);

        // Copy-constructor with ref counter
        IAssetEditorWindow(RefCounter* refCounter, const IAssetEditorWindow& other);

        // Virtual destructor
        ~IAssetEditorWindow() override;

		// Returns asset type that this editor window can edit
        virtual const Type& GetAssetType() const;

		// Sets asset to edit
        virtual void EditAsset(const AssetRef<Asset>& asset);

		// Sets asset to edit with component
		virtual void EditAsset(const AssetRef<Asset>& asset, const Ref<Component>& component, 
                               const Ref<IAssetEditablePreview>& preview = nullptr);

        // Sets asset to edit with asset property and component
        virtual void EditAsset(const Ref<AssetProperty>& assetProperty, const Ref<Component>& component,
                               const Ref<IAssetEditablePreview>& preview = nullptr);

		// Enables or disables component preview mode
		void SetComponentPreview(bool enable);

		// Creates a new asset and starts editing it
        virtual void CreateNewAsset();

        // Saves the currently editing asset
		virtual void SaveEditingAsset();

		// Reverts the currently editing asset to its last saved state
		virtual void RevertEditingAsset();

		// Returns currently editing asset
		virtual const AssetRef<Asset>& GetEditingAsset() const;

        // Called when asset has changed, marks asset as dirty
        virtual void OnAssetChanged();

        // Initializes window, called after all windows created. Creates new asset
		void Initialize() override;

		// Updates window logic; updates save icon state
		void Update(float dt) override;

        IOBJECT(IAssetEditorWindow);

    protected:
        AssetRef<Asset>            mEditingAsset;                // Currently editing asset
        Ref<AssetProperty>         mEditingAssetProperty;        // Property of the asset being edited
        Ref<Component>             mEditingComponent;            // Component of the asset being edited
        Ref<IAssetEditablePreview> mEditingAssetEditablePreview; // Asset editable preview of the asset being edited

		DataDocument mEditingAssetInstanceCache; // Cache for asset instance data, used to restore asset state

        Ref<HorizontalLayout> mUpPanel; // Upper panel with buttons

        Ref<Toggle> mPreviewToggle;          // Toggle for enabling preview mode while editing asset
        bool        mPreviewEnabled = false; // Flag for preview mode

		Ref<Button> mNewAssetButton;    // New asset button
		Ref<Button> mOpenAssetButton;   // Open asset button
		Ref<Button> mSaveAssetButton;   // Save button
		Ref<Button> mSaveAsAssetButton; // Save as button
		Ref<Button> mRevertAssetButton; // Reset button

    protected:
        // Initializes window and controls
        virtual void InitializeWindow();

        // Called when asset editing starts
        virtual void OnStartEditingAsset() {}

        // Called when asset editing ends
        virtual void OnCompletedEditingAsset() {}

		// Called when component editing starts
		virtual void OnStartEditingComponent() {}

		// Called when component editing ends
		virtual void OnCompletedEditingComponent() {}

		// Returns true if component preview is available for this asset type
        virtual bool IsComponentPreviewAvailable() const;

		// Called when component preview is enabled
        virtual void OnComponentPreviewEnabled() {}

		// Called when component preview is disabled
		virtual void OnComponentPreviewDisabled() {}

        // Called when asset editable preview is enabled
        virtual void OnAssetEditablePreviewEnabled() {}

        // Called when asset editable preview is disabled
        virtual void OnAssetEditablePreviewDisabled() {}

		// Sets current component asset
        virtual void ComponentSetAsset(const AssetRef<Asset>& asset) {}

        // Sets current component and property asset
        virtual void SetComponentAndPropertyAsset(const AssetRef<Asset>& asset);

        // Called when asset is saved
        virtual void OnAssetSaved() {}

		// Creates new asset instance by editing asset type
        virtual AssetRef<Asset> CreateAssetInstance();

		// Called to toggle preview mode
        virtual void OnMenuPreviewToggle(bool preview);

        // Called when new asset button is pressed
        virtual void OnNewAssetPressed();

		// Opens file selector and opens asset
        virtual void OnOpenAssetPressed();

		// Called when save as button is pressed
		virtual void OnSaveAsAssetPressed();

		// Called when revert button is pressed
		virtual void OnRevertAssetPressed();

		// Checks if current asset is dirty and shows save dialog, then executes callback
		void CheckDirtyAssetAndExecute(const Function<void()>& callback);

		// Creates extension map for file dialogs based on current asset type
		Map<String, String> CreateFileExtensionMap() const;
    };
}
// --- META ---

CLASS_BASES_META(Editor::IAssetEditorWindow)
{
    BASE_CLASS(Editor::IEditorWindow);
}
END_META;
CLASS_FIELDS_META(Editor::IAssetEditorWindow)
{
    FIELD().PROTECTED().NAME(mEditingAsset);
    FIELD().PROTECTED().NAME(mEditingAssetProperty);
    FIELD().PROTECTED().NAME(mEditingComponent);
    FIELD().PROTECTED().NAME(mEditingAssetEditablePreview);
    FIELD().PROTECTED().NAME(mEditingAssetInstanceCache);
    FIELD().PROTECTED().NAME(mUpPanel);
    FIELD().PROTECTED().NAME(mPreviewToggle);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mPreviewEnabled);
    FIELD().PROTECTED().NAME(mNewAssetButton);
    FIELD().PROTECTED().NAME(mOpenAssetButton);
    FIELD().PROTECTED().NAME(mSaveAssetButton);
    FIELD().PROTECTED().NAME(mSaveAsAssetButton);
    FIELD().PROTECTED().NAME(mRevertAssetButton);
}
END_META;
CLASS_METHODS_META(Editor::IAssetEditorWindow)
{

    typedef Map<String, String> _tmp1;

    FUNCTION().PUBLIC().CONSTRUCTOR();
    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*);
    FUNCTION().PUBLIC().CONSTRUCTOR(const IAssetEditorWindow&);
    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*, const IAssetEditorWindow&);
    FUNCTION().PUBLIC().SIGNATURE(const Type&, GetAssetType);
    FUNCTION().PUBLIC().SIGNATURE(void, EditAsset, const AssetRef<Asset>&);
    FUNCTION().PUBLIC().SIGNATURE(void, EditAsset, const AssetRef<Asset>&, const Ref<Component>&, const Ref<IAssetEditablePreview>&);
    FUNCTION().PUBLIC().SIGNATURE(void, EditAsset, const Ref<AssetProperty>&, const Ref<Component>&, const Ref<IAssetEditablePreview>&);
    FUNCTION().PUBLIC().SIGNATURE(void, SetComponentPreview, bool);
    FUNCTION().PUBLIC().SIGNATURE(void, CreateNewAsset);
    FUNCTION().PUBLIC().SIGNATURE(void, SaveEditingAsset);
    FUNCTION().PUBLIC().SIGNATURE(void, RevertEditingAsset);
    FUNCTION().PUBLIC().SIGNATURE(const AssetRef<Asset>&, GetEditingAsset);
    FUNCTION().PUBLIC().SIGNATURE(void, OnAssetChanged);
    FUNCTION().PUBLIC().SIGNATURE(void, Initialize);
    FUNCTION().PUBLIC().SIGNATURE(void, Update, float);
    FUNCTION().PROTECTED().SIGNATURE(void, InitializeWindow);
    FUNCTION().PROTECTED().SIGNATURE(void, OnStartEditingAsset);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCompletedEditingAsset);
    FUNCTION().PROTECTED().SIGNATURE(void, OnStartEditingComponent);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCompletedEditingComponent);
    FUNCTION().PROTECTED().SIGNATURE(bool, IsComponentPreviewAvailable);
    FUNCTION().PROTECTED().SIGNATURE(void, OnComponentPreviewEnabled);
    FUNCTION().PROTECTED().SIGNATURE(void, OnComponentPreviewDisabled);
    FUNCTION().PROTECTED().SIGNATURE(void, OnAssetEditablePreviewEnabled);
    FUNCTION().PROTECTED().SIGNATURE(void, OnAssetEditablePreviewDisabled);
    FUNCTION().PROTECTED().SIGNATURE(void, ComponentSetAsset, const AssetRef<Asset>&);
    FUNCTION().PROTECTED().SIGNATURE(void, SetComponentAndPropertyAsset, const AssetRef<Asset>&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnAssetSaved);
    FUNCTION().PROTECTED().SIGNATURE(AssetRef<Asset>, CreateAssetInstance);
    FUNCTION().PROTECTED().SIGNATURE(void, OnMenuPreviewToggle, bool);
    FUNCTION().PROTECTED().SIGNATURE(void, OnNewAssetPressed);
    FUNCTION().PROTECTED().SIGNATURE(void, OnOpenAssetPressed);
    FUNCTION().PROTECTED().SIGNATURE(void, OnSaveAsAssetPressed);
    FUNCTION().PROTECTED().SIGNATURE(void, OnRevertAssetPressed);
    FUNCTION().PROTECTED().SIGNATURE(void, CheckDirtyAssetAndExecute, const Function<void()>&);
    FUNCTION().PROTECTED().SIGNATURE(_tmp1, CreateFileExtensionMap);
}
END_META;
// --- END META ---
