#pragma once
#include "o2/Utils/Basic/ICloneable.h"
#include "o2/Utils/Editor/Attributes/EditorPropertyAttribute.h"
#include "o2/Utils/Math/Basis.h"
#include "o2/Utils/Math/Layout.h"
#include "o2/Utils/Serialization/Serializable.h"
#include "o2/Utils/Types/Containers/Vector.h"
#include "o2/Utils/Types/String.h"

namespace o2
{
#if IS_EDITOR
    struct ActorDifferences;

    // ----------------------------------------------------------
    // Object, that can be shown in editor's tree view and edited
    // ----------------------------------------------------------
    class SceneEditableObject: public RefCounterable, virtual public ISerializable, virtual public ICloneableRef
    {
    public:
        int changedFrame = 0; // Index of frame, when object has changed @EDITOR_IGNORE

    public:
        // Default constructor. Registers itself in scene editable objects list
        SceneEditableObject(RefCounter* refCounter);

        // Destructor, unregisters from scene editable objects list
        ~SceneEditableObject() override;

        // Updates object
        virtual void Update(float dt);

        // Returns true when object is on scene
        virtual bool IsOnScene() const;

        // Destroys object
        virtual void Destroy();

        // Returns unique id
        virtual SceneUID GetID() const;

        // Generates new random id 
        virtual void GenerateNewID(bool childs = true);

        // Returns name of object
        virtual const String& GetName() const;

        // Sets name of object
        virtual void SetName(const String& name);

        // Returns object's link to prototype
        virtual Ref<SceneEditableObject> GetEditableLink() const;

        // Returns is object linked to another object
        virtual bool IsEditableLinkedTo(const Ref<SceneEditableObject>& link) const;

        // Is supports linking to prototype
        virtual bool IsSupportsLinking() const;

        // Returns list of object's children
        virtual Vector<Ref<SceneEditableObject>> GetEditableChildren() const;

        // Collects all children in hierarchy
        void GetAllEditableChildren(Vector<Ref<SceneEditableObject>>& children);

        // Returns object's parent object. Return nullptr when it is a root scene object
        virtual Ref<SceneEditableObject> GetEditableParent() const;

        // Sets parent object. nullptr means make this object as root. idx == -1 means last
        virtual void SetEditableParent(const Ref<SceneEditableObject>& object, int idx = -1);

        // Adds child. idx is place in parent children. idx == -1 means last
        virtual void AddEditableChild(const Ref<SceneEditableObject>& object, int idx = -1);

        // Sets index in siblings - children of parent
        virtual void SetIndexInSiblings(int idx);

        // Checks that this object can be added as child to another object
        virtual bool CanBeParentedTo(const Type& parentType);

        // Returns is that type of object can be enabled and disabled
        virtual bool IsSupportsDisabling() const;

        // Returns is object enabled, override when it's supports
        virtual bool IsEnabled() const;

        // Returns is object enabled and all parent are enabled too
        virtual bool IsEnabledInHierarchy() const;

        // Sets enabling of object, override when it's supports
        virtual void SetEnabled(bool enabled);

        // Returns is that type of object can be locked
        virtual bool IsSupportsLocking() const;

        // Returns is object locked, override when it's supports
        virtual bool IsLocked() const;

        // Returns is object locked and all parent are locked too
        virtual bool IsLockedInHierarchy() const;

        // Sets locking of object, override when it's supports
        virtual void SetLocked(bool locked);

        // Returns is that type of object can be transformed
        virtual bool IsSupportsTransforming() const;

        // Returns transform, override when it's supports
        virtual Basis GetTransform() const;

        // Sets transform of object, override when it's supports
        virtual void SetTransform(const Basis& transform);

        // Updates transform immediately
        virtual void UpdateTransform();

        // Returns is object supports pivot 
        virtual bool IsSupportsPivot() const;

        // Sets transform pivot point
        virtual void SetPivot(const Vec2F& pivot);

        // Returns transform pivot
        virtual Vec2F GetPivot() const;

        // Returns is that type of object can be transformed with layout
        virtual bool IsSupportsLayout() const;

        // Returns layout, override when it's supports
        virtual Layout GetLayout() const;

        // Sets layout of object, override when it's supports
        virtual void SetLayout(const Layout& layout);

        // Returns is that type of object can be used to snapping while transforming
        virtual bool IsSupportsSnapping() const;

        // Returns is that type of object can be deleted from editor
        virtual bool IsSupportsDeleting() const;

        // Called when something changed in this object
        virtual void OnChanged();

        // Called when actor's locking was changed
        virtual void OnLockChanged();

        // Called when actor's name was changed
        virtual void OnNameChanged();

        // Called when child changed
        virtual void OnChildrenChanged();

