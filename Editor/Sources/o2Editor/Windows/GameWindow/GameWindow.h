#pragma once

#include "o2/Events/CursorAreaEventsListenersLayer.h"
#include "o2/Render/Meshes/Mesh.h"
#include "o2Editor/Windows/IEditorWindow.h"

namespace o2
{
    class Widget;
    class DropDown;
}

namespace Editor
{
    FORWARD_CLASS_REF(Vec2IProperty);
    struct GameDevice;
    class GameViewNavigation;

    // ---------------------------------------------------------------------------------------------------------------
    // Game window. Draws the scene's cameras into the game's own resolution and passes the input to the game. Emulates
    // a device: its real resolution and safe area, its body drawn around the picture, all fitted into the window.
    // Alt + wheel zooms around the cursor, the middle button or Alt + left button pans, the toolbar's zoom button fits
    // the picture again; clicks reach the game wherever its picture is drawn
    // ---------------------------------------------------------------------------------------------------------------
    class GameWindow : public IEditorWindow
    {
    public:
        // ----------------------------------------
        // Game view widget, used for drawing scene
        // ----------------------------------------
        class GameView : public Widget
        {
        public:
            bool  fixedResolution = false; // Is view resolution fixed (without a device)
            Vec2I resolution;             // View resolution
            bool  gameInput = true;       // Does the game get the input this frame (not while the view is panned)

        public:
            // Default constructor, initializes view
            GameView(RefCounter* refCounter);

            // Draws view
            void Draw() override;

            // Updates render target size
            void UpdateRenderTargetSize();

            // Emulates the device: its real resolution, its body around the picture; null - the picture fills the view
            // in the view's own resolution
            void SetDevice(const GameDevice* device, bool landscape);

            // Returns the emulated device, null when the picture fills the view
            const GameDevice* GetDevice() const;

            // Returns true when the device lies on its side
            bool IsLandscape() const;

            // Sets the zoom (1 - fitted into the view) keeping the point of the window under the same place of the picture
            void SetZoom(float zoom, const Vec2F& pivot);

            // Returns the zoom: 1 - fitted into the view
            float GetZoom() const;

            // Moves the picture by the delta in window coordinates
            void Pan(const Vec2F& delta);

            // Returns the offset of the picture from the view's centre
            Vec2F GetPan() const;

            // Fits the picture into the view again
            void ResetZoom();

            // Returns where the game's picture is drawn, in window coordinates
            RectF GetScreenRect() const;

            // Returns where the device's body is drawn, in window coordinates; empty without a device
            RectF GetBodyRect() const;

            // Returns the size of the render target the game draws into
            Vec2I GetRenderTargetSize() const;

            // Returns create menu category in editor
            static String GetCreateMenuCategory();

            SERIALIZABLE(GameView);
            CLONEABLE_REF(GameView);

        protected:
            Ref<Sprite> mRenderTargetSprite; // Render target sprite, using for drawing scene render target
            TextureRef  mRenderTarget;       // Render target texture, using for rendering scene

            const GameDevice* mDevice = nullptr; // Emulated device
            bool  mLandscape = false;            // The device lies on its side
            float mZoom = 1.0f;                  // Zoom: 1 - fitted into the view
            Vec2F mPan;                          // Offset of the picture from the view's centre

            Mesh        mScreenMesh;            // The picture with the screen's rounded corners
            bool        mRoundedScreen = false; // The picture is drawn by the mesh

            Ref<Sprite> mBodySprite;   // The device's body drawn over the picture's edges
            RectF       mBodyRect;     // Where the body is drawn
            RectF       mScreenRect;   // Where the picture is drawn
            String      mBodyKey;      // Device and orientation of the body raster
            float       mBodyRasterScale = 0.0f; // Pixels of the body raster per device point

        protected:
            // Called when transformation was changed and updated, updates render texture and sprite
            void OnTransformUpdated() override;

            // Places the picture and the body by the view, the device, the zoom and the pan
            void UpdateRects();

