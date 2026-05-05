#include "o2/stdafx.h"
#include "support/EditorWindowsFixture.h"

using namespace o2;

namespace Editor::Tests
{
    namespace
    {
        bool gUIRootInitialized = false;
    }

    void EnsureEditorUIRoot()
    {
        if (gUIRootInitialized)
            return;

        if (!UIRoot::IsSingletonInitialzed())
        {
            PushEditorScopeOnStack scope;
            mmake<UIRoot>();
        }

        gUIRootInitialized = true;
    }

    void EditorWindowsFixture::SetUp()
    {
        EnsureEditorUIRoot();
    }

    void EditorWindowsFixture::TearDown()
    {
        if (UIRoot::IsSingletonInitialzed())
            EditorUIRoot.RemoveAllWidgets();
    }

    Ref<DockableWindow> MakeDockable(const String& name)
    {
        PushEditorScopeOnStack scope;
        auto wnd = mmake<DockableWindow>();
        wnd->name = name;
        wnd->layout->size = Vec2F(200, 200);
        return wnd;
    }

    Ref<DockWindowPlace> MakeDock(const String& name)
    {
        PushEditorScopeOnStack scope;
        auto place = mmake<DockWindowPlace>();
        place->name = name;
        *place->layout = WidgetLayout::BothStretch();
        return place;
    }
}
