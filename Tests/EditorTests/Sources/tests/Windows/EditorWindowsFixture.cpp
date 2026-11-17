#include "o2/stdafx.h"
#include "support/EditorWindowsFixture.h"

#include "o2/Scene/UI/Widgets/Window.h"
#include "o2Editor/Dialogs/YesNoCancelDlg.h"

using namespace o2;

namespace Editor::Tests
{
    namespace
    {
        bool gUIRootInitialized = false;

        // The confirmation dialog is a singleton the tests make: it goes while the application runs, not with the
        // statics after it is deinitialized (that crashed the test process at exit on Windows)
        class ConfirmationDialogCleanup: public ::testing::Environment
        {
        public:
            void TearDown() override
            {
                if (!YesNoCancelDlg::IsSingletonInitialzed())
                    return;

                PushEditorScopeOnStack scope;
                if (UIRoot::IsSingletonInitialzed())
                {
                    if (auto window = EditorUIRoot.GetRootWidget()->FindChildByTypeAndName<o2::Window>("Confirmation window"))
                        EditorUIRoot.RemoveWidget(window);
                }

                Ref<YesNoCancelDlg> dialog(YesNoCancelDlg::InstancePtr());
                YesNoCancelDlg::DestroySingleton(dialog);
            }
        };

        const auto* gConfirmationDialogCleanup = ::testing::AddGlobalTestEnvironment(new ConfirmationDialogCleanup());
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
        wnd->layout->size2D = Vec2F(200, 200);
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
