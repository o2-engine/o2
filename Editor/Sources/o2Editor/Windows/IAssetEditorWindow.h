#pragma once

#include "IEditorWindow.h"
#include "o2Editor/Actions/ActionsList.h"

#include "o2/Assets/Asset.h"
#include "o2/Assets/AssetRef.h"
#include "o2/Events/ShortcutKeysListener.h"
#include "o2/Scene/UI/Widget.h"
#include "o2/Utils/Editor/AssetEditablePreview.h"
#include "o2/Utils/Function/Function.h"
#include "o2Editor/Properties/Basic/AssetProperty.h"

using namespace o2;

namespace Editor
{
	// ------------------------------------------------
	// Asset editor window interface for editing assets
	// Base interface for all asset editor windows
	// ------------------------------------------------
    class IAssetEditorWindow: public IEditorWindow, public ActionsList
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

        // Called when new asset button is pressed
        virtual void MenuCreateNewAsset();

        // Called when open asset button is pressed
        virtual void MenuOpenAsset();

        // Called when save asset button is pressed
        virtual void MenuSaveAsset();

        // Called when save as asset button is pressed
        virtual void MenuSaveAsAsset();

        // Called when revert button is pressed
        virtual void MenuRevertAsset();

		// Returns currently editing asset
		virtual AssetRef<Asset> GetEditingAsset() const;

        // Called when asset has changed, marks asset as dirty
        virtual void OnAssetChanged();

        // Initializes window, called after all windows created. Creates new asset
		void Initialize() override;

		// Updates window logic; updates save icon state
		void Update(float dt) override;

		// Casts Ref<IAssetEditorWindow> to Ref<RefCounterable>
		static Ref<RefCounterable> CastToRefCounterable(const Ref<IAssetEditorWindow>& ref);

        IOBJECT(IAssetEditorWindow);
        REF_COUNTERABLE_IMPL(IEditorWindow, ActionsList);

    protected:
        WeakRef<Asset> mEditingAsset;                    // Currently editing asset
        bool           mIsEditingAssetInstance = false;  // Flag for editing asset instance
        bool           mPrevEditingAssetAlive = false;   // Previous state of editing asset alive

        WeakRef<AssetProperty>         mEditingAssetProperty;        // Property of the asset being edited
        WeakRef<Component>             mEditingComponent;            // Component of the asset being edited
        WeakRef<IAssetEditablePreview> mEditingAssetEditablePreview; // Asset editable preview of the asset being edited

		DataDocument mEditingAssetInstanceCache; // Cache for asset instance data, used to restore asset state

        Ref<HorizontalLayout> mUpPanel;      // Upper panel with buttons and other controls
        Ref<HorizontalLayout> mButtonsPanel; // Buttons panel

        Ref<Toggle> mPreviewToggle;          // Toggle for enabling preview mode while editing asset
        bool        mPreviewEnabled = false; // Flag for preview mode

		Ref<Button> mNewAssetButton;    // New asset button
		Ref<Button> mOpenAssetButton;   // Open asset button
		Ref<Button> mSaveAssetButton;   // Save button
		Ref<Button> mSaveAsAssetButton; // Save as button
		Ref<Button> mRevertAssetButton; // Reset button

		Ref<FunctionalShortcutKeysListener> mUndoActionListener; // Listener for undo action shortcut
		Ref<FunctionalShortcutKeysListener> mRedoActionListener; // Listener for redo action shortcut

    protected:
        // Initializes window and controls
        virtual void InitializeWindow();

        // Returns window title
        virtual String GetWindowTitle() const;

		// Called when window is focused, updates shortcut listeners priority
		void OnFocused() override;

		// Called when window is unfocused, updates shortcut listeners priority
		void OnUnfocused() override;

        // Called when asset editing starts
        virtual void OnStartEditingAsset() {}

        // Called when asset editing ends
        virtual void OnCompletedEditingAsset() {}

