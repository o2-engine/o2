#pragma once

#include "o2/Scene/SceneLayer.h"
#include "o2/Utils/Editor/Attributes/EditorPropertyAttribute.h"
#include "o2/Utils/Serialization/Serializable.h"

namespace o2
{
	class Actor;
	class ComponentRef;

	// ---------------------------
	// Actor's component interface
	// ---------------------------
	class Component: virtual public ISerializable
	{
	public:
		PROPERTIES(Component);
		GETTER(Actor*, actor, GetOwnerActor);                   // Owner actor getter
		PROPERTY(bool, enabled, SetEnabled, IsEnabled);         // Enabling property @EDITOR_IGNORE
		GETTER(bool, enabledInHierarchy, IsEnabledInHierarchy); // Is enabled in hierarchy property

	public:
		// Default constructor
		Component();

		// Copy-constructor
		Component(const Component& other);

		// Virtual destructor
		virtual ~Component();

		// Copy-operator
		Component& operator=(const Component& other);

		// Returns component id
		SceneUID GetID() const;

		// Updates component
		virtual void Update(float dt);

		// Updates component with fixed delta time
		virtual void FixedUpdate(float dt);

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
		Component* GetPrototypeLink() const;

		// Returns is this linked to specified component with depth links search
		bool IsLinkedToComponent(Component* component) const;

		// Returns owner actor
		Actor* GetOwnerActor() const;

		// Returns component with type
		template<typename _type>
		_type* GetComponent() const;

		// Returns component with type in children
		template<typename _type>
		_type* GetComponentInChildren() const;

		// Returns components with type
		template<typename _type>
		Vector<_type*> GetComponents() const;

		// Returns components with type in children
		template<typename _type>
		Vector<_type*> GetComponentsInChildren() const;

		// Returns name of component
		static String GetName();

		// Returns category of component
		static String GetCategory();

		// Returns name of component icon
		static String GetIcon();

		// Is component visible in create menu
		static bool IsAvailableFromCreateMenu();

#if IS_EDITOR
		// It is called when component added from editor
		virtual void OnAddedFromEditor() {}
#endif

		SERIALIZABLE(Component);

	protected:
		Component* mPrototypeLink = nullptr; // Prototype actor component pointer. Null if no actor prototype
		SceneUID   mId;                      // Component id @EDITOR_IGNORE
		Actor*     mOwner = nullptr;         // Owner actor
		bool       mEnabled = true;          // Is component enabled @SERIALIZABLE @EDITOR_IGNORE
		bool       mResEnabled = true;       // Is component enabled in hierarchy

		Vector<ComponentRef*> mReferences; // References to this component

	protected:
		// Beginning serialization callback
		void OnSerialize(DataValue& node) const override;

		// Completion deserialization callback
		void OnDeserialized(const DataValue& node) override;

		// Beginning serialization delta callback
		void OnSerializeDelta(DataValue& node, const IObject& origin) const override;

		// Completion deserialization delta callback
		void OnDeserializedDelta(const DataValue& node, const IObject& origin) override;

		// Sets owner actor
		virtual void SetOwnerActor(Actor* actor);

		// It is called when actor was included to scene
		virtual void OnAddToScene();

		// It is called when actor was excluded from scene
		virtual void OnRemoveFromScene();

		// It is called when component started working on first update frame
		virtual void OnStart() {}

		// Updates component enable
		virtual void UpdateEnabled();

		// Is is called when actor enabled in hierarchy
		virtual void OnEnabled() {}

		// It is called when actor disabled in hierarchy
		virtual void OnDisabled() {}

		// It is called when transformation was changed 
		virtual void OnTransformChanged() {}

		// It is called when actor's transform was changed
		virtual void OnTransformUpdated() {}

		// It is called when parent changed
		virtual void OnParentChanged(Actor* oldParent) {}

		// It is called when child actor was added
		virtual void OnChildAdded(Actor* child) {}

		// It is called when child actor was removed
		virtual void OnChildRemoved(Actor* child) {}

		// It is called when layer was changed
		virtual void OnLayerChanged(SceneLayer* oldLayer) {}

		// It is called when new component has added to actor
		virtual void OnComponentAdded(Component* component) {}

		// It is called when component going to be removed from actor
		virtual void OnComponentRemoving(Component* component) {}

		friend class Actor;
		friend struct ActorDifferences;
		friend class ActorRefResolver;
		friend class ComponentRef;
		friend class Scene;
		friend class Widget;
	};
}

