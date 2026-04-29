#include "o2/stdafx.h"

#include "tests/Scene/SceneTestHelpers.h"

namespace o2
{
    TestComponent::TestComponent() = default;
    TestComponent::TestComponent(RefCounter* refCounter): Component(refCounter) {}
    TestComponent::TestComponent(const TestComponent& other):
        Component(other), testValue(other.testValue)
    {}
    TestComponent::TestComponent(RefCounter* refCounter, const TestComponent& other):
        Component(refCounter, other), testValue(other.testValue)
    {}

    void TestComponent::OnInitialized()           { ++onInitializedCount; }
    void TestComponent::OnStart()                 { ++onStartCount; }
    void TestComponent::OnUpdate(float)           { ++onUpdateCount; }
    void TestComponent::OnFixedUpdate(float)      { ++onFixedUpdateCount; }
    void TestComponent::OnEnabled()               { ++onEnabledCount; }
    void TestComponent::OnDisabled()              { ++onDisabledCount; }
    void TestComponent::OnAddToScene()            { ++onAddToSceneCount; }
    void TestComponent::OnRemoveFromScene()       { ++onRemoveFromSceneCount; }
    void TestComponent::OnDestroy()               { ++onDestroyCount; }
    void TestComponent::OnTransformUpdated()      { ++onTransformUpdatedCount; }
    void TestComponent::OnParentChanged(const Ref<Actor>&)   { ++onParentChangedCount; }
    void TestComponent::OnChildrenChanged()       { ++onChildrenChangedCount; }
    void TestComponent::OnChildAdded(const Ref<Actor>&)      { ++onChildAddedCount; }
    void TestComponent::OnChildRemoved(const Ref<Actor>&)    { ++onChildRemovedCount; }
    void TestComponent::OnComponentAdded(const Ref<Component>&)      { ++onComponentAddedCount; }
    void TestComponent::OnComponentRemoving(const Ref<Component>&)   { ++onComponentRemovingCount; }
    void TestComponent::OnDraw()                  { ++onDrawCount; }

    TestComponent2::TestComponent2() = default;
    TestComponent2::TestComponent2(RefCounter* refCounter): Component(refCounter) {}
    TestComponent2::TestComponent2(const TestComponent2& other):
        Component(other), marker(other.marker)
    {}
    TestComponent2::TestComponent2(RefCounter* refCounter, const TestComponent2& other):
        Component(refCounter, other), marker(other.marker)
    {}

    SceneCleanGuard::SceneCleanGuard() = default;

    SceneCleanGuard::~SceneCleanGuard()
    {
        o2Scene.Clear(true);
        o2Scene.UpdateDestroyingEntities();
    }

    void TickFrame(float dt)
    {
        o2Scene.UpdateAddedEntities();
        o2Scene.UpdateTransforms();
        o2Scene.Update(dt);
        o2Scene.UpdateDestroyingEntities();
    }

    void TickFrames(int count, float dt)
    {
        for (int i = 0; i < count; ++i)
            TickFrame(dt);
    }

    Ref<Actor> MakeActor(const String& name, ActorCreateMode mode)
    {
        auto actor = mmake<Actor>(mode);
        actor->SetName(name);
        return actor;
    }

    Ref<Actor> MakeActorWithTestComponent(const String& name, Ref<TestComponent>& outComponent,
                                          ActorCreateMode mode)
    {
        auto actor = MakeActor(name, mode);
        outComponent = actor->AddComponent<TestComponent>();
        return actor;
    }
}
// --- META ---

DECLARE_CLASS(o2::TestComponent, o2__TestComponent);

DECLARE_CLASS(o2::TestComponent2, o2__TestComponent2);
// --- END META ---
