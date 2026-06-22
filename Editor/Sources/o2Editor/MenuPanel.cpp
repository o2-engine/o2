#include "o2Editor/stdafx.h"
#include "MenuPanel.h"

#include "o2/Application/Application.h"
#include "o2/Assets/Assets.h"
#include "o2/Scene/Scene.h"
#include "o2/Scene/UI/UIManager.h"
#include "o2/Scene/UI/Widgets/Button.h"
#include "o2/Scene/UI/Widgets/HorizontalLayout.h"
#include "o2/Scene/UI/Widgets/Label.h"
#include "o2/Scene/UI/Widgets/MenuPanel.h"
#include "o2/Scene/UI/Widgets/VerticalLayout.h"
#include "o2/Utils/Editor/EditorScope.h"
#include "o2/Utils/FileSystem/FileSystem.h"
#include "o2/Utils/Math/Curve.h"
#include "o2/Utils/Memory/MemoryAnalyzer.h"
#include "o2/Utils/Tasks/TaskManager.h"
#include "o2Editor/Dialogs/CurveEditorDlg.h"
#include "o2Editor/Dialogs/System/OpenSaveDialog.h"
#include "o2Editor/EditorApplication.h"
#include "o2Editor/EditorConfig.h"
#include "o2Editor/Windows/LogWindow/LogWindow.h"
#include "o2Editor/Properties/Basic/FloatProperty.h"
#include "o2Editor/Properties/Basic/IntegerProperty.h"
#include "o2Editor/Properties/Basic/ObjectPtrProperty.h"
#include "o2Editor/UI/Style/EditorUIStyle.h"
#include "o2Editor/UIRoot.h"
#include "o2Editor/Windows/AnimationStateGraphWindow/AnimationStateGraphWindow.h"
#include "o2Editor/Windows/AnimationWindow/AnimationWindow.h"
#include "o2Editor/Windows/AssetsWindow/AssetsWindow.h"
#include "o2Editor/Windows/GameWindow/GameWindow.h"
#include "o2Editor/Windows/MemoryAnalyzerWindow/MemoryAnalyzerWindow.h"
#include "o2Editor/Windows/PropertiesWindow/PropertiesWindow.h"
#include "o2Editor/Windows/SceneWindow/SceneWindow.h"
#include "o2Editor/Windows/TreeWindow/SceneHierarchyTree.h"
#include "o2Editor/Windows/TreeWindow/TreeWindow.h"
#include "o2Editor/Windows/WindowsManager.h"

DECLARE_SINGLETON(Editor::MenuPanel);

