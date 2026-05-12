#include "o2/stdafx.h"
#include "SceneLayer.h"

#include "o2/Scene/Actor.h"
#include "o2/Scene/ISceneDrawable.h"
#include "o2/Scene/Scene.h"

namespace o2
{
    SceneLayer::SceneLayer()
    {
        mRootDrawables = mmake<SceneLayerRootDrawablesContainer>();
        RegisterDrawable(mRootDrawables.Get());
    }

    void SceneLayer::SetName(const String& name)
    {
        String oldName = mName;
        mName = name;

        o2Scene.OnLayerRenamed(this, oldName);
    }

    const String& SceneLayer::GetName() const
    {
        return mName;
    }

    void SceneLayer::SetVisible(bool visible)
    {
        if (mVisible == visible)
            return;

        mVisible = visible;

#if IS_EDITOR
        o2Scene.onLayersListChanged();
#endif
    }

    bool SceneLayer::IsVisible() const
    {
        return mVisible;
    }

    const Vector<Ref<ISceneDrawable>>& SceneLayer::GetDrawables() const
    {
        return mDrawables;
    }

    const Ref<SceneLayerRootDrawablesContainer>& SceneLayer::GetRootDrawables()
    {
        return mRootDrawables;
    }

    void SceneLayer::RegisterDrawable(ISceneDrawable* drawable)
    {
        const int binSearchRangeSizeStop = 5;
        int rangeMin = 0, rangeMax = mDrawables.Count();
        float targetDepth = drawable->mDrawingDepth;
        int position = 0;
        bool skipLinearSearch = false;

        while (rangeMax - rangeMin > binSearchRangeSizeStop)
        {
            int center = (rangeMin + rangeMax) >> 1;

            float centerValue = mDrawables[center]->mDrawingDepth;

            if (targetDepth < centerValue)
                rangeMax = center;
            else if (targetDepth > centerValue)
                rangeMin = center;
            else
            {
                position = center;
                skipLinearSearch = true;
                break;
            }
        }

        if (!skipLinearSearch)
        {
            for (position = rangeMin; position < rangeMax; position++)
                if (mDrawables[position]->mDrawingDepth > targetDepth)
                    break;
        }

        mDrawables.Insert(Ref(drawable), position);
    }

    void SceneLayer::UnregisterDrawable(ISceneDrawable* drawable)
    {
        mDrawables.Remove(Ref(drawable));
    }

    void SceneLayer::SetLastByDepth(const Ref<ISceneDrawable>& drawable)
    {
        mDrawables.Remove(drawable);

        for (int position = 0; position < mDrawables.Count(); position++)
        {
            if (mDrawables[position]->mDrawingDepth > drawable->mDrawingDepth)
            {
                mDrawables.Insert(drawable, position);
                return;
            }
        }

        mDrawables.Add(drawable);
    }
}
// --- META ---

DECLARE_CLASS(o2::SceneLayer, o2__SceneLayer);
// --- END META ---
