#include "o2Editor/stdafx.h"
#include "GameWindow.h"

#include "o2/Scene/CameraActor.h"
#include "o2/Scene/UI/Widgets/Button.h"
#include "o2/Utils/Editor/EditorScope.h"
#include "o2Editor/EditorApplication.h"
#include "o2Editor/Properties/Basic/Vector2IntProperty.h"

namespace Editor
{
    GameWindow::GameWindow():
        IEditorWindow()
    {
        InitializeWindow();
    }

    GameWindow::GameWindow(const GameWindow& other):
        IEditorWindow(other)
    {
        InitializeWindow();
    }

    GameWindow::~GameWindow()
    {}

    void GameWindow::InitializeWindow()
    {
        mWindow->caption = "Game";
        mWindow->name = "game window";
        mWindow->SetIcon(mmake<Sprite>("ui/UI4_game_icon.png"));
        mWindow->SetIconLayout(Layout::Based(BaseCorner::LeftTop, Vec2F(20, 20), Vec2F(-1, 2)));
        mWindow->SetViewLayout(Layout::BothStretch(-1, 0, 0, 18));

        mGameView = mmake<GameView>();
        *mGameView->layout = WidgetLayout::BothStretch(0, 0, 0, 19);
        mWindow->AddChild(mGameView);

        InitializeDevicesMenu();

        auto upPanel = mmake<HorizontalLayout>();
        upPanel->name = "up panel";
        upPanel->baseCorner = BaseCorner::Right;
        upPanel->expandWidth = false;
        upPanel->spacing = 5;
        *upPanel->layout = WidgetLayout::HorStretch(VerAlign::Top, 0, 0, 20, 0);
        upPanel->AddLayer("back", mmake<Sprite>("ui/UI4_small_panel_back.png"), Layout::BothStretch(-5, -4, -4, -5));
        mWindow->AddChild(upPanel);

        mResolutionsButton = o2UI.CreateWidget<Button>("panel down");
        mResolutionsButton->caption = "Resolution";
        mResolutionsButton->layout->minWidth = 300;
        mResolutionsButton->onClick = [=]() { mDevicesMenu->Show(mResolutionsButton->layout->GetWorldLeftBottom()); };
        upPanel->AddChild(mResolutionsButton);

        auto resolutionLabel = o2UI.CreateLabel("Resolution:");
        resolutionLabel->horOverflow = Label::HorOverflow::Expand;
        upPanel->AddChild(resolutionLabel);

        OnCurrentWindowSize(true);
    }

    void GameWindow::InitializeDevicesMenu()
    {
        DataDocument doc;
        doc.LoadFromFile(GetEditorAssetsPath() + String("SimulationDevicesList.json"));

        Vector<SimulationDevice> devicesList;
        devicesList = doc["list"];

        for (auto& device: devicesList)
            mDevicesList[device.deviceName] = device;

        mDevicesMenu = o2UI.CreateWidget<ContextMenu>();

        mCurrentWindowSizeItem = mDevicesMenu->AddToggleItem("Current window size", true, THIS_FUNC(OnCurrentWindowSize));

//         mCustomSizeItem = mDevicesMenu->AddToggleItem("Custom resolution", false, THIS_FUNC(OnCustomResolution));
//         mCustomSizeProperty = o2UI.CreateWidget<Vec2IProperty>();
//         mCustomSizeProperty->SetValue(Vec2I(800, 600));
//         *mCustomSizeProperty->layout = WidgetLayout::Based(BaseCorner::Right, Vec2F(150, 20));
//         mCustomSizeProperty->onChangeCompleted = [&](const String&, const Vector<DataDocument>&, const Vector<DataDocument>&) { OnCustomResolution(true); };
//         mCustomSizeItem->AddChild(mCustomSizeProperty);

        mDevicesMenu->AddItem("---");

        for (auto& kv : mDevicesList)
        {
            String caption = kv.first + " (" + (String)kv.second.resolution.x + " x " + (String)kv.second.resolution.y + ")";
            auto item = mDevicesMenu->AddToggleItem(caption, false, [](bool) {});
            item->onChecked = [=](bool checked) { OnDeviceSelected(kv.first, item); };
        }

        mWindow->AddChild(mDevicesMenu);
    }

    void GameWindow::SetResolution(const Vec2I& resolution)
    {
        mGameView->fixedResolution = true;
        mGameView->resolution = resolution;
        mGameView->UpdateRenderTargetSize();
    }

