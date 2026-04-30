#include "o2/stdafx.h"

#include <gtest/gtest.h>

#include "o2/Render/Sprite.h"
#include "o2/Scene/UI/Widget.h"
#include "o2/Scene/UI/WidgetLayer.h"
#include "o2/Scene/UI/WidgetState.h"
#include "Scene/SceneTestHelpers.h"
#include "Scene/UI/UITestHelpers.h"

using namespace o2;

// ===== Construction =====

TEST(Widget, DefaultConstructionIsValid)
{
    SceneCleanGuard guard;
    auto w = mmake<Widget>(ActorCreateMode::InScene);
    ASSERT_TRUE(w);
    EXPECT_TRUE(w->layout != nullptr);
}

TEST(Widget, IsAnActor)
{
    SceneCleanGuard guard;
    auto w = mmake<Widget>(ActorCreateMode::InScene);
    Ref<Actor> asActor = w;
    EXPECT_TRUE(asActor);
    EXPECT_NE(w->GetID(), 0);
}

TEST(Widget, ConstructorWithComponentsAddsThem)
{
    SceneCleanGuard guard;
    Vector<Ref<Component>> components;
    components.Add(mmake<TestComponent>());
    auto w = mmake<Widget>(components, ActorCreateMode::InScene);
    EXPECT_EQ(w->GetComponents().Count(), 1);
    EXPECT_TRUE(w->GetComponent<TestComponent>());
}

TEST(Widget, CopyConstructorPreservesNameAndTransparency)
{
    SceneCleanGuard guard;
    auto src = mmake<Widget>(ActorCreateMode::InScene);
    src->SetName("source");
    src->SetTransparency(0.5f);

    auto copy = src->CloneAsRef<Widget>();
    EXPECT_EQ(copy->GetName(), "source");
    EXPECT_FLOAT_EQ(copy->GetTransparency(), 0.5f);
}

// ===== Hierarchy =====

TEST(Widget, AddChildWidgetSetsParentWidget)
{
    SceneCleanGuard guard;
    auto parent = mmake<Widget>(ActorCreateMode::InScene);
    auto child = mmake<Widget>(ActorCreateMode::InScene);
    parent->AddChildWidget(child);

    EXPECT_EQ(child->GetParentWidget().Lock(), parent);
    EXPECT_EQ(parent->GetChildWidgets().Count(), 1);
    EXPECT_EQ(parent->GetChildWidgets()[0], child);
}

TEST(Widget, AddChildAcceptsActorAndKeepsParentWidget)
{
    SceneCleanGuard guard;
    auto parent = mmake<Widget>(ActorCreateMode::InScene);
    auto child = mmake<Widget>(ActorCreateMode::InScene);
    parent->AddChild(DynamicCast<Actor>(child));
    EXPECT_EQ(parent->GetChildren().Count(), 1);
    EXPECT_EQ(parent->GetChildWidgets().Count(), 1);
}

TEST(Widget, GetChildWidgetByPath)
{
    SceneCleanGuard guard;
    auto root = MakeWidget("root");
    auto a = MakeChildWidget(root, "a");
    auto b = MakeChildWidget(a, "b");

    EXPECT_EQ(root->GetChildWidget("a"), a);
    EXPECT_EQ(root->GetChildWidget("a/b"), b);
}

TEST(Widget, ChildWidgetMixedWithNonWidgetActor)
{
    SceneCleanGuard guard;
    auto parent = mmake<Widget>(ActorCreateMode::InScene);
    auto widgetChild = mmake<Widget>(ActorCreateMode::InScene);
    auto actorChild = mmake<Actor>(ActorCreateMode::InScene);
    parent->AddChild(widgetChild);
    parent->AddChild(actorChild);

    EXPECT_EQ(parent->GetChildren().Count(), 2);
    EXPECT_EQ(parent->GetChildWidgets().Count(), 1);
    EXPECT_EQ(parent->GetChildWidgets()[0], widgetChild);
}

// ===== Layers =====

TEST(Widget, AddLayerAppearsInLayers)
{
    SceneCleanGuard guard;
    auto w = mmake<Widget>(ActorCreateMode::InScene);
    auto layer = mmake<WidgetLayer>();
    layer->name = "back";
    w->AddLayer(layer);
    EXPECT_EQ(w->GetLayers().Count(), 1);
    EXPECT_EQ(w->GetLayer("back"), layer);
}

