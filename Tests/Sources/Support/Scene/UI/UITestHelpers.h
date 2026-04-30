#pragma once

#include "o2/Render/Sprite.h"
#include "o2/Scene/UI/UIManager.h"
#include "o2/Scene/UI/Widget.h"
#include "o2/Scene/UI/WidgetLayout.h"
#include "Scene/SceneTestHelpers.h"

namespace o2
{
    Ref<Widget> MakeWidget(const String& name = "widget",
                           ActorCreateMode mode = ActorCreateMode::InScene);

    Ref<Widget> MakeChildWidget(const Ref<Widget>& parent, const String& name);

    void TickAndUpdateLayout(int frames = 1);

    Ref<IRectDrawable> MakeStubRectDrawable();

    struct WidgetEventCounter
    {
        int onShowCount = 0;
        int onHideCount = 0;
        int onFocusedCount = 0;
        int onUnfocusedCount = 0;
        int onLayoutUpdatedCount = 0;
    };

    void AttachEventCounter(const Ref<Widget>& widget, WidgetEventCounter* counter);

    // RAII helper to register a UI style sample in o2UI for the duration of a
    // test, removing it on scope exit so the global style table doesn't bleed
    // between tests.
    template<typename WidgetType>
    class UIStyleGuard
    {
    public:
        UIStyleGuard(const Ref<WidgetType>& sample, const String& name): mName(name)
        {
            o2UI.AddWidgetStyle(sample, name);
        }
        ~UIStyleGuard()
        {
            o2UI.RemoveWidgetStyle<WidgetType>(mName);
        }

    private:
        String mName;
    };
}
