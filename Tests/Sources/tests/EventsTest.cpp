#include "o2/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Application/Input.h"
#include "o2/Events/ApplicationEventsListener.h"
#include "o2/Events/CursorAreaEventsListener.h"
#include "o2/Events/CursorAreaEventsListenersLayer.h"
#include "o2/Events/EventSystem.h"
#include "o2/Events/IEventsListener.h"
#include "o2/Events/KeyboardEventsListener.h"
#include "o2/Events/ShortcutKeysListener.h"
#include "o2/Utils/System/ShortcutKeys.h"
#include "o2/Utils/Types/Ref.h"

using namespace o2;

namespace {

class TestableKeyboardListener : public RefCounterable, public KeyboardEventsListener
{
public:
    int pressedCount = 0;
    int releasedCount = 0;
    int stayDownCount = 0;
    Input::Key lastKey;

    void TriggerKeyPressed(const Input::Key& key) { OnKeyPressed(key); }
    void TriggerKeyReleased(const Input::Key& key) { OnKeyReleased(key); }
    void TriggerKeyStayDown(const Input::Key& key) { OnKeyStayDown(key); }

    RefCounter* GetRefCounter() const override { return RefCounterable::GetRefCounter(); }

protected:
    void OnKeyPressed(const Input::Key& key) override { pressedCount++; lastKey = key; }
    void OnKeyReleased(const Input::Key& key) override { releasedCount++; }
    void OnKeyStayDown(const Input::Key& key) override { stayDownCount++; }
};

class TestableApplicationListener : public RefCounterable, public ApplicationEventsListener
{
public:
    int started = 0, closing = 0, activated = 0, deactivated = 0, sized = 0;

    void TriggerStarted() { OnApplicationStarted(); }
    void TriggerClosing() { OnApplicationClosing(); }
    void TriggerActivated() { OnApplicationActivated(); }
    void TriggerDeactivated() { OnApplicationDeactivated(); }
    void TriggerSized() { OnApplicationSized(); }

    RefCounter* GetRefCounter() const override { return RefCounterable::GetRefCounter(); }

    void OnApplicationStarted() override { started++; }
    void OnApplicationClosing() override { closing++; }
    void OnApplicationActivated() override { activated++; }
    void OnApplicationDeactivated() override { deactivated++; }
    void OnApplicationSized() override { sized++; }
};

class TestableCursorAreaListener : public RefCounterable, public CursorAreaEventsListener
{
public:
    bool isUnderPointReturn = true;
    int interactableHookCount = 0;
    int notInteractableHookCount = 0;
    int cursorPressedCount = 0;
    int cursorReleasedCount = 0;

    void TriggerCursorPressed(const Input::Cursor& c) { OnCursorPressed(c); }
    void TriggerCursorReleased(const Input::Cursor& c) { OnCursorReleased(c); }

    void SetScissor(const RectF& r) { mScissorRect = r; }
    bool IsUnderPoint(const Vec2F&) override { return isUnderPointReturn; }

    RefCounter* GetRefCounter() const override { return RefCounterable::GetRefCounter(); }

protected:
    void OnBecomeInteractable() override { interactableHookCount++; }
    void OnBecomeNotInteractable() override { notInteractableHookCount++; }
    void OnCursorPressed(const Input::Cursor& cursor) override
    {
        cursorPressedCount++;
        CursorAreaEventsListener::OnCursorPressed(cursor);
    }
    void OnCursorReleased(const Input::Cursor& cursor) override
    {
        cursorReleasedCount++;
        CursorAreaEventsListener::OnCursorReleased(cursor);
    }
};

const RectF kWideScissor(-1000.0f, 1000.0f, 1000.0f, -1000.0f);

class TestableLayer : public CursorAreaEventListenersLayer
{
public:
    bool isUnderPointReturn = true;
    bool IsUnderPoint(const Vec2F&) override { return isUnderPointReturn; }
    void SetScissor(const RectF& r) { mScissorRect = r; }
};

class TestableShortcutListener : public RefCounterable, public ShortcutKeysListener
{
public:
    int firedCount = 0;
    void TriggerOnShortcutPressed() { OnShortcutPressed(); }