namespace Editor
{
    MenuPanel::MenuPanel(RefCounter* refCounter):
        Singleton<MenuPanel>(refCounter)
    {
        mMenuPanel = o2UI.CreateWidget<o2::MenuPanel>();
        *mMenuPanel->layout = WidgetLayout::HorStretch(VerAlign::Top, 0, 0, 20, 0);
        EditorUIRoot.AddWidget(mMenuPanel);

        // FILE
        mMenuPanel->AddItem("File/New scene", [&]() { OnNewScenePressed(); }, AssetRef<ImageAsset>(), ShortcutKeys({VK_N, VK_CTRL_CMD, VK_SHIFT}));
        mMenuPanel->AddItem("File/Open scene", [&]() { OnOpenScenePressed(); }, AssetRef<ImageAsset>(), ShortcutKeys({VK_O, VK_CTRL_CMD}));
        mMenuPanel->AddItem("File/Save scene", [&]() { OnSaveScenePressed(); }, AssetRef<ImageAsset>(), ShortcutKeys({VK_S, VK_CTRL_CMD}));
        mMenuPanel->AddItem("File/Save scene as ...", [&]() { OnSaveSceneAsPressed(); }, AssetRef<ImageAsset>(), ShortcutKeys({VK_S, VK_CTRL_CMD, VK_MENU}));

        mMenuPanel->AddItem("File/---");

        mMenuPanel->AddItem("File/Exit", [&]() { OnExitPressed(); });

        // EDIT
        mMenuPanel->AddItem("Edit/Undo", [&]() { OnUndoPressed(); }, AssetRef<ImageAsset>(), ShortcutKeys({VK_Z, VK_CTRL_CMD}));
        mMenuPanel->AddItem("Edit/Redo", [&]() { OnRedoPressed(); }, AssetRef<ImageAsset>(), ShortcutKeys({VK_Y, VK_CTRL_CMD}));

        mMenuPanel->AddItem("Edit/---");

        mMenuPanel->AddItem("Edit/Copy", [&]() {}, AssetRef<ImageAsset>(), ShortcutKeys({VK_C, VK_CTRL_CMD}));
        mMenuPanel->AddItem("Edit/Cut", [&]() {}, AssetRef<ImageAsset>(), ShortcutKeys({VK_X, VK_CTRL_CMD}));
        mMenuPanel->AddItem("Edit/Paste", [&]() {}, AssetRef<ImageAsset>(), ShortcutKeys({VK_V, VK_CTRL_CMD}));
        mMenuPanel->AddItem("Edit/Delete", [&]() {}, AssetRef<ImageAsset>(), ShortcutKeys({VK_DELETE}));

        mMenuPanel->AddItem("Edit/---");

        mMenuPanel->AddItem("Edit/Project/Physics", [&]() {
            o2EditorPropertiesWindow.SetTarget(&o2Config.physics);
        });

        // VIEW
        mMenuPanel->AddItem("View/Show Tree", [&]() { OnShowTreePressed(); });
        mMenuPanel->AddItem("View/Show Scene", [&]() { OnShowScenePressed(); });
        mMenuPanel->AddItem("View/Show Assets", [&]() { OnShowAssetsPressed(); });
		mMenuPanel->AddItem("View/Show Properties", [&]() { OnShowPropertiesPressed(); });
		mMenuPanel->AddItem("View/Show Animation", [&]() { OnShowAnimationPressed(); });
		mMenuPanel->AddItem("View/Show Animation State graph", [&]() { OnShowAnimationStateGraphPressed(); });
        mMenuPanel->AddItem("View/Show Log", [&]() { OnShowLogPressed(); });
        mMenuPanel->AddItem("View/Show Game", [&]() { OnShowGamePressed(); });

#if ENABLE_MEMORY_ANALYZE
        mMenuPanel->AddItem("View/Show memory analyzer", [&]() { OnShowMemoryAnalyzerPressed(); });
#endif

        mMenuPanel->AddItem("View/---");
        mMenuPanel->AddItem("View/Reset layout", [&]() { OnResetLayoutPressed(); });

        // BUILD
        mMenuPanel->AddItem("Run/Connect scripts debugger", [&]() { o2Scripts.ConnectDebugger(); }, AssetRef<ImageAsset>(), ShortcutKeys({VK_F5}));
        mMenuPanel->AddItem("Run/---");
        mMenuPanel->AddItem("Run/Build & Run", [&]() { OnBuildAndRunPressed(); }, AssetRef<ImageAsset>(), ShortcutKeys({VK_R, VK_CTRL_CMD}));
        mMenuPanel->AddItem("Run/Build", [&]() { OnBuildPressed(); }, AssetRef<ImageAsset>(), ShortcutKeys({VK_R, VK_CTRL_CMD, VK_SHIFT}));

        // HELP
        mMenuPanel->AddItem("Help/About", [&]() { OnAboutPressed(); });
        mMenuPanel->AddItem("Help/Documentation", [&]() { OnDocumentationPressed(); });

        // DEBUG
        mMenuPanel->AddItem("Debug/Curve editor test", [&]() { OnCurveEditorTestPressed(); });
        mMenuPanel->AddItem("Debug/Save layout as default", [&]() { OnSaveDefaultLayoutPressed(); });
        mMenuPanel->AddItem("Debug/Add property", [&]() {
            static float xx = 0, yy = 1;
            ForcePopEditorScopeOnStack scope;
            auto prop = o2UI.CreateWidget<FloatProperty>("with caption");
            prop->SetValueAndPrototypeProxy({ { mmake<PointerValueProxy<float>>(&xx), mmake<PointerValueProxy<float>>(&yy) } });
                            });

        mMenuPanel->AddItem("Debug/Randomize IDs", [&]() {
            Function<void(const Ref<Actor>&)> fixActor = [&fixActor](const Ref<Actor>& actor) {
                actor->GenerateNewID();
                actor->GetComponents().ForEach([](auto comp) { comp->GenerateNewID(); });
                actor->GetChildren().ForEach([&](const Ref<Actor>& x) { fixActor(x); });
            };

            for (auto& actor : o2Scene.GetRootActors())
                fixActor(actor);
        });

        mMenuPanel->AddToggleItem("Debug/View editor UI tree", false, [&](bool x) { o2EditorTree.GetSceneTree()->SetEditorWatching(x); });

        mMenuPanel->AddItem("Debug/Dump memory", [&]() { o2Memory.DumpInfo(); });

        mMenuPanel->AddItem("Debug/JS collect garbage", [&]() { o2Scripts.CollectGarbage(); });

        mMenuPanel->AddItem("Debug/Rebuild assets", [&]() { o2Assets.RebuildAssets(); });
        mMenuPanel->AddItem("Debug/Rebuild assets forcible", [&]() { o2Assets.RebuildAssets(true); });
    }

