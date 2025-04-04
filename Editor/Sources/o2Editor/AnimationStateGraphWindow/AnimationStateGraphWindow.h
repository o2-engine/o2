#pragma once

#include "o2/Assets/Types/AnimationStateGraphAsset.h"
#include "o2/Scene/Components/AnimationStateGraphComponent.h"
#include "o2Editor/AnimationStateGraphWindow/AnimationStateGraphEditor.h"
#include "o2Editor/Core/WindowsSystem/IEditorWindow.h"

using namespace o2;

// Editor animation window accessor macros
#define o2EditorAnimationStateGraphWindow AnimationStateGraphWindow::Instance()

namespace Editor
{
    // ----------------
    // Log window class
    // ----------------
    class AnimationStateGraphWindow : public Singleton<AnimationStateGraphWindow>, public IEditorWindow
    {
    public:
        // Default constructor
        AnimationStateGraphWindow(RefCounter* refCounter);

        // Destructor
        ~AnimationStateGraphWindow();

        // Updates window logic
        void Update(float dt) override;

		// Sets graph and component
		void SetGraph(const Ref<AnimationStateGraphAsset>& graph,
					  const Ref<AnimationStateGraphComponent>& component);

		// Dynamic cast to RefCounterable via Singleton<AnimationStateGraphWindow>
		static Ref<RefCounterable> CastToRefCounterable(const Ref<AnimationStateGraphWindow>& ref);

		IOBJECT(AnimationStateGraphWindow);
		REF_COUNTERABLE_IMPL(IEditorWindow, Singleton<AnimationStateGraphWindow>);

    protected:
		Ref<AnimationStateGraphEditor> mEditor; // Animation state graph editor

    protected:
        // Initializes window
        void InitializeWindow();
    };
}
// --- META ---

CLASS_BASES_META(Editor::AnimationStateGraphWindow)
{
    BASE_CLASS(o2::Singleton<AnimationStateGraphWindow>);
    BASE_CLASS(Editor::IEditorWindow);
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
    FUNCTION().PUBLIC().SIGNATURE(void, Update, float);
    FUNCTION().PUBLIC().SIGNATURE(void, SetGraph, const Ref<AnimationStateGraphAsset>&, const Ref<AnimationStateGraphComponent>&);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(Ref<RefCounterable>, CastToRefCounterable, const Ref<AnimationStateGraphWindow>&);
    FUNCTION().PROTECTED().SIGNATURE(void, InitializeWindow);
}
END_META;
// --- END META ---
