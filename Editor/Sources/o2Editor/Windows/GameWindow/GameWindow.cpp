#include "o2Editor/stdafx.h"
#include "GameWindow.h"

#include "o2/Application/Application.h"
#include "o2/Render/VectorGraphics/SvgParser.h"
#include "o2/Render/VectorGraphics/VectorImage.h"
#include "o2/Render/VectorGraphics/VectorRasterizer.h"
#include "o2/Render/VectorGraphics/VectorSprite.h"
#include "o2/Scene/CameraActor.h"
#include "o2/Scene/UI/Widgets/Button.h"
#include "o2/Utils/Bitmap/Bitmap.h"
#include "o2/Utils/Debug/Profiling/ProfilerOverlay.h"
#include "o2/Utils/Editor/EditorScope.h"
#include "o2Editor/EditorApplication.h"
#include "o2Editor/EditorConfig.h"
#include "o2Editor/Windows/GameWindow/GameDevices.h"

namespace Editor
{
    namespace
    {
        const float kMinZoom = 0.25f;
        const float kMaxZoom = 16.0f;
        const float kFitMargin = 0.94f; // the fitted device leaves a little of the view around it
    }

    // Zooms and pans the game view by the mouse, as the input comes in: Alt + wheel zooms around the cursor, the middle
    // button or Alt with the left one drags the picture
    class GameViewNavigation: public CursorEventsListener
    {
    public:
        GameViewNavigation(RefCounter* refCounter, GameWindow* window):
            CursorEventsListener(refCounter), mWindow(window)
        {}

        // The view is being dragged: the game gets no input meanwhile
        bool IsPanning() const { return mPanning; }

    protected:
        GameWindow* mWindow = nullptr;
        bool  mPanning = false;
        Vec2F mLast;

    protected:
        bool IsOver(const Vec2F& point) const
        {
            return mWindow && mWindow->IsVisible() && mWindow->GetGameViewWidget()->IsEnabledInHierarchy() &&
                mWindow->GetGameViewWidget()->layout->IsPointInside(point) && !mWindow->IsDevicesMenuOpen();
        }

        void Start(const Vec2F& point)
        {
            mPanning = true;
            mLast = point;
        }

        void Drag(const Vec2F& point)
        {
            if (!mPanning)
                return;
            mWindow->GetGameViewWidget()->Pan(point - mLast);
            mLast = point;
            mWindow->OnViewNavigated();
        }

        void OnScrolled(float scroll) override
        {
            Vec2F cursor = o2Input.GetCursorPos();
            if (!o2Input.IsKeyDown(VK_MENU) || !IsOver(cursor))
                return;

            // a wheel notch reports about 120, a touchpad far less: any turn zooms visibly
            float notches = Math::Clamp(scroll/120.0f, -2.0f, 2.0f);
            if (Math::Abs(notches) < 0.25f)
                notches = notches < 0 ? -0.25f : 0.25f;

            auto view = mWindow->GetGameViewWidget();
            view->SetZoom(view->GetZoom()*Math::Pow(1.25f, notches), cursor);
            mWindow->OnViewNavigated();
        }

        void OnCursorMiddleMousePressed(const Input::Cursor& cursor) override
        {
            if (IsOver(cursor.position))
                Start(cursor.position);
        }

        void OnCursorMiddleMouseStillDown(const Input::Cursor& cursor) override { Drag(cursor.position); }
        void OnCursorMiddleMouseReleased(const Input::Cursor& cursor) override { mPanning = false; }

        void OnCursorPressed(const Input::Cursor& cursor) override
        {
            if (o2Input.IsKeyDown(VK_MENU) && IsOver(cursor.position))
                Start(cursor.position);
        }

