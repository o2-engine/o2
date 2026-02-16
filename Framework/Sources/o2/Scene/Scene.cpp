#include "o2/stdafx.h"
#include "Scene.h"

#include "o2/Application/Input.h"
#include "o2/Assets/Assets.h"
#include "o2/Assets/Types/ActorAsset.h"
#include "o2/Physics/PhysicsWorld.h"
#include "o2/Render/Render.h"
#include "o2/Render/Text.h"
#include "o2/Render/VectorFontEffects.h"
#include "o2/Scene/Actor.h"
#include "o2/Scene/ActorRefResolver.h"
#include "o2/Scene/CameraActor.h"
#include "o2/Scene/Component.h"
#include "o2/Scene/Component.h"
#include "o2/Scene/SceneLayer.h"
#include "o2/Scene/Tags.h"
#include "o2/Scene/UI/Widget.h"
#include "o2/Scene/UI/WidgetLayout.h"
#include "o2/Utils/Debug/Debug.h"
#include "o2/Utils/Debug/Log/LogStream.h"

namespace o2
{
    DECLARE_SINGLETON(Scene);

    Scene::Scene(RefCounter* refCounter):
        Singleton<Scene>(refCounter)
    {
        mDefaultLayer = AddLayer("Default");

        mLog = mmake<LogStream>("Scene");
        o2Debug.GetLog()->BindStream(mLog);
    }

    Scene::~Scene()
    {
        Clear();
        ClearCache();
    }

    const LogStream& Scene::GetLogStream() const
    {
        return *mLog;
    }

    const Vector<WeakRef<CameraActor>>& Scene::GetCameras() const
    {
        return mCameras;
    }

    void Scene::Update(float dt)
    {
        mIsUpdatingScene = true;

		UpdateDestroyingEntities();
        UpdateAddedEntities();
        UpdateStartingEntities();
        UpdateActors(dt);

        mIsUpdatingScene = false;
    }

    void Scene::FixedUpdate(float dt)
    {
        for (auto& actor : mRootActors)
            actor->FixedUpdate(dt);

        for (auto& actor : mRootActors)
            actor->FixedUpdateChildren(dt);
    }

    void Scene::UpdateAddedEntities()
    {
        auto addedActors = mAddedActors;
        mAddedActors.Clear();

        auto startedActorsCache = mStartActors;
        mStartActors = addedActors;
        startedActorsCache.Clear();

		auto forEachInitializingActor = [&](const Ref<Actor>& actor, const Function<void(const Ref<Actor>&)>& func)
			{
				actor->ForEachActor([&](Actor& actor)
									{
										if (actor.mState == Actor::State::Initializing)
										{
                                            func(Ref(&actor));
											return true;
										}

										return false;
									});
			};

        for (auto& actor : addedActors)
        {
            if (actor->IsOnScene())
				forEachInitializingActor(actor, [&](const Ref<Actor>& actor) { AddActorToScene(actor); });
        }

        for (auto& actor : addedActors)
			forEachInitializingActor(actor, [&](const Ref<Actor>& actor) { actor->UpdateResEnabledInHierarchy(); });

        for (auto& actor : addedActors)
			forEachInitializingActor(actor, [&](const Ref<Actor>& actor) { actor->OnInitialized(); });
    }

    void Scene::UpdateTransforms()
    {
        for (auto& actor : mRootActors)
            actor->UpdateSelfTransform();

        for (auto& actor : mRootActors)
            actor->UpdateChildrenTransforms();
    }

    void Scene::UpdateStartingEntities()
    {
        auto startActors = mStartActors;
        mStartActors.Clear();

        for (auto& actor : startActors)
            actor->OnStart();

        auto startComponents = mStartComponents;
        mStartComponents.Clear();

        for (auto& comp : startComponents)
            comp->OnStart();
    }

    void Scene::UpdateDestroyingEntities()
    {
        auto destroyActors = mDestroyActors;
        auto destroyComponents = mDestroyComponents;

        mDestroyActors.Clear();
        mDestroyComponents.Clear();

        for (auto& component : destroyComponents)
        {
            if (auto owner = component->GetActor())
                owner->RemoveComponent(component);
        }

        for (auto& actor : destroyActors)
        {
            if (auto parent = actor->GetParent().Lock())
                parent->RemoveChild(actor);

            actor->RemoveFromScene();
            actor->OnBeforeDestroy();
        }

        destroyActors.Clear();
    }