            // Rasterizes the device's body when the device or its drawn size has changed enough
            void UpdateBodySprite(float pointsToWindow);

            // Keeps a part of the picture in the view
            void ClampPan();

            // Builds the picture's mesh with the screen's rounded corners: top left, top right, bottom right, bottom left
            void BuildScreenMesh(float tl, float tr, float br, float bl);
        };

    public:
        // Returns the game view widget, so the page can map world points into canvas pixels
        const Ref<GameView>& GetGameViewWidget() const { return mGameView; }

        // Default constructor, initializes window and creates controls
        GameWindow();

        // Copy constructor
        GameWindow(const GameWindow& other);

        // Destructor
        ~GameWindow();

        // Updates the toolbar; the mouse is the view's while Alt is held over the picture or the picture is dragged
        void Update(float dt) override;

        // Emulates the device by its id ("" - the picture fills the view), the device stands or lies on its side
        void SelectDevice(const String& id, bool landscape);

        // Returns the id of the emulated device, empty when the picture fills the view
        String GetSelectedDevice() const;

        // Returns true while the devices menu is open
        bool IsDevicesMenuOpen() const;

        // Called when the view was zoomed or panned by the mouse: the toolbar follows
        void OnViewNavigated();

        IOBJECT(GameWindow);

    protected:
        Ref<GameView> mGameView; // Game view widget

        Ref<Button> mDeviceButton;      // Devices menu button
        Ref<Button> mOrientationButton; // Portrait / landscape
        Ref<Button> mZoomButton;        // Shows the zoom, fits the picture again
        Ref<Button> mProfilerButton;    // Shows and hides the profiler panel over this window

        Ref<ContextMenu>             mDevicesMenu;   // Devices menu
        Ref<ContextMenu::Item>       mFitItem;       // The picture fills the view
        Map<String, Ref<ContextMenu::Item>> mDeviceItems; // Items of the devices by their ids

        Ref<GameViewNavigation> mNavigation; // Zooms and pans the view by the mouse

    protected:
        // Initializes window
        void InitializeWindow();

        // Shows or hides the profiler panel
        void OnProfilerPressed();

        // Puts the current profiler panel state into the button caption
        void UpdateProfilerButtonCaption();

        // Initializes devices menu
        void InitializeDevicesMenu();

        // Puts the device, the orientation and the zoom into the toolbar and the menu
        void UpdateToolbar();

        // Creates a button of the toolbar; menu - with the arrow of a menu
        Ref<Button> CreatePanelButton(const String& name, const WString& caption, float width, bool menu);
    };
}
// --- META ---

CLASS_BASES_META(Editor::GameWindow)
{
    BASE_CLASS(Editor::IEditorWindow);
}
END_META;
CLASS_FIELDS_META(Editor::GameWindow)
{
    FIELD().PROTECTED().NAME(mGameView);
    FIELD().PROTECTED().NAME(mDeviceButton);
    FIELD().PROTECTED().NAME(mOrientationButton);
    FIELD().PROTECTED().NAME(mZoomButton);
    FIELD().PROTECTED().NAME(mProfilerButton);
    FIELD().PROTECTED().NAME(mDevicesMenu);
    FIELD().PROTECTED().NAME(mFitItem);
    FIELD().PROTECTED().NAME(mDeviceItems);
    FIELD().PROTECTED().NAME(mNavigation);
}
END_META;
CLASS_METHODS_META(Editor::GameWindow)
{

    FUNCTION().PUBLIC().SIGNATURE(const Ref<GameView>&, GetGameViewWidget);
    FUNCTION().PUBLIC().CONSTRUCTOR();
    FUNCTION().PUBLIC().CONSTRUCTOR(const GameWindow&);
    FUNCTION().PUBLIC().SIGNATURE(void, Update, float);
    FUNCTION().PUBLIC().SIGNATURE(void, SelectDevice, const String&, bool);
    FUNCTION().PUBLIC().SIGNATURE(String, GetSelectedDevice);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsDevicesMenuOpen);
    FUNCTION().PUBLIC().SIGNATURE(void, OnViewNavigated);
    FUNCTION().PROTECTED().SIGNATURE(void, InitializeWindow);
    FUNCTION().PROTECTED().SIGNATURE(void, OnProfilerPressed);
    FUNCTION().PROTECTED().SIGNATURE(void, UpdateProfilerButtonCaption);
    FUNCTION().PROTECTED().SIGNATURE(void, InitializeDevicesMenu);
    FUNCTION().PROTECTED().SIGNATURE(void, UpdateToolbar);
    FUNCTION().PROTECTED().SIGNATURE(Ref<Button>, CreatePanelButton, const String&, const WString&, float, bool);
}
END_META;

