#include "o2Editor/stdafx.h"
#include "AnimationViewer.h"

#include "o2/Scene/UI/UIManager.h"
#include "o2/Scene/UI/Widgets/Button.h"
#include "o2/Scene/UI/Widgets/VerticalLayout.h"
#include "o2Editor/Windows/AnimationWindow/AnimationWindow.h"
#include "o2/Utils/Editor/EditorScope.h"
#include "o2Editor/Windows/SceneWindow/SceneEditScreen.h"

namespace Editor
{
    const Type* AnimationViewer::GetViewingObjectType() const
    {
        return GetViewingObjectTypeStatic();
    }

    const Type* AnimationViewer::GetViewingObjectTypeStatic()
    {
        return &TypeOf(AnimationClip);
    }

    void AnimationViewer::RebuildProperties(const Vector<Pair<IObject*, IObject*>>& targetObjets)
    {
        PushEditorScopeOnStack scope;

        mEditButton = o2UI.CreateButton("Edit", THIS_FUNC(OnEditPressed));

        DefaultObjectPropertiesViewer::RebuildProperties(targetObjets);
    }

    void AnimationViewer::OnRefreshed(const Vector<Pair<IObject*, IObject*>>& targetObjets)
    {
        mSpoiler->AddChild(mEditButton);
        mEditButton->SetIndexInSiblings(mSpoiler->GetChildren().Count() - 1);

        mCurrentAnimation = !targetObjets.IsEmpty() ? Ref(dynamic_cast<AnimationClip*>(targetObjets[0].first)) : nullptr;
    }

    void AnimationViewer::OnEditPressed()
    {
//         o2EditorAnimationWindow.SetAnimation(mCurrentAnimation);
// 
//         if (!o2EditorSceneScreen.GetSelectedObjects().IsEmpty())
//             o2EditorAnimationWindow.SetTarget(DynamicCast<Actor>(o2EditorSceneScreen.GetSelectedObjects().Last()));
// 
//         o2EditorAnimationWindow.SetAnimationEditable(Ref(mPropertiesContext->FindOnStack<IEditableAnimation>()));
//         o2EditorAnimationWindow.GetWindow()->Focus();
    }

}
// --- META ---

DECLARE_CLASS(Editor::AnimationViewer, Editor__AnimationViewer);
// --- END META ---
