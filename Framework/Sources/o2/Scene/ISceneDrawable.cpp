#include "o2/stdafx.h"
#include "ISceneDrawable.h"

#include "o2/Scene/Actor.h"
#include "o2/Scene/Scene.h"
#include "o2/Scene/SceneLayer.h"
#include <o2/Utils/Debug/StackTrace.h>

namespace o2
{
    ISceneDrawable::ISceneDrawable()
    {}

    ISceneDrawable::ISceneDrawable(const ISceneDrawable& other) :
        mDrawingDepth(other.mDrawingDepth), 
        mInheritDrawingDepthFromParent(other.mInheritDrawingDepthFromParent), drawDepth(this)
    {}

    ISceneDrawable::~ISceneDrawable()
    {
        if (mRegistered)
            Unregister();

        for (auto& child : mChildrenInheritedDepth)
        {
            child->mParentRegistry = nullptr;
            child->mRegistered = false;
        }
    }

    ISceneDrawable& ISceneDrawable::operator=(const ISceneDrawable& other)
    {
        if (other.mInheritDrawingDepthFromParent)
            SetDrawingDepthInheritFromParent(true);
        else
            SetDrawingDepth(other.mDrawingDepth);

        return *this;
    }

    void ISceneDrawable::Draw()
    {
        OnDrawn();
        DrawInheritedDepthChildren();
    }

    void ISceneDrawable::SetDrawingDepth(float depth)
    {
        mDrawingDepth = depth;
        mInheritDrawingDepthFromParent = false;

        Reregister();
    }

    float ISceneDrawable::GetDrawingDepth() const
    {
        return mDrawingDepth;
    }

    void ISceneDrawable::SetDrawingDepthInheritFromParent(bool inherit)
    {
        mInheritDrawingDepthFromParent = inherit;

        Reregister();
    }

    bool ISceneDrawable::IsDrawingDepthInheritedFromParent() const
    {
        return mInheritDrawingDepthFromParent;
    }

    void ISceneDrawable::OnDrawbleParentChanged()
    {
        Reregister();
    }

    void ISceneDrawable::OnDrawableLayerChanged()
    {
        Reregister();
    }

    void ISceneDrawable::SortInheritedDrawables()
    {
        mChildrenInheritedDepth.SortBy<int>([](const Ref<ISceneDrawable>& x) { return x->GetIndexInParentDrawable(); });
    }

	void ISceneDrawable::DrawInheritedDepthChildren()
	{
		for (auto& child : mChildrenInheritedDepth)
			child->Draw();
	}

	void ISceneDrawable::OnEnabled()
    {
        mDrawableEnabled = true;

        if (mRegistered)
            Unregister();

        Register();
    }

    void ISceneDrawable::OnDisabled()
    {
        mDrawableEnabled = false;

        if (mRegistered)
            Unregister();
    }

    void ISceneDrawable::OnAddToScene()
    {
        mIsOnScene = true;
        Reregister();
    }

    void ISceneDrawable::OnRemoveFromScene()
    {
        mIsOnScene = false;

        if (mRegistered)
            Unregister();
    }

    void ISceneDrawable::Reregister()
    {
        if (mRegistered)
            Unregister();

        if (mDrawableEnabled)
            Register();
    }

    void ISceneDrawable::Register()
    {
        if (mInheritDrawingDepthFromParent)
        {
            if (mDrawableEnabled)
            {
                mParentRegistry = GetParentDrawable();
                if (!mParentRegistry && mIsOnScene)
                    mParentRegistry = GetSceneDrawableSceneLayer()->GetRootDrawables();

                if (mParentRegistry)
                {
                    auto parentRegistry = mParentRegistry.Lock();
                    parentRegistry->mChildrenInheritedDepth.Add(Ref(this));
                    parentRegistry->SortInheritedDrawables();

                    mRegistered = true;
                }
            }
        }
        else
        {
            if (mDrawableEnabled && mIsOnScene)
            {
                mLayerRegistry = GetSceneDrawableSceneLayer();
                mLayerRegistry.Lock()->RegisterDrawable(this);

                mRegistered = true;
            }
        }

    }

    void ISceneDrawable::Unregister()
    {
        if (mParentRegistry)
            mParentRegistry.Lock()->mChildrenInheritedDepth.Remove(Ref(this));
        else
            mLayerRegistry.Lock()->UnregisterDrawable(this);

        mParentRegistry = nullptr;
        mLayerRegistry = nullptr;

        mRegistered = false;
    }

    void ISceneDrawable::SetLastOnCurrentDepth()
    {
        if (!mRegistered)
            return;

        if (mParentRegistry)
        {
            auto parentRegistry = mParentRegistry.Lock();
            parentRegistry->mChildrenInheritedDepth.Remove(Ref(this));
            parentRegistry->mChildrenInheritedDepth.Add(Ref(this));
        }
        else
            mLayerRegistry.Lock()->SetLastByDepth(Ref(this));
    }

    const Vector<Ref<ISceneDrawable>>& ISceneDrawable::GetChildrenInheritedDepth() const
    {
        return mChildrenInheritedDepth;
    }

#if IS_EDITOR
    Ref<SceneEditableObject> ISceneDrawable::GetEditableOwner()
    {
        return nullptr;
    }

    void ISceneDrawable::OnDrawn()
    {
        //PROFILE_SAMPLE_FUNC();

        if (auto obj = GetEditableOwner())
            o2Scene.OnObjectDrawn(obj);

        IDrawable::OnDrawn();

        drawCallIdx = o2Render.GetDrawCallsCount();
    }
#endif
}
// --- META ---

DECLARE_CLASS(o2::ISceneDrawable, o2__ISceneDrawable);
// --- END META ---
