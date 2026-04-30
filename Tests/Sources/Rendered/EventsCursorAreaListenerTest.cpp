#include "o2/stdafx.h"
#include <gtest/gtest.h>

#include "o2/Application/Input.h"
#include "o2/Events/CursorAreaEventsListener.h"
#include "o2/Events/CursorAreaEventsListenersLayer.h"
#include "o2/Events/EventSystem.h"
#include "o2/Utils/Types/Ref.h"

using namespace o2;

namespace
{
    class TestableCursorAreaListener : public RefCounterable, public CursorAreaEventsListener
    {
    public:
        bool isUnderPointReturn = true;

        bool IsUnderPoint(const Vec2F&) override { return isUnderPointReturn; }

        RefCounter* GetRefCounter() const override { return RefCounterable::GetRefCounter(); }
    };
}

// Tests for CursorAreaEventsListener::OnDrawn registration with the current layer.
// Live in the rendered tier because Integration::PreDrawFrame pushes a current
// cursor-area layer; in headless that path doesn't run and o2Events has no current
// layer.

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
