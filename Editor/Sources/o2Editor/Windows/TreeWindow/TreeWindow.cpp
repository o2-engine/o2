#include "o2Editor/stdafx.h"
#include "TreeWindow.h"

#include "o2/Animation/Tracks/AnimationFloatTrack.h"
#include "o2/Scene/Actor.h"
#include "o2/Scene/Components/AnimationComponent.h"
#include "o2/Scene/Components/EditorTestComponent.h"
#include "o2/Scene/Components/ImageComponent.h"
#include "o2/Scene/Components/ParticlesEmitterComponent.h"
#include "o2/Scene/Scene.h"
#include "o2/Scene/UI/UIManager.h"
#include "o2/Scene/UI/WidgetLayout.h"
#include "o2/Scene/UI/WidgetState.h"
#include "o2/Scene/UI/Widgets/Button.h"
#include "o2/Scene/UI/Widgets/ContextMenu.h"
#include "o2/Scene/UI/Widgets/EditBox.h"
#include "o2/Scene/UI/Widgets/Toggle.h"
#include "o2/Scene/UI/Widgets/Tree.h"
#include "o2/Utils/Editor/EditorScope.h"
#include "o2/Utils/Editor/SceneEditableObject.h"
#include "o2/Utils/StringUtils.h"
#include "o2/Utils/System/Clipboard.h"
#include "o2Editor/Actions/Create.h"
#include "o2Editor/Actions/Delete.h"
#include "o2Editor/Actions/Enable.h"
#include "o2Editor/Actions/Lock.h"
#include "o2Editor/Actions/Reparent.h"
#include "o2Editor/EditorApplication.h"
#include "o2Editor/Properties/Properties.h"
#include "o2Editor/Windows/PropertiesWindow/PropertiesWindow.h"
#include "o2Editor/Windows/SceneWindow/SceneEditScreen.h"
#include "o2Editor/Windows/TreeWindow/DrawOrderTree.h"
#include "o2Editor/Windows/TreeWindow/SceneHierarchyTree.h"
#include "o2Editor/Windows/WindowsManager.h"
#include "o2Editor/Windows/SceneWindow/SceneWindow.h"

DECLARE_SINGLETON(Editor::TreeWindow);

namespace Editor
{
    TreeWindow::TreeWindow(RefCounter* refCounter):
        Singleton<TreeWindow>(refCounter), IEditorWindow(refCounter)
    {
        InitializeWindow();
    }

    TreeWindow::TreeWindow(RefCounter* refCounter, const TreeWindow& other) :
        Singleton<TreeWindow>(refCounter), IEditorWindow(refCounter, other)
    {
        InitializeWindow();
    }

    TreeWindow::~TreeWindow()
    {}

    const Ref<SceneHierarchyTree>& TreeWindow::GetSceneTree() const
    {
        return mSceneTree;
    }

    const Ref<DrawOrderTree>& TreeWindow::GetDrawOrderTree() const
    {
        return mDrawOrderTree;
    }

	void TreeWindow::OnSceneFocused()
	{
		mTreeContextMenu->SetItemsMaxPriority();
	}

    void TreeWindow::SetSelectedObjects(const Vector<Ref<SceneEditableObject>>& objects)
    {
        if (mSceneTree->IsEnabled())
            mSceneTree->SetSelectedObjects(objects);
        else
            mDrawOrderTree->SetSelectedObjects(objects);
    }

    void TreeWindow::UpdateTreeView()
    {
        if (mSceneTree->IsEnabled())
            mSceneTree->UpdateNodesView();

        if (mDrawOrderTree->IsEnabled())
            mDrawOrderTree->UpdateNodesView();
    }

    void TreeWindow::RestoreExpandedFromCache()
    {
        if (mSceneTree->IsEnabled())
            mSceneTree->RestoreExpandedFromCache();
    }

    void TreeWindow::HighlightObjectTreeNode(const Ref<SceneEditableObject>& targetObject)
    {
        if (mSceneTree->IsEnabled())
            mSceneTree->ScrollToAndHighlight(targetObject);
        else
            mDrawOrderTree->ScrollToAndHighlight(targetObject);
    }

