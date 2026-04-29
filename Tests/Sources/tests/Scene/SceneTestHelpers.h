#pragma once

#include "o2/Scene/Actor.h"
#include "o2/Scene/Component.h"
#include "o2/Scene/Scene.h"

namespace o2
{
    class TestComponent: public Component
    {
    public:
        int testValue = 0; // @SERIALIZABLE

        int onInitializedCount = 0;
        int onStartCount = 0;
        int onUpdateCount = 0;
        int onFixedUpdateCount = 0;
        int onEnabledCount = 0;
        int onDisabledCount = 0;
        int onAddToSceneCount = 0;
        int onRemoveFromSceneCount = 0;
        int onDestroyCount = 0;
        int onTransformUpdatedCount = 0;
        int onParentChangedCount = 0;
        int onChildrenChangedCount = 0;
        int onChildAddedCount = 0;
        int onChildRemovedCount = 0;
        int onComponentAddedCount = 0;
        int onComponentRemovingCount = 0;
        int onDrawCount = 0;

        TestComponent();
        explicit TestComponent(RefCounter* refCounter);
        TestComponent(const TestComponent& other);
        TestComponent(RefCounter* refCounter, const TestComponent& other);

        SERIALIZABLE(TestComponent);
        CLONEABLE_REF(TestComponent);

    protected:
        void OnInitialized() override;
        void OnStart() override;
        void OnUpdate(float dt) override;
        void OnFixedUpdate(float dt) override;
        void OnEnabled() override;
        void OnDisabled() override;
        void OnAddToScene() override;
        void OnRemoveFromScene() override;
        void OnDestroy() override;
        void OnTransformUpdated() override;
        void OnParentChanged(const Ref<Actor>& oldParent) override;
        void OnChildrenChanged() override;
        void OnChildAdded(const Ref<Actor>& child) override;
        void OnChildRemoved(const Ref<Actor>& child) override;
        void OnComponentAdded(const Ref<Component>& component) override;
        void OnComponentRemoving(const Ref<Component>& component) override;
        void OnDraw() override;
    };

    class TestComponent2: public Component
    {
    public:
        int marker = 0; // @SERIALIZABLE

        TestComponent2();
        explicit TestComponent2(RefCounter* refCounter);
        TestComponent2(const TestComponent2& other);
        TestComponent2(RefCounter* refCounter, const TestComponent2& other);

        SERIALIZABLE(TestComponent2);
        CLONEABLE_REF(TestComponent2);
    };

    class SceneCleanGuard
    {
    public:
        SceneCleanGuard();
        ~SceneCleanGuard();
    };

    // Saves o2Time.GetLocalTime() in ctor and restores it in dtor. Use when a
    // test calls SetLocalTime/ResetLocalTime so the global doesn't bleed into
    // subsequent tests.
    class TimeGuard
    {
    public:
        TimeGuard();
        ~TimeGuard();

    private:
        float mSavedLocalTime;
    };

    void TickFrame(float dt = 0.0f);
    void TickFrames(int count, float dt = 0.0f);

    Ref<Actor> MakeActor(const String& name = "actor", ActorCreateMode mode = ActorCreateMode::InScene);
    Ref<Actor> MakeActorWithTestComponent(const String& name, Ref<TestComponent>& outComponent,
                                          ActorCreateMode mode = ActorCreateMode::InScene);
}
// --- META ---

CLASS_BASES_META(o2::TestComponent)
{
    BASE_CLASS(o2::Component);
}
END_META;
CLASS_FIELDS_META(o2::TestComponent)
{
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(0).NAME(testValue);
    FIELD().PUBLIC().DEFAULT_VALUE(0).NAME(onInitializedCount);
    FIELD().PUBLIC().DEFAULT_VALUE(0).NAME(onStartCount);
    FIELD().PUBLIC().DEFAULT_VALUE(0).NAME(onUpdateCount);
    FIELD().PUBLIC().DEFAULT_VALUE(0).NAME(onFixedUpdateCount);
    FIELD().PUBLIC().DEFAULT_VALUE(0).NAME(onEnabledCount);
    FIELD().PUBLIC().DEFAULT_VALUE(0).NAME(onDisabledCount);
    FIELD().PUBLIC().DEFAULT_VALUE(0).NAME(onAddToSceneCount);
    FIELD().PUBLIC().DEFAULT_VALUE(0).NAME(onRemoveFromSceneCount);
    FIELD().PUBLIC().DEFAULT_VALUE(0).NAME(onDestroyCount);
    FIELD().PUBLIC().DEFAULT_VALUE(0).NAME(onTransformUpdatedCount);
    FIELD().PUBLIC().DEFAULT_VALUE(0).NAME(onParentChangedCount);
    FIELD().PUBLIC().DEFAULT_VALUE(0).NAME(onChildrenChangedCount);
    FIELD().PUBLIC().DEFAULT_VALUE(0).NAME(onChildAddedCount);
    FIELD().PUBLIC().DEFAULT_VALUE(0).NAME(onChildRemovedCount);
    FIELD().PUBLIC().DEFAULT_VALUE(0).NAME(onComponentAddedCount);
    FIELD().PUBLIC().DEFAULT_VALUE(0).NAME(onComponentRemovingCount);
    FIELD().PUBLIC().DEFAULT_VALUE(0).NAME(onDrawCount);
}
END_META;
CLASS_METHODS_META(o2::TestComponent)
{

    FUNCTION().PUBLIC().CONSTRUCTOR();
    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*);
    FUNCTION().PUBLIC().CONSTRUCTOR(const TestComponent&);
    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*, const TestComponent&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnInitialized);
    FUNCTION().PROTECTED().SIGNATURE(void, OnStart);
    FUNCTION().PROTECTED().SIGNATURE(void, OnUpdate, float);
    FUNCTION().PROTECTED().SIGNATURE(void, OnFixedUpdate, float);
    FUNCTION().PROTECTED().SIGNATURE(void, OnEnabled);
    FUNCTION().PROTECTED().SIGNATURE(void, OnDisabled);
    FUNCTION().PROTECTED().SIGNATURE(void, OnAddToScene);
    FUNCTION().PROTECTED().SIGNATURE(void, OnRemoveFromScene);
    FUNCTION().PROTECTED().SIGNATURE(void, OnDestroy);
    FUNCTION().PROTECTED().SIGNATURE(void, OnTransformUpdated);
    FUNCTION().PROTECTED().SIGNATURE(void, OnParentChanged, const Ref<Actor>&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnChildrenChanged);
    FUNCTION().PROTECTED().SIGNATURE(void, OnChildAdded, const Ref<Actor>&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnChildRemoved, const Ref<Actor>&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnComponentAdded, const Ref<Component>&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnComponentRemoving, const Ref<Component>&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnDraw);
}
END_META;

CLASS_BASES_META(o2::TestComponent2)
{
    BASE_CLASS(o2::Component);
}
END_META;
CLASS_FIELDS_META(o2::TestComponent2)
{
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(0).NAME(marker);
}
END_META;
CLASS_METHODS_META(o2::TestComponent2)
{

    FUNCTION().PUBLIC().CONSTRUCTOR();
    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*);
    FUNCTION().PUBLIC().CONSTRUCTOR(const TestComponent2&);
    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*, const TestComponent2&);
}
END_META;
// --- END META ---
