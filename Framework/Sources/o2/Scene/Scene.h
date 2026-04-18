#pragma once

#include "o2/Assets/Types/ActorAsset.h"
#include "o2/Scene/ComponentLinkRef.h"
#include "o2/Utils/Property.h"
#include "o2/Utils/Serialization/Serializable.h"
#include "o2/Utils/Singleton.h"
#include "o2/Utils/Types/Containers/Vector.h"
#include "o2/Utils/Types/Ref.h"
#include "o2/Utils/Types/String.h"
#include "o2/Utils/Types/UID.h"

// Scene graph access macros
#define o2Scene o2::Scene::Instance()

namespace o2
{
    class Actor;
    class CameraActor;
    class Component;
    class Tag;

    FORWARD_CLASS_REF(SceneLayer);

#if IS_EDITOR
    class SceneEditableObject;
#endif

    // -------------------------------------------------------
    // Actors scene. Contains and manages actors, tags, layers
    // -------------------------------------------------------
    class Scene : public Singleton<Scene>, public IObject
    {
    public:
        // Default constructor
        Scene(RefCounter* refCounter);

        // Destructor
        ~Scene();

        // Returns scene log stream
        const LogStream& GetLogStream() const;

        // Checks is layer exists
        bool HasLayer(const String& name) const;

        // Returns layer by name
        Ref<SceneLayer> GetLayer(const String& name);

        // Returns default layer
        const Ref<SceneLayer>& GetDefaultLayer() const;

        // Adds layer with name
        Ref<SceneLayer> AddLayer(const String& name);

        // Removes layer
        void RemoveLayer(const Ref<SceneLayer>& layer);

        // Removes layer by name
        void RemoveLayer(const String& name);

        // Sets layer order
        void SetLayerOrder(const Ref<SceneLayer>& layer, int idx);

        // Returns layers array
        const Vector<Ref<SceneLayer>>& GetLayers() const;

        // Returns layers names array
        Vector<String> GetLayersNames() const;

        // Returns layers map by name
        const Map<String, WeakRef<SceneLayer>>& GetLayersMap() const;

        // Returns tag with name
        Ref<Tag> GetTag(const String& name) const;

        // Adds tag with name
        Ref<Tag> AddTag(const String& name);

        // Removes tag
        void RemoveTag(const Ref<Tag>& tag);

        // Removes tag with name
        void RemoveTag(const String& name);

        // Returns tags array
        const Vector<Ref<Tag>>& GetTags() const;

        // Returns root actors
        const Vector<Ref<Actor>>& GetRootActors() const;

        // Returns root actors
        Vector<Ref<Actor>>& GetRootActors();

        // Returns all actors
        const Vector<WeakRef<Actor>>& GetAllActors() const;

        // Returns all actors
        Vector<WeakRef<Actor>>& GetAllActors();

        // Returns actor by id
        Ref<Actor> GetActorByID(SceneUID id) const;

        // Returns asset actor by asset id. Tries to find in cache
        Ref<Actor> GetAssetActorByID(const UID& id);

        // Returns actor by path (ex "some node/other/target")
        Ref<Actor> FindActor(const String& path);

        // Returns actor with type in scene
        template<typename _type>
        Ref<_type> FindActorByType();

        // Returns cameras on scene
        const Vector<WeakRef<CameraActor>>& GetCameras() const;

        // Returns component with type in scene
        template<typename _type>
        Ref<_type> FindActorComponent();

        // Returns all components with type in scene
        template<typename _type>
        Vector<Ref<_type>> FindAllActorsComponents();

        // Removes all actors
        void Clear(bool keepDefaultLayer = true);

        // Clears assets cache
        void ClearCache();

        // Loads scene from file. If append is true, old actors will not be destroyed
        void Load(const String& path, bool append = false);

        // Loads scene from document. If append is true, old actors will not be destroyed
        void Load(const DataDocument& doc, bool append = false);

        // Saves scene into file
        void Save(const String& path);

        // Saves scene into document
        void Save(DataDocument& doc);

        // Draws scene drawable objects
        void Draw();

        // Updates root actors
        void Update(float dt);

        // Updates root actors with fixed delta time
        void FixedUpdate(float dt);

        // Updates just added actors and components
        void UpdateAddedEntities();