    void Scene::DestroyActor(const Ref<Actor>& actor)
    {
        if (actor->mState != Actor::State::Destroying)
        {
            actor->mState = Actor::State::Destroying;
            mDestroyActors.Add(actor);

#if IS_EDITOR
            mChangedObjects.Remove(actor);
#endif
        }
    }

    void Scene::DestroyComponent(const Ref<Component>& component)
    {
        mDestroyComponents.Add(component);
    }

    bool Scene::IsUpdating() const
    {
        return mIsUpdatingScene;
    }

    bool Scene::IsEditor() const
    {
#if IS_EDITOR
        return true;
#endif

        return false;
    }

    void Scene::OnActorCreated(const Ref<Actor>& actor)
    {
        if (!IsSingletonInitialzed())
            return;

        mInstance->mAddedActors.Add(actor);
    }

    void Scene::OnActorDestroy(const WeakRef<Actor>& actor)
    {
        if (!IsSingletonInitialzed())
            return;

//         if (actor->IsOnScene())
//             Instance().RemoveActorFromScene(actor, false);
    }

    void Scene::OnNewActorParented(const Ref<Actor>& actor)
    {
        if (!IsSingletonInitialzed())
            return;

        mInstance->mAddedActors.Remove(actor);
    }

    void Scene::OnAddActorToScene(const Ref<Actor>& actor)
    {
        Assert(IsSingletonInitialzed(), "Cant add actor to scene, because scene not initialized")

        mInstance->AddActorToScene(actor);
    }

    void Scene::OnRemoveActorFromScene(const Ref<Actor>& actor, bool keepEditorObjects /*= false*/)
    {
        if (!IsSingletonInitialzed())
            return;

        mInstance->RemoveActorFromScene(actor, keepEditorObjects);
    }

    void Scene::OnActorIdChanged(const Ref<Actor>& actor, SceneUID prevId)
    {
        if (!IsSingletonInitialzed())
            return

            mInstance->mActorsMap.Remove(prevId);
        mInstance->mActorsMap[actor->mId] = actor;
    }

    void Scene::UpdateActors(float dt)
    {
        for (auto& actor : mRootActors)
            actor->Update(dt);

        for (auto& actor : mRootActors)
            actor->UpdateChildren(dt);
    }

#undef DrawText

    void Scene::Draw()
    {
        DrawCameras();
    }

    void Scene::DrawCameras()
    {
#if IS_EDITOR
        BeginDrawingScene();
#endif

        if (mCameras.IsEmpty())
        {
            if constexpr (IS_EDITOR)
            {
                o2Render.SetCamera(Camera());
				o2Render.Clear();

				Text text("debugFont.ttf");
				text.SetText("No camera");
				text.Draw();
            }
        }
        else
        {
            PROFILE_SAMPLE("Draw cameras");

            for (auto& camera : mCameras)
            {
                if (auto cameraRef = camera.Lock()) 
                {
                    cameraRef->SetupAndDraw();
                    cameraRef->listenersLayer->OnDrawn(Camera().GetBasis());
                }
            }
        }

        DrawCursorDebugInfo();

#if IS_EDITOR
        EndDrawingScene();
#endif
    }

    void Scene::DrawCursorDebugInfo()
    {
        if (!o2Input.IsKeyDown(VK_F4))
            return;

        struct helper
        {
            static void Process(String& debugInfo, Actor* actor)
            {
                if (actor->transform->IsPointInside(o2Input.GetCursorPos()))
                {
                    auto parent = actor;
                    String path;
                    while (parent)
                    {
                        path = parent->GetName() + "/" + path;
                        parent = parent->GetParent().Lock().Get();
                    }

                    debugInfo += " -- " + path + " - " + actor->GetType().GetName() + "\n";
                    debugInfo += "Rect: " + (String)actor->transform->worldRect + "\n";

                    Widget* widget = dynamic_cast<Widget*>(actor);
                    if (widget)
                    {
                        debugInfo += "layout - " +
                            (String)widget->layout->anchorMin + " " + (String)widget->layout->offsetMin + " - " +
                            (String)widget->layout->anchorMax + " " + (String)widget->layout->offsetMax + "\n";

                        debugInfo += (widget->mIsClipped ? (String)"clipped, " : (String)"not clipped, ") +
                            (widget->IsEnabled() ? (String)"visible\n" : ((String)"hidden" + (!widget->mResEnabledInHierarchy ? (String)" fully\n" : (String)"\n")));

                        debugInfo += "alpha: " + (String)widget->mTransparency +
                            ", res alpha: " + (String)widget->mResTransparency + "\n";
                    }

                    debugInfo += "\n";

                    if (widget)
                    {
                        for (auto& child : widget->mInternalWidgets)
                            Process(debugInfo, child.Get());
                    }
                }

                for (auto& child : actor->mChildren)
                    Process(debugInfo, child.Get());
            }
        };

        String debugInfo;
        for (auto& actor : mAllActors)
            helper::Process(debugInfo, actor.Lock().Get());

        o2Debug.DrawText(((Vec2F)o2Render.GetResolution().InvertedX()) * 0.5f, debugInfo);
    }