    void TreeWindow::SetWidgetsLayersVisible(bool visible)
    {
        Widget::isEditorLayersVisible = visible;
        mSceneTree->UpdateNodesView();
    }

    bool TreeWindow::IsWidgetsLayersVisible() const
    {
        return Widget::isEditorLayersVisible;
    }

    void TreeWindow::SetWidgetsInternalChildrenVisible(bool visible)
    {
        Widget::isEditorInternalChildrenVisible = visible;
        mSceneTree->UpdateNodesView();
    }

    bool TreeWindow::IsWidgetsInternalChildrenVisible() const
    {
        return Widget::isEditorInternalChildrenVisible;
    }

    Ref<RefCounterable> TreeWindow::CastToRefCounterable(const Ref<TreeWindow>& ref)
    {
        return DynamicCast<Singleton<TreeWindow>>(ref);
    }

    void TreeWindow::InitializeWindow()
    {
        mWindow->caption = "Tree";
        mWindow->name = "tree window";
        mWindow->SetIcon(mmake<Sprite>("ui/UI4_tree_wnd_icon.png"));
        mWindow->SetIconLayout(Layout::Based(BaseCorner::LeftTop, Vec2F(20, 20), Vec2F(0, 1)));
        mWindow->SetViewLayout(Layout::BothStretch(0, -2, 0, 18));

        InitializeTopPanel();
        InitializeSceneTree();
        InitializeDrawOrderTree();
        InitializeContextMenu();

        mSceneTree->SetEnabled(true);
        mDrawOrderTree->SetEnabled(false);

        mSceneTree->UpdateNodesView();
    }

    void TreeWindow::InitializeSceneTree()
    {
        mSceneTree = o2UI.CreateWidget<SceneHierarchyTree>("standard");
        *mSceneTree->layout = WidgetLayout::BothStretch(2, 0, 0, 18);

        mSceneTree->onNodeRightButtonClicked = THIS_FUNC(OnTreeRBPressed);

        mWindow->AddChild(mSceneTree);
    }

    void TreeWindow::InitializeDrawOrderTree()
    {
        mDrawOrderTree = o2UI.CreateWidget<DrawOrderTree>("standard");
        *mDrawOrderTree->layout = WidgetLayout::BothStretch(2, 0, 0, 18);

        mDrawOrderTree->onNodeRightButtonClicked = THIS_FUNC(OnTreeRBPressed);

        mWindow->AddChild(mDrawOrderTree);
    }