        // Updates actors transforms
        void UpdateTransforms();

        // Updates destroying actors and components
        void UpdateDestroyingEntities();

        // Adds actor to destroy list, will be removed at next frame
        void DestroyActor(const Ref<Actor>& actor);

        // Adds component to destroy list, will be removed at next frame
        void DestroyComponent(const Ref<Component>& component);

        // Returns true if scene updating
        bool IsUpdating() const;

        // Returns true if scene works in editor
        bool IsEditor() const;

        IOBJECT(Scene);

    protected:
        Ref<LogStream> mLog; // Scene log

        Vector<Ref<Actor>> mRootActors; // Scene root actors     

        Vector<WeakRef<CameraActor>> mCameras; // List of cameras on scene
   
        Vector<WeakRef<Actor>>        mAllActors; // All scene actors
        Map<SceneUID, WeakRef<Actor>> mActorsMap; // Actors map by unique ID

        Vector<Ref<Actor>> mAddedActors; // List of added on previous frame actors. Will receive OnAddToScene at current frame

        Vector<Ref<Actor>>     mStartActors;     // List of starting on current frame actors. Will receive OnStart at current frame
        Vector<Ref<Component>> mStartComponents; // List of starting on current frame components. Will receive OnStart at current frame

        Vector<Ref<Actor>>     mDestroyActors;     // List of destroying on current frame actors
        Vector<Ref<Component>> mDestroyComponents; // List of destroying on current frame components

        Map<String, WeakRef<SceneLayer>> mLayersMap; // Layers by names map
        Vector<Ref<SceneLayer>>          mLayers;    // Scene layers

        Ref<SceneLayer> mDefaultLayer; // Default scene layer

        Vector<Ref<Tag>> mTags; // Scene tags

        Vector<AssetRef<ActorAsset>> mAssetsCache; // Cached actors assets

        bool mIsUpdatingScene = false; // Sets true when started updating scene, and false when not

    protected:
        // Called when actor added to scene, adds to scene deffered
        static void OnActorCreated(const Ref<Actor>& actor);

        // Called when actor destroyed, removes from scene
        static void OnActorDestroy(const WeakRef<Actor>& actor);

        // Called when new actor was parented to another actor, removes it from added list
        static void OnNewActorParented(const Ref<Actor>& actor);

        // Called when actor unique id was changed; updates actors map
        static void OnActorIdChanged(const Ref<Actor>& actor, SceneUID prevId);

        // Called when actor adding to scene; registers in actors list and events list
        static void OnAddActorToScene(const Ref<Actor>& actor);

        // Called when actor removing from scene; unregisters from actors list and events list
        static void OnRemoveActorFromScene(const Ref<Actor>& actor, bool keepEditorObjects = false);

    protected:
        // Draws cameras
        void DrawCameras();

        // Updates root actors and their children
        void UpdateActors(float dt);

        // Updates starting actors and components
        void UpdateStartingEntities();

        // Draws debug info for actor under cursor
        void DrawCursorDebugInfo();

        // Called when actor adding to scene; registers in actors list and events list
        void AddActorToScene(const Ref<Actor>& actor);

        // Called when actor removing from scene; unregisters from actors list and events list
        void RemoveActorFromScene(const Ref<Actor>& actor, bool keepEditorObjects = false);

        // Called when component added to actor, registers for calling OnAddOnScene
        void OnComponentAdded(Component* component);

        // Called when component removed, register for calling OnRemovFromScene
        void OnComponentRemoved(Component* component);

        // Called when scene layer renamed, updates layers map
        void OnLayerRenamed(SceneLayer* layer, const String& oldName);

        // Called when camera was added to scene
        void OnCameraAddedOnScene(CameraActor* camera);

        // Called when camera was removed from scene
        void OnCameraRemovedScene(CameraActor* camera);

        friend class Actor;
        friend class Application;
        friend class CameraActor;
        friend class Component;
        friend class Component;
        friend class SceneLayer;
        friend class Widget;
        friend class WidgetLayer;

#if IS_EDITOR          
    public:
        Function<void(const Ref<SceneEditableObject>&)> onAddedToScene;             // Actor added to scene event
        Function<void(const Ref<SceneEditableObject>&)> onRemovedFromScene;         // Actor removed from scene event
        Function<void(const Ref<SceneEditableObject>&)> onEnableChanged;            // Actor enable changing
        Function<void(const Ref<SceneEditableObject>&)> onLockChanged;              // Actor locking change
        Function<void(const Ref<SceneEditableObject>&)> onNameChanged;              // Actor name changing event
        Function<void(const Ref<SceneEditableObject>&)> onChildrenHierarchyChanged; // Actor childs hierarchy change event