    void Scene::AddActorToScene(const Ref<Actor>& actor)
    {
        if (!actor->mParent)
            mRootActors.Add(actor);

        mAllActors.Add(actor);
        mActorsMap[actor->mId] = actor;

        actor->OnAddToScene();

#if IS_EDITOR
        mChangedObjects.Add(actor);
        AddEditableObjectToScene(actor);
        onAddedToScene(actor);
#endif
    }

    void Scene::RemoveActorFromScene(const Ref<Actor>& actor, bool keepEditorObjects /*= false*/)
    {
        if (!actor->mParent)
            mRootActors.Remove(actor);

        mAllActors.Remove(actor);
        mActorsMap.Remove(actor->mId);

        mStartActors.Remove(actor);
        mAddedActors.Remove(actor);

        actor->OnRemoveFromScene();

#if IS_EDITOR
        if (!keepEditorObjects)
            RemoveEditableObjectFromScene(actor);

        OnObjectRemoveFromScene(actor);
#endif
    }

    void Scene::OnComponentAdded(Component* component)
    {
        mStartComponents.Add(Ref(component));
    }

    void Scene::OnComponentRemoved(Component* component)
    {
        mStartComponents.RemoveFirst([&](auto& x) { return x == component; });
    }

    void Scene::OnLayerRenamed(SceneLayer* layer, const String& oldName)
    {
        mLayersMap.Remove(oldName);
        mLayersMap[layer->GetName()] = Ref(layer);

#if IS_EDITOR
        onLayersListChanged();
#endif
    }

    void Scene::OnCameraAddedOnScene(CameraActor* camera)
    {
        mCameras.Add(Ref(camera));
    }

    void Scene::OnCameraRemovedScene(CameraActor* camera)
    {
        mCameras.Remove(Ref(camera));
    }

    bool Scene::HasLayer(const String& name) const
    {
        return mLayersMap.ContainsKey(name);
    }

    Ref<SceneLayer> Scene::GetLayer(const String& name)
    {
        WeakRef<SceneLayer> layer;
        if (mLayersMap.TryGetValue(name, layer))
            return layer.Lock();

        return AddLayer(name);
    }

    const Ref<SceneLayer>& Scene::GetDefaultLayer() const
    {
        return mDefaultLayer;
    }

    Ref<SceneLayer> Scene::AddLayer(const String& name)
    {
        WeakRef<SceneLayer> layer;
        if (mLayersMap.TryGetValue(name, layer))
            return layer.Lock();

        auto newLayer = mmake<SceneLayer>();
        newLayer->mName = name;
        mLayers.Add(newLayer);
        mLayersMap[name] = newLayer;

#if IS_EDITOR
        onLayersListChanged();
#endif

        return newLayer;
    }

    void Scene::RemoveLayer(const Ref<SceneLayer>& layer)
    {
        if (layer == mDefaultLayer)
            return;

        mLayers.Remove(layer);
        mLayersMap.Remove(layer->mName);

#if IS_EDITOR
        onLayersListChanged();
#endif
    }

    void Scene::RemoveLayer(const String& name)
    {
        RemoveLayer(GetLayer(name));
    }

    void Scene::SetLayerOrder(const Ref<SceneLayer>& layer, int idx)
    {
        mLayers.Remove(layer);
        mLayers.Insert(layer, idx);
    }

    Ref<Tag> Scene::GetTag(const String& name) const
    {
        return mTags.FindOrDefault([&](auto x) { return x->GetName() == name; });
    }

