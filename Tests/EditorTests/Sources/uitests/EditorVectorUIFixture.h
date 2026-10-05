#pragma once

#include <gtest/gtest.h>

#include <tuple>

#include "o2/Application/Application.h"
#include "o2/Application/Input.h"
#include "o2/Assets/Assets.h"
#include "o2/Render/Render.h"
#include "o2/Render/Sprite.h"
#include "o2/Render/VectorSprite.h"
#include "o2/Scene/Actor.h"
#include "o2/Scene/Scene.h"
#include "o2/Scene/UI/UIManager.h"
#include "o2/Scene/UI/Widget.h"
#include "o2/Scene/UI/WidgetLayer.h"
#include "o2/Scene/UI/WidgetLayout.h"
#include "o2/Scene/UI/WidgetState.h"
#include "o2/Scene/UI/Widgets/ContextMenu.h"
#include "o2/Scene/UI/Widgets/CustomList.h"
#include "o2/Scene/UI/Widgets/EditBox.h"
#include "o2/Scene/UI/Widgets/Label.h"
#include "o2/Scene/UI/Widgets/LongList.h"
#include "o2/Scene/UI/Widgets/MenuPanel.h"
#include "o2/Scene/UI/Widgets/Tree.h"
#include "o2/Scene/UI/Widgets/Window.h"
#include "o2/Utils/Bitmap/Bitmap.h"
#include "o2/Utils/Editor/EditorScope.h"
#include "o2/Utils/Debug/Debug.h"
#include "o2/Utils/FileSystem/FileSystem.h"
#include "o2/Utils/Math/Curve.h"
#include "o2/Utils/Singleton.h"
#include "o2/Utils/Test/AppTestDriver.h"
#include "o2Editor/Dialogs/ColorPickerDlg.h"
#include "o2Editor/Dialogs/CurveEditorDlg.h"
#include "o2Editor/Dialogs/EditNameDlg.h"
#include "o2Editor/Dialogs/KeyEditDlg.h"
#include "o2Editor/Dialogs/YesNoCancelDlg.h"
#include "o2Editor/EditorConfig.h"
#include "o2Editor/MenuPanel.h"
#include "o2Editor/ToolsPanel.h"
#include "o2Editor/UI/Style/EditorUIStyle.h"
#include "o2Editor/UIRoot.h"
#include "o2Editor/Utils/CommonTextures.h"
#include "o2Editor/Windows/AnimationStateGraphWindow/AnimationStateGraphWindow.h"
#include "o2Editor/Windows/AnimationWindow/AnimationWindow.h"
#include "o2Editor/Windows/AnimationWindow/PropertiesListDlg.h"
#include "o2Editor/Windows/AnimationWindow/Tree.h"
#include "o2Editor/Windows/AssetsWindow/AssetsWindow.h"
#include "o2Editor/Windows/DockableWindow.h"
#include "o2Editor/Windows/MemoryAnalyzerWindow/MemoryAnalyzerWindow.h"
#include "o2Editor/Windows/PipelineWindow/PipelineWindow.h"
#include "o2Editor/Windows/PropertiesWindow/PropertiesWindow.h"
#include "o2Editor/Windows/SceneWindow/SceneEditScreen.h"
#include "o2Editor/Windows/SceneWindow/SceneWindow.h"
#include "o2Editor/Windows/TreeWindow/SceneHierarchyTree.h"
#include "o2Editor/Windows/TreeWindow/TreeWindow.h"
#include "o2Editor/Windows/WindowsLayout.h"
#include "o2Editor/Windows/WindowsManager.h"

// The editor as EditorApplication builds it, drawn with raster sprites or with vector graphics: shared by the
// parity and the performance suites
namespace EditorVectorUITest
{
    using namespace o2;
    using namespace Editor;

    inline const String stylesFolder = "Editor UI styles";
    inline const Vec2I frameSize(1366, 768);
    inline const float settleStep = 0.1f;
    inline const int settleSteps = 20;

    struct Frame
    {
        Ref<Bitmap> bitmap;
        UInt64      vectorTriangles = 0; // Triangles the vector sprites sent to render for the captured frame
    };

    // Keeps the instances of the process while a pass builds its own ones, and destroys what the pass has created
    template<typename ... _types>
    struct SingletonsScope
    {
        std::tuple<_types* ...> instances = { Singleton<_types>::mInstance ... };
        int count = GetSingletonsList().Count();

        ~SingletonsScope()
        {
            auto& list = GetSingletonsList();
            while (list.Count() > count)
                list.PopBack();

            ((Singleton<_types>::mInstance = std::get<_types*>(instances)), ...);
        }
    };