    void GameWindow::OnCurrentWindowSize(bool enabled)
    {
        SetDeviceMenuCheckedItem(mCurrentWindowSizeItem);
        mResolutionsButton->caption = "Current window size";
        mGameView->fixedResolution = false;
        mGameView->UpdateRenderTargetSize();
    }

    void GameWindow::OnCustomResolution(bool enabled)
    {
        SetDeviceMenuCheckedItem(mCustomSizeItem);
        mResolutionsButton->caption = "Fixed " + (String)mCustomSizeProperty->GetCommonValue();
        SetResolution(mCustomSizeProperty->GetCommonValue());
    }

    void GameWindow::OnDeviceSelected(const String& name, const Ref<ContextMenu::Item>& item)
    {
        SetDeviceMenuCheckedItem(item);
        mResolutionsButton->caption = item->text;
        SetResolution(mDevicesList[name].resolution);
    }

    void GameWindow::SetDeviceMenuCheckedItem(const Ref<ContextMenu::Item>& item)
    {
        for (auto& child : mDevicesMenu->GetItemsLayout()->GetChildren())
        {
            if (auto childItem = DynamicCast<ContextMenuItem>(child))
            {
                if (childItem->IsCheckable())
                    childItem->SetChecked(childItem == item->widget.Lock());
            }
        }
    }

    GameWindow::GameView::GameView(RefCounter* refCounter):
        Widget(refCounter)
    {
        mRenderTarget = TextureRef(Vec2I(256, 256), TextureFormat::R8G8B8A8, Texture::Usage::RenderTarget);
        mRenderTargetSprite = mmake<Sprite>(mRenderTarget, RectI(0, 0, 256, 256));
    }

    void GameWindow::GameView::Draw()
    {
        Widget::Draw();

        o2Render.BindRenderTexture(mRenderTarget);

        int editorDepth = EditorScope::GetDepth();
        EditorScope::Exit(editorDepth);

        o2Scene.Draw();

        if (!o2Scene.GetCameras().IsEmpty())
        {
            auto prevCamera = o2Render.GetCamera();
            auto cameraActor = o2Scene.GetCameras()[0].Lock();
            cameraActor->Setup();
            
            if (o2Input.IsKeyDown(VK_F1))
            {
                auto localCursor = cameraActor->listenersLayer->ScreenToLocal(o2Input.GetCursorPos());
                o2Render.DrawCross(localCursor, 25.0f, Color4::Red());
                o2Debug.Log((String)localCursor);
            }

            o2Render.SetCamera(prevCamera);
        }

        EditorScope::Enter(editorDepth);

        o2Render.UnbindRenderTexture();

        mRenderTargetSprite->Draw();

        EditorScope::Exit(editorDepth);

        for (auto& camera : o2Scene.GetCameras())
            camera.Lock()->listenersLayer->OnDrawn(mRenderTargetSprite->GetBasis());

        EditorScope::Enter(editorDepth);
    }

    void GameWindow::GameView::UpdateRenderTargetSize()
    {
        Vec2I size = fixedResolution ? resolution : (Vec2I)layout->size2D.Get();
        size.x = Math::Max(size.x, 32);
        size.y = Math::Max(size.y, 32);

        if (size != mRenderTarget->GetSize())
        {
            mRenderTarget = TextureRef(size, TextureFormat::R8G8B8A8, Texture::Usage::RenderTarget);
            *mRenderTargetSprite = Sprite(mRenderTarget, RectI(Vec2I(), size));
            mRenderTargetSprite->SetMode(SpriteMode::FixedAspect);
        }

        mRenderTargetSprite->SetRect(layout->worldRect);
    }

    String GameWindow::GameView::GetCreateMenuCategory()
    {
        return "UI/Editor";
    }

    void GameWindow::GameView::OnTransformUpdated()
    {
        Widget::OnTransformUpdated();
        UpdateRenderTargetSize();
    }

    bool GameWindow::SimulationDevice::operator==(const SimulationDevice& other) const
    {
        return deviceName == other.deviceName;
    }
}

DECLARE_TEMPLATE_CLASS(o2::LinkRef<Editor::GameWindow::GameView>);
// --- META ---

DECLARE_CLASS(Editor::GameWindow, Editor__GameWindow);

DECLARE_CLASS(Editor::GameWindow::GameView, Editor__GameWindow__GameView);

DECLARE_CLASS(Editor::GameWindow::SimulationDevice, Editor__GameWindow__SimulationDevice);
// --- END META ---