        void OnCursorStillDown(const Input::Cursor& cursor) override { Drag(cursor.position); }
        void OnCursorReleased(const Input::Cursor& cursor) override { mPanning = false; }
    };

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
        mWindow->SetIcon(mmake<VectorSprite>("ui/UI4_game_icon.svg"));
        mWindow->SetIconLayout(Layout::Based(BaseCorner::LeftTop, Vec2F(20, 20), Vec2F(-1, 2)));
        mWindow->SetViewLayout(Layout::BothStretch(-1, 0, 0, 18));

        mGameView = mmake<GameView>();
        *mGameView->layout = WidgetLayout::BothStretch(0, 0, 0, 19);
        mWindow->AddChild(mGameView);

        InitializeDevicesMenu();
        mNavigation = mmake<GameViewNavigation>(this);

        auto upPanel = mmake<HorizontalLayout>();
        upPanel->name = "up panel";
        upPanel->baseCorner = BaseCorner::Right;
        upPanel->expandWidth = false;
        upPanel->spacing = 0; // The buttons' backs are meant to sit flush, a gap breaks the strip
        *upPanel->layout = WidgetLayout::HorStretch(VerAlign::Top, 0, 0, 20, 0);
        upPanel->AddLayer("back", mmake<VectorSprite>("ui/UI4_small_panel_back.svg"), Layout::BothStretch(-5, -4, -4, -5));
        mWindow->AddChild(upPanel);

        // The panel arranges its children from the right, so the first one added sits rightmost
        mProfilerButton = o2UI.CreateWidget<Button>("panel down");
        mProfilerButton->name = "profiler button";
        mProfilerButton->layout->minWidth = 96;
        mProfilerButton->onClick = THIS_FUNC(OnProfilerPressed);
        upPanel->AddChild(mProfilerButton);

        UpdateProfilerButtonCaption();

        auto zoomIn = CreatePanelButton("zoom in", "+", 24, false);
        zoomIn->onClick = [=]() { mGameView->SetZoom(mGameView->GetZoom()*1.25f, mGameView->layout->GetWorldRect().Center()); UpdateToolbar(); };
        upPanel->AddChild(zoomIn);

        mZoomButton = CreatePanelButton("zoom", "Fit", 52, false);
        mZoomButton->onClick = [=]() { mGameView->ResetZoom(); UpdateToolbar(); };
        upPanel->AddChild(mZoomButton);

        auto zoomOut = CreatePanelButton("zoom out", "-", 24, false);
        zoomOut->onClick = [=]() { mGameView->SetZoom(mGameView->GetZoom()/1.25f, mGameView->layout->GetWorldRect().Center()); UpdateToolbar(); };
        upPanel->AddChild(zoomOut);

        mOrientationButton = CreatePanelButton("orientation", "Portrait", 76, false);
        mOrientationButton->onClick = [=]() { SelectDevice(GetSelectedDevice(), !mGameView->IsLandscape()); };
        upPanel->AddChild(mOrientationButton);

        mDeviceButton = CreatePanelButton("device", "Fit to window", 116, true);
        mDeviceButton->onClick = [=]() { mDevicesMenu->Show(mDeviceButton->layout->GetWorldLeftBottom()); };
        upPanel->AddChild(mDeviceButton);