    RefCounter* GetRefCounter() const override { return RefCounterable::GetRefCounter(); }

protected:
    void OnShortcutPressed() override { firedCount++; }
};

class ExposedFunctionalShortcutListener : public FunctionalShortcutKeysListener
{
public:
    using FunctionalShortcutKeysListener::FunctionalShortcutKeysListener;
    void TriggerOnShortcutPressed() { OnShortcutPressed(); }
};

class GlobalDefaultGuard
{
public:
    GlobalDefaultGuard() : mOriginal(EventSystem::eventsListenersEnabledByDefault) {}
    ~GlobalDefaultGuard() { EventSystem::eventsListenersEnabledByDefault = mOriginal; }

private:
    bool mOriginal;
};

} // namespace

// ===== IEventsListener =====

TEST(IEventsListener, DefaultEnabledMatchesGlobalSettingTrue) {
    GlobalDefaultGuard guard;
    EventSystem::eventsListenersEnabledByDefault = true;
    auto listener = mmake<TestableKeyboardListener>();
    EXPECT_TRUE(listener->IsListeningEvents());
}

TEST(IEventsListener, DefaultEnabledMatchesGlobalSettingFalse) {
    GlobalDefaultGuard guard;
    EventSystem::eventsListenersEnabledByDefault = false;
    auto listener = mmake<TestableKeyboardListener>();
    EXPECT_FALSE(listener->IsListeningEvents());
}

TEST(IEventsListener, SetListeningEventsTogglesFlag) {
    auto listener = mmake<TestableKeyboardListener>();
    listener->SetListeningEvents(false);
    EXPECT_FALSE(listener->IsListeningEvents());
    listener->SetListeningEvents(true);
    EXPECT_TRUE(listener->IsListeningEvents());
}

// ===== KeyboardEventsListener =====

TEST(KeyboardEventsListener, SubclassOverrideReceivesAllKeyEvents) {
    auto listener = mmake<TestableKeyboardListener>();
    Input::Key key;
    key.keyCode = 'A';

    listener->TriggerKeyPressed(key);
    listener->TriggerKeyStayDown(key);
    listener->TriggerKeyStayDown(key);
    listener->TriggerKeyReleased(key);

    EXPECT_EQ(listener->pressedCount, 1);
    EXPECT_EQ(listener->stayDownCount, 2);
    EXPECT_EQ(listener->releasedCount, 1);
    EXPECT_EQ(listener->lastKey.keyCode, 'A');
}

TEST(KeyboardEventsListener, MultipleListenersCreatedAndDestroyedDoNotCrash) {
    {
        auto a = mmake<TestableKeyboardListener>();
        auto b = mmake<TestableKeyboardListener>();
        auto c = mmake<TestableKeyboardListener>();
        EXPECT_TRUE(a && b && c);
    }
    auto fresh = mmake<TestableKeyboardListener>();
    EXPECT_TRUE(fresh);
}

// ===== ApplicationEventsListener =====

TEST(ApplicationEventsListener, SubclassOverrideReceivesAllLifecycleEvents) {
    auto listener = mmake<TestableApplicationListener>();
    listener->TriggerStarted();
    listener->TriggerActivated();
    listener->TriggerDeactivated();
    listener->TriggerSized();
    listener->TriggerClosing();

    EXPECT_EQ(listener->started, 1);
    EXPECT_EQ(listener->activated, 1);
    EXPECT_EQ(listener->deactivated, 1);
    EXPECT_EQ(listener->sized, 1);
    EXPECT_EQ(listener->closing, 1);
}

// ===== CursorAreaEventsListener =====

TEST(CursorAreaEventsListener, IsInteractableDefaultsToTrue) {
    auto listener = mmake<TestableCursorAreaListener>();
    EXPECT_TRUE(listener->IsInteractable());
}

TEST(CursorAreaEventsListener, SetInteractableTogglesFlag) {
    auto listener = mmake<TestableCursorAreaListener>();
    listener->SetInteractable(false);
    EXPECT_FALSE(listener->IsInteractable());
    listener->SetInteractable(true);
    EXPECT_TRUE(listener->IsInteractable());
}