    MenuPanel::~MenuPanel()
    {
    }

    Ref<Widget> MenuPanel::AddItem(const o2::MenuPanel::Item& item)
    {
        return mMenuPanel->AddItem(item);
    }

    void MenuPanel::AddItem(const WString& path, const Function<void()>& clickFunc /*= Function<void()>()*/,
                            const AssetRef<ImageAsset>& icon /*= AssetRef<ImageAsset>()*/,
                            const ShortcutKeys& shortcut /*= ShortcutKeys()*/)
    {
        mMenuPanel->AddItem(path, clickFunc, icon, shortcut);
    }

    void MenuPanel::InsertItem(const o2::MenuPanel::Item& item, int position)
    {
        mMenuPanel->InsertItem(item, position);
    }

    void MenuPanel::AddItems(Vector<o2::MenuPanel::Item> items)
    {
        mMenuPanel->AddItems(items);
    }

    void MenuPanel::InsertItems(Vector<o2::MenuPanel::Item> items, int position)
    {
        mMenuPanel->InsertItems(items, position);
    }

    o2::MenuPanel::Item MenuPanel::GetItem(int position)
    {
        return mMenuPanel->GetItem(position);
    }

    Vector<o2::MenuPanel::Item> MenuPanel::GetItems() const
    {
        return mMenuPanel->GetItems();
    }

    void MenuPanel::RemoveItem(int position)
    {
        return mMenuPanel->RemoveItem(position);
    }

    void MenuPanel::RemoveItem(const WString& path)
    {
        return mMenuPanel->RemoveItem(path);
    }

    void MenuPanel::CheckSceneSaving(const Function<void()>& onCompleted)
    {
        if (o2EditorApplication.IsSceneChanged())
        {
            auto wnd = EditorUIRoot.AddWidget(o2UI.CreateWindow("Save scene?"));
            *wnd->layout = WidgetLayout::Based(BaseCorner::Center, Vec2F(400, 150));

            auto verLayout = o2UI.CreateVerLayout();
            wnd->AddChild(verLayout);
            *verLayout->layout = WidgetLayout::BothStretch();
            verLayout->baseCorner = BaseCorner::Top;

            auto text = o2UI.CreateLabel("Current scene was modified but wasn't saved.\nDo you want to save it?");
            text->horOverflow = Label::HorOverflow::Wrap;
            verLayout->AddChild(text);

            auto horLayout = o2UI.CreateHorLayout();
            verLayout->AddChild(horLayout);

            *horLayout->layout = WidgetLayout::BothStretch();
            horLayout->border = BorderF(10, 10, 10, 10);
            horLayout->spacing = 10;
            horLayout->AddChild(o2UI.CreateButton("Save", [=]() {
                o2EditorApplication.SaveScene();
                onCompleted();
                wnd->Hide();
            }));

            horLayout->AddChild(o2UI.CreateButton("Don't save", [=]() {
                onCompleted();
                wnd->Hide();
            }));

            horLayout->AddChild(o2UI.CreateButton("Cancel", [=]() { wnd->Hide(); }));

            return;
        }

        onCompleted();
    }