        Function<void(const Vector<Ref<SceneEditableObject>>&)> onObjectsChanged; // Actors some change event

        Function<void()> onLayersListChanged; // Called when layer added, removed or renamed

    public:
        // Links actor recursively to prototypes and their parent prototypes
        static void LinkActorToPrototypesHierarchy(Ref<Actor> actor, AssetRef<ActorAsset> proto);

        // Sets scene playing in editor
        void SetEditorPlaying(bool playing);

        // Returns true if scene in editor in play mode
        bool IsEditorPlaying() const;

        // Sets scene is in editor application (not in a standalone runner).
        // This distinguishes an editor-host (where edit-time reshape logic is
        // valid) from a runner that was merely compiled with O2_EDITOR=ON.
        void SetIsEditor(bool isEditor);

        // Returns root editable objects
        Vector<Ref<SceneEditableObject>> GetRootEditableObjects();

        // Adds editable object to scene
        void AddEditableObjectToScene(const Ref<SceneEditableObject>& object);

        // Removes object from scene
        void RemoveEditableObjectFromScene(const Ref<SceneEditableObject>& object);

        // Returns all editable objects
        const Vector<WeakRef<SceneEditableObject>>& GetAllEditableObjects();

        // Returns current changed actors
        const Vector<Ref<SceneEditableObject>>& GetChangedObjects() const;

        // Returns drawn on last frame editable objects
        const Vector<WeakRef<SceneEditableObject>>& GetDrawnEditableObjects() const;

        // Returns actor by id
        Ref<SceneEditableObject> GetEditableObjectByID(SceneUID id) const;

        // Returns object's index in hierarchy
        int GetObjectHierarchyIdx(const Ref<SceneEditableObject>& object) const;

        // Reparent scene editableobjects to new parent at next of prevActor;
        void ReparentEditableObjects(const Vector<Ref<SceneEditableObject>>& objects,
                                     const Ref<SceneEditableObject>& newParent, const Ref<SceneEditableObject>& prevObject);

        // Checks is any actors was changed and calls OnChanged() if changed
        void CheckChangedObjects();

        // Returns cache of linked to prototypes actors
        Map<AssetRef<ActorAsset>, Vector<WeakRef<Actor>>>& GetPrototypesLinksCache();

        // Called when scene started to draw. When scene draw started, drawn scene objects will be collected
        void BeginDrawingScene();

        // Called when scene finished draw. After scene objects will not be collected 
        void EndDrawingScene();

        // Called when object was created
        void OnObjectAddToScene(const Ref<SceneEditableObject>& object);

        // Called when object is destroying
        void OnObjectRemoveFromScene(const Ref<SceneEditableObject>& object);

        // Called when object was changed
        void OnObjectChanged(const Ref<SceneEditableObject>& object);

        // Called when object was drawn 
        void OnObjectDrawn(const Ref<SceneEditableObject>& object);

        // Called when created actor with prototype, updates cache
        void OnActorWithPrototypeCreated(const Ref<Actor>& actor);

        // Called when some actor created and linked to prototype, updates linked actors cache
        void OnActorLinkedToPrototype(AssetRef<ActorAsset>& assetRef, const Ref<Actor>& actor);

        // Called when actor destroying or prototype link broken, updates cache
        void OnActorPrototypeBroken(Actor* actor);

    protected:
        Map<AssetRef<ActorAsset>, Vector<WeakRef<Actor>>> mPrototypeLinksCache; // Cache of linked to prototypes actors

        Vector<Ref<SceneEditableObject>> mChangedObjects;    // Changed actors array

        Vector<WeakRef<SceneEditableObject>>                mEditableObjects;      // All scene editable objects
        mutable Map<SceneUID, WeakRef<SceneEditableObject>> mEditableObjectsByUID; // All scene editable objects by UID

