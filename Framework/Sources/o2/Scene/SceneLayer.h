#pragma once

#include "o2/Utils/Serialization/Serializable.h"
#include "o2/Utils/Types/Ref.h"
#include "o2/Utils/Types/String.h"

namespace o2
{
    class Actor;

    FORWARD_CLASS_REF(SceneLayerRootDrawablesContainer);
    FORWARD_CLASS_REF(ISceneDrawable);

    // --------------------------------------------------------------------------------
    // Scene layer. It contains Actors and their Drawable parts, managing sorting order
    // --------------------------------------------------------------------------------
    class SceneLayer: public ISerializable, public RefCounterable, public ICloneableRef
    {
    public:
        // Default constructor
        SceneLayer();

        // Sets layer name
        void SetName(const String& name);

        // Returns layer name
        const String& GetName() const;

        // Sets layer visibility; in editor builds also notifies scene listeners
        void SetVisible(bool visible);

        // Returns layer visibility flag
        bool IsVisible() const;

        // Returns all drawable objects of actors in layer
        const Vector<Ref<ISceneDrawable>>& GetDrawables() const;

        // Returns root drawable objects of actors in layer
        const Ref<SceneLayerRootDrawablesContainer>& GetRootDrawables();

        SERIALIZABLE(SceneLayer);
        CLONEABLE_REF(SceneLayer);

    protected:
        String mName; // Name of layer @SERIALIZABLE

        bool mVisible = true; // Is layer visible

        Vector<Ref<ISceneDrawable>> mDrawables; // Drawable objects in layer

        Ref<SceneLayerRootDrawablesContainer> mRootDrawables; // Root drawables with inherited depth. Draws at 0 priority

    protected:
        // Registers drawable object
        void RegisterDrawable(ISceneDrawable* drawable);

        // Unregisters drawable object
        void UnregisterDrawable(ISceneDrawable* drawable);

        // Sets drawable order as last of all objects with same depth
        void SetLastByDepth(const Ref<ISceneDrawable>& drawable);

        friend class Actor;
        friend class CameraActor;
        friend class Component;
        friend class Scene;
        friend class ISceneDrawable;
        friend class Widget;
    };
}
// --- META ---

CLASS_BASES_META(o2::SceneLayer)
{
    BASE_CLASS(o2::ISerializable);
    BASE_CLASS(o2::RefCounterable);
    BASE_CLASS(o2::ICloneableRef);
}
END_META;
CLASS_FIELDS_META(o2::SceneLayer)
{
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().NAME(mName);
    FIELD().PROTECTED().DEFAULT_VALUE(true).NAME(mVisible);
    FIELD().PROTECTED().NAME(mDrawables);
    FIELD().PROTECTED().NAME(mRootDrawables);
}
END_META;
CLASS_METHODS_META(o2::SceneLayer)
{

    FUNCTION().PUBLIC().CONSTRUCTOR();
    FUNCTION().PUBLIC().SIGNATURE(void, SetName, const String&);
    FUNCTION().PUBLIC().SIGNATURE(const String&, GetName);
    FUNCTION().PUBLIC().SIGNATURE(void, SetVisible, bool);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsVisible);
    FUNCTION().PUBLIC().SIGNATURE(const Vector<Ref<ISceneDrawable>>&, GetDrawables);
    FUNCTION().PUBLIC().SIGNATURE(const Ref<SceneLayerRootDrawablesContainer>&, GetRootDrawables);
    FUNCTION().PROTECTED().SIGNATURE(void, RegisterDrawable, ISceneDrawable*);
    FUNCTION().PROTECTED().SIGNATURE(void, UnregisterDrawable, ISceneDrawable*);
    FUNCTION().PROTECTED().SIGNATURE(void, SetLastByDepth, const Ref<ISceneDrawable>&);
}
END_META;
// --- END META ---