    Ref<Tag> Scene::AddTag(const String& name)
    {
        if (GetTag(name))
        {
            o2Debug.LogError("Can't create new tag with name " + name + ": already exist");
            return nullptr;
        }

        auto newTag = mmake<Tag>();
        newTag->SetName(name);
        mTags.Add(newTag);

        return newTag;
    }

    void Scene::RemoveTag(const Ref<Tag>& tag)
    {
        mTags.Remove(tag);
    }

    void Scene::RemoveTag(const String& name)
    {
        RemoveTag(GetTag(name));
    }

    const Vector<Ref<SceneLayer>>& Scene::GetLayers()const
    {
        return mLayers;
    }

    Vector<String> Scene::GetLayersNames() const
    {
        return mLayers.Convert<String>([](auto& x) { return x->GetName(); });
    }

    const Map<String, WeakRef<SceneLayer>>& Scene::GetLayersMap() const
    {
        return mLayersMap;
    }

    const Vector<Ref<Tag>>& Scene::GetTags() const
    {
        return mTags;
    }

    const Vector<Ref<Actor>>& Scene::GetRootActors() const
    {
        return mRootActors;
    }

    Vector<Ref<Actor>>& Scene::GetRootActors()
    {
        return mRootActors;
    }

    const Vector<WeakRef<Actor>>& Scene::GetAllActors() const
    {
        return mAllActors;
    }

    Vector<WeakRef<Actor>>& Scene::GetAllActors()
    {
        return mAllActors;
    }

    Ref<Actor> Scene::GetActorByID(SceneUID id) const
    {
        return mActorsMap.FindKey(id).second.Lock();
    }

    Ref<Actor> Scene::GetAssetActorByID(const UID& id)
    {
        auto cached = mAssetsCache.FindOrDefault([=](const AssetRef<ActorAsset>& x) { 
            if (x) 
                return x->GetUID() == id; 
            else
                return false; 
        });

        if (!cached)
        {
            cached = AssetRef<ActorAsset>(id);
            mAssetsCache.Add(cached);

            return nullptr;
        }

        return cached->GetActor();
    }

    Ref<Actor> Scene::FindActor(const String& path)
    {
        int delPos = path.Find("/");
        String pathPart = path.SubStr(0, delPos);

        for (auto& actor : mRootActors)
        {
            if (actor->mName == pathPart)
            {
                if (delPos == -1)
                    return actor;
                else
                    return actor->GetChild(path.SubStr(delPos + 1));
            }
        }

        return nullptr;
    }

    void Scene::Clear(bool keepDefaultLayer /*= true*/)
    {
        mDestroyActors = mRootActors;
        UpdateDestroyingEntities();

        mAddedActors.Clear();
        mStartActors.Clear();
        mStartComponents.Clear();
        mLayers.Clear();
        mLayersMap.Clear();

        if (keepDefaultLayer)
            mDefaultLayer = AddLayer("Default");

        mTags.Clear();
    }

    void Scene::ClearCache()
    {
        mAssetsCache.Clear();
    }

    void Scene::Load(const String& path, bool append /*= false*/)
    {
        DataDocument data;
        if (data.LoadFromFile(path))
            Load(data, append);
        else
            o2Debug.LogError("Failed to load scene " + path);
    }

    void Scene::Load(const DataDocument& doc, bool append /*= false*/)
    {
        ActorRefResolver::mInstance->LockResolving();

        if (!append)
            Clear(false);

        auto& layersNode = doc.GetMember("Layers");
        if (!layersNode.IsEmpty())
        {
            for (auto& layerNode : layersNode)
            {
                auto layer = mmake<SceneLayer>();
                layer->Deserialize(layerNode);
                mLayers.Add(layer);
                mLayersMap[layer->mName] = layer;
            }
        }

        auto& defaultLayerNode = doc.GetMember("DefaultLayer");
        if (!defaultLayerNode.IsEmpty())
            mDefaultLayer = GetLayer(defaultLayerNode);
        else
            mDefaultLayer = AddLayer("Default");

#if IS_EDITOR
        onLayersListChanged();
#endif

        auto& tagsNode = doc.GetMember("Tags");
        if (!tagsNode.IsEmpty())
        {
            for (auto& tagNode : tagsNode)
            {
                auto tag = mmake<Tag>();
                tag->Deserialize(tagNode);
                mTags.Add(tag);
            }
        }

        auto& actorsNode = doc.GetMember("Actors");
        if (!actorsNode.IsEmpty())
        {
            mRootActors = actorsNode;
        }

        ActorRefResolver::mInstance->UnlockResolving();
        ActorRefResolver::mInstance->ResolveRefs();

		UpdateTransforms();
		mRootActors.Clear();

		UpdateAddedEntities();
		UpdateTransforms();

        UpdateDestroyingEntities();

#if IS_EDITOR
        mChangedObjects.Clear();
#endif
    }