        Vector<WeakRef<SceneEditableObject>> mDrawnObjects; // List of drawn on last frame editable objects

        bool mIsDrawingScene = false;  // Sets true when started drawing scene, and false when not
        bool mIsPlaying = false;       // Is scene is in play mode
        bool mIsEditor = false;        // True only when running inside the editor application

        friend class SceneEditableObject;
#endif
    };
}

//#include "o2/Scene/Actor.h"

namespace o2
{
    template<typename _type>
    Vector<Ref<_type>> Scene::FindAllActorsComponents()
    {
        Vector<Ref<_type>> res;
//         for (auto& actor : mRootActors)
//             res.Add(actor->GetComponentInChildren<_type>());

        return res;
    }

    template<typename _type>
    Ref<_type> Scene::FindActorByType()
    {
//         for (auto& actor : mRootActors)
//         {
//             if (auto res = dynamic_cast<_type*>(actor))
//                 return res;
// 
//             if (auto res = actor->FindChildByType<_type>())
//                 return res;
//         }

        return nullptr;
    }

    template<typename _type>
    Ref<_type> Scene::FindActorComponent()
    {
//         for (auto& actor : mRootActors)
//         {
//             _type* res = actor->GetComponentInChildren<_type>();
//             if (res)
//                 return res;
//         }

        return nullptr;
    }
};
// --- META ---

CLASS_BASES_META(o2::Scene)
{
    BASE_CLASS(o2::Singleton<Scene>);
    BASE_CLASS(o2::IObject);
}
END_META;
CLASS_FIELDS_META(o2::Scene)
{
    FIELD().PROTECTED().NAME(mLog);
    FIELD().PROTECTED().NAME(mRootActors);
    FIELD().PROTECTED().NAME(mCameras);
    FIELD().PROTECTED().NAME(mAllActors);
    FIELD().PROTECTED().NAME(mActorsMap);
    FIELD().PROTECTED().NAME(mAddedActors);
    FIELD().PROTECTED().NAME(mStartActors);
    FIELD().PROTECTED().NAME(mStartComponents);
    FIELD().PROTECTED().NAME(mDestroyActors);
    FIELD().PROTECTED().NAME(mDestroyComponents);
    FIELD().PROTECTED().NAME(mLayersMap);
    FIELD().PROTECTED().NAME(mLayers);
    FIELD().PROTECTED().NAME(mDefaultLayer);
    FIELD().PROTECTED().NAME(mTags);
    FIELD().PROTECTED().NAME(mAssetsCache);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mIsUpdatingScene);
#if  IS_EDITOR          
    FIELD().PUBLIC().NAME(onAddedToScene);
    FIELD().PUBLIC().NAME(onRemovedFromScene);
    FIELD().PUBLIC().NAME(onEnableChanged);
    FIELD().PUBLIC().NAME(onLockChanged);
    FIELD().PUBLIC().NAME(onNameChanged);
    FIELD().PUBLIC().NAME(onChildrenHierarchyChanged);
    FIELD().PUBLIC().NAME(onObjectsChanged);
    FIELD().PUBLIC().NAME(onLayersListChanged);
    FIELD().PROTECTED().NAME(mPrototypeLinksCache);
    FIELD().PROTECTED().NAME(mChangedObjects);
    FIELD().PROTECTED().NAME(mEditableObjects);
    FIELD().PROTECTED().NAME(mEditableObjectsByUID);
    FIELD().PROTECTED().NAME(mDrawnObjects);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mIsDrawingScene);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mIsPlaying);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mIsEditor);