        String device;
        bool landscape = false;
        if (EditorConfig::IsSingletonInitialzed())
        {
            device = o2EditorConfig.projectConfig.GetGameDevice();
            landscape = o2EditorConfig.projectConfig.GetGameDeviceLandscape();
        }
        SelectDevice(device, landscape);
    }

    void GameWindow::Update(float dt)
    {
        IEditorWindow::Update(dt);

        // The panel has its own hotkey, so the caption follows the panel and not the other way round
        UpdateProfilerButtonCaption();

        // while Alt is held over the picture or the picture is dragged, the mouse is the view's and not the game's
        bool over = mGameView->layout->IsPointInside(o2Input.GetCursorPos());
        mGameView->gameInput = !(mNavigation && mNavigation->IsPanning()) && !(over && o2Input.IsKeyDown(VK_MENU));
    }

    void GameWindow::OnViewNavigated()
    {
        UpdateToolbar();
    }

    bool GameWindow::IsDevicesMenuOpen() const
    {
        return mDevicesMenu && mDevicesMenu->IsEnabled();
    }

    void GameWindow::OnProfilerPressed()
    {
#if defined(O2_PROFILER_ENABLED)
        if (auto profiler = ProfilerOverlay::InstancePtr())
            profiler->SetVisible(!profiler->IsVisible());
#endif
    }

    void GameWindow::UpdateProfilerButtonCaption()
    {
        if (!mProfilerButton)
            return;

#if defined(O2_PROFILER_ENABLED)
        auto profiler = ProfilerOverlay::InstancePtr();
        mProfilerButton->caption = profiler && profiler->IsVisible() ? "Profiler: on" : "Profiler: off";
#else
        mProfilerButton->caption = "Profiler: off";
        mProfilerButton->SetEnabled(false);
#endif
    }

    void GameWindow::InitializeDevicesMenu()
    {
        mDevicesMenu = o2UI.CreateWidget<ContextMenu>();

        mFitItem = mDevicesMenu->AddToggleItem("Fit to window", true);
        mFitItem->onChecked = [=](bool) { SelectDevice("", false); };

        String group;
        for (auto& device : GetGameDevices())
        {
            if (device.group != group)
            {
                mDevicesMenu->AddItem("---");
                group = device.group;
            }

            auto item = mDevicesMenu->AddToggleItem(device.name, false);
            item->hint = device.GetCaption().SubStr(device.name.Length()).Trimed(" ");
            String id = device.id;
            item->onChecked = [=](bool) { SelectDevice(id, mGameView->IsLandscape()); };
            mDeviceItems[id] = item;
        }

        mWindow->AddChild(mDevicesMenu);
    }

    void GameWindow::SelectDevice(const String& id, bool landscape)
    {
        const GameDevice* device = FindGameDevice(id);
        mGameView->SetDevice(device, device && landscape && !device->IsFixedLandscape());

        // the game gets the device's safe area in its own pixels, as the device would report it in its points
        if (device)
        {
            BorderF insets = device->GetSafeArea(mGameView->IsLandscape());
            float scale = (float)device->pixels.x/device->size.x;
            o2Application.SetSafeAreaOverride(BorderF(insets.left*scale, insets.bottom*scale, insets.right*scale, insets.top*scale));
        }
        else
            o2Application.ResetSafeAreaOverride();

        if (EditorConfig::IsSingletonInitialzed())
        {
            o2EditorConfig.projectConfig.SetGameDevice(device ? id : String());
            o2EditorConfig.projectConfig.SetGameDeviceLandscape(mGameView->IsLandscape());
        }

        UpdateToolbar();
    }

    String GameWindow::GetSelectedDevice() const
    {
        auto device = mGameView->GetDevice();
        return device ? device->id : String();
    }

    void GameWindow::UpdateToolbar()
    {
        auto device = mGameView->GetDevice();

        if (mDeviceButton)
            mDeviceButton->caption = device ? device->name : String("Fit to window");

        if (mOrientationButton)
        {
            mOrientationButton->caption = mGameView->IsLandscape() ? "Landscape" : "Portrait";
            mOrientationButton->SetEnabled(device && !device->IsFixedLandscape());
        }

        if (mZoomButton)
            mZoomButton->caption = Math::Equals(mGameView->GetZoom(), 1.0f, 0.001f) && mGameView->GetPan() == Vec2F() ?
                String("Fit") : String::Format("%.0f%%", mGameView->GetZoom()*100.0f);

        if (mFitItem && mFitItem->widget)
            mFitItem->widget.Lock()->SetChecked(device == nullptr);

        for (auto& kv : mDeviceItems)
        {
            if (kv.second->widget)
                kv.second->widget.Lock()->SetChecked(device && device->id == kv.first);
        }
    }

    Ref<Button> GameWindow::CreatePanelButton(const String& name, const WString& caption, float width, bool menu)
    {
        auto button = o2UI.CreateWidget<Button>("panel down");
        button->name = name;
        button->caption = caption;
        button->layout->minWidth = width;
        if (!menu)
        {
            // a button of the panel's look without the arrow of a menu
            if (auto arrow = button->FindLayer("arrow"))
                arrow->enabled = false;
            if (auto text = button->FindLayer("caption"))
                text->layout = Layout::BothStretch(2, 0, 2, 0);
        }
        return button;
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

        auto detailView = VectorSprite::GetDetailView();
        VectorSprite::SetDetailView(VectorSprite::DetailView::Game);
        o2Scene.Draw();
        VectorSprite::SetDetailView(detailView);

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

        // a zoomed picture stays inside the view, and so do the game's listeners: clicks outside it are not the game's
        RectF view = layout->GetWorldRect();
        o2Render.EnableScissorTest(RectI((int)Math::Floor(view.left), (int)Math::Floor(view.bottom),
                                         (int)Math::Ceil(view.right), (int)Math::Ceil(view.top)));

        if (mRoundedScreen)
            mScreenMesh.Draw();
        else
            mRenderTargetSprite->Draw();
        if (mDevice && mBodySprite)
            mBodySprite->Draw();

        if (gameInput)
        {
            EditorScope::Exit(editorDepth);

            for (auto& camera : o2Scene.GetCameras())
                camera.Lock()->listenersLayer->OnDrawn(mRenderTargetSprite->GetBasis());

            EditorScope::Enter(editorDepth);
        }

        o2Render.DisableScissorTest();

#if defined(O2_PROFILER_ENABLED)
        // The panel measures the game, so in the editor it belongs to the window the game is drawn in.
        // Drawn inside the editor scope, so its own draw calls stay out of the scene counters
        if (auto profiler = ProfilerOverlay::InstancePtr())
            profiler->DrawIn(layout->GetWorldRect());
#endif
    }

    void GameWindow::GameView::SetDevice(const GameDevice* device, bool landscape)
    {
        bool changed = device != mDevice || landscape != mLandscape;
        mDevice = device;
        mLandscape = device ? landscape : false;
        if (changed)
        {
            mZoom = 1.0f;
            mPan = Vec2F();
        }

        UpdateRenderTargetSize();
    }

    const GameDevice* GameWindow::GameView::GetDevice() const
    {
        return mDevice;
    }

    bool GameWindow::GameView::IsLandscape() const
    {
        return mLandscape;
    }

    void GameWindow::GameView::SetZoom(float zoom, const Vec2F& pivot)
    {
        zoom = Math::Clamp(zoom, kMinZoom, kMaxZoom);

        // the point of the picture under the pivot stays under it
        Vec2F center = layout->GetWorldRect().Center();
        Vec2F fromCenter = pivot - (center + mPan);
        mPan += fromCenter - fromCenter*(zoom/mZoom);
        mZoom = zoom;

        ClampPan();
        UpdateRects();
    }

    float GameWindow::GameView::GetZoom() const
    {
        return mZoom;
    }

    void GameWindow::GameView::Pan(const Vec2F& delta)
    {
        mPan += delta;
        ClampPan();
        UpdateRects();
    }

    Vec2F GameWindow::GameView::GetPan() const
    {
        return mPan;
    }

    void GameWindow::GameView::ResetZoom()
    {
        mZoom = 1.0f;
        mPan = Vec2F();
        UpdateRects();
    }

    RectF GameWindow::GameView::GetScreenRect() const
    {
        return mScreenRect;
    }

    RectF GameWindow::GameView::GetBodyRect() const
    {
        return mDevice ? mBodyRect : RectF();
    }

    Vec2I GameWindow::GameView::GetRenderTargetSize() const
    {
        return mRenderTarget->GetSize();
    }

    void GameWindow::GameView::ClampPan()
    {
        // at least a quarter of the picture's smaller side stays in the view
        RectF view = layout->GetWorldRect();
        Vec2F content = (mDevice ? mBodyRect.Size() : mScreenRect.Size());
        Vec2F reach = (view.Size() + content)*0.5f - Vec2F(1, 1)*Math::Min(content.x, content.y)*0.25f;
        mPan.x = Math::Clamp(mPan.x, -Math::Max(reach.x, 0.0f), Math::Max(reach.x, 0.0f));
        mPan.y = Math::Clamp(mPan.y, -Math::Max(reach.y, 0.0f), Math::Max(reach.y, 0.0f));
    }

    void GameWindow::GameView::UpdateRenderTargetSize()
    {
        // a device draws in its real pixels; otherwise the game takes the view's size, or a fixed resolution
        Vec2I size = (Vec2I)layout->size2D.Get();
        if (mDevice)
            size = mDevice->GetScreenPixels(mLandscape);
        else if (fixedResolution && resolution != Vec2I())
            size = resolution;

        size.x = Math::Max(size.x, 32);
        size.y = Math::Max(size.y, 32);

        if (size != mRenderTarget->GetSize())
        {
            mRenderTarget = TextureRef(size, TextureFormat::R8G8B8A8, Texture::Usage::RenderTarget);
            *mRenderTargetSprite = Sprite(mRenderTarget, RectI(Vec2I(), size));
            mRenderTargetSprite->SetMode(SpriteMode::Default);
        }

        UpdateRects();
    }

    void GameWindow::GameView::UpdateRects()
    {
        RectF view = layout->GetWorldRect();
        Vec2F center = view.Center() + mPan;

        if (mDevice)
        {
            Vec2F bodySize, screenOffset;
            mDevice->GetBodySvg(mLandscape, bodySize, screenOffset);
            Vec2F screen = mDevice->GetScreenSize(mLandscape);

            // the device in its points fitted into the view, then zoomed; the SVG counts from the top left
            float fit = Math::Min(view.Width()*kFitMargin/bodySize.x, view.Height()*kFitMargin/bodySize.y);
            float scale = Math::Max(fit, 0.0001f)*mZoom;
            mBodyRect = RectF(center - bodySize*scale*0.5f, center + bodySize*scale*0.5f);
            Vec2F screenLeftTop(mBodyRect.left + screenOffset.x*scale, mBodyRect.top - screenOffset.y*scale);
            mScreenRect = RectF(screenLeftTop.x, screenLeftTop.y - screen.y*scale, screenLeftTop.x + screen.x*scale, screenLeftTop.y);

            UpdateBodySprite(scale);
        }
        else
        {
            Vec2F size = view.Size()*mZoom;
            mScreenRect = RectF(center - size*0.5f, center + size*0.5f);
            mBodyRect = RectF();
        }

        mRenderTargetSprite->SetRect(mScreenRect);

        // the screen's corners are rounded on the real device: the picture is cut by them, as the portal's frame does
        mRoundedScreen = false;
        if (mDevice)
        {
            float scale = mScreenRect.Width()/Math::Max(mDevice->GetScreenSize(mLandscape).x, 1.0f);
            if (mDevice->look == "window")
                BuildScreenMesh(0, 0, 9*scale, 9*scale);
            else if (mDevice->radius > 0)
                BuildScreenMesh(mDevice->radius*scale, mDevice->radius*scale, mDevice->radius*scale, mDevice->radius*scale);
        }
    }

    void GameWindow::GameView::BuildScreenMesh(float tl, float tr, float br, float bl)
    {
        const int segments = 10;
        RectF r = mScreenRect;
        Vec2F size = r.Size();
        if (size.x < 1.0f || size.y < 1.0f)
            return;

        // the outline: four arcs, each from its edge to the next, the centre fans them
        struct Corner { Vec2F center; float radius; float from; };
        auto clampR = [&](float v) { return Math::Clamp(v, 0.0f, Math::Min(size.x, size.y)*0.5f); };
        Corner corners[4] = {
            { Vec2F(r.right - clampR(tr), r.top - clampR(tr)), clampR(tr), 0.0f },
            { Vec2F(r.left + clampR(tl), r.top - clampR(tl)), clampR(tl), 90.0f },
            { Vec2F(r.left + clampR(bl), r.bottom + clampR(bl)), clampR(bl), 180.0f },
            { Vec2F(r.right - clampR(br), r.bottom + clampR(br)), clampR(br), 270.0f },
        };

        UInt outline = 4*(segments + 1);
        mScreenMesh = Mesh(mRenderTarget, outline + 1, outline);
        mScreenMesh.SetTextureSrcRect(RectI(Vec2I(), mRenderTarget->GetSize()));

        ULong white = Color4::White().ABGR();
        Vertex* verts = mScreenMesh.GetVertices<Vertex>();
        VertexIndex* indexes = mScreenMesh.GetIndexes();
        auto uv = [&](const Vec2F& p) { return Vec2F((p.x - r.left)/size.x, (r.top - p.y)/size.y); };

        Vec2F c = r.Center();
        Vec2F cuv = uv(c);
        verts[0].Set(c, white, cuv.x, cuv.y, Vec2F(1, 0));

        UInt n = 1;
        for (auto& corner : corners)
        {
            for (int i = 0; i <= segments; i++)
            {
                float angle = Math::Deg2rad(corner.from + 90.0f*(float)i/(float)segments);
                Vec2F p = corner.center + Vec2F(Math::Cos(angle), Math::Sin(angle))*corner.radius;
                Vec2F t = uv(p);
                verts[n++].Set(p, white, t.x, t.y, Vec2F(1, 0));
            }
        }

        for (UInt i = 0; i < outline; i++)
        {
            indexes[i*3] = 0;
            indexes[i*3 + 1] = 1 + i;
            indexes[i*3 + 2] = 1 + (i + 1)%outline;
        }

        mScreenMesh.vertexCount = outline + 1;
        mScreenMesh.polyCount = outline;
        mRoundedScreen = true;
    }

    void GameWindow::GameView::UpdateBodySprite(float pointsToWindow)
    {
        // the raster follows the drawn size (crisp when zoomed), but only when it changes enough to show
        float pixels = pointsToWindow*o2Application.GetGraphicsScale();
        String key = mDevice->id + (mLandscape ? ":land" : ":port");
        Vec2F bodySize, screenOffset;
        String svg = mDevice->GetBodySvg(mLandscape, bodySize, screenOffset);
        float maxScale = 4096.0f/Math::Max(bodySize.x, bodySize.y);
        pixels = Math::Min(pixels, maxScale);

        bool stale = key != mBodyKey || !mBodySprite || pixels > mBodyRasterScale*1.25f || pixels < mBodyRasterScale*0.6f;
        if (stale)
        {
            VectorImage image;
            if (SvgParser::Parse(svg, image))
            {
                auto bitmap = VectorRasterizer::Rasterize(image, pixels);
                if (bitmap)
                {
                    VectorRasterizer::Unpremultiply(*bitmap);
                    TextureRef texture(*bitmap);
                    mBodySprite = mmake<Sprite>(texture, RectI(Vec2I(), bitmap->GetSize()));
                    mBodySprite->SetMode(SpriteMode::Default);
                    mBodyKey = key;
                    mBodyRasterScale = pixels;
                }
            }
        }

        if (mBodySprite)
            mBodySprite->SetRect(mBodyRect);
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
}

DECLARE_TEMPLATE_CLASS(o2::LinkRef<Editor::GameWindow::GameView>);
// --- META ---

DECLARE_CLASS(Editor::GameWindow, Editor__GameWindow);

DECLARE_CLASS(Editor::GameWindow::GameView, Editor__GameWindow__GameView);
// --- END META ---
