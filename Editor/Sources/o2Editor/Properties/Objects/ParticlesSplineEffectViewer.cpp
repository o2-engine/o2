#include "o2Editor/stdafx.h"
#include "ParticlesSplineEffectViewer.h"

#include "o2/Render/Particles/ParticlesEffects.h"
#include "o2/Scene/UI/UIManager.h"
#include "o2/Scene/UI/Widgets/VerticalLayout.h"
#include "o2/Utils/Editor/EditorScope.h"
#include "o2Editor/Windows/AnimationWindow/AnimationWindow.h"
#include "o2Editor/Windows/SceneWindow/SceneEditScreen.h"
#include "o2/Scene/Components/ParticlesEmitterComponent.h"

namespace Editor
{

    ParticlesSplineEffectViewer::ParticlesSplineEffectViewer()
    {
        mSplineTool = mmake<SplineTool>();
    }

    ParticlesSplineEffectViewer::~ParticlesSplineEffectViewer()
    {
        o2EditorSceneScreen.RemoveTool(mSplineTool);
    }

    const Type* ParticlesSplineEffectViewer::GetViewingObjectType() const
    {
        return GetViewingObjectTypeStatic();
    }

    const Type* ParticlesSplineEffectViewer::GetViewingObjectTypeStatic()
    {
        return &TypeOf(ParticlesSplineEffect);
    }

    void ParticlesSplineEffectViewer::RebuildProperties(const Vector<Pair<IObject*, IObject*>>& targetObjets)
    {
        PushEditorScopeOnStack scope;

        DefaultObjectPropertiesViewer::RebuildProperties(targetObjets);
    }

    void ParticlesSplineEffectViewer::OnRefreshed(const Vector<Pair<IObject*, IObject*>>& targetObjets)
    {
        auto prevSplineEffect = mSplineEffect;

        mSplineEffect = !targetObjets.IsEmpty() ? Ref(dynamic_cast<ParticlesSplineEffect*>(targetObjets[0].first)) : nullptr;

        DefaultObjectPropertiesViewer::OnRefreshed(targetObjets);

        if (!mTargetObjects.IsEmpty() && prevSplineEffect != mSplineEffect)
        {
            Function<Vec2F()> getOrigin = [=]() { return mSplineEffect->GetEmitter()->GetBasis().origin; };

            // Spline tool
            mSplineTool->SetSpline(mSplineEffect->spline, getOrigin);
            mSplineTool->onChanged = [=]() { 
                auto emitter = mSplineEffect->GetEmitter();
                DynamicCast<ParticlesEmitterComponent>(emitter)->GetActor()->OnChanged();
            };
        }
    }

    void ParticlesSplineEffectViewer::OnPropertiesEnabled()
    {
        o2EditorSceneScreen.AddTool(mSplineTool);

        mPrevSelectedTool = o2EditorSceneScreen.GetSelectedTool();
        o2EditorSceneScreen.SelectTool<SplineTool>();
    }

    void ParticlesSplineEffectViewer::OnPropertiesDisabled()
    {
        auto selectedTool = o2EditorSceneScreen.GetSelectedTool();
        if (selectedTool == mSplineTool)
            o2EditorSceneScreen.SelectTool(mPrevSelectedTool.Lock());

        o2EditorSceneScreen.RemoveTool(mSplineTool);
    }

}
// --- META ---

DECLARE_CLASS(Editor::ParticlesSplineEffectViewer, Editor__ParticlesSplineEffectViewer);
// --- END META ---