#endif
}
END_META;
CLASS_METHODS_META(o2::Scene)
{

    typedef const Map<String, WeakRef<SceneLayer>>& _tmp1;
    typedef Map<AssetRef<ActorAsset>, Vector<WeakRef<Actor>>>& _tmp2;

    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*);
    FUNCTION().PUBLIC().SIGNATURE(const LogStream&, GetLogStream);
    FUNCTION().PUBLIC().SIGNATURE(bool, HasLayer, const String&);
    FUNCTION().PUBLIC().SIGNATURE(Ref<SceneLayer>, GetLayer, const String&);
    FUNCTION().PUBLIC().SIGNATURE(const Ref<SceneLayer>&, GetDefaultLayer);
    FUNCTION().PUBLIC().SIGNATURE(Ref<SceneLayer>, AddLayer, const String&);
    FUNCTION().PUBLIC().SIGNATURE(void, RemoveLayer, const Ref<SceneLayer>&);
    FUNCTION().PUBLIC().SIGNATURE(void, RemoveLayer, const String&);
    FUNCTION().PUBLIC().SIGNATURE(void, SetLayerOrder, const Ref<SceneLayer>&, int);
    FUNCTION().PUBLIC().SIGNATURE(const Vector<Ref<SceneLayer>>&, GetLayers);
    FUNCTION().PUBLIC().SIGNATURE(Vector<String>, GetLayersNames);
    FUNCTION().PUBLIC().SIGNATURE(_tmp1, GetLayersMap);
    FUNCTION().PUBLIC().SIGNATURE(Ref<Tag>, GetTag, const String&);
    FUNCTION().PUBLIC().SIGNATURE(Ref<Tag>, AddTag, const String&);
    FUNCTION().PUBLIC().SIGNATURE(void, RemoveTag, const Ref<Tag>&);
    FUNCTION().PUBLIC().SIGNATURE(void, RemoveTag, const String&);
    FUNCTION().PUBLIC().SIGNATURE(const Vector<Ref<Tag>>&, GetTags);
    FUNCTION().PUBLIC().SIGNATURE(const Vector<Ref<Actor>>&, GetRootActors);
    FUNCTION().PUBLIC().SIGNATURE(Vector<Ref<Actor>>&, GetRootActors);
    FUNCTION().PUBLIC().SIGNATURE(const Vector<WeakRef<Actor>>&, GetAllActors);
    FUNCTION().PUBLIC().SIGNATURE(Vector<WeakRef<Actor>>&, GetAllActors);
    FUNCTION().PUBLIC().SIGNATURE(Ref<Actor>, GetActorByID, SceneUID);
    FUNCTION().PUBLIC().SIGNATURE(Ref<Actor>, GetAssetActorByID, const UID&);
    FUNCTION().PUBLIC().SIGNATURE(Ref<Actor>, FindActor, const String&);
    FUNCTION().PUBLIC().SIGNATURE(const Vector<WeakRef<CameraActor>>&, GetCameras);
    FUNCTION().PUBLIC().SIGNATURE(void, Clear, bool);
    FUNCTION().PUBLIC().SIGNATURE(void, ClearCache);
    FUNCTION().PUBLIC().SIGNATURE(void, Load, const String&, bool);
    FUNCTION().PUBLIC().SIGNATURE(void, Load, const DataDocument&, bool);
    FUNCTION().PUBLIC().SIGNATURE(void, Save, const String&);
    FUNCTION().PUBLIC().SIGNATURE(void, Save, DataDocument&);
    FUNCTION().PUBLIC().SIGNATURE(void, Draw);
    FUNCTION().PUBLIC().SIGNATURE(void, Update, float);
    FUNCTION().PUBLIC().SIGNATURE(void, FixedUpdate, float);
    FUNCTION().PUBLIC().SIGNATURE(void, UpdateAddedEntities);
    FUNCTION().PUBLIC().SIGNATURE(void, UpdateTransforms);
    FUNCTION().PUBLIC().SIGNATURE(void, UpdateDestroyingEntities);
    FUNCTION().PUBLIC().SIGNATURE(void, DestroyActor, const Ref<Actor>&);
    FUNCTION().PUBLIC().SIGNATURE(void, DestroyComponent, const Ref<Component>&);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsUpdating);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsEditor);
    FUNCTION().PROTECTED().SIGNATURE_STATIC(void, OnActorCreated, const Ref<Actor>&);
    FUNCTION().PROTECTED().SIGNATURE_STATIC(void, OnActorDestroy, const WeakRef<Actor>&);
    FUNCTION().PROTECTED().SIGNATURE_STATIC(void, OnNewActorParented, const Ref<Actor>&);
    FUNCTION().PROTECTED().SIGNATURE_STATIC(void, OnActorIdChanged, const Ref<Actor>&, SceneUID);
    FUNCTION().PROTECTED().SIGNATURE_STATIC(void, OnAddActorToScene, const Ref<Actor>&);
    FUNCTION().PROTECTED().SIGNATURE_STATIC(void, OnRemoveActorFromScene, const Ref<Actor>&, bool);
    FUNCTION().PROTECTED().SIGNATURE(void, DrawCameras);
    FUNCTION().PROTECTED().SIGNATURE(void, UpdateActors, float);
    FUNCTION().PROTECTED().SIGNATURE(void, UpdateStartingEntities);
    FUNCTION().PROTECTED().SIGNATURE(void, DrawCursorDebugInfo);
    FUNCTION().PROTECTED().SIGNATURE(void, AddActorToScene, const Ref<Actor>&);
    FUNCTION().PROTECTED().SIGNATURE(void, RemoveActorFromScene, const Ref<Actor>&, bool);
    FUNCTION().PROTECTED().SIGNATURE(void, OnComponentAdded, Component*);
    FUNCTION().PROTECTED().SIGNATURE(void, OnComponentRemoved, Component*);
    FUNCTION().PROTECTED().SIGNATURE(void, OnLayerRenamed, SceneLayer*, const String&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCameraAddedOnScene, CameraActor*);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCameraRemovedScene, CameraActor*);