    void TreeWindow::InitializeContextMenu()
    {
        // Context menu
        mTreeContextMenu = o2UI.CreateWidget<ContextMenu>("standard");
        mTreeContextMenu->AddItem("Debug update view", [&]() { mSceneTree->UpdateNodesView(); });

        mTreeContextMenu->AddItem("---");

        mTreeContextMenu->AddItem("Create empty actor", [&]() { OnContextCreateNewPressed(); }, AssetRef<ImageAsset>(),
                                  ShortcutKeys('N', true, false));

        mTreeContextMenu->AddItem("Create/UI/Empty layer.alayers", [&]() { CreateObject<WidgetLayer>("Layer"); });
        mTreeContextMenu->AddItem("Create/UI/Sprite layer.alayers", [&]()
                                  {
                                      ForcePopEditorScopeOnStack scope;

                                      auto newLayer = mmake<WidgetLayer>();
                                      newLayer->SetDrawable(mmake<Sprite>());
                                      newLayer->name = "Sprite";
                                      OnCreateObject(newLayer);
                                  });

        mTreeContextMenu->AddItem("Create/UI/Text layer.alayers", [&]()
                                  {
                                      ForcePopEditorScopeOnStack scope;

                                      auto newLayer = mmake<WidgetLayer>();
                                      newLayer->SetDrawable(mmake<Text>());
                                      newLayer->name = "Text";
                                      OnCreateObject(newLayer);
                                  });

        InitializeCreateMenu();
        InitUIStyleCreateMenu();

        mTreeContextMenu->AddItem("---");

        mTreeContextMenu->AddItem("Copy", [&]() { OnContextCopyPressed(); }, AssetRef<ImageAsset>(), ShortcutKeys('C', true));
        mTreeContextMenu->AddItem("Cut", [&]() { OnContextCutPressed(); }, AssetRef<ImageAsset>(), ShortcutKeys('X', true));
        mTreeContextMenu->AddItem("Paste", [&]() { OnContextPastePressed(); }, AssetRef<ImageAsset>(), ShortcutKeys('V', true));
        mTreeContextMenu->AddItem("Delete", [&]() { OnContextDeletePressed(); }, AssetRef<ImageAsset>(), ShortcutKeys(VK_DELETE));
        mTreeContextMenu->AddItem("Duplicate", [&]() { OnContextDuplicatePressed(); }, AssetRef<ImageAsset>(), ShortcutKeys('D', true));

        mTreeContextMenu->AddItem("---");

        mTreeContextMenu->AddItem("Collapse all", [&]() { OnContextCollapseAllPressed(); });
        mTreeContextMenu->AddItem("Expand all", [&]() { OnContextExpandAllPressed(); });

        mTreeContextMenu->AddItem("---");

        mTreeContextMenu->AddItem("Enable\\disable", [&]() { OnContextEnablePressed(); }, AssetRef<ImageAsset>(), ShortcutKeys('L', true));
        mTreeContextMenu->AddItem("Lock\\unlock", [&]() { OnContextLockPressed(); }, AssetRef<ImageAsset>(), ShortcutKeys('O', true));

        mTreeContextMenu->AddItem("---");

        mTreeContextMenu->AddItem("Save prototype", [&]() { OnContextCopyPressed(); });
        mTreeContextMenu->AddItem("Reset to prototype", [&]() { OnContextCopyPressed(); });
        mTreeContextMenu->AddItem("Break link to prototype", [&]() { OnContextCopyPressed(); });

        mTreeContextMenu->AddItem("---");

        mTreeContextMenu->AddToggleItem("View widgets layers", true, THIS_FUNC(OnViewLayersToggled));
        mTreeContextMenu->AddToggleItem("View widgets internal children", true, THIS_FUNC(OnViewInternalChildrenToggled));

        mWindow->AddChild(mTreeContextMenu);

        mSceneTree->onFocused = THIS_FUNC(OnSceneFocused);
    }

    void TreeWindow::InitializeCreateMenu()
    {
        auto subTypes = TypeOf(Actor).GetDerivedTypes();

        for (auto& subType : subTypes)
        {
            String category = subType->InvokeStatic<String>("GetCreateMenuCategory");
            String path;
            if (category.IsEmpty())
                path = GetSmartName(subType->GetName().ReplacedAll("o2::", "").ReplacedAll("::", "/"));
            else
            {
                String name = subType->GetName().ReplacedAll("o2::", "");
                int fnd = name.FindLast("::");
                if (fnd >= 0)
                    name = name.SubStr(fnd + 2);

                path = category + "/" + GetSmartName(name);
            }

            String group = subType->InvokeStatic<String>("GetCreateMenuGroup");

            mTreeContextMenu->AddItem("Create/" + path + "." + group, [=]()
                                      {
                                          ForcePopEditorScopeOnStack scope;
                                          auto newActor = DynamicCast<Actor>(subType->CreateSampleRef());
                                          newActor->name = path.ReplacedAll("/", " ");
                                          OnCreateObject(newActor);
                                      });
        }
    }

    void TreeWindow::InitUIStyleCreateMenu()
    {
        auto styleWidgets = o2UI.GetWidgetStyles();

        for (auto& styleWidgetAsset : styleWidgets)
        {
            auto path = styleWidgetAsset->GetActor()->GetType().GetName() + "/" + styleWidgetAsset->GetActor()->GetName();
            path.ReplaceAll("o2::", "");
            path.ReplaceAll("::", "/");

            mTreeContextMenu->AddItem(String("Create/UI/Style/") + path, [=]()
                                      {
                                          ForcePopEditorScopeOnStack scope;
                                          auto newWidget = styleWidgetAsset->GetActor()->CloneAsRef<Widget>();
                                          newWidget->SetEnabledForcible(true);
                                          OnCreateObject(newWidget);
                                      });
        }
    }

