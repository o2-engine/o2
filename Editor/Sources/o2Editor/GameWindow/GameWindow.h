#pragma once

#include "o2/Events/CursorAreaEventsListenersLayer.h"
#include "o2Editor/Core/WindowsSystem/IEditorWindow.h"

namespace o2
{
    class Widget;
    class DropDown;
}

namespace Editor
{
    FORWARD_CLASS_REF(Vec2IProperty);

    // ---------------------------------------
    // Game window. Draws scene, handles input
    // ---------------------------------------
    class GameWindow : public IEditorWindow
    {
    public:
        // ----------------------------------------
        // Game view widget, used for drawing scene
        // ----------------------------------------
        class GameView : public Widget
        {
        public:
            bool  fixedResolution = true; // Is view resolution fixed
            Vec2I resolution;             // View resolution

        public:
            // Default constructor, initializes view
            GameView(RefCounter* refCounter);

            // Draws view
            void Draw() override;

            // Updates render target size
            void UpdateRenderTargetSize();

            // Returns create menu category in editor
            static String GetCreateMenuCategory();

            SERIALIZABLE(GameView);
            CLONEABLE_REF(GameView);

        protected:
            Ref<Sprite> mRenderTargetSprite; // Render target sprite, using for drawing scene render target
            TextureRef  mRenderTarget;       // Render target texture, using for rendering scene

        protected:
            // Called when transformation was changed and updated, updates render texture and sprite
            void OnTransformUpdated() override;
        };

        // ---------------------------
        // Simulation devicedefinition
        // ---------------------------
        class SimulationDevice : public ISerializable
        {
        public:
            String deviceName; // Name of device @SERIALIZABLE
            Vec2I  resolution; // Target resolution @SERIALIZABLE

        public:
            // Equality operator
            bool operator==(const SimulationDevice& other) const;

            SERIALIZABLE(SimulationDevice);
        };

    public:
        // Default constructor, initializes window and creates controls
        GameWindow();

        // Copy constructor
        GameWindow(const GameWindow& other);

        // Destructor
        ~GameWindow();

        IOBJECT(GameWindow);

    protected:
        Ref<GameView> mGameView; // Game view widget

        Ref<Button> mResolutionsButton; // Resolutions popup button

        Map<String, SimulationDevice> mDevicesList; // List of devices

        Ref<ContextMenu>       mDevicesMenu;           // Devices menu
        Ref<ContextMenu::Item> mCurrentWindowSizeItem; // Current window size item
        Ref<ContextMenu::Item> mCustomSizeItem;        // Custom size item

        Ref<Vec2IProperty> mCustomSizeProperty; // Custom view resolution size property

    protected:
        // Initializes window
        void InitializeWindow();

        // Initializes devices menu
        void InitializeDevicesMenu();

        // Sets game resolution
        void SetResolution(const Vec2I& resolution);

        // When current window size resolution checked
        void OnCurrentWindowSize(bool enabled);

        // When custom resolution checked
        void OnCustomResolution(bool enabled);

        // When device selected
        void OnDeviceSelected(const String& name, const Ref<ContextMenu::Item>& item);

        // Sets device menu checked item by name
        void SetDeviceMenuCheckedItem(const Ref<ContextMenu::Item>& item);
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
    FIELD().PROTECTED().NAME(mResolutionsButton);
    FIELD().PROTECTED().NAME(mDevicesList);
    FIELD().PROTECTED().NAME(mDevicesMenu);
    FIELD().PROTECTED().NAME(mCurrentWindowSizeItem);
    FIELD().PROTECTED().NAME(mCustomSizeItem);
    FIELD().PROTECTED().NAME(mCustomSizeProperty);
}
END_META;
CLASS_METHODS_META(Editor::GameWindow)
{

    FUNCTION().PUBLIC().CONSTRUCTOR();
    FUNCTION().PUBLIC().CONSTRUCTOR(const GameWindow&);
    FUNCTION().PROTECTED().SIGNATURE(void, InitializeWindow);
    FUNCTION().PROTECTED().SIGNATURE(void, InitializeDevicesMenu);
    FUNCTION().PROTECTED().SIGNATURE(void, SetResolution, const Vec2I&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCurrentWindowSize, bool);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCustomResolution, bool);
    FUNCTION().PROTECTED().SIGNATURE(void, OnDeviceSelected, const String&, const Ref<ContextMenu::Item>&);
    FUNCTION().PROTECTED().SIGNATURE(void, SetDeviceMenuCheckedItem, const Ref<ContextMenu::Item>&);
}
END_META;

CLASS_BASES_META(Editor::GameWindow::GameView)
{
    BASE_CLASS(o2::Widget);
}
END_META;
CLASS_FIELDS_META(Editor::GameWindow::GameView)
{
    FIELD().PUBLIC().DEFAULT_VALUE(true).NAME(fixedResolution);
    FIELD().PUBLIC().NAME(resolution);
    FIELD().PROTECTED().NAME(mRenderTargetSprite);
    FIELD().PROTECTED().NAME(mRenderTarget);
}
END_META;
CLASS_METHODS_META(Editor::GameWindow::GameView)
{

    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*);
    FUNCTION().PUBLIC().SIGNATURE(void, Draw);
    FUNCTION().PUBLIC().SIGNATURE(void, UpdateRenderTargetSize);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(String, GetCreateMenuCategory);
    FUNCTION().PROTECTED().SIGNATURE(void, OnTransformUpdated);
}
END_META;

CLASS_BASES_META(Editor::GameWindow::SimulationDevice)
{
    BASE_CLASS(o2::ISerializable);
}
END_META;
CLASS_FIELDS_META(Editor::GameWindow::SimulationDevice)
{
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().NAME(deviceName);
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().NAME(resolution);
}
END_META;
CLASS_METHODS_META(Editor::GameWindow::SimulationDevice)
{
}
END_META;
// --- END META ---