    using EditorSingletonsScope = SingletonsScope<EditorConfig, WindowsManager, Editor::MenuPanel, ToolsPanel, ColorPickerDlg,
        CurveEditorDlg, NameEditDlg, KeyEditDlg, YesNoCancelDlg, TreeWindow, PipelineWindow, AssetsWindow, SceneWindow,
        SceneEditScreen, PropertiesWindow, AnimationStateGraphWindow, AnimationWindow, PropertiesListDlg, MemoryAnalyzerWindow>;

    inline void MakeOpaque(Bitmap& bitmap)
    {
        UInt8* data = bitmap.GetData();
        size_t pixels = (size_t)bitmap.GetSize().x*bitmap.GetSize().y;
        for (size_t i = 0; i < pixels; i++)
            data[i*4 + 3] = 255;
    }

    inline bool rasterPass = false;

    // Returns the sprite of the png twin of a vector sprite: the old look of the drawable; null when there is no twin
    inline Ref<Sprite> MakeRasterTwin(const Ref<IRectDrawable>& drawable)
    {
        auto vectorSprite = DynamicCast<VectorSprite>(drawable);
        if (!vectorSprite || !vectorSprite->GetImageName().EndsWith(".svg"))
            return nullptr;

        const String& vectorPath = vectorSprite->GetImageName();
        String rasterPath = vectorPath.SubStr(0, vectorPath.Length() - 3) + "png";
        if (!o2Assets.IsAssetExist(rasterPath))
            return nullptr;

        auto sprite = mmake<Sprite>(rasterPath);
        sprite->SetMode(vectorSprite->GetMode());
        sprite->SetSliceBorder(vectorSprite->GetSliceBorder());
        sprite->SetBasis(vectorSprite->GetBasis());
        sprite->SetColor(vectorSprite->GetColor());
        sprite->SetOverrideColor(vectorSprite->GetOverrideColor());
        sprite->SetEnabled(vectorSprite->IsEnabled());
        return sprite;
    }

    using TwinsAlpha = Vector<Pair<Ref<Sprite>, float>>;

    inline void RasterizeLayer(const Ref<WidgetLayer>& layer, TwinsAlpha& alphas)
    {
        if (auto twin = MakeRasterTwin(layer->GetDrawable()))
        {
            alphas.Add({ twin, layer->GetDrawable()->GetTransparency() });
            layer->SetDrawable(twin);
        }

        for (auto& child : layer->GetChildren())
            RasterizeLayer(child, alphas);
    }

    inline void RasterizeWidget(const Ref<Widget>& widget, TwinsAlpha& alphas)
    {
        if (!widget)
            return;

        for (auto& layer : widget->GetLayers())
            RasterizeLayer(layer, alphas);

        if (auto list = DynamicCast<CustomList>(widget))
        {
            if (auto twin = MakeRasterTwin(list->GetSelectionRectDrawable()))
                list->SetSelectionDrawable(twin);

            if (auto twin = MakeRasterTwin(list->GetHoverRectDrawable()))
                list->SetHoverDrawable(twin);
        }
        else if (auto longList = DynamicCast<LongList>(widget))
        {
            if (auto twin = MakeRasterTwin(longList->GetSelectionRectDrawable()))
                longList->SetSelectionDrawable(twin);

            if (auto twin = MakeRasterTwin(longList->GetHoverRectDrawable()))
                longList->SetHoverDrawable(twin);
        }
        else if (auto tree = DynamicCast<Tree>(widget))
        {
            if (auto twin = MakeRasterTwin(tree->GetHoverRectDrawable()))
                tree->SetHoverDrawable(twin);

            if (auto twin = MakeRasterTwin(tree->GetHighlightRectDrawable()))
                tree->SetHighlightDrawable(twin);

            if (auto twin = MakeRasterTwin(tree->GetZebraBackLineDrawable()))
                tree->SetZebraBackLine(twin);
        }
        else if (auto contextMenu = DynamicCast<ContextMenu>(widget))
        {
            if (auto twin = MakeRasterTwin(contextMenu->GetSelectionRectDrawable()))
                contextMenu->SetSelectionDrawable(twin);
        }
        else if (auto menuPanel = DynamicCast<o2::MenuPanel>(widget))
        {
            if (auto twin = MakeRasterTwin(menuPanel->GetSelectionRectDrawable()))
                menuPanel->SetSelectionDrawable(twin);
        }
        else if (auto editBox = DynamicCast<EditBox>(widget))
        {
            if (auto twin = MakeRasterTwin(editBox->GetCaretRectDrawable()))
                editBox->SetCaretDrawable(twin);
        }

        for (auto& child : widget->GetChildWidgets())
            RasterizeWidget(child, alphas);

        for (auto& internal : widget->internalWidget.GetAll())
            RasterizeWidget(internal.second, alphas);
    }