        // Called when parent changed
        virtual void OnEditableParentChanged(const Ref<SceneEditableObject>& oldParent);

        // Called before making prototype from this object
        virtual void BeginMakePrototype() const {}

        // Called before instantiate from this object
        virtual void BeginInstantiatePrototype() const {}

        SERIALIZABLE(SceneEditableObject);

    protected:
        // Collects differences between this and prototype
        virtual void GetDifferences(ActorDifferences& differences) const;

        friend struct ActorDifferences;
    };
#endif
}
// --- META ---

#if  IS_EDITOR
CLASS_BASES_META(o2::SceneEditableObject)
{
    BASE_CLASS(o2::RefCounterable);
    BASE_CLASS(o2::ISerializable);
    BASE_CLASS(o2::ICloneableRef);
}
END_META;
CLASS_FIELDS_META(o2::SceneEditableObject)
{
#if  IS_EDITOR
    FIELD().PUBLIC().EDITOR_IGNORE_ATTRIBUTE().DEFAULT_VALUE(0).NAME(changedFrame);
#endif
}
END_META;
CLASS_METHODS_META(o2::SceneEditableObject)
{

#if  IS_EDITOR
    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*);
    FUNCTION().PUBLIC().SIGNATURE(void, Update, float);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsOnScene);
    FUNCTION().PUBLIC().SIGNATURE(void, Destroy);
    FUNCTION().PUBLIC().SIGNATURE(SceneUID, GetID);
    FUNCTION().PUBLIC().SIGNATURE(void, GenerateNewID, bool);
    FUNCTION().PUBLIC().SIGNATURE(const String&, GetName);
    FUNCTION().PUBLIC().SIGNATURE(void, SetName, const String&);
    FUNCTION().PUBLIC().SIGNATURE(Ref<SceneEditableObject>, GetEditableLink);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsEditableLinkedTo, const Ref<SceneEditableObject>&);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsSupportsLinking);
    FUNCTION().PUBLIC().SIGNATURE(Vector<Ref<SceneEditableObject>>, GetEditableChildren);
    FUNCTION().PUBLIC().SIGNATURE(void, GetAllEditableChildren, Vector<Ref<SceneEditableObject>>&);
    FUNCTION().PUBLIC().SIGNATURE(Ref<SceneEditableObject>, GetEditableParent);
    FUNCTION().PUBLIC().SIGNATURE(void, SetEditableParent, const Ref<SceneEditableObject>&, int);
    FUNCTION().PUBLIC().SIGNATURE(void, AddEditableChild, const Ref<SceneEditableObject>&, int);
    FUNCTION().PUBLIC().SIGNATURE(void, SetIndexInSiblings, int);
    FUNCTION().PUBLIC().SIGNATURE(bool, CanBeParentedTo, const Type&);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsSupportsDisabling);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsEnabled);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsEnabledInHierarchy);
    FUNCTION().PUBLIC().SIGNATURE(void, SetEnabled, bool);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsSupportsLocking);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsLocked);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsLockedInHierarchy);
    FUNCTION().PUBLIC().SIGNATURE(void, SetLocked, bool);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsSupportsTransforming);
    FUNCTION().PUBLIC().SIGNATURE(Basis, GetTransform);
    FUNCTION().PUBLIC().SIGNATURE(void, SetTransform, const Basis&);
    FUNCTION().PUBLIC().SIGNATURE(void, UpdateTransform);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsSupportsPivot);
    FUNCTION().PUBLIC().SIGNATURE(void, SetPivot, const Vec2F&);
    FUNCTION().PUBLIC().SIGNATURE(Vec2F, GetPivot);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsSupportsLayout);
    FUNCTION().PUBLIC().SIGNATURE(Layout, GetLayout);
    FUNCTION().PUBLIC().SIGNATURE(void, SetLayout, const Layout&);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsSupportsSnapping);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsSupportsDeleting);
    FUNCTION().PUBLIC().SIGNATURE(void, OnChanged);
    FUNCTION().PUBLIC().SIGNATURE(void, OnLockChanged);
    FUNCTION().PUBLIC().SIGNATURE(void, OnNameChanged);
    FUNCTION().PUBLIC().SIGNATURE(void, OnChildrenChanged);
    FUNCTION().PUBLIC().SIGNATURE(void, OnEditableParentChanged, const Ref<SceneEditableObject>&);
    FUNCTION().PUBLIC().SIGNATURE(void, BeginMakePrototype);
    FUNCTION().PUBLIC().SIGNATURE(void, BeginInstantiatePrototype);
    FUNCTION().PROTECTED().SIGNATURE(void, GetDifferences, ActorDifferences&);
#endif
}
END_META;
#endif
// --- END META ---