TEST(Widget, AddLayerByNameAndDrawable)
{
    SceneCleanGuard guard;
    auto w = mmake<Widget>(ActorCreateMode::InScene);
    auto added = w->AddLayer("front", MakeStubRectDrawable());
    ASSERT_TRUE(added);
    EXPECT_EQ(added->name, "front");
    EXPECT_EQ(w->FindLayer("front"), added);
}

TEST(Widget, RemoveLayerByRefRemoves)
{
    SceneCleanGuard guard;
    auto w = mmake<Widget>(ActorCreateMode::InScene);
    auto layer = mmake<WidgetLayer>();
    layer->name = "x";
    w->AddLayer(layer);
    w->RemoveLayer(layer);
    EXPECT_EQ(w->GetLayers().Count(), 0);
}

TEST(Widget, RemoveLayerByPathRemoves)
{
    SceneCleanGuard guard;
    auto w = mmake<Widget>(ActorCreateMode::InScene);
    auto added = w->AddLayer("x", MakeStubRectDrawable());
    w->RemoveLayer(String("x"));
    EXPECT_EQ(w->GetLayers().Count(), 0);
}

TEST(Widget, RemoveAllLayersClears)
{
    SceneCleanGuard guard;
    auto w = mmake<Widget>(ActorCreateMode::InScene);
    w->AddLayer("a", MakeStubRectDrawable());
    w->AddLayer("b", MakeStubRectDrawable());
    w->RemoveAllLayers();
    EXPECT_EQ(w->GetLayers().Count(), 0);
}

// ===== States =====

TEST(Widget, AddStateByNameAppearsInStates)
{
    SceneCleanGuard guard;
    auto w = mmake<Widget>(ActorCreateMode::InScene);
    auto s = w->AddState("highlight");
    ASSERT_TRUE(s);
    EXPECT_EQ(s->name, "highlight");
    EXPECT_TRUE(w->GetStateObject("highlight"));
}

TEST(Widget, SetStateAndGetStateRoundTrip)
{
    SceneCleanGuard guard;
    auto w = mmake<Widget>(ActorCreateMode::InScene);
    w->AddState("on");
    w->SetStateForcible("on", true);
    EXPECT_TRUE(w->GetState("on"));
    w->SetStateForcible("on", false);
    EXPECT_FALSE(w->GetState("on"));
}

TEST(Widget, RemoveStateByNameReturnsTrueAndRemoves)
{
    SceneCleanGuard guard;
    auto w = mmake<Widget>(ActorCreateMode::InScene);
    w->AddState("temp");
    EXPECT_TRUE(w->RemoveState("temp"));
    EXPECT_FALSE(w->GetStateObject("temp"));
}

TEST(Widget, RemoveAllStatesClears)
{
    SceneCleanGuard guard;
    auto w = mmake<Widget>(ActorCreateMode::InScene);
    w->AddState("a");
    w->AddState("b");
    w->RemoveAllStates();
    EXPECT_EQ(w->GetStates().Count(), 0);
}

TEST(Widget, GetStateOnUnknownReturnsFalse)
{
    SceneCleanGuard guard;
    auto w = mmake<Widget>(ActorCreateMode::InScene);
    EXPECT_FALSE(w->GetState("unknown"));
}

// ===== Transparency =====

TEST(Widget, DefaultTransparencyIsOne)
{
    SceneCleanGuard guard;
    auto w = mmake<Widget>(ActorCreateMode::InScene);
    EXPECT_FLOAT_EQ(w->GetTransparency(), 1.0f);
    EXPECT_FLOAT_EQ(w->GetResTransparency(), 1.0f);
}

TEST(Widget, SetTransparencyRoundTrip)
{
    SceneCleanGuard guard;
    auto w = mmake<Widget>(ActorCreateMode::InScene);
    w->SetTransparency(0.25f);
    EXPECT_FLOAT_EQ(w->GetTransparency(), 0.25f);
}

