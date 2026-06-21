#include "o2Editor/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Scene/Actor.h"
#include "o2/Scene/Component.h"
#include "o2/Scene/Components/EditorTestComponent.h"
#include "o2/Scene/SceneLayersList.h"
#include "o2/Scene/Tags.h"
#include "o2/Utils/ValueProxy.h"

#include "o2Editor/Properties/Basic/ActorProperty.h"
#include "o2Editor/Properties/Basic/BooleanProperty.h"
#include "o2Editor/Properties/Basic/ComponentProperty.h"
#include "o2Editor/Properties/Basic/EnumProperty.h"
#include "o2Editor/Properties/Basic/FloatProperty.h"
#include "o2Editor/Properties/Basic/IntegerProperty.h"
#include "o2Editor/Properties/Basic/SceneLayersListProperty.h"
#include "o2Editor/Properties/Basic/StringProperty.h"
#include "o2Editor/Properties/Basic/TagProperty.h"
#include "o2Editor/Properties/Basic/WStringProperty.h"
#include "support/EditorTestScene.h"

using namespace o2;
using namespace Editor;
using namespace Editor::Tests;

// Per concrete property type, assert two behaviors:
//   - action mode: the user change goes through the action; the proxy is NOT written by the field.
//   - proxy mode (the "old way"): the field writes the value through the proxy.
// A Probe exposes the protected user-change entry point and no-ops the view so the field works
// without its real widget tree (which needs UI styles unavailable headless).

namespace
{
    template<typename TProperty, typename TValue>
    class Probe : public TProperty
    {
    public:
        Probe(RefCounter* refCounter): TProperty(refCounter) {}

        void Init() { this->InitializeControls(); }
        void Commit(const TValue& value) { this->SetValueByUserAndComplete(value); }
        void UpdateValueView() override {}
    };

    template<typename TProperty, typename TValue>
    void ExpectActionModeDoesNotWriteProxy(const TValue& from, const TValue& to)
    {
        TValue backing = from;
        auto f = mmake<Probe<TProperty, TValue>>();
        f->Init();

        EXPECT_TRUE(f->IsValueChangeAppliedByAction()) << "type must opt into action mode in InitializeControls";

        f->SetValueProxy({ mmake<PointerValueProxy<TValue>>(&backing) });

        bool completed = false;
        f->onChangeCompleted = [&](const String&, const Vector<DataDocument>&, const Vector<DataDocument>&) { completed = true; };

        f->Commit(to);

        EXPECT_TRUE(completed) << "action mode must emit onChangeCompleted so the action applies the value";
        EXPECT_TRUE(backing == from) << "action mode must NOT write the proxy directly; the action owns the mutation";
    }

    template<typename TProperty, typename TValue>
    void ExpectProxyModeWritesProxy(const TValue& from, const TValue& to)
    {
        TValue backing = from;
        auto f = mmake<Probe<TProperty, TValue>>();
        f->Init();
        f->SetValueChangeAppliedByAction(false); // the old way: affect the value through the proxy

        f->SetValueProxy({ mmake<PointerValueProxy<TValue>>(&backing) });
        f->Commit(to);

        EXPECT_TRUE(backing == to) << "proxy mode must write the value through the proxy";
    }
}

TEST(PropertyTypeActionApply, Float_ActionModeDoesNotWriteProxy)  { ExpectActionModeDoesNotWriteProxy<FloatProperty, float>(5.0f, 42.0f); }
TEST(PropertyTypeActionApply, Float_ProxyModeWritesProxy)         { ExpectProxyModeWritesProxy<FloatProperty, float>(5.0f, 42.0f); }

TEST(PropertyTypeActionApply, Integer_ActionModeDoesNotWriteProxy){ ExpectActionModeDoesNotWriteProxy<IntegerProperty, int>(5, 42); }
TEST(PropertyTypeActionApply, Integer_ProxyModeWritesProxy)       { ExpectProxyModeWritesProxy<IntegerProperty, int>(5, 42); }

