#pragma once

#include "o2/Render/IDrawable.h"
#include "o2/Scene/SceneLayer.h"
#include "o2/Utils/Property.h"
#include "o2/Utils/Serialization/Serializable.h"

#if IS_EDITOR
#include "o2/Utils/Editor/SceneEditableObject.h"
#endif

namespace o2
{
    // --------------------------------------------------------------------------------------------------------------
    // Scene drawable object. Has virtual draw function and sorting depth. Depth shows how later object will be drawn
    // Can be used to create your own drawable objects. For that you need to override Draw() function, 
    // GetSceneDrawableSceneLayer(), GetParentDrawable() and GetIndexInParentDrawable(). And call "on" methods: 
    // OnDrawbleParentChanged(), OnEnabled(), OnDisabled(), OnAddToScene(), OnRemoveFromScene()
    // --------------------------------------------------------------------------------------------------------------
    class ISceneDrawable : virtual public ISerializable, virtual public IRefCounterable, virtual public ICloneableRef, virtual public IDrawable
    {
    public:
        PROPERTIES(ISceneDrawable);
        PROPERTY(float, drawDepth, SetDrawingDepth, GetDrawingDepth); // Drawing depth property. Objects with higher depth will be drawn later @EDITOR_IGNORE

    public:
        // Default constructor
        ISceneDrawable();

        // Copy-constructor
        ISceneDrawable(const ISceneDrawable& other);

        // Destructor
        ~ISceneDrawable() override;

        // Copy operator
        ISceneDrawable& operator=(const ISceneDrawable& other);

        // Draws content
        void Draw() override;

        // Sets drawing depth. Objects with higher depth will be drawn later
        virtual void SetDrawingDepth(float depth);

        // Returns drawing depth
        float GetDrawingDepth() const;

        // Sets that depth is inherited from the parent
        void SetDrawingDepthInheritFromParent(bool inherit);

        // Returns whether the depth is inherited from the parent
        bool IsDrawingDepthInheritedFromParent() const;

        // Sets this drawable as last drawing object in layer with same depth
        void SetLastOnCurrentDepth();

        // Returns list of inherited depth drawables
        const Vector<Ref<ISceneDrawable>>& GetChildrenInheritedDepth() const;

        SERIALIZABLE(ISceneDrawable);

    private:
        bool mRegistered = false; //  Was the drawable registered in the layer or in the parent

        bool mDrawableEnabled = false; // Is drawable enabled
        bool mIsOnScene = false;       // Is drawable on scene

        WeakRef<ISceneDrawable> mParentRegistry; // Parent registry drawable if inherited depth is used
        WeakRef<SceneLayer>     mLayerRegistry;  // Layer registry if inherited depth isn't used

    protected:
        float mDrawingDepth = 0.0f;                  // Drawing depth. Objects with higher depth will be drawn later @SERIALIZABLE
        bool  mInheritDrawingDepthFromParent = true; // If parent depth is used @SERIALIZABLE

        Vector<Ref<ISceneDrawable>> mChildrenInheritedDepth; // List of children who inherited depth

    protected:
        // Returns current scene layer
        virtual Ref<SceneLayer> GetSceneDrawableSceneLayer() const { return nullptr; }

        // Returns parent scene drawable
        virtual Ref<ISceneDrawable> GetParentDrawable() { return nullptr; }

        // Returns the index in the parent's list of children, used to sort the rendering
        virtual int GetIndexInParentDrawable() const { return 0; }

        // Sorts depth-inheriting drawables
		void SortInheritedDrawables();

		// Draws children with inherited depth
		virtual void DrawInheritedDepthChildren();

        // Called when the parent changes
        virtual void OnDrawbleParentChanged();

        // Called when drawable layer changed
        virtual void OnDrawableLayerChanged();

        // Called when drawable has enabled
        void OnEnabled();

        // Called when drawable has enabled
        void OnDisabled();

        // Called when actor was included to scene
        void OnAddToScene();

        // Called when actor was excluded from scene
        void OnRemoveFromScene();

        // Refreshing drawable registration
        void Reregister();

        // Registers drawable in layer or in parent
        void Register();

        // Unregisters drawable from layer or from parent
        void Unregister();

        friend class Scene;
        friend class SceneLayer;

#if IS_EDITOR
    public:
        int drawCallIdx = 0; // Draw call number in frame @EDITOR_IGNORE

    public:
        // Returns pointer to owner editable object
        virtual Ref<SceneEditableObject> GetEditableOwner();