TEST(Widget, ChildResTransparencyMultipliesParent)
{
    SceneCleanGuard guard;
    auto parent = mmake<Widget>(ActorCreateMode::InScene);
    auto child = mmake<Widget>(ActorCreateMode::InScene);
    parent->AddChildWidget(child);

    parent->SetTransparency(0.5f);
    child->SetTransparency(0.5f);
    TickFrame();

    EXPECT_NEAR(child->GetResTransparency(), 0.25f, 0.001f);
}

TEST(Widget, TransparencyCascadeAcrossThreeLevelsMultiplies)
{
    SceneCleanGuard guard;
    auto root = mmake<Widget>(ActorCreateMode::InScene);
    auto mid = mmake<Widget>(ActorCreateMode::InScene);
    auto leaf = mmake<Widget>(ActorCreateMode::InScene);
    root->AddChildWidget(mid);
    mid->AddChildWidget(leaf);

    root->SetTransparency(0.5f);
    mid->SetTransparency(0.5f);
    leaf->SetTransparency(0.5f);
    TickFrame();

    EXPECT_NEAR(leaf->GetResTransparency(), 0.125f, 0.001f);
}

TEST(Widget, TransparencyZeroDoesNotFireOnHide)
{
    SceneCleanGuard guard;
    auto w = mmake<Widget>(ActorCreateMode::InScene);
    WidgetEventCounter counter;
    AttachEventCounter(w, &counter);
    TickFrame();
    int before = counter.onHideCount;
    w->SetTransparency(0.0f);
    TickFrame();
    EXPECT_EQ(counter.onHideCount, before);
}

// ===== Focus =====

TEST(Widget, NewWidgetIsNotFocused)
{
    SceneCleanGuard guard;
    auto w = mmake<Widget>(ActorCreateMode::InScene);
    EXPECT_FALSE(w->IsFocused());
}

TEST(Widget, SetFocusableRoundTrip)
{
    SceneCleanGuard guard;
    auto w = mmake<Widget>(ActorCreateMode::InScene);
    EXPECT_FALSE(w->IsFocusable());
    w->SetFocusable(true);
    EXPECT_TRUE(w->IsFocusable());
}

TEST(Widget, FocusOnFocusableSetsFocused)
{
    SceneCleanGuard guard;
    auto w = mmake<Widget>(ActorCreateMode::InScene);
    w->SetFocusable(true);
    TickFrame();
    w->Focus();
    EXPECT_TRUE(w->IsFocused());
    w->Unfocus();
    EXPECT_FALSE(w->IsFocused());
}

// ===== Visibility =====

TEST(Widget, ShowEnablesWidget)
{
    SceneCleanGuard guard;
    auto w = mmake<Widget>(ActorCreateMode::InScene);
    w->Hide(true);
    EXPECT_FALSE(w->IsEnabled());
    w->Show(true);
    EXPECT_TRUE(w->IsEnabled());
}

TEST(Widget, ShowEventFiresOnShow)
{
    SceneCleanGuard guard;
    auto w = mmake<Widget>(ActorCreateMode::InScene);
    WidgetEventCounter counter;
    AttachEventCounter(w, &counter);

    w->Hide(true);
    int hideBefore = counter.onHideCount;
    w->Show(true);
    EXPECT_GE(counter.onShowCount, 1);
    EXPECT_GE(counter.onHideCount, hideBefore);
}

TEST(Widget, SetEnabledForciblyIsAlias)
{
    SceneCleanGuard guard;
    auto w = mmake<Widget>(ActorCreateMode::InScene);
    w->SetEnabledForcible(false);
    EXPECT_FALSE(w->IsEnabled());
    w->SetEnabledForcible(true);
    EXPECT_TRUE(w->IsEnabled());
}

// ===== Internal widgets =====

TEST(Widget, AddInternalWidgetAppearsInInternals)
{
    SceneCleanGuard guard;
    auto parent = mmake<Widget>(ActorCreateMode::InScene);
    auto internal = mmake<Widget>(ActorCreateMode::InScene);
    internal->SetName("guts");
    parent->AddInternalWidget(internal);

    EXPECT_TRUE(parent->GetInternalWidget("guts"));
    EXPECT_EQ(parent->GetInternalWidget("guts"), internal);
}

