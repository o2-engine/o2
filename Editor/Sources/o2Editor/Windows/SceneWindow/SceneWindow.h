#pragma once

#include "o2Editor/Windows/IAssetEditorWindow.h"
#include "o2/Assets/Types/SceneAsset.h"

namespace o2
{
    class PopupWidget;
}

namespace Editor
{
    FORWARD_CLASS_REF(LayersPopup);
    FORWARD_CLASS_REF(SceneEditScreen);

    // --------------------
    // Scene editing window
    // --------------------
    class SceneWindow: public IAssetEditorWindow
    {
    public:
        IOBJECT(SceneWindow);

    protected:
        Ref<SceneEditScreen> mEditWidget; // Scene editing widget

        Ref<Button>      mLayersButton; // Layers button
        Ref<LayersPopup> mLayersPopup;  // Layers popup

        Ref<PopupWidget> mGizomsView; // Gizoms view

    public:
        // Default constructor
        SceneWindow();

        // Copy constructor
        SceneWindow(const SceneWindow& other);

        // Destructor
        ~SceneWindow();

        // Returns asset type that this editor window can edit
        const Type& GetAssetType() const override;

    protected:
        // Initializes window and controls
        void InitializeWindow() override;

        // Returns window title
        String GetWindowTitle() const override;

        // Called after that all windows was created
        void PostInitializeWindow() override;

        // Returns true if create new asset at startup is enabled
        bool IsCreateNewAssetAtStartupEnabled() const override;

        // Called when asset editing starts
        void OnStartEditingAsset() override;

        // Called when asset editing ends
        void OnCompletedEditingAsset() override;

        // Creates new asset instance by editing asset type
        AssetRef<Asset> CreateAssetInstance() override;

        // Called when asset is saved
        void OnAssetSaved() override;
    };
}
// --- META ---

CLASS_BASES_META(Editor::SceneWindow)
{
    BASE_CLASS(Editor::IAssetEditorWindow);
}
END_META;
CLASS_FIELDS_META(Editor::SceneWindow)
{
    FIELD().PROTECTED().NAME(mEditWidget);
    FIELD().PROTECTED().NAME(mLayersButton);
    FIELD().PROTECTED().NAME(mLayersPopup);
    FIELD().PROTECTED().NAME(mGizomsView);
}
END_META;
CLASS_METHODS_META(Editor::SceneWindow)
{

    FUNCTION().PUBLIC().CONSTRUCTOR();
    FUNCTION().PUBLIC().CONSTRUCTOR(const SceneWindow&);
    FUNCTION().PUBLIC().SIGNATURE(const Type&, GetAssetType);
    FUNCTION().PROTECTED().SIGNATURE(void, InitializeWindow);
    FUNCTION().PROTECTED().SIGNATURE(String, GetWindowTitle);
    FUNCTION().PROTECTED().SIGNATURE(void, PostInitializeWindow);
    FUNCTION().PROTECTED().SIGNATURE(bool, IsCreateNewAssetAtStartupEnabled);
    FUNCTION().PROTECTED().SIGNATURE(void, OnStartEditingAsset);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCompletedEditingAsset);
    FUNCTION().PROTECTED().SIGNATURE(AssetRef<Asset>, CreateAssetInstance);
    FUNCTION().PROTECTED().SIGNATURE(void, OnAssetSaved);
}
END_META;
// --- END META ---