#include "o2/Scene/Actor.h"

namespace o2
{
	template<typename _type>
	Vector<_type*> Component::GetComponentsInChildren() const
	{
		if (mOwner)
			return mOwner->GetComponentsInChildren<_type>();

		return Vector<_type*>();
	}

	template<typename _type>
	Vector<_type*> Component::GetComponents() const
	{
		if (mOwner)
			return mOwner->GetComponents();

		return Vector<_type*>();
	}

	template<typename _type>
	_type* Component::GetComponentInChildren() const
	{
		if (mOwner)
			return mOwner->GetComponentInChildren<_type>();

		return nullptr;
	}

	template<typename _type>
	_type* Component::GetComponent() const
	{
		if (mOwner)
			return mOwner->GetComponent<_type>();

		return nullptr;
	}

}

CLASS_BASES_META(o2::Component)
{
	BASE_CLASS(o2::ISerializable);
}
END_META;
CLASS_FIELDS_META(o2::Component)
{
	FIELD().NAME(actor).PUBLIC();
	FIELD().EDITOR_IGNORE_ATTRIBUTE().NAME(enabled).PUBLIC();
	FIELD().NAME(enabledInHierarchy).PUBLIC();
	FIELD().DEFAULT_VALUE(nullptr).NAME(mPrototypeLink).PROTECTED();
	FIELD().EDITOR_IGNORE_ATTRIBUTE().NAME(mId).PROTECTED();
	FIELD().DEFAULT_VALUE(nullptr).NAME(mOwner).PROTECTED();
	FIELD().EDITOR_IGNORE_ATTRIBUTE().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(true).NAME(mEnabled).PROTECTED();
	FIELD().DEFAULT_VALUE(true).NAME(mResEnabled).PROTECTED();
	FIELD().NAME(mReferences).PROTECTED();
}
END_META;
CLASS_METHODS_META(o2::Component)
{

	PUBLIC_FUNCTION(SceneUID, GetID);
	PUBLIC_FUNCTION(void, Update, float);
	PUBLIC_FUNCTION(void, FixedUpdate, float);
	PUBLIC_FUNCTION(void, SetEnabled, bool);
	PUBLIC_FUNCTION(void, Enable);
	PUBLIC_FUNCTION(void, Disable);
	PUBLIC_FUNCTION(bool, IsEnabled);
	PUBLIC_FUNCTION(bool, IsEnabledInHierarchy);
	PUBLIC_FUNCTION(Component*, GetPrototypeLink);
	PUBLIC_FUNCTION(bool, IsLinkedToComponent, Component*);
	PUBLIC_FUNCTION(Actor*, GetOwnerActor);
	PUBLIC_STATIC_FUNCTION(String, GetName);
	PUBLIC_STATIC_FUNCTION(String, GetCategory);
	PUBLIC_STATIC_FUNCTION(String, GetIcon);
	PUBLIC_STATIC_FUNCTION(bool, IsAvailableFromCreateMenu);
	PUBLIC_FUNCTION(void, OnAddedFromEditor);
	PROTECTED_FUNCTION(void, OnSerialize, DataValue&);
	PROTECTED_FUNCTION(void, OnDeserialized, const DataValue&);
	PROTECTED_FUNCTION(void, OnSerializeDelta, DataValue&, const IObject&);
	PROTECTED_FUNCTION(void, OnDeserializedDelta, const DataValue&, const IObject&);
	PROTECTED_FUNCTION(void, SetOwnerActor, Actor*);
	PROTECTED_FUNCTION(void, OnAddToScene);
	PROTECTED_FUNCTION(void, OnRemoveFromScene);
	PROTECTED_FUNCTION(void, OnStart);
	PROTECTED_FUNCTION(void, UpdateEnabled);
	PROTECTED_FUNCTION(void, OnEnabled);
	PROTECTED_FUNCTION(void, OnDisabled);
	PROTECTED_FUNCTION(void, OnTransformChanged);
	PROTECTED_FUNCTION(void, OnTransformUpdated);
	PROTECTED_FUNCTION(void, OnParentChanged, Actor*);
	PROTECTED_FUNCTION(void, OnChildAdded, Actor*);
	PROTECTED_FUNCTION(void, OnChildRemoved, Actor*);
	PROTECTED_FUNCTION(void, OnLayerChanged, SceneLayer*);
	PROTECTED_FUNCTION(void, OnComponentAdded, Component*);
	PROTECTED_FUNCTION(void, OnComponentRemoving, Component*);
}
END_META;
