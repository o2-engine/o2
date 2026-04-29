#include "o2/stdafx.h"

#include "tests/Scene/UI/UITestHelpers.h"

#include "o2/Scene/Scene.h"

namespace o2
{
    Ref<Widget> MakeWidget(const String& name, ActorCreateMode mode)
    {
        auto w = mmake<Widget>(mode);
        w->SetName(name);
        return w;
    }

    Ref<Widget> MakeChildWidget(const Ref<Widget>& parent, const String& name)
    {
        auto child = mmake<Widget>(ActorCreateMode::InScene);
        child->SetName(name);
        parent->AddChildWidget(child);
        return child;
    }

    void TickAndUpdateLayout(int frames)
    {
        for (int i = 0; i < frames; ++i)
        {
            o2Scene.UpdateAddedEntities();
            o2Scene.UpdateTransforms();
            o2Scene.Update(0.0f);
            o2Scene.UpdateDestroyingEntities();
        }
    }

    Ref<IRectDrawable> MakeStubRectDrawable()
    {
        return mmake<Sprite>();
    }

    void AttachEventCounter(const Ref<Widget>& widget, WidgetEventCounter* counter)
    {
        widget->onShow = [counter]() { counter->onShowCount++; };
        widget->onHide = [counter]() { counter->onHideCount++; };
        widget->onFocused = [counter]() { counter->onFocusedCount++; };
        widget->onUnfocused = [counter]() { counter->onUnfocusedCount++; };
        widget->onLayoutUpdated = [counter]() { counter->onLayoutUpdatedCount++; };
    }
}