    // Replaces the vector sprites of the widgets tree by the raster twins, the alpha set by code is kept
    inline void RasterizeWidget(const Ref<Widget>& widget)
    {
        TwinsAlpha alphas;
        RasterizeWidget(widget, alphas);

        for (auto& alpha : alphas)
            alpha.first->SetTransparency(alpha.second);
    }

    // The first frame after switching to a capture target needs a repeat, the second one is kept
    inline Frame Capture(const Function<void()>& draw)
    {
        if (rasterPass)
        {
            PushEditorScopeOnStack scope;
            RasterizeWidget(EditorUIRoot.GetRootWidget());
        }

        Frame res;
        for (int i = 0; i < 2; i++)
        {
            UInt64 trianglesBefore = VectorSprite::GetTotalDrawnTriangles();

            res.bitmap = nullptr;
            o2Render.CaptureNextFrame([&](const Ref<Bitmap>& bitmap) { res.bitmap = bitmap; });

            PushEditorScopeOnStack scope;
            o2Render.Begin();
            o2Render.SetCamera(Camera());
            draw();
            o2UI.DrawCurrentLayerTopWidgets();
            o2Render.End();

            res.vectorTriangles = VectorSprite::GetTotalDrawnTriangles() - trianglesBefore;
        }

        if (res.bitmap)
            MakeOpaque(*res.bitmap);

        return res;
    }

    inline void ParkCursor()
    {
        o2Input.OnCursorMoved(Vec2F(-10000, -10000), 0, false);
    }

    inline void FitRootToFrame()
    {
        auto root = EditorUIRoot.GetRootWidget();
        *root->layout = WidgetLayout::Based(BaseCorner::Center, (Vec2F)o2Application.GetContentSize());
        root->UpdateTransform();
    }

    inline void UpdateRoot(float dt)
    {
        auto root = EditorUIRoot.GetRootWidget();
        root->Update(dt);
        root->UpdateChildren(dt);
        root->UpdateChildrenTransforms();
    }

    // The raster pass is the vector style and editor with every vector sprite replaced by the sprite of its png twin
    inline void BuildStyle(bool raster)
    {
        rasterPass = raster;
        EditorUIStyleBuilder().RebuildEditorUIManager(stylesFolder, false, false);

        if (raster)
        {
            for (auto& style : o2UI.GetWidgetStyles())
                RasterizeWidget(DynamicCast<Widget>(style->GetActor()));
        }
    }

    inline WindowsLayout::WindowDockPlaceInfo DockPlace(const RectF& anchors, const Vector<String>& windows = {},
                                                 const String& active = "")
    {
        WindowsLayout::WindowDockPlaceInfo res;
        res.anchors = anchors;
        res.windows = windows;
        res.active = active;
        return res;
    }

    // The usual layout of the editor: tree, scene, bottom tabs and the properties column
    inline WindowsLayout MakeLayout(const String& activeCenter, const String& activeBottom, const String& activeRight)
    {
        auto top = DockPlace(RectF(0.0f, 0.37f, 1.0f, 1.0f));
        top.childs.Add(DockPlace(RectF(0.3f, 0.0f, 1.0f, 1.0f),
                                 { "animation state graph window", "scene window", "pipeline window" }, activeCenter));
        top.childs.Add(DockPlace(RectF(0.0f, 0.0f, 0.3f, 1.0f), { "tree window" }));

        auto left = DockPlace(RectF(0.0f, 0.0f, 0.72f, 1.0f));
        left.childs.Add(top);
        left.childs.Add(DockPlace(RectF(0.0f, 0.0f, 1.0f, 0.37f), { "animation window", "log window", "assets window" },
                                  activeBottom));

        WindowsLayout res;
        res.mainDock = DockPlace(RectF(0.0f, 0.0f, 1.0f, 1.0f));
        res.mainDock.childs.Add(left);
        res.mainDock.childs.Add(DockPlace(RectF(0.72f, 0.0f, 1.0f, 1.0f), { "properties window", "game window" },
                                          activeRight));
        return res;
    }

    // The editor as EditorApplication::OnStarted builds it, without configs of the developer
    struct EditorShell
    {
        EditorSingletonsScope singletons;

        Ref<IRectDrawable> background;
        Ref<IRectDrawable> backSign;

        Ref<EditorConfig>      config;
        Ref<WindowsManager>    windows;
        Ref<Editor::MenuPanel> menu;
        Ref<ToolsPanel>        tools;