    void TreeWindow::InitUILayersCreateMenu()
    {
        mTreeContextMenu->AddItem("Create/UI layer/Empty", [&]()
                                  {
                                      auto newLayer = mmake<WidgetLayer>();
                                      newLayer->name = "empty";
                                      OnCreateObject(newLayer);
                                  });

        mTreeContextMenu->AddItem("Create/UI layer/Sprite", [&]()
                                  {
                                      auto newLayer = mmake<WidgetLayer>();
                                      newLayer->SetDrawable(mmake<Sprite>());
                                      newLayer->name = "sprite";
                                      OnCreateObject(newLayer);
                                  });

        mTreeContextMenu->AddItem("Create/UI layer/Text", [&]()
                                  {
                                      auto newLayer = mmake<WidgetLayer>();
                                      newLayer->SetDrawable(mmake<Text>());
                                      newLayer->name = "text";
                                      OnCreateObject(newLayer);
                                  });
    }

    void TreeWindow::PostInitializeWindow()
    {
        mSceneTree->AttachToSceneEvents();
    }

    void TreeWindow::OnSearchPressed()
    {}

    void TreeWindow::OnListTreeToggled(bool value)
    {
        mSceneTree->SetEnabled(!value);
        mDrawOrderTree->SetEnabled(value);

        mDrawOrderTree->RebuildOrderTree();
        mDrawOrderTree->UpdateNodesView();
    }

    void TreeWindow::OnSearchEdited(const WString& searchStr)
    {
        mInSearch = searchStr.Length() > 0;

        if (mInSearch)
        {
            mSearchObjects.Clear();

            for (auto& actor : o2Scene.GetRootActors())
                SearchObjectsRecursive(actor, (String)searchStr);
        }

        mSceneTree->UpdateNodesView();
    }

    void TreeWindow::SearchObjectsRecursive(const Ref<SceneEditableObject>& object, const String& searchStr)
    {
        if (object->GetName().CountOf(searchStr) > 0)
            mSearchObjects.Add(object);

        for (auto& child : object->GetEditableChildren())
            SearchObjectsRecursive(child, searchStr);
    }

    void TreeWindow::OnTreeRBPressed(const Ref<TreeNode>& node)
    {
        bool canCreateUILayers = false;
        if (node)
        {
            SceneEditableObject* object = (SceneEditableObject*)node->GetObject();
            canCreateUILayers = dynamic_cast<WidgetLayer*>(object) != nullptr ||
                dynamic_cast<Widget::LayersEditable*>(object) != nullptr ||
                dynamic_cast<Widget*>(object) != nullptr;
        }

        //         mTreeContextMenu->FindItemWidget("Create UI/Empty layer")->SetEnabled(canCreateUILayers);
        //         mTreeContextMenu->FindItemWidget("Create UI/Sprite layer")->SetEnabled(canCreateUILayers);
        //         mTreeContextMenu->FindItemWidget("Create UI/Text layer")->SetEnabled(canCreateUILayers);

        mTreeContextMenu->Show();
    }