    void Scene::Save(const String& path)
    {
        DataDocument data;
        Save(data);
        data.SaveToFile(path);
    }

    void Scene::Save(DataDocument& doc)
    {
        auto& layersNode = doc.AddMember("Layers");
        for (auto& layer : mLayers)
            layer->Serialize(layersNode.AddElement());

        doc.AddMember("DefaultLayer") = mDefaultLayer->mName;

        auto& tagsNode = doc.AddMember("Tags");
        for (auto& tag : mTags)
            tag->Serialize(tagsNode.AddElement());

        doc.AddMember("Actors") = mRootActors;
    }

#if IS_EDITOR
    void Scene::LinkActorToPrototypesHierarchy(Ref<Actor> actor, AssetRef<ActorAsset> proto)
    {
        if (!IsSingletonInitialzed())
            return;

        while (proto)
        {
            o2Scene.OnActorLinkedToPrototype(proto, actor);
            proto = proto->GetActor()->GetPrototype();
        }
    }

    bool Scene::IsEditorPlaying() const
    {
        return mIsPlaying;
    }
    
    void Scene::SetEditorPlaying(bool playing)
    {
        mIsPlaying = playing;
    }

    Vector<Ref<SceneEditableObject>> Scene::GetRootEditableObjects()
    {
        return mRootActors.Convert<Ref<SceneEditableObject>>([](const Ref<Actor>& x) { return DynamicCast<SceneEditableObject>(x); });
    }

    void Scene::AddEditableObjectToScene(const Ref<SceneEditableObject>& object)
    {
        mEditableObjects.Add(object);
        mEditableObjectsByUID[object->GetID()] = object;
    }

    void Scene::RemoveEditableObjectFromScene(const Ref<SceneEditableObject>& object)
    {
        mChangedObjects.RemoveAll([&](auto x) { return x == object; });
        mEditableObjects.RemoveAll([&](auto x) { return x == object; });
        mEditableObjectsByUID.Remove(object->GetID());
    }

    const Vector<WeakRef<SceneEditableObject>>& Scene::GetAllEditableObjects()
    {
        return mEditableObjects;
    }

    Ref<SceneEditableObject> Scene::GetEditableObjectByID(SceneUID id) const
    {
        return mEditableObjectsByUID[id].Lock();
    }

    int Scene::GetObjectHierarchyIdx(const Ref<SceneEditableObject>& object) const
    {
        if (object->GetEditableParent())
        {
            return object->GetEditableParent()->GetEditableChildren().IndexOf(object) + GetObjectHierarchyIdx(object->GetEditableParent());
        }

        return mRootActors.IndexOf([=](const Ref<Actor>& x) { return DynamicCast<SceneEditableObject>(x) == object; });
    }

    void Scene::ReparentEditableObjects(const Vector<Ref<SceneEditableObject>>& objects,
                                        const Ref<SceneEditableObject>& newParent, const Ref<SceneEditableObject>& prevObject)
    {
        struct Object
        {
            Ref<SceneEditableObject> object;

            int   idx;
            Basis transform;

            bool operator==(const Object& other) const { return object == other.object; }
        };
        Vector<Object> objectsDefs;

        for (auto& object : objects)
        {
            Object def;
            def.object = object;
            def.transform = object->GetTransform();
            def.idx = o2Scene.GetObjectHierarchyIdx(object);
            objectsDefs.Add(def);

            object->SetEditableParent(nullptr);
            mRootActors.Remove(DynamicCast<Actor>(object));
        }

        objectsDefs.Sort([](auto& a, auto& b) { return a.idx < b.idx; });

        if (newParent)
        {
            int insertIdx = newParent->GetEditableChildren().IndexOf(prevObject) + 1;

            for (auto& def : objectsDefs)
            {
                newParent->AddEditableChild(def.object, insertIdx++);
                def.object->SetTransform(def.transform);
            }
        }
        else
        {
            int insertIdx = 0;

            if (prevObject)
            {
                insertIdx = mRootActors.IndexOf([=](const Ref<Actor>& x) { return DynamicCast<SceneEditableObject>(x) == prevObject; });

                if (insertIdx < 0)
                    insertIdx = mRootActors.Count();
                else
                    insertIdx++;
            }

            for (auto& def : objectsDefs)
            {
                auto actorEditableObject = DynamicCast<Actor>(def.object);
                if (actorEditableObject)
                {
                    mRootActors.Insert(actorEditableObject, insertIdx++);
                    def.object->OnEditableParentChanged(nullptr);
                    def.object->SetTransform(def.transform);
                }
            }
        }

        if (newParent)
            OnObjectChanged(newParent);

        if (prevObject)
            OnObjectChanged(prevObject);
    }

