#pragma once

#include "o2/Utils/Editor/Attributes/EditorPropertyAttribute.h"
#include "o2/Utils/Serialization/Serializable.h"

namespace o2 
{
    class Actor;
    
        // ---------------------------
        // Actor's component interface
        // ---------------------------
    class Component: public RefCounterable, virtual public ISerializable, virtual public ICloneableRef
    {
    public:
        PROPERTIES(Component);
        GETTER(Ref<Actor>, actor, GetActor);               // Owner actor getter
        PROPERTY(bool, enabled, SetEnabled, IsEnabled);         // Enabling property @EDITOR_IGNORE
        GETTER(bool, enabledInHierarchy, IsEnabledInHierarchy); // Is enabled in hierarchy property
        
    public:
            // Default constructor
        Component();
        
            // Default constructor with ref counter
        explicit Component(RefCounter* refCounter);
        
            // Copy-constructor
        Component(const Component& other);
        
            // Copy-constructor with ref counter
        Component(RefCounter* refCounter, const Component& other);
        
            // Virtual destructor
        virtual ~Component();
        
            // Copy-operator
        Component& operator=(const Component& other);
        
            // Returns component id
        SceneUID GetID() const;
        
            // Regenerates component id
        void GenerateNewID();
        
            // Sets component enable
        virtual void SetEnabled(bool active);
        
            // Enables component
        void Enable();
        
            // Disables component
        void Disable();
        
            // Returns is component enabled
        bool IsEnabled() const;
        
            // Returns is component enabled in hierarchy
        bool IsEnabledInHierarchy() const;
        
            // Returns prototype link
        const WeakRef<Component>& GetPrototypeLink() const;
        
            // Returns is this linked to specified component with depth links search
        bool IsLinkedToComponent(const Ref<Component>& component) const;
        
            // Returns owner actor
        Ref<Actor> GetActor() const;
        
            // Returns component with type
        template<typename _type>
        Ref<_type> GetComponent() const;
        
            // Returns component with type in children
        template<typename _type>
        Ref<_type> GetComponentInChildren() const;
        
            // Returns components with type
        template<typename _type>
        Vector<Ref<_type>> GetComponents() const;
        
            // Returns components with type in children
        template<typename _type>
        Vector<Ref<_type>> GetComponentsInChildren() const;
        
            // Returns name of component
        static String GetName();
        
            // Returns category of component
        static String GetCategory();
        
            // Returns name of component icon
        static String GetIcon();
        
            // Is component visible in create menu
        static bool IsAvailableFromCreateMenu();
        
#if IS_EDITOR
            // Called when component added from editor
        virtual void OnAddedFromEditor() {}
#endif
        
        SERIALIZABLE(Component);
        CLONEABLE_REF(Component);
        
    protected:
        WeakRef<Actor> mOwner; // Owner actor
        
        SceneUID mId; // Component id @EDITOR_IGNORE
        
        WeakRef<Component> mPrototypeLink; // Prototype actor component pointer. Null if no actor prototype
        
        bool mEnabled = true;             // Is component enabled @SERIALIZABLE @EDITOR_IGNORE
        bool mEnabledInHierarchy = false; // Is component enabled in hierarchy
        
    protected:
            // Beginning serialization callback
        void OnSerialize(DataValue& node) const override;
        
            // Completion deserialization callback
        void OnDeserialized(const DataValue& node) override;
        
            // Beginning serialization delta callback
        void OnSerializeDelta(DataValue& node, const IObject& origin) const override;
        
            // Completion deserialization delta callback
        void OnDeserializedDelta(const DataValue& node, const IObject& origin) override;
        
            // Adds component to scene
        virtual void AddToScene();
        
            // Removes component from scene
        virtual void RemoveFromScene();
        
            // Updates component enable
        virtual void UpdateEnabledInHierarchy();
        
            // Sets owner actor
        virtual void SetOwnerActor(const Ref<Actor>& actor);
        
            // Called when actor was included to scene
        virtual void OnAddToScene() {}
        
            // Called when actor was excluded from scene
        virtual void OnRemoveFromScene() {}
        
            // Called when component, actor and scene was initialized
        virtual void OnInitialized() {}
        
            // Called when component started working on first update frame
        virtual void OnStart() {}
        
            // Called when component will be destroyed
        virtual void OnDestroy() {}
        
            // Updates component
        virtual void OnUpdate(float dt) {}
        
            // Draws component
        virtual void OnDraw() {}
        
            // Updates component with fixed delta time
        virtual void OnFixedUpdate(float dt) {}
        
            // Called when actor enabled in hierarchy
        virtual void OnEnabled() {}
        
            // Called when actor disabled in hierarchy
        virtual void OnDisabled() {}
        
            // Called when actor's transform was changed
        virtual void OnTransformUpdated() {}
        
            // Called when parent changed
        virtual void OnParentChanged(const Ref<Actor>& oldParent) {}
        