TEST(Widget, RemoveInternalWidgetClears)
{
    SceneCleanGuard guard;
    auto parent = mmake<Widget>(ActorCreateMode::InScene);
    auto internal = mmake<Widget>(ActorCreateMode::InScene);
    internal->SetName("guts");
    parent->AddInternalWidget(internal);
    parent->RemoveInternalWidget(internal);
    EXPECT_FALSE(parent->GetInternalWidget("guts"));
}

TEST(Widget, FindInternalWidgetByName)
{
    SceneCleanGuard guard;
    auto parent = mmake<Widget>(ActorCreateMode::InScene);
    auto internal = mmake<Widget>(ActorCreateMode::InScene);
    internal->SetName("nested");
    parent->AddInternalWidget(internal);

    EXPECT_EQ(parent->FindInternalWidget("nested"), internal);
}

// ===== Layout =====

TEST(Widget, SetLayoutDirtyTriggersOnLayoutUpdated)
{
    SceneCleanGuard guard;
    auto w = mmake<Widget>(ActorCreateMode::InScene);
    WidgetEventCounter counter;
    AttachEventCounter(w, &counter);

    TickFrame();
    int before = counter.onLayoutUpdatedCount;
    w->SetLayoutDirty();
    TickFrame();
    EXPECT_GT(counter.onLayoutUpdatedCount, before);
}

// ===== FindActorById =====

TEST(Widget, FindActorByIdLocatesNestedChild)
{
    SceneCleanGuard guard;
    auto root = mmake<Widget>(ActorCreateMode::InScene);
    auto child = mmake<Widget>(ActorCreateMode::InScene);
    root->AddChildWidget(child);

    EXPECT_EQ(root->FindActorById(child->GetID()), DynamicCast<Actor>(child));
}

// ===== Sibling order =====

TEST(Widget, SetIndexInSiblingsReordersChildWidgetsList)
{
    SceneCleanGuard guard;
    auto parent = mmake<Widget>(ActorCreateMode::InScene);
    auto a = MakeChildWidget(parent, "a");
    auto b = MakeChildWidget(parent, "b");
    auto c = MakeChildWidget(parent, "c");

    ASSERT_EQ(parent->GetChildWidgets().Count(), 3);
    EXPECT_EQ(parent->GetChildWidgets()[0], a);
    EXPECT_EQ(parent->GetChildWidgets()[1], b);
    EXPECT_EQ(parent->GetChildWidgets()[2], c);

    // SetIndexInSiblings inserts at the new position then removes the old —
    // so moving 'a' from index 0 to index 2 yields [b, a, c]. See
    // Actor::SetIndexInSiblings for the exact semantics.
    a->SetIndexInSiblings(2);
    EXPECT_EQ(parent->GetChildWidgets()[0], b);
    EXPECT_EQ(parent->GetChildWidgets()[1], a);
    EXPECT_EQ(parent->GetChildWidgets()[2], c);
}

#if IS_EDITOR
// ===== Editor: LayersEditable =====

TEST(Widget, LayersEditableHasIdAndName)
{
    SceneCleanGuard guard;
    auto w = mmake<Widget>(ActorCreateMode::InScene);
    ASSERT_TRUE(w->layersEditable);
    EXPECT_NE(w->layersEditable->GetID(), 0);
    EXPECT_FALSE(w->layersEditable->GetName().IsEmpty());
}

TEST(Widget, LayersEditableNotSupportsDeleting)
{
    SceneCleanGuard guard;
    auto w = mmake<Widget>(ActorCreateMode::InScene);
    EXPECT_FALSE(w->layersEditable->IsSupportsDeleting());
}

TEST(Widget, InternalChildrenEditableHasIdAndName)
{
    SceneCleanGuard guard;
    auto w = mmake<Widget>(ActorCreateMode::InScene);
    ASSERT_TRUE(w->internalChildrenEditable);
    EXPECT_NE(w->internalChildrenEditable->GetID(), 0);
    EXPECT_FALSE(w->internalChildrenEditable->GetName().IsEmpty());
}

TEST(Widget, InternalChildrenEditableNotSupportsDeleting)
{
    SceneCleanGuard guard;
    auto w = mmake<Widget>(ActorCreateMode::InScene);
    EXPECT_FALSE(w->internalChildrenEditable->IsSupportsDeleting());
}
#endif // IS_EDITOR
