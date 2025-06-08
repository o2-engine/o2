#pragma once

#include "o2/Events/CursorEventsArea.h"
#include "o2/Events/CursorAreaEventsListener.h"
#include "o2/Render/TextureRef.h"
#include "o2/Utils/Function/Function.h"
#include "o2/Utils/Math/Color.h"
#include "o2/Utils/Singleton.h"

using namespace o2;

namespace o2
{
    class Bitmap;
    class DropDown;
    class EditBox;
    class HorizontalProgress;
    class Image;
    class Label;
    class VerticalProgress;
    class Widget;
    class WidgetLayer;
    class Window;
}

namespace Editor
{
    // --------------------
    // Color picking dialog
    // --------------------
    class ColorPickerDlg : public Singleton<ColorPickerDlg>, public CursorEventsListener
    {
    public:
        enum class ParameterType { H, S, L, R, G, B, A, General, Picker, RGBABox, HEXBox };

    public:
        // Default constructor
        ColorPickerDlg(RefCounter* refCounter);

        // Destructor
        ~ColorPickerDlg();

        // Shows color picking window. Delegate onChanged is called when color changed and onCompleted when ok pressed
        static void Show(const Color4& color, const Function<void(const Color4&)>& onChanged,
                         const Function<void()>& onCompleted = {});

        REF_COUNTERABLE_IMPL(Singleton<ColorPickerDlg>, CursorEventsListener);

    protected:
        struct ColorProperty
        {
            WeakRef<ColorPickerDlg> dialog;

            ParameterType type;

            Ref<Label>              nameLabel;
            Ref<HorizontalProgress> progressBar;
            Ref<EditBox>            editBox;
            Ref<Bitmap>             bitmap;
            TextureRef              progressBarTexture;

            Function<void(float value, Color4& color)> setValue;
            Function<float(const Color4& value)>       getValue;
            Function<Color4(int x)>                    updateBitmap;

        public:
            Ref<Widget> Initialize(const String& name, ParameterType type, const Ref<ColorPickerDlg>& dialog,
                                   const Function<void(float value, Color4& color)>& setValue,
                                   const Function<float(const Color4& value)>& getValue,
                                   const Function<Color4(int x)>& updateBitmap);

            void UpdateValue(const Color4& value);

        protected:
            bool mChanging = false;

        protected:
            void OnProgressBarChanged(float value);
            void UpdateBitmap();
        };

    protected:
        Function<void(const Color4&)> mOnChangedCallback;   // On changed callback
        Function<void()>              mOnCompletedCallback; // On completed callback

        Color4 mColorValue; // Current color value

        Ref<o2::Window> mWindow;
        Ref<Image>      mColorSampleImage;

        TextureRef mChessBackTexture;

        Ref<Image>            mColorPickAreaHandle;
        Ref<Bitmap>           mColorPickAreaBitmap;
        Ref<WidgetLayer>      mColorPickAreaColor;
        TextureRef            mColorPickAreaTexture;
        Ref<CursorEventsArea> mColorPickHandle;

        Ref<EditBox> mRGBAEditBox;
        Ref<EditBox> mHEXEditBox;

        ColorProperty mColorHProperty;
        ColorProperty mColorSProperty;
        ColorProperty mColorLProperty;
        ColorProperty mColorRProperty;
        ColorProperty mColorGProperty;
        ColorProperty mColorBProperty;
        ColorProperty mColorAProperty;

    protected:
        // Called when color picking dialog were hidden
        void OnHide();

        // Initializes all controls
        void InitializeControls();

        // Initializes chess back texture
        void InitializeChessBackTexture();

        // Initializes color preview
        void InitializeColorPreview();

        // Initializes color pick area
        void InitializePickArea();

        // Initializes HUE/RGB bars
        void InitializeColorParams();

        // Called when color pick handle moved, updates actual color value
        void OnColorPickHandleMoved(const Input::Cursor& cursor);

        // Called when color value changed, updates all controls
        void OnColorChanged(ParameterType changedParameter);

        // Called when color changed, updates color bars
        void UpdateValues(ParameterType changedParameter);

        // Updates color pick bitmap
        void UpdateColorPickBitmap();

        // Updates color pick handle position
        void UpdateColorPickHandle();

        // Updates color edit box RGBA
        void UpdateColorEditBoxeRGBA();

        // Updates color edit box HEX
        void UpdateColorEditBoxHEX();

        // Called when color edit box RGBA changed, updates color value
        void OnColorEditBoxRGBAChanged(const String& text);

        // Called when color edit box HEX changed, updates color value
        void OnColorEditBoxHEXChanged(const String& text);

        // Called when cursor pressed outside of color picking dialog, closes it
        void OnCursorPressedOutside();
    };
}
// --- META ---

PRE_ENUM_META(Editor::ColorPickerDlg::ParameterType);
// --- END META ---