        // Called when drawable was drawn. Storing render scissor rect, calling onDraw callback, adding in drawnEditableObjects
        void OnDrawn() override;
#endif
    };

    // ------------------------------------------------------------------------------------
    // Root drawables container. It is used to draw all root drawables with inherited depth
    // ------------------------------------------------------------------------------------
    class SceneLayerRootDrawablesContainer : public ISceneDrawable, public RefCounterable
    {
    public:
        SceneLayerRootDrawablesContainer(RefCounter* refCounter): 
            RefCounterable(refCounter)
        {}

        REF_COUNTERABLE_IMPL(RefCounterable);
    };
}
// --- META ---

CLASS_BASES_META(o2::ISceneDrawable)
{
    BASE_CLASS(o2::ISerializable);
    BASE_CLASS(o2::IRefCounterable);
    BASE_CLASS(o2::ICloneableRef);
    BASE_CLASS(o2::IDrawable);
}
END_META;
CLASS_FIELDS_META(o2::ISceneDrawable)
{
    FIELD().PUBLIC().EDITOR_IGNORE_ATTRIBUTE().NAME(drawDepth);
    FIELD().PRIVATE().DEFAULT_VALUE(false).NAME(mRegistered);
    FIELD().PRIVATE().DEFAULT_VALUE(false).NAME(mDrawableEnabled);
    FIELD().PRIVATE().DEFAULT_VALUE(false).NAME(mIsOnScene);
    FIELD().PRIVATE().NAME(mParentRegistry);
    FIELD().PRIVATE().NAME(mLayerRegistry);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(0.0f).NAME(mDrawingDepth);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(true).NAME(mInheritDrawingDepthFromParent);
    FIELD().PROTECTED().NAME(mChildrenInheritedDepth);
#if  IS_EDITOR
    FIELD().PUBLIC().EDITOR_IGNORE_ATTRIBUTE().DEFAULT_VALUE(0).NAME(drawCallIdx);
#endif
}
END_META;
CLASS_METHODS_META(o2::ISceneDrawable)
{

    FUNCTION().PUBLIC().CONSTRUCTOR();
    FUNCTION().PUBLIC().CONSTRUCTOR(const ISceneDrawable&);
    FUNCTION().PUBLIC().SIGNATURE(void, Draw);
    FUNCTION().PUBLIC().SIGNATURE(void, SetDrawingDepth, float);
    FUNCTION().PUBLIC().SIGNATURE(float, GetDrawingDepth);
    FUNCTION().PUBLIC().SIGNATURE(void, SetDrawingDepthInheritFromParent, bool);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsDrawingDepthInheritedFromParent);
    FUNCTION().PUBLIC().SIGNATURE(void, SetLastOnCurrentDepth);
    FUNCTION().PUBLIC().SIGNATURE(const Vector<Ref<ISceneDrawable>>&, GetChildrenInheritedDepth);
    FUNCTION().PROTECTED().SIGNATURE(Ref<SceneLayer>, GetSceneDrawableSceneLayer);
    FUNCTION().PROTECTED().SIGNATURE(Ref<ISceneDrawable>, GetParentDrawable);
    FUNCTION().PROTECTED().SIGNATURE(int, GetIndexInParentDrawable);
    FUNCTION().PROTECTED().SIGNATURE(void, SortInheritedDrawables);
    FUNCTION().PROTECTED().SIGNATURE(void, DrawInheritedDepthChildren);
    FUNCTION().PROTECTED().SIGNATURE(void, OnDrawbleParentChanged);
    FUNCTION().PROTECTED().SIGNATURE(void, OnDrawableLayerChanged);
    FUNCTION().PROTECTED().SIGNATURE(void, OnEnabled);
    FUNCTION().PROTECTED().SIGNATURE(void, OnDisabled);
    FUNCTION().PROTECTED().SIGNATURE(void, OnAddToScene);
    FUNCTION().PROTECTED().SIGNATURE(void, OnRemoveFromScene);
    FUNCTION().PROTECTED().SIGNATURE(void, Reregister);
    FUNCTION().PROTECTED().SIGNATURE(void, Register);
    FUNCTION().PROTECTED().SIGNATURE(void, Unregister);
#if  IS_EDITOR
    FUNCTION().PUBLIC().SIGNATURE(Ref<SceneEditableObject>, GetEditableOwner);
    FUNCTION().PUBLIC().SIGNATURE(void, OnDrawn);
#endif
}
END_META;
// --- END META ---