    void MenuPanel::OnNewScenePressed()
    {
        o2EditorSceneWindow.MenuCreateNewAsset();
    }

    void MenuPanel::OnOpenScenePressed()
    {
        o2EditorSceneWindow.MenuOpenAsset();
    }

    void MenuPanel::OnSaveScenePressed()
    {
        o2EditorSceneWindow.MenuSaveAsset();
    }

    void MenuPanel::OnSaveSceneAsPressed()
    {
        o2EditorSceneWindow.MenuSaveAsAsset();
    }

    void MenuPanel::OnExitPressed()
    {
        o2Application.Shutdown();
    }

    void MenuPanel::OnUndoPressed()
    {
        o2EditorSceneWindow.UndoAction();
    }

    void MenuPanel::OnRedoPressed()
    {
        o2EditorSceneWindow.RedoAction();
    }

    void MenuPanel::OnShowTreePressed()
    {
        auto window = o2EditorWindows.GetWindow<TreeWindow>();
        if (window)
            window->Show();
    }

    void MenuPanel::OnShowScenePressed()
    {
        o2EditorSceneWindow.Show();
    }

    void MenuPanel::OnShowAssetsPressed()
    {
        auto window = o2EditorWindows.GetWindow<AssetsWindow>();
        if (window)
            window->Show();
    }

    void MenuPanel::OnShowPropertiesPressed()
    {
        auto window = o2EditorWindows.GetWindow<PropertiesWindow>();
        if (window)
            window->Show();
    }

    void MenuPanel::OnShowAnimationPressed()
    {
        auto window = o2EditorWindows.GetWindow<AnimationWindow>();
        if (window)
            window->Show();
    }

    void MenuPanel::OnShowLogPressed()
    {
        auto window = o2EditorWindows.GetWindow<LogWindow>();
        if (window)
            window->Show();
    }

	void MenuPanel::OnShowAnimationStateGraphPressed()
	{
		auto window = o2EditorWindows.GetWindow<AnimationStateGraphWindow>();
		if (window)
			window->Show();
	}

	void MenuPanel::OnShowGamePressed()
    {
        auto window = o2EditorWindows.GetWindow<GameWindow>();
        if (window)
            window->Show();
    }

#if ENABLE_MEMORY_ANALYZE
    void MenuPanel::OnShowMemoryAnalyzerPressed()
    {
        auto appRef = DynamicCast<RefCounterable>(Ref(Application::InstancePtr()));

        std::vector<MemoryAnalyzeObject*> objects = { &appRef };

        for (auto& singleton : GetSingletonsList())
            objects.push_back(&singleton);

        auto data = MemoryAnalyzer::BuildMemoryTree(objects);
        MemoryAnalyzerWindow::Show(data);
    }
#endif

    void MenuPanel::OnResetLayoutPressed()
    {
        o2EditorWindows.SetDefaultWindowsLayout();
    }

    void MenuPanel::OnRunPressed()
    {

    }

    void MenuPanel::OnBuildAndRunPressed()
    {

    }

    void MenuPanel::OnBuildPressed()
    {

    }

    void MenuPanel::OnAboutPressed()
    {

    }

    void MenuPanel::OnDocumentationPressed()
    {

    }

    void MenuPanel::OnSaveDefaultLayoutPressed()
    {
        o2EditorConfig.globalConfig.defaultLayout = o2EditorWindows.GetWindowsLayout();
        o2Debug.Log("Default windows layout saved!");
    }

    void MenuPanel::OnCurveEditorTestPressed()
    {
        CurveEditorDlg::Show(Function<void()>());

        int testCurves = 50;
        int testKeys = 50;
        for (int i = 0; i < testCurves; i++)
        {
            Ref<Curve> curve = mmake<Curve>();

            for (int j = 0; j < testKeys; j++)
                curve->AppendKey(Math::Random(0.1f, 2.0f), Math::Random(-1.0f, 1.0f), 0.0f, 1.0f);

            CurveEditorDlg::AddEditingCurve("test" + (String)i, curve);
        }
    }
}