            // Called when children list changed
        virtual void OnChildrenChanged() {}
        
            // Called when child actor was added
        virtual void OnChildAdded(const Ref<Actor>& child) {}
        
            // Called when child actor was removed
        virtual void OnChildRemoved(const Ref<Actor>& child) {}
        
            // Called when new component has added to actor
        virtual void OnComponentAdded(const Ref<Component>& component) {}
        
            // Called when component going to be removed from actor
        virtual void OnComponentRemoving(const Ref<Component>& component) {}
        
        friend class Actor;
        friend class ActorRefResolver;
        friend class BaseComponentLinkRef;
        friend class Scene;
        friend class Widget;
        friend struct ActorDifferences;
    };
}

#include "o2/Scene/Actor.h"

namespace o2
{

}
// --- META ---

CLASS_BASES_META(o2::Component)
{
    BASE_CLASS(o2::RefCounterable);
    BASE_CLASS(o2::ISerializable);
    BASE_CLASS(o2::ICloneableRef);
}
END_META;
CLASS_FIELDS_META(o2::Component)
{
    FIELD().PUBLIC().NAME(actor);
    FIELD().PUBLIC().EDITOR_IGNORE_ATTRIBUTE().NAME(enabled);
    FIELD().PUBLIC().NAME(enabledInHierarchy);
    FIELD().PROTECTED().NAME(mOwner);
    FIELD().PROTECTED().EDITOR_IGNORE_ATTRIBUTE().NAME(mId);
    FIELD().PROTECTED().NAME(mPrototypeLink);
    FIELD().PROTECTED().EDITOR_IGNORE_ATTRIBUTE().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(true).NAME(mEnabled);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mEnabledInHierarchy);
}
END_META;
CLASS_METHODS_META(o2::Component)
{

    FUNCTION().PUBLIC().CONSTRUCTOR();
    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*);
    FUNCTION().PUBLIC().CONSTRUCTOR(const Component&);
    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*, const Component&);
    FUNCTION().PUBLIC().SIGNATURE(SceneUID, GetID);
    FUNCTION().PUBLIC().SIGNATURE(void, GenerateNewID);
    FUNCTION().PUBLIC().SIGNATURE(void, SetEnabled, bool);
    FUNCTION().PUBLIC().SIGNATURE(void, Enable);
    FUNCTION().PUBLIC().SIGNATURE(void, Disable);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsEnabled);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsEnabledInHierarchy);
    FUNCTION().PUBLIC().SIGNATURE(const WeakRef<Component>&, GetPrototypeLink);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsLinkedToComponent, const Ref<Component>&);
    FUNCTION().PUBLIC().SIGNATURE(Ref<Actor>, GetActor);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(String, GetName);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(String, GetCategory);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(String, GetIcon);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(bool, IsAvailableFromCreateMenu);
#if  IS_EDITOR
    FUNCTION().PUBLIC().SIGNATURE(void, OnAddedFromEditor);
#endif
    FUNCTION().PROTECTED().SIGNATURE(void, OnSerialize, DataValue&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnDeserialized, const DataValue&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnSerializeDelta, DataValue&, const IObject&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnDeserializedDelta, const DataValue&, const IObject&);
    FUNCTION().PROTECTED().SIGNATURE(void, AddToScene);
    FUNCTION().PROTECTED().SIGNATURE(void, RemoveFromScene);
    FUNCTION().PROTECTED().SIGNATURE(void, UpdateEnabledInHierarchy);
    FUNCTION().PROTECTED().SIGNATURE(void, SetOwnerActor, const Ref<Actor>&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnAddToScene);
    FUNCTION().PROTECTED().SIGNATURE(void, OnRemoveFromScene);
    FUNCTION().PROTECTED().SIGNATURE(void, OnInitialized);
    FUNCTION().PROTECTED().SIGNATURE(void, OnStart);
    FUNCTION().PROTECTED().SIGNATURE(void, OnDestroy);
    FUNCTION().PROTECTED().SIGNATURE(void, OnUpdate, float);
    FUNCTION().PROTECTED().SIGNATURE(void, OnDraw);
    FUNCTION().PROTECTED().SIGNATURE(void, OnFixedUpdate, float);
    FUNCTION().PROTECTED().SIGNATURE(void, OnEnabled);
    FUNCTION().PROTECTED().SIGNATURE(void, OnDisabled);
    FUNCTION().PROTECTED().SIGNATURE(void, OnTransformUpdated);
    FUNCTION().PROTECTED().SIGNATURE(void, OnParentChanged, const Ref<Actor>&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnChildrenChanged);
    FUNCTION().PROTECTED().SIGNATURE(void, OnChildAdded, const Ref<Actor>&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnChildRemoved, const Ref<Actor>&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnComponentAdded, const Ref<Component>&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnComponentRemoving, const Ref<Component>&);
}
END_META;
// --- END META ---