TEST(CursorAreaEventsListener, SetInteractableInvokesHooksOnTransitionsOnly) {
    auto listener = mmake<TestableCursorAreaListener>();
    listener->SetInteractable(true); // already true, no transition
    EXPECT_EQ(listener->interactableHookCount, 0);
    EXPECT_EQ(listener->notInteractableHookCount, 0);

    listener->SetInteractable(false); // transition
    EXPECT_EQ(listener->notInteractableHookCount, 1);
    EXPECT_EQ(listener->interactableHookCount, 0);

    listener->SetInteractable(false); // no transition
    EXPECT_EQ(listener->notInteractableHookCount, 1);

    listener->SetInteractable(true); // transition
    EXPECT_EQ(listener->interactableHookCount, 1);
}

TEST(CursorAreaEventsListener, IsUnderPointDefaultReturnsTrue) {
    auto listener = mmake<TestableCursorAreaListener>();
    EXPECT_TRUE(listener->IsUnderPoint(Vec2F(0, 0)));
    EXPECT_TRUE(listener->IsUnderPoint(Vec2F(100, 100)));
}

TEST(CursorAreaEventsListener, IsInputTransparentAndScrollableDefaultsAreFalse) {
    auto listener = mmake<TestableCursorAreaListener>();
    EXPECT_FALSE(listener->IsInputTransparent());
    EXPECT_FALSE(listener->IsScrollable());
    EXPECT_FALSE(listener->IsPressed());
}

TEST(CursorAreaEventsListener, MessageFallDownListenerForwardsCursorPressed) {
    auto top = mmake<TestableCursorAreaListener>();
    auto fallDown = mmake<TestableCursorAreaListener>();
    top->messageFallDownListener = fallDown.Get();

    Input::Cursor cursor;
    top->TriggerCursorPressed(cursor);

    EXPECT_EQ(top->cursorPressedCount, 1);
    EXPECT_EQ(fallDown->cursorPressedCount, 1);

    top->TriggerCursorReleased(cursor);
    EXPECT_EQ(fallDown->cursorReleasedCount, 1);
}

// ===== ShortcutKeysListener =====

TEST(ShortcutKeysListener, SetShortcutGetShortcutRoundtrip) {
    auto listener = mmake<TestableShortcutListener>();
    ShortcutKeys sc({(KeyboardKey)'A', VK_CONTROL});
    listener->SetShortcut(sc);
    EXPECT_TRUE(listener->GetShortcut() == sc);
}

TEST(ShortcutKeysListener, IsEnabledDefaultsToTrue) {
    auto listener = mmake<TestableShortcutListener>();
    EXPECT_TRUE(listener->IsEnabled());
}

TEST(ShortcutKeysListener, SetEnabledTogglesFlag) {
    auto listener = mmake<TestableShortcutListener>();
    listener->SetEnabled(false);
    EXPECT_FALSE(listener->IsEnabled());
    listener->SetEnabled(true);
    EXPECT_TRUE(listener->IsEnabled());
}

TEST(ShortcutKeysListener, SubclassOnShortcutPressedOverrideFires) {
    auto listener = mmake<TestableShortcutListener>();
    listener->TriggerOnShortcutPressed();
    listener->TriggerOnShortcutPressed();
    EXPECT_EQ(listener->firedCount, 2);
}

TEST(ShortcutKeysListener, RegisterDestroyReregisterDoesNotCrash) {
    ShortcutKeys sc({(KeyboardKey)'B'});
    {
        auto a = mmake<TestableShortcutListener>();
        a->SetShortcut(sc);
        a->SetMaxPriority();
        a->SetMinPriority();
    }
    {
        auto b = mmake<TestableShortcutListener>();
        b->SetShortcut(sc);
    }
    SUCCEED();
}

TEST(ShortcutKeysListener, EmptyShortcutSilentlyNotRegistered) {
    auto listener = mmake<TestableShortcutListener>();
    ShortcutKeys empty;
    EXPECT_TRUE(empty.IsEmpty());
    listener->SetShortcut(empty);
    EXPECT_TRUE(listener->GetShortcut().IsEmpty());
}

// ===== FunctionalShortcutKeysListener =====

TEST(FunctionalShortcutKeysListener, OnShortcutPressedFiresUserFunction) {
    int counter = 0;
    auto listener = mmake<ExposedFunctionalShortcutListener>([&] { counter++; });
    listener->TriggerOnShortcutPressed();
    listener->TriggerOnShortcutPressed();
    EXPECT_EQ(counter, 2);
}