    void TreeWindow::OnCreateObject(const Ref<SceneEditableObject>& newObject)
    {
        auto selectedObjects = mSceneTree->GetSelectedObjects();

        if (newObject->IsSupportsLayout())
        {
            if (!selectedObjects.IsEmpty())
                newObject->SetLayout(Layout::BothStretch());
            else
                newObject->SetLayout(Layout::Based(BaseCorner::Center, Vec2F(100, 100)));
        }
        else
            newObject->SetTransform(Basis(Vec2F(), Vec2F(100, 0), Vec2F(0, 100)));

        if (selectedObjects.Count() > 0)
        {
            auto obj = selectedObjects.Last();

            auto parentObject = obj;
            parentObject->AddEditableChild(newObject);

            auto parentChilds = parentObject->GetEditableChildren();
            auto action = mmake<CreateAction>(Vector<Ref<SceneEditableObject>>{ newObject }, parentObject,
                                              parentChilds.Count() > 1 ? parentChilds[parentChilds.Count() - 2] : Ref<SceneEditableObject>());

            o2EditorSceneWindow.DoneAction(action);
        }
        else
        {
            auto sceneObjects = o2Scene.GetRootEditableObjects();
            auto action = mmake<CreateAction>(Vector<Ref<SceneEditableObject>>{ newObject }, nullptr,
                                              sceneObjects.Count() > 1 ? sceneObjects[sceneObjects.Count() - 2] : nullptr);

            o2EditorSceneWindow.DoneAction(action);
        }

        newObject->UpdateTransform();

        mSceneTree->ScrollToAndHighlight(newObject);
        o2EditorSceneScreen.SelectObjectWithoutAction(newObject, false);
    }

    void TreeWindow::OnContextCreateNewPressed()
    {
        ForcePopEditorScopeOnStack scope;

        auto newActor = mmake<Actor>();
        newActor->name = "Empty";
        OnCreateObject(newActor);
    }

    void TreeWindow::OnContextCreateSprite()
    {
        ForcePopEditorScopeOnStack scope;

        auto newActor = mmake<Actor>();
        newActor->AddComponent(mmake<ImageComponent>());
        newActor->name = "Sprite";
        newActor->transform->size = Vec2F(10, 10);
        OnCreateObject(newActor);
    }

    void TreeWindow::OnContextCreateButton()
    {}

    void TreeWindow::OnContextCopyPressed()
    {
        DataDocument data;
        data.Set(mSceneTree->GetSelectedObjects());

        WString clipboardData = data.SaveAsString();

        Clipboard::SetText(clipboardData);
    }

    void TreeWindow::OnContextCutPressed()
    {
        OnContextCopyPressed();
        OnContextDeletePressed();
    }

    void TreeWindow::OnContextPastePressed()
    {
        ForcePopEditorScopeOnStack scope;

        auto selectedObjects = mSceneTree->GetSelectedObjects();

        auto parent = selectedObjects.Count() > 0 ? selectedObjects.Last() : nullptr;
        auto parentChilds = parent ? parent->GetEditableChildren() : o2Scene.GetRootEditableObjects();
        auto prevObject = parentChilds.Count() > 0 ? parentChilds.Last() : nullptr;

        WString clipboardData = Clipboard::GetText();
        DataDocument data;
        data.LoadFromData(clipboardData);

        Vector<Ref<SceneEditableObject>> objects;
        data.Get(objects);

        for (auto& object : objects)
            object->GenerateNewID();

        for (auto& object : objects)
            object->SetEditableParent(parent);

        mSceneTree->UpdateNodesView();

        auto action = mmake<CreateAction>(objects, parent, prevObject);
        o2EditorSceneWindow.DoneAction(action);

        mSceneTree->SetSelectedObjects(objects);
    }

    void TreeWindow::OnContextDeletePressed()
    {
        o2EditorPropertiesWindow.SetTarget(nullptr);

        auto selectedObjects = o2EditorSceneScreen.GetTopSelectedObjects();
        o2EditorSceneScreen.ClearSelectionWithoutAction();

        auto action = mmake<DeleteAction>(selectedObjects);
        o2EditorSceneWindow.DoneAction(action);

        for (auto& object : selectedObjects)
        {
            if (object->IsSupportsDeleting())
            {
                if (auto actor = DynamicCast<Actor>(object))
                    o2Scene.DestroyActor(actor);
            }
        }

        mSceneTree->UpdateNodesView();
    }

