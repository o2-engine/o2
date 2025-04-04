#include "o2Editor/stdafx.h"
#include "AnimationStateGraphViewer.h"

#include "o2/Scene/UI/UIManager.h"
#include "o2/Scene/UI/Widgets/Button.h"
#include "o2/Scene/UI/Widgets/VerticalLayout.h"
#include "o2/Utils/Editor/EditorScope.h"
#include "o2Editor/SceneWindow/SceneEditScreen.h"
#include "o2Editor/AnimationStateGraphWindow/AnimationStateGraphWindow.h"

namespace Editor
{
    const Type* AnimationStateGraphViewer::GetViewingObjectType() const
    {
        return GetViewingObjectTypeStatic();
    }

    const Type* AnimationStateGraphViewer::GetViewingObjectTypeStatic()
    {
        return &TypeOf(AnimationStateGraphAsset);
    }

    void AnimationStateGraphViewer::RebuildProperties(const Vector<Pair<IObject*, IObject*>>& targetObjets)
    {
        PushEditorScopeOnStack scope;

        mEditButton = o2UI.CreateButton("Edit", THIS_FUNC(OnEditPressed));

        DefaultObjectPropertiesViewer::RebuildProperties(targetObjets);
    }

    void AnimationStateGraphViewer::OnRefreshed(const Vector<Pair<IObject*, IObject*>>& targetObjets)
    {
        mSpoiler->AddChild(mEditButton);
        mEditButton->SetIndexInSiblings(mSpoiler->GetChildren().Count() - 1);

        mCurrentAnimation = !targetObjets.IsEmpty() ? Ref(dynamic_cast<AnimationStateGraphAsset*>(targetObjets[0].first)) : nullptr;
    }

    void AnimationStateGraphViewer::OnEditPressed()
    {
        Ref<AnimationStateGraphComponent> graphComponent;

        if (!o2EditorSceneScreen.GetSelectedObjects().IsEmpty())
			graphComponent = DynamicCast<Actor>(o2EditorSceneScreen.GetSelectedObjects().Last())->GetComponent<AnimationStateGraphComponent>();

		o2EditorAnimationStateGraphWindow.SetGraph(mCurrentAnimation, graphComponent);
        o2EditorAnimationStateGraphWindow.GetWindow()->Focus();
    }

}
// --- META ---

DECLARE_CLASS(Editor::AnimationStateGraphViewer, Editor__AnimationStateGraphViewer);
// --- END META ---