TEST(FunctionalShortcutKeysListener, DefaultConstructorEmptyFunctionDoesNotCrash) {
    auto listener = mmake<ExposedFunctionalShortcutListener>();
    listener->TriggerOnShortcutPressed();
    SUCCEED();
}

// ===== EventSystem =====

TEST(EventSystem, GetCurrentCursorAreaEventsLayerReturnsBasicLayer) {
    auto layer = o2Events.GetCurrentCursorAreaEventsLayer();
    EXPECT_TRUE(layer != nullptr);
}

TEST(EventSystem, GetAllCursorListenersUnderCursorDoesNotCrashWithoutInput) {
    auto result = o2Events.GetAllCursorListenersUnderCursor(0);
    SUCCEED();
}

// ===== CursorAreaEventListenersLayer =====

namespace {

Ref<TestableCursorAreaListener> MakeHittableListener()
{
    auto l = mmake<TestableCursorAreaListener>();
    l->SetScissor(kWideScissor);
    l->isUnderPointReturn = true;
    return l;
}

} // namespace

TEST(Layer, UpdateReversesDrawOrderSoLastDrawnBecomesTopmost) {
    auto layer = mmake<CursorAreaEventListenersLayer>();
    auto a = MakeHittableListener();
    auto b = MakeHittableListener();
    auto c = MakeHittableListener();

    layer->cursorEventAreaListeners.Add(a);
    layer->cursorEventAreaListeners.Add(b);
    layer->cursorEventAreaListeners.Add(c);

    layer->Update();

    ASSERT_EQ(layer->cursorEventAreaListeners.Count(), 3);
    EXPECT_EQ(layer->cursorEventAreaListeners[0], c);
    EXPECT_EQ(layer->cursorEventAreaListeners[1], b);
    EXPECT_EQ(layer->cursorEventAreaListeners[2], a);
}

TEST(Layer, GetAllCursorListenersReturnsListenerInsideScissorAndUnderPoint) {
    auto layer = mmake<CursorAreaEventListenersLayer>();
    auto l = MakeHittableListener();
    layer->cursorEventAreaListeners.Add(l);

    auto under = layer->GetAllCursorListenersUnderCursor(Vec2F(0, 0));
    ASSERT_EQ(under.Count(), 1);
    EXPECT_EQ(under[0], l);
}

TEST(Layer, GetAllCursorListenersExcludesListenerNotUnderPoint) {
    auto layer = mmake<CursorAreaEventListenersLayer>();
    auto miss = MakeHittableListener();
    miss->isUnderPointReturn = false;
    auto hit = MakeHittableListener();

    layer->cursorEventAreaListeners.Add(miss);
    layer->cursorEventAreaListeners.Add(hit);

    auto under = layer->GetAllCursorListenersUnderCursor(Vec2F(0, 0));
    ASSERT_EQ(under.Count(), 1);
    EXPECT_EQ(under[0], hit);
}

TEST(Layer, GetAllCursorListenersExcludesListenerWhenPointOutsideScissor) {
    auto layer = mmake<CursorAreaEventListenersLayer>();
    auto clipped = MakeHittableListener();
    clipped->SetScissor(RectF(-10.0f, 10.0f, 10.0f, -10.0f)); // tight box around origin
    auto wide = MakeHittableListener();

    layer->cursorEventAreaListeners.Add(clipped);
    layer->cursorEventAreaListeners.Add(wide);

    auto inside = layer->GetAllCursorListenersUnderCursor(Vec2F(0, 0));
    EXPECT_EQ(inside.Count(), 2);

    auto outside = layer->GetAllCursorListenersUnderCursor(Vec2F(50, 50));
    ASSERT_EQ(outside.Count(), 1);
    EXPECT_EQ(outside[0], wide);
}

TEST(Layer, GetAllCursorListenersExcludesNonInteractableListener) {
    auto layer = mmake<CursorAreaEventListenersLayer>();
    auto disabled = MakeHittableListener();
    disabled->SetInteractable(false);
    auto enabled = MakeHittableListener();

    layer->cursorEventAreaListeners.Add(disabled);
    layer->cursorEventAreaListeners.Add(enabled);

    auto under = layer->GetAllCursorListenersUnderCursor(Vec2F(0, 0));
    ASSERT_EQ(under.Count(), 1);
    EXPECT_EQ(under[0], enabled);
}