        EditorShell(const WindowsLayout& layout)
        {
            PushEditorScopeOnStack scope;

            EditorUIRoot.RemoveAllWidgets();
            FitRootToFrame();

            background = mmake<VectorSprite>("ui/UI4_Background.svg");
            backSign = mmake<VectorSprite>("ui/UI4_o2_sign.svg");

            if (rasterPass)
            {
                background = MakeRasterTwin(background);
                backSign = MakeRasterTwin(backSign);
            }

            background->SetSize2D((Vec2F)o2Render.GetResolution() + Vec2F(20, 20));
            backSign->position = (Vec2F)(o2Render.GetResolution()).InvertedX()*0.5f + Vec2F(40.0f, -85.0f);

            CommonTextures::Initialize();

            config = mmake<EditorConfig>();
            config->projectConfig.layout = layout;

            windows = mmake<WindowsManager>();
            menu = mmake<Editor::MenuPanel>();
            tools = mmake<ToolsPanel>();

            FitRootToFrame();
        }

        ~EditorShell()
        {
            PushEditorScopeOnStack scope;

            EditorUIRoot.RemoveAllWidgets();
            tools = nullptr;
            menu = nullptr;
            windows = nullptr;
            config = nullptr;
        }

        // ToolsPanel::Update is skipped: it reads the play state of EditorApplication, which the test runner has not
        void Update(float dt)
        {
            PushEditorScopeOnStack scope;

            windows->Update(dt);
            UpdateRoot(dt);
        }

        void Draw()
        {
            o2Render.Clear();
            background->Draw();
            backSign->Draw();
            windows->Draw();
            EditorUIRoot.GetRootWidget()->Draw();
        }

        Frame Settle()
        {
            ParkCursor();
            for (int i = 0; i < settleSteps; i++)
            {
                Update(settleStep);
                if (i%5 == 0)
                    Capture([&]() { Draw(); });
            }

            return Capture([&]() { Draw(); });
        }

        // Puts the shown dialogs over the docked windows, as focusing by a click does
        void RaiseDialogs()
        {
            PushEditorScopeOnStack scope;

            auto root = EditorUIRoot.GetRootWidget();
            for (auto& child : Vector<Ref<Widget>>(root->GetChildWidgets()))
            {
                if (DynamicCast<o2::Window>(child) && !DynamicCast<DockableWindow>(child) && child->IsEnabled())
                    child->SetIndexInSiblings(root->GetChildren().Count() - 1);
            }
        }

        // Hides the dialogs shown over the editor, the docked windows stay
        void HideDialogs()
        {
            PushEditorScopeOnStack scope;

            for (auto& child : EditorUIRoot.GetRootWidget()->GetChildWidgets())
            {
                if (DynamicCast<o2::Window>(child) && !DynamicCast<DockableWindow>(child))
                    child->Hide(true);
            }

            o2UI.FocusWidget(nullptr);
        }
    };

    // Actors without visual assets, the same for both passes
    struct TinyScene
    {
        Ref<Actor> root;

        TinyScene()
        {
            ForcePopEditorScopeOnStack scope;

            root = mmake<Actor>(ActorCreateMode::InScene);
            root->name = "Root";

            for (const char* name : { "First child", "Second child", "Third child" })
            {
                auto child = mmake<Actor>(ActorCreateMode::InScene);
                child->name = name;
                root->AddChild(child);
            }

            auto other = mmake<Actor>(ActorCreateMode::InScene);
            other->name = "Other";

            o2Scene.UpdateAddedEntities();
            o2Scene.UpdateTransforms();
        }

        ~TinyScene()
        {
            ForcePopEditorScopeOnStack scope;

            root = nullptr;
            o2Scene.Clear(true);
            o2Scene.UpdateDestroyingEntities();
        }
    };

    struct EditorVectorUIFixture: ::testing::Test
    {
        Vec2I previousSize;

        void SetUp() override
        {
            if (!UIRoot::IsSingletonInitialzed())
            {
                PushEditorScopeOnStack scope;
                mmake<UIRoot>();
            }

            previousSize = o2Application.GetContentSize();
            o2Application.SetContentSize(frameSize);
            AppTestDriver::PumpFrames(3);
            ASSERT_EQ(o2Render.GetResolution(), frameSize) << "the window has not taken the frame size";

            o2Scene.Clear(true);
            o2Scene.UpdateDestroyingEntities();
        }

        void TearDown() override
        {
            EditorUIRoot.RemoveAllWidgets();
            BuildStyle(false);

            o2Application.SetContentSize(previousSize);
            AppTestDriver::PumpFrames(3);
            FitRootToFrame();
        }

        // Builds the style and the editor of a pass and captures what the scenario shows in it
        Vector<Frame> RenderEditor(bool raster, const Function<void(EditorShell&, Vector<Frame>&)>& scenario,
                                   const WindowsLayout& layout = MakeLayout("scene window", "assets window",
                                                                            "properties window"))
        {
            BuildStyle(raster);

            Vector<Frame> res;
            EditorShell shell(layout);
            scenario(shell, res);
            return res;
        }
    };
}