#if  IS_EDITOR          
    FUNCTION().PUBLIC().SIGNATURE_STATIC(void, LinkActorToPrototypesHierarchy, Ref<Actor>, AssetRef<ActorAsset>);
    FUNCTION().PUBLIC().SIGNATURE(void, SetEditorPlaying, bool);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsEditorPlaying);
    FUNCTION().PUBLIC().SIGNATURE(void, SetIsEditor, bool);
    FUNCTION().PUBLIC().SIGNATURE(Vector<Ref<SceneEditableObject>>, GetRootEditableObjects);
    FUNCTION().PUBLIC().SIGNATURE(void, AddEditableObjectToScene, const Ref<SceneEditableObject>&);
    FUNCTION().PUBLIC().SIGNATURE(void, RemoveEditableObjectFromScene, const Ref<SceneEditableObject>&);
    FUNCTION().PUBLIC().SIGNATURE(const Vector<WeakRef<SceneEditableObject>>&, GetAllEditableObjects);
    FUNCTION().PUBLIC().SIGNATURE(const Vector<Ref<SceneEditableObject>>&, GetChangedObjects);
    FUNCTION().PUBLIC().SIGNATURE(const Vector<WeakRef<SceneEditableObject>>&, GetDrawnEditableObjects);
    FUNCTION().PUBLIC().SIGNATURE(Ref<SceneEditableObject>, GetEditableObjectByID, SceneUID);
    FUNCTION().PUBLIC().SIGNATURE(int, GetObjectHierarchyIdx, const Ref<SceneEditableObject>&);
    FUNCTION().PUBLIC().SIGNATURE(void, ReparentEditableObjects, const Vector<Ref<SceneEditableObject>>&, const Ref<SceneEditableObject>&, const Ref<SceneEditableObject>&);
    FUNCTION().PUBLIC().SIGNATURE(void, CheckChangedObjects);
    FUNCTION().PUBLIC().SIGNATURE(_tmp2, GetPrototypesLinksCache);
    FUNCTION().PUBLIC().SIGNATURE(void, BeginDrawingScene);
    FUNCTION().PUBLIC().SIGNATURE(void, EndDrawingScene);
    FUNCTION().PUBLIC().SIGNATURE(void, OnObjectAddToScene, const Ref<SceneEditableObject>&);
    FUNCTION().PUBLIC().SIGNATURE(void, OnObjectRemoveFromScene, const Ref<SceneEditableObject>&);
    FUNCTION().PUBLIC().SIGNATURE(void, OnObjectChanged, const Ref<SceneEditableObject>&);
    FUNCTION().PUBLIC().SIGNATURE(void, OnObjectDrawn, const Ref<SceneEditableObject>&);
    FUNCTION().PUBLIC().SIGNATURE(void, OnActorWithPrototypeCreated, const Ref<Actor>&);
    FUNCTION().PUBLIC().SIGNATURE(void, OnActorLinkedToPrototype, AssetRef<ActorAsset>&, const Ref<Actor>&);
    FUNCTION().PUBLIC().SIGNATURE(void, OnActorPrototypeBroken, Actor*);
#endif
}
END_META;
// --- END META ---