TEST(Layer, GetAllCursorListenersReturnsListenersInStoredOrder) {
    auto layer = mmake<CursorAreaEventListenersLayer>();
    auto a = MakeHittableListener();
    auto b = MakeHittableListener();
    auto c = MakeHittableListener();

    layer->cursorEventAreaListeners.Add(a);
    layer->cursorEventAreaListeners.Add(b);
    layer->cursorEventAreaListeners.Add(c);

    auto under = layer->GetAllCursorListenersUnderCursor(Vec2F(0, 0));
    ASSERT_EQ(under.Count(), 3);
    EXPECT_EQ(under[0], a);
    EXPECT_EQ(under[1], b);
    EXPECT_EQ(under[2], c);
}

TEST(Layer, GetAllCursorListenersRecursesIntoNestedLayer) {
    auto outer = mmake<CursorAreaEventListenersLayer>();
    auto inner = mmake<TestableLayer>();
    inner->SetScissor(kWideScissor);
    inner->isUnderPointReturn = true;

    auto innerListener = MakeHittableListener();
    inner->cursorEventAreaListeners.Add(innerListener);

    outer->cursorEventAreaListeners.Add(inner);

    auto under = outer->GetAllCursorListenersUnderCursor(Vec2F(0, 0));
    ASSERT_EQ(under.Count(), 1);
    EXPECT_EQ(under[0], innerListener);
}

TEST(Layer, PostUpdateClearsCursorEventAreaListeners) {
    auto layer = mmake<CursorAreaEventListenersLayer>();
    layer->cursorEventAreaListeners.Add(MakeHittableListener());
    layer->cursorEventAreaListeners.Add(MakeHittableListener());
    ASSERT_EQ(layer->cursorEventAreaListeners.Count(), 2);

    layer->PostUpdate();
    EXPECT_EQ(layer->cursorEventAreaListeners.Count(), 0);
}

TEST(Layer, UnregCursorAreaListenerRemovesFromList) {
    auto layer = mmake<CursorAreaEventListenersLayer>();
    auto a = MakeHittableListener();
    auto b = MakeHittableListener();
    layer->cursorEventAreaListeners.Add(a);
    layer->cursorEventAreaListeners.Add(b);

    layer->UnregCursorAreaListener(a.Get());
    ASSERT_EQ(layer->cursorEventAreaListeners.Count(), 1);
    EXPECT_EQ(layer->cursorEventAreaListeners[0], b);
}

TEST(Layer, IsInputTransparentReflectsIsTransparentField) {
    auto layer = mmake<CursorAreaEventListenersLayer>();
    EXPECT_FALSE(layer->IsInputTransparent());
    layer->isTransparent = true;
    EXPECT_TRUE(layer->IsInputTransparent());
}

TEST(CursorAreaEventsListener, OnDrawnAddsListenerToCurrentLayer) {
    auto current = o2Events.GetCurrentCursorAreaEventsLayer();
    ASSERT_TRUE(current != nullptr);

    int before = current->cursorEventAreaListeners.Count();
    auto l = mmake<TestableCursorAreaListener>();
    l->OnDrawn();

    EXPECT_EQ(current->cursorEventAreaListeners.Count(), before + 1);
    EXPECT_EQ(current->cursorEventAreaListeners.Last(), l);
}

TEST(CursorAreaEventsListener, OnDrawnSkippedWhenNotInteractable) {
    auto current = o2Events.GetCurrentCursorAreaEventsLayer();
    int before = current->cursorEventAreaListeners.Count();

    auto l = mmake<TestableCursorAreaListener>();
    l->SetInteractable(false);
    l->OnDrawn();

    EXPECT_EQ(current->cursorEventAreaListeners.Count(), before);
}

TEST(CursorAreaEventsListener, OnDrawnSkippedWhenNotListening) {
    auto current = o2Events.GetCurrentCursorAreaEventsLayer();
    int before = current->cursorEventAreaListeners.Count();

    auto l = mmake<TestableCursorAreaListener>();
    l->SetListeningEvents(false);
    l->OnDrawn();

    EXPECT_EQ(current->cursorEventAreaListeners.Count(), before);
}