CLASS_BASES_META(Editor::GameWindow::GameView)
{
    BASE_CLASS(o2::Widget);
}
END_META;
CLASS_FIELDS_META(Editor::GameWindow::GameView)
{
    FIELD().PUBLIC().DEFAULT_VALUE(false).NAME(fixedResolution);
    FIELD().PUBLIC().NAME(resolution);
    FIELD().PUBLIC().DEFAULT_VALUE(true).NAME(gameInput);
    FIELD().PROTECTED().NAME(mRenderTargetSprite);
    FIELD().PROTECTED().NAME(mRenderTarget);
    FIELD().PROTECTED().DEFAULT_VALUE(nullptr).NAME(mDevice);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mLandscape);
    FIELD().PROTECTED().DEFAULT_VALUE(1.0f).NAME(mZoom);
    FIELD().PROTECTED().NAME(mPan);
    FIELD().PROTECTED().NAME(mScreenMesh);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mRoundedScreen);
    FIELD().PROTECTED().NAME(mBodySprite);
    FIELD().PROTECTED().NAME(mBodyRect);
    FIELD().PROTECTED().NAME(mScreenRect);
    FIELD().PROTECTED().NAME(mBodyKey);
    FIELD().PROTECTED().DEFAULT_VALUE(0.0f).NAME(mBodyRasterScale);
}
END_META;
CLASS_METHODS_META(Editor::GameWindow::GameView)
{

    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*);
    FUNCTION().PUBLIC().SIGNATURE(void, Draw);
    FUNCTION().PUBLIC().SIGNATURE(void, UpdateRenderTargetSize);
    FUNCTION().PUBLIC().SIGNATURE(void, SetDevice, const GameDevice*, bool);
    FUNCTION().PUBLIC().SIGNATURE(const GameDevice*, GetDevice);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsLandscape);
    FUNCTION().PUBLIC().SIGNATURE(void, SetZoom, float, const Vec2F&);
    FUNCTION().PUBLIC().SIGNATURE(float, GetZoom);
    FUNCTION().PUBLIC().SIGNATURE(void, Pan, const Vec2F&);
    FUNCTION().PUBLIC().SIGNATURE(Vec2F, GetPan);
    FUNCTION().PUBLIC().SIGNATURE(void, ResetZoom);
    FUNCTION().PUBLIC().SIGNATURE(RectF, GetScreenRect);
    FUNCTION().PUBLIC().SIGNATURE(RectF, GetBodyRect);
    FUNCTION().PUBLIC().SIGNATURE(Vec2I, GetRenderTargetSize);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(String, GetCreateMenuCategory);
    FUNCTION().PROTECTED().SIGNATURE(void, OnTransformUpdated);
    FUNCTION().PROTECTED().SIGNATURE(void, UpdateRects);
    FUNCTION().PROTECTED().SIGNATURE(void, UpdateBodySprite, float);
    FUNCTION().PROTECTED().SIGNATURE(void, ClampPan);
    FUNCTION().PROTECTED().SIGNATURE(void, BuildScreenMesh, float, float, float, float);
}
END_META;
// --- END META ---
