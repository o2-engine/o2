#pragma once

#include "o2/Utils/Singleton.h"
#include "o2Editor/Windows/IAssetEditorWindow.h"
#include "o2/Assets/Types/SceneAsset.h"

namespace o2
{
    class PopupWidget;
}

namespace Editor
{
    FORWARD_CLASS_REF(GizmosPopup);
    FORWARD_CLASS_REF(LayersPopup);
    FORWARD_CLASS_REF(SceneEditScreen);

    // Scene window accessor macro
    #define o2EditorSceneWindow SceneWindow::Instance()

    // --------------------
    // Scene editing window
    // --------------------
    class SceneWindow: public Singleton<SceneWindow>, public IAssetEditorWindow
    {
    public:
        IOBJECT(SceneWindow);
        REF_COUNTERABLE_IMPL(IAssetEditorWindow, Singleton<SceneWindow>);

        // Dynamic cast to RefCounterable via Singleton<SceneWindow>
        static Ref<RefCounterable> CastToRefCounterable(const Ref<SceneWindow>& ref);

    protected:
        Ref<SceneEditScreen> mEditWidget; // Scene editing widget

        Ref<Button>      mLayersButton; // Layers button
        Ref<LayersPopup> mLayersPopup;  // Layers popup

        Ref<Button> mView3DButton; // 3D view mode toggle button

        Ref<Button> mCameraModeButton; // Stable/scene camera pipeline toggle button

        Ref<Button>      mGizmosButton; // Gizmos view button
        Ref<GizmosPopup> mGizmosPopup;  // Gizmos view settings popup

    public:
        // Default constructor with ref counter
        SceneWindow(RefCounter* refCounter);

        // Copy constructor with ref counter
        SceneWindow(RefCounter* refCounter, const SceneWindow& other);

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

        // Called when an action has been done (including redo)
        void OnActionDone(const Ref<IAction>& action) override;

        // Called when an action has been undone
        void OnActionUndo(const Ref<IAction>& action) override;
    };
}
// --- META ---

CLASS_BASES_META(Editor::SceneWindow)
{
    BASE_CLASS(o2::Singleton<SceneWindow>);
    BASE_CLASS(Editor::IAssetEditorWindow);
}
END_META;
CLASS_FIELDS_META(Editor::SceneWindow)
{
    FIELD().PROTECTED().NAME(mEditWidget);
    FIELD().PROTECTED().NAME(mLayersButton);
    FIELD().PROTECTED().NAME(mLayersPopup);
    FIELD().PROTECTED().NAME(mView3DButton);
    FIELD().PROTECTED().NAME(mCameraModeButton);
    FIELD().PROTECTED().NAME(mGizmosButton);
    FIELD().PROTECTED().NAME(mGizmosPopup);
}
END_META;
CLASS_METHODS_META(Editor::SceneWindow)
{

    FUNCTION().PUBLIC().SIGNATURE_STATIC(Ref<RefCounterable>, CastToRefCounterable, const Ref<SceneWindow>&);
    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*);
    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*, const SceneWindow&);
    FUNCTION().PUBLIC().SIGNATURE(const Type&, GetAssetType);
    FUNCTION().PROTECTED().SIGNATURE(void, InitializeWindow);
    FUNCTION().PROTECTED().SIGNATURE(String, GetWindowTitle);
    FUNCTION().PROTECTED().SIGNATURE(void, PostInitializeWindow);
    FUNCTION().PROTECTED().SIGNATURE(bool, IsCreateNewAssetAtStartupEnabled);
    FUNCTION().PROTECTED().SIGNATURE(void, OnStartEditingAsset);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCompletedEditingAsset);
    FUNCTION().PROTECTED().SIGNATURE(AssetRef<Asset>, CreateAssetInstance);
    FUNCTION().PROTECTED().SIGNATURE(void, OnAssetSaved);
    FUNCTION().PROTECTED().SIGNATURE(void, OnActionDone, const Ref<IAction>&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnActionUndo, const Ref<IAction>&);
}
END_META;
// --- END META ---