    void TreeWindow::OnContextDuplicatePressed()
    {
        ForcePopEditorScopeOnStack scope;

        auto selectedObjects = o2EditorSceneScreen.GetTopSelectedObjects();

        Vector<Ref<SceneEditableObject>> newObjects;

        for (auto& object : selectedObjects)
        {
            auto copy = object->CloneAsRef<SceneEditableObject>();
            copy->SetEditableParent(object->GetEditableParent());
            newObjects.Add(copy);
        }

        mSceneTree->UpdateNodesView();
        mSceneTree->SetSelectedObjects(newObjects);
    }

    void TreeWindow::OnContextExpandAllPressed()
    {
        auto selectedObjects = mSceneTree->GetSelectedObjects();

        for (auto& object : selectedObjects)
        {
            auto node = mSceneTree->GetNode(object);

            if (node)
                node->Expand();
        }
    }

    void TreeWindow::OnContextCollapseAllPressed()
    {
        auto selectedObject = mSceneTree->GetSelectedObjects();

        for (auto& object : selectedObject)
        {
            auto node = mSceneTree->GetNode(object);

            if (node)
                node->Collapse();
        }
    }

    void TreeWindow::OnContextLockPressed()
    {
        auto selectedObjects = mSceneTree->GetSelectedObjects();

        bool value = selectedObjects.Count() > 0 ? !selectedObjects.Last()->IsLocked() : true;
        auto action = mmake<LockAction>(selectedObjects, value);
        o2EditorSceneWindow.DoneAction(action);

        for (auto& object : selectedObjects)
        {
            object->SetLocked(value);
        }
    }

    void TreeWindow::OnContextEnablePressed()
    {
        auto selectedObjects = mSceneTree->GetSelectedObjects();

        bool value = selectedObjects.Count() > 0 ? !selectedObjects.Last()->IsEnabled() : true;
        auto action = mmake<LockAction>(selectedObjects, value);
        o2EditorSceneWindow.DoneAction(action);

        for (auto& object : selectedObjects)
        {
            object->SetEnabled(value);
        }
    }

    void TreeWindow::OnViewLayersToggled(bool view)
    {
        SetWidgetsLayersVisible(view);
    }

    void TreeWindow::OnViewInternalChildrenToggled(bool view)
    {
        SetWidgetsInternalChildrenVisible(view);
    }

    void TreeWindow::OnActorCreated(const Ref<SceneEditableObject>& actor)
    {
        mSceneTree->UpdateNodesView();
    }

    void TreeWindow::OnActorDestroyed(const Ref<SceneEditableObject>& actor)
    {
        mSceneTree->UpdateNodesView();
    }

    void TreeWindow::InitializeTopPanel()
    {
        auto upPanel = mmake<Widget>();
        upPanel->name = "up panel";
        *upPanel->layout = WidgetLayout::HorStretch(VerAlign::Top, 0, 0, 20, 0);
        upPanel->AddLayer("back", mmake<Sprite>("ui/UI4_square_field.png"), Layout::BothStretch(-4, -4, -5, -5));

        auto searchButton = o2UI.CreateWidget<Button>("search");
        *searchButton->layout = WidgetLayout::Based(BaseCorner::Left, Vec2F(20, 20), Vec2F(-1, 1));
        searchButton->onClick += THIS_FUNC(OnSearchPressed);
        upPanel->AddChild(searchButton);

        mListTreeToggle = o2UI.CreateWidget<Toggle>("list-tree");
        *mListTreeToggle->layout = WidgetLayout::Based(BaseCorner::Right, Vec2F(20, 20), Vec2F(0, 0));
        mListTreeToggle->onToggle += THIS_FUNC(OnListTreeToggled);
        upPanel->AddChild(mListTreeToggle);

        mSearchEditBox = o2UI.CreateWidget<EditBox>("backless");
        *mSearchEditBox->layout = WidgetLayout::BothStretch(19, 2, 21, -2);
        mSearchEditBox->onChanged += THIS_FUNC(OnSearchEdited);
        upPanel->AddChild(mSearchEditBox);

        mWindow->AddChild(upPanel);
    }

}
// --- META ---

DECLARE_CLASS(Editor::TreeWindow, Editor__TreeWindow);
// --- END META ---