TEST(PropertyTypeActionApply, Boolean_ActionModeDoesNotWriteProxy){ ExpectActionModeDoesNotWriteProxy<BooleanProperty, bool>(false, true); }
TEST(PropertyTypeActionApply, Boolean_ProxyModeWritesProxy)       { ExpectProxyModeWritesProxy<BooleanProperty, bool>(false, true); }

TEST(PropertyTypeActionApply, String_ActionModeDoesNotWriteProxy) { ExpectActionModeDoesNotWriteProxy<StringProperty, String>(String("old"), String("new")); }
TEST(PropertyTypeActionApply, String_ProxyModeWritesProxy)        { ExpectProxyModeWritesProxy<StringProperty, String>(String("old"), String("new")); }

TEST(PropertyTypeActionApply, WString_ActionModeDoesNotWriteProxy){ ExpectActionModeDoesNotWriteProxy<WStringProperty, WString>(WString("old"), WString("new")); }
TEST(PropertyTypeActionApply, WString_ProxyModeWritesProxy)       { ExpectProxyModeWritesProxy<WStringProperty, WString>(WString("old"), WString("new")); }

TEST(PropertyTypeActionApply, Enum_ActionModeDoesNotWriteProxy)   { ExpectActionModeDoesNotWriteProxy<EnumProperty, int>(0, 2); }
TEST(PropertyTypeActionApply, Enum_ProxyModeWritesProxy)          { ExpectProxyModeWritesProxy<EnumProperty, int>(0, 2); }

TEST(PropertyTypeActionApply, Tags_ActionModeDoesNotWriteProxy)
{
    TagGroup from;
    TagGroup to; to.AddTag("a"); to.AddTag("b");
    ExpectActionModeDoesNotWriteProxy<TagsProperty, TagGroup>(from, to);
}

TEST(PropertyTypeActionApply, Tags_ProxyModeWritesProxy)
{
    TagGroup from;
    TagGroup to; to.AddTag("a"); to.AddTag("b");
    ExpectProxyModeWritesProxy<TagsProperty, TagGroup>(from, to);
}

TEST(PropertyTypeActionApply, SceneLayersList_ActionModeDoesNotWriteProxy)
{
    SceneLayersList from;
    SceneLayersList to(Vector<String>{ "Layer1" });
    ExpectActionModeDoesNotWriteProxy<SceneLayersListProperty, SceneLayersList>(from, to);
}

TEST(PropertyTypeActionApply, SceneLayersList_ProxyModeWritesProxy)
{
    SceneLayersList from;
    SceneLayersList to(Vector<String>{ "Layer1" });
    ExpectProxyModeWritesProxy<SceneLayersListProperty, SceneLayersList>(from, to);
}

TEST(PropertyTypeActionApply, Actor_ActionModeDoesNotWriteProxy)
{
    SceneCleanGuard guard;
    auto actor = MakeActor();
    TickScene();
    ExpectActionModeDoesNotWriteProxy<ActorProperty, LinkRef<Actor>>(LinkRef<Actor>(), LinkRef<Actor>(actor));
}

TEST(PropertyTypeActionApply, Actor_ProxyModeWritesProxy)
{
    SceneCleanGuard guard;
    auto actor = MakeActor();
    TickScene();
    ExpectProxyModeWritesProxy<ActorProperty, LinkRef<Actor>>(LinkRef<Actor>(), LinkRef<Actor>(actor));
}

TEST(PropertyTypeActionApply, Component_ActionModeDoesNotWriteProxy)
{
    SceneCleanGuard guard;
    auto actor = MakeActor();
    Ref<Component> comp = actor->AddComponent<EditorTestComponent>();
    TickScene();
    ExpectActionModeDoesNotWriteProxy<ComponentProperty, LinkRef<Component>>(LinkRef<Component>(), LinkRef<Component>(comp));
}

TEST(PropertyTypeActionApply, Component_ProxyModeWritesProxy)
{
    SceneCleanGuard guard;
    auto actor = MakeActor();
    Ref<Component> comp = actor->AddComponent<EditorTestComponent>();
    TickScene();
    ExpectProxyModeWritesProxy<ComponentProperty, LinkRef<Component>>(LinkRef<Component>(), LinkRef<Component>(comp));
}