    void Scene::CheckChangedObjects()
    {
        mChangedObjects.RemoveAll([](auto x) { return x == nullptr; });

        if (mChangedObjects.Count() > 0)
        {
            onObjectsChanged(mChangedObjects);

            auto changedObjectsCopy = mChangedObjects;
            for (auto& obj : changedObjectsCopy)
                obj->Update(0.0f);

            mChangedObjects.Clear();

            UpdateAddedEntities();
            UpdateDestroyingEntities();
        }
    }

    const Vector<Ref<SceneEditableObject>>& Scene::GetChangedObjects() const
    {
        return mChangedObjects;
    }

    const Vector<WeakRef<SceneEditableObject>>& Scene::GetDrawnEditableObjects() const
    {
        return mDrawnObjects;
    }

    Map<AssetRef<ActorAsset>, Vector<WeakRef<Actor>>>& Scene::GetPrototypesLinksCache()
    {
        return mPrototypeLinksCache;
    }

    void Scene::BeginDrawingScene()
    {
        mIsDrawingScene = true;
        mDrawnObjects.Clear();
    }

    void Scene::EndDrawingScene()
    {
        mIsDrawingScene = false;
    }

    void Scene::OnObjectAddToScene(const Ref<SceneEditableObject>& object)
    {
        onAddedToScene(object);
        OnObjectChanged(object);
    }

    void Scene::OnObjectRemoveFromScene(const Ref<SceneEditableObject>& object)
    {
        onRemovedFromScene(object);
        OnObjectChanged(nullptr);
    }

    void Scene::OnObjectChanged(const Ref<SceneEditableObject>& object)
    {
        if (mIsUpdatingScene)
            return;

        if (PhysicsWorld::IsSingletonInitialzed() && o2Physics.IsUpdatingPhysicsNow())
            return;

        if (object && object->changedFrame == o2Time.GetCurrentFrame())
            return;

        if (object)
            object->changedFrame = o2Time.GetCurrentFrame();

        mChangedObjects.Add(object);
    }

    void Scene::OnObjectDrawn(const Ref<SceneEditableObject>& object)
    {
        if (mIsDrawingScene)
            mDrawnObjects.Add(object);
    }

    void Scene::OnActorWithPrototypeCreated(const Ref<Actor>& actor)
    {
        auto proto = actor->GetPrototype();
        OnActorLinkedToPrototype(proto, actor);
    }

    void Scene::OnActorLinkedToPrototype(AssetRef<ActorAsset>& assetRef, const Ref<Actor>& actor)
    {
        if (!mPrototypeLinksCache.ContainsKey(assetRef))
            mPrototypeLinksCache.Add(assetRef, Vector<WeakRef<Actor>>());

        mPrototypeLinksCache[assetRef].Add(actor);
    }

    void Scene::OnActorPrototypeBroken(Actor* actor)
    {
        // !!! TODO: Optimize this
        for (auto it = mPrototypeLinksCache.Begin(); it != mPrototypeLinksCache.End();)
        {
            it->second.RemoveFirst([=](auto& x) { return x == actor; });
            if (it->second.IsEmpty())
            {
                mPrototypeLinksCache.Remove(it->first);
                it = mPrototypeLinksCache.Begin();
            }
            else it++;
        }
    }

#endif
}
// --- META ---

DECLARE_CLASS(o2::Scene, o2__Scene);
// --- END META ---
