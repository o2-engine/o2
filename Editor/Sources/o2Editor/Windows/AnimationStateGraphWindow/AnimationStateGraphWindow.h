#pragma once

#include "o2/Assets/Types/AnimationStateGraphAsset.h"
#include "o2/Scene/Components/AnimationStateGraphComponent.h"
#include "o2Editor/Windows/AnimationStateGraphWindow/AnimationStateGraphEditor.h"
#include "o2Editor/Windows/IAssetEditorWindow.h"

using namespace o2;

// Editor animation window accessor macros
#define o2EditorAnimationStateGraphWindow AnimationStateGraphWindow::Instance()

namespace Editor
{
    // ----------------
    // Log window class
    // ----------------
    class AnimationStateGraphWindow : public Singleton<AnimationStateGraphWindow>, public IAssetEditorWindow
    {
    public:
        // Default constructor
        AnimationStateGraphWindow(RefCounter* refCounter);

        // Destructor
        ~AnimationStateGraphWindow();

		// Returns asset type that this editor window can edit
		const Type& GetAssetType() const override;

		// Dynamic cast to RefCounterable via Singleton<AnimationStateGraphWindow>
		static Ref<RefCounterable> CastToRefCounterable(const Ref<AnimationStateGraphWindow>& ref);

		IOBJECT(AnimationStateGraphWindow);
		REF_COUNTERABLE_IMPL(IEditorWindow, Singleton<AnimationStateGraphWindow>);

    protected:
		Ref<AnimationStateGraphEditor> mEditor; // Animation state graph editor

    protected:
        // Initializes window
		void InitializeWindow() override;

        // Returns window title
        String GetWindowTitle() const override;

		// Called when asset editing starts
        void OnStartEditingAsset() override;

		// Called when asset editing ends
        void OnCompletedEditingAsset() override;

		// Called when component editing starts
        void OnStartEditingComponent() override;

		// Called when component editing ends
        void OnCompletedEditingComponent() override;

		// Called when component preview is enabled
        void OnComponentPreviewEnabled() override;

		// Called when component preview is disabled
		void OnComponentPreviewDisabled() override;

		// Sets current component asset
        void ComponentSetAsset(const AssetRef<Asset>& asset) override;
    };
}
// --- META ---

CLASS_BASES_META(Editor::AnimationStateGraphWindow)
{
    BASE_CLASS(o2::Singleton<AnimationStateGraphWindow>);
    BASE_CLASS(Editor::IAssetEditorWindow);
}
END_META;
CLASS_FIELDS_META(Editor::AnimationStateGraphWindow)
{
    FIELD().PROTECTED().NAME(mEditor);
}
END_META;
CLASS_METHODS_META(Editor::AnimationStateGraphWindow)
{

    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*);
    FUNCTION().PUBLIC().SIGNATURE(const Type&, GetAssetType);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(Ref<RefCounterable>, CastToRefCounterable, const Ref<AnimationStateGraphWindow>&);
    FUNCTION().PROTECTED().SIGNATURE(void, InitializeWindow);
    FUNCTION().PROTECTED().SIGNATURE(String, GetWindowTitle);
    FUNCTION().PROTECTED().SIGNATURE(void, OnStartEditingAsset);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCompletedEditingAsset);
    FUNCTION().PROTECTED().SIGNATURE(void, OnStartEditingComponent);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCompletedEditingComponent);
    FUNCTION().PROTECTED().SIGNATURE(void, OnComponentPreviewEnabled);
    FUNCTION().PROTECTED().SIGNATURE(void, OnComponentPreviewDisabled);
    FUNCTION().PROTECTED().SIGNATURE(void, ComponentSetAsset, const AssetRef<Asset>&);
}
END_META;
// --- END META ---