		// Called when component editing starts
		virtual void OnStartEditingComponent() {}

		// Called when component editing ends
		virtual void OnCompletedEditingComponent() {}

        // Returns true if create new asset at startup is enabled
        virtual bool IsCreateNewAssetAtStartupEnabled() const;

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

        // Updates window title
        void UpdateWindowTitle();

        // Checks if current asset is alive. Resets editing asset if it is not alive
        void CheckAssetAlive();

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
    BASE_CLASS(Editor::ActionsList);
}
END_META;
CLASS_FIELDS_META(Editor::IAssetEditorWindow)
{
    FIELD().PROTECTED().NAME(mEditingAsset);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mIsEditingAssetInstance);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mPrevEditingAssetAlive);
    FIELD().PROTECTED().NAME(mEditingAssetProperty);
    FIELD().PROTECTED().NAME(mEditingComponent);
    FIELD().PROTECTED().NAME(mEditingAssetEditablePreview);
    FIELD().PROTECTED().NAME(mEditingAssetInstanceCache);
    FIELD().PROTECTED().NAME(mUpPanel);
    FIELD().PROTECTED().NAME(mButtonsPanel);
    FIELD().PROTECTED().NAME(mPreviewToggle);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mPreviewEnabled);
    FIELD().PROTECTED().NAME(mNewAssetButton);
    FIELD().PROTECTED().NAME(mOpenAssetButton);
    FIELD().PROTECTED().NAME(mSaveAssetButton);
    FIELD().PROTECTED().NAME(mSaveAsAssetButton);
    FIELD().PROTECTED().NAME(mRevertAssetButton);
    FIELD().PROTECTED().NAME(mUndoActionListener);
    FIELD().PROTECTED().NAME(mRedoActionListener);
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
    FUNCTION().PUBLIC().SIGNATURE(void, MenuCreateNewAsset);
    FUNCTION().PUBLIC().SIGNATURE(void, MenuOpenAsset);
    FUNCTION().PUBLIC().SIGNATURE(void, MenuSaveAsset);
    FUNCTION().PUBLIC().SIGNATURE(void, MenuSaveAsAsset);
    FUNCTION().PUBLIC().SIGNATURE(void, MenuRevertAsset);
    FUNCTION().PUBLIC().SIGNATURE(AssetRef<Asset>, GetEditingAsset);
    FUNCTION().PUBLIC().SIGNATURE(void, OnAssetChanged);
    FUNCTION().PUBLIC().SIGNATURE(void, Initialize);
    FUNCTION().PUBLIC().SIGNATURE(void, Update, float);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(Ref<RefCounterable>, CastToRefCounterable, const Ref<IAssetEditorWindow>&);
    FUNCTION().PROTECTED().SIGNATURE(void, InitializeWindow);
    FUNCTION().PROTECTED().SIGNATURE(String, GetWindowTitle);
    FUNCTION().PROTECTED().SIGNATURE(void, OnFocused);
    FUNCTION().PROTECTED().SIGNATURE(void, OnUnfocused);
    FUNCTION().PROTECTED().SIGNATURE(void, OnStartEditingAsset);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCompletedEditingAsset);
    FUNCTION().PROTECTED().SIGNATURE(void, OnStartEditingComponent);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCompletedEditingComponent);
    FUNCTION().PROTECTED().SIGNATURE(bool, IsCreateNewAssetAtStartupEnabled);
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
    FUNCTION().PROTECTED().SIGNATURE(void, UpdateWindowTitle);
    FUNCTION().PROTECTED().SIGNATURE(void, CheckAssetAlive);
    FUNCTION().PROTECTED().SIGNATURE(void, CheckDirtyAssetAndExecute, const Function<void()>&);
    FUNCTION().PROTECTED().SIGNATURE(_tmp1, CreateFileExtensionMap);
}
END_META;
// --- END META ---
