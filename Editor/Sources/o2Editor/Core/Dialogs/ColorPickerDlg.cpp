#include "o2Editor/stdafx.h"
#include "ColorPickerDlg.h"

#include "o2/Scene/UI/UIManager.h"
#include "o2/Scene/UI/WidgetLayer.h"
#include "o2/Scene/UI/WidgetLayout.h"
#include "o2/Scene/UI/Widgets/DropDown.h"
#include "o2/Scene/UI/Widgets/EditBox.h"
#include "o2/Scene/UI/Widgets/HorizontalLayout.h"
#include "o2/Scene/UI/Widgets/HorizontalProgress.h"
#include "o2/Scene/UI/Widgets/Image.h"
#include "o2/Scene/UI/Widgets/Label.h"
#include "o2/Scene/UI/Widgets/VerticalLayout.h"
#include "o2/Scene/UI/Widgets/VerticalProgress.h"
#include "o2/Scene/UI/Widgets/Window.h"
#include "o2Editor/Core/UIRoot.h"
#include "o2Editor/Utils/CommonTextures.h"

DECLARE_SINGLETON(Editor::ColorPickerDlg);

namespace Editor
{
    ColorPickerDlg::ColorPickerDlg(RefCounter* refCounter) :
        CursorEventsListener(refCounter), Singleton<ColorPickerDlg>(refCounter)
    {
        mWindow = DynamicCast<o2::Window>(EditorUIRoot.AddWidget(o2UI.CreateWindow("Color picker")));

        InitializeControls();

        mWindow->Hide(true);
        mWindow->layout->size = Vec2F(400, 600);
        mWindow->layout->worldPosition = Vec2F();

        mWindow->GetBackCursorListener().onCursorReleased = [&](const Input::Cursor& c) { OnCursorPressedOutside(); };
        mWindow->onHide = MakeFunction(this, &ColorPickerDlg::OnHide);
    }

    ColorPickerDlg::~ColorPickerDlg()
    {}

    void ColorPickerDlg::Show(const Color4& color, const Function<void(const Color4&)>& onChanged,
                              const Function<void()>& onCompleted/* = Function<void()>()*/)
    {
        mInstance->mColorValue = color;

        mInstance->mWindow->ShowModal();

        mInstance->mOnChangedCallback = onChanged;
        mInstance->mOnCompletedCallback = onCompleted;

        mInstance->UpdateValues(ParameterType::General);
    }

    void ColorPickerDlg::OnHide()
    {
        mOnCompletedCallback();
    }

    void ColorPickerDlg::InitializeControls()
    {
        InitializeChessBackTexture();
        InitializeColorPreview();
        InitializePickArea();
        InitializeColorParams();
    }

    void ColorPickerDlg::InitializeChessBackTexture()
    {
        Color4 color1(1.0f, 1.0f, 1.0f, 1.0f), color2(0.7f, 0.7f, 0.7f, 1.0f);
        Bitmap backLayerBitmap(PixelFormat::R8G8B8A8, Vec2I(20, 20));
        backLayerBitmap.Fill(color1);
        backLayerBitmap.FillRect(0, 10, 10, 0, color2);
        backLayerBitmap.FillRect(10, 20, 20, 10, color2);

        mChessBackTexture = TextureRef(backLayerBitmap);
    }

    void ColorPickerDlg::InitializeColorPreview()
    {
        auto colorPreviewContainer = mmake<Widget>();
        *colorPreviewContainer->layout = WidgetLayout::HorStretch(VerAlign::Top, 0, 0, 30, 0);

        auto colorPreview = o2UI.CreateWidget<Widget>("colorProperty");
        *colorPreview->layout = WidgetLayout::BothStretch(5, 5, 5, 5);

        auto backImage = mmake<Image>();
        backImage->image = mmake<Sprite>(CommonTextures::checkedBackground, RectI(0, 0, 20, 20));
        backImage->GetImage()->mode = SpriteMode::Tiled;
        *backImage->layout = WidgetLayout::BothStretch(1, 1, 1, 1);
        colorPreview->AddChild(backImage);

        Bitmap colorLayerBitmap(PixelFormat::R8G8B8A8, Vec2I(20, 20));
        colorLayerBitmap.Fill(Color4::White());
        mColorSampleImage = mmake<Image>();
        mColorSampleImage->image = mmake<Sprite>(colorLayerBitmap);
        *mColorSampleImage->layout = WidgetLayout::BothStretch(1, 1, 1, 1);
        colorPreview->AddChild(mColorSampleImage);
        colorPreviewContainer->AddChild(colorPreview);

        mWindow->AddChild(colorPreviewContainer);
    }

    void ColorPickerDlg::InitializePickArea()
    {
        auto pickAreaContainer = mmake<Widget>();
        *pickAreaContainer->layout = WidgetLayout(Vec2F(0.0f, 0.5f), Vec2F(1.0f, 1.0f), Vec2F(0, 0), Vec2F(0, -30));

        auto pickArea = mmake<Widget>();
        *pickArea->layout = WidgetLayout::BothStretch(5, 5, 5, 5);

        pickArea->AddLayer("back", mmake<Sprite>("ui/UI4_Editbox_regular.png"),
                           Layout::BothStretch(-9, -9, -9, -9));

        mColorPickAreaBitmap = mmake<Bitmap>(PixelFormat::R8G8B8A8, Vec2I(80, 80));
        mColorPickAreaBitmap->Clear(Color4::White());
        mColorPickAreaTexture = TextureRef(*mColorPickAreaBitmap);
        mColorPickAreaColor = pickArea->AddLayer("color", mmake<Sprite>(mColorPickAreaTexture, RectI(0, 0, 80, 80)),
                                                 Layout::BothStretch(1, 1, 1, 1));

        mColorPickAreaHandle = o2UI.CreateImage("ui/circle_hole_handle.png");
        *mColorPickAreaHandle->layout = WidgetLayout::Based(BaseCorner::Center, Vec2F(15, 15));
        pickArea->AddChild(mColorPickAreaHandle);

        pickAreaContainer->AddChild(pickArea);

        mWindow->AddChild(pickAreaContainer);

        mColorPickHandle = mmake<CursorEventsArea>();
        mColorPickAreaHandle->onDraw += [&]() { mColorPickHandle->OnDrawn(); };
        mColorPickHandle->isUnderPoint = [&](const Vec2F& point) { return mColorPickAreaColor->IsUnderPoint(point); };
        mColorPickHandle->onMoved = MakeFunction(this, &ColorPickerDlg::OnColorPickHandleMoved);
    }

    void ColorPickerDlg::InitializeColorParams()
    {
        auto colorParamsArea = mmake<VerticalLayout>();
        *colorParamsArea->layout = WidgetLayout(Vec2F(0.0f, 0.0f), Vec2F(1.0f, 0.5f), Vec2F(0, 0), Vec2F(0, 0));
        colorParamsArea->border = BorderF(5, 5, 5, 5);

        // HSL/RGB sliders
        auto hParamWidget = mColorHProperty.Initialize("Hue", ParameterType::H, Ref(this),
                                                       [](float value, Color4& color) { float h, s, l; color.ToHSL(h, s, l); h = value; color.SetHSL(h, s, l); },
                                                       [](const Color4& color) { float h, s, l; color.ToHSL(h, s, l); return h; },
                                                       [](int x) { Color4 color; float h = x/255.0f, s = 1.0f, l = 0.5f; color.SetHSL(h, s, l); return color; });

        auto sParamWidget = mColorSProperty.Initialize("Saturation", ParameterType::S, Ref(this),
                                                       [](float value, Color4& color) { float h, s, l; color.ToHSL(h, s, l); s = value; color.SetHSL(h, s, l); },
                                                       [](const Color4& color) { float h, s, l; color.ToHSL(h, s, l); return s; },
                                                       [this](int x) { Color4 color = mColorValue; float h, s, l; color.ToHSL(h, s, l); s = x/255.0f; color.SetHSL(h, s, l); return color; });

        auto lParamWidget = mColorLProperty.Initialize("Lightness", ParameterType::L, Ref(this),
                                                       [](float value, Color4& color) { float h, s, l; color.ToHSL(h, s, l); l = value; color.SetHSL(h, s, l); },
                                                       [](const Color4& color) { float h, s, l; color.ToHSL(h, s, l); return l; },
                                                       [this](int x) { Color4 color = mColorValue; float h, s, l; color.ToHSL(h, s, l); l = x/255.0f; color.SetHSL(h, s, l); return color; });

        auto rParamWidget = mColorRProperty.Initialize("Red", ParameterType::R, Ref(this),
                                                       [](float value, Color4& color) { color.r = (int)(value*255.0f); },
                                                       [](const Color4& color) { return color.r/255.0f; },
                                                       [this](int x) { return Color4(x, mColorValue.g, mColorValue.b, mColorValue.a); });

        auto gParamWidget = mColorGProperty.Initialize("Green", ParameterType::G, Ref(this),
                                                       [](float value, Color4& color) { color.g = (int)(value*255.0f); },
                                                       [](const Color4& color) { return color.g/255.0f; },
                                                       [this](int x) { return Color4(mColorValue.r, x, mColorValue.b, mColorValue.a); });

        auto bParamWidget = mColorBProperty.Initialize("Blue", ParameterType::B, Ref(this),
                                                       [](float value, Color4& color) { color.b = (int)(value*255.0f); },
                                                       [](const Color4& color) { return color.b/255.0f; },
                                                       [this](int x) { return Color4(mColorValue.r, mColorValue.g, x, mColorValue.a); });

        auto aParamWidget = mColorAProperty.Initialize("Alpha", ParameterType::A, Ref(this),
                                                       [](float value, Color4& color) { color.a = (int)(value*255.0f); },
                                                       [](const Color4& color) { return color.a/255.0f; },
                                                       [this](int x) { return Color4(mColorValue.r, mColorValue.g, mColorValue.b, x); });

        // RGBA
        auto rgbaContainer = mmake<Widget>();

        auto rgbaLabel = o2UI.CreateLabel("RGBA");
        *rgbaLabel->layout = WidgetLayout::Based(BaseCorner::Left, Vec2F(100, 20));
        rgbaLabel->horAlign = HorAlign::Left;
        rgbaContainer->AddChild(rgbaLabel);

        mRGBAEditBox = o2UI.CreateEditBox("singleline");
        *mRGBAEditBox->layout = WidgetLayout::HorStretch(VerAlign::Middle, 100, 0, 20);
        mRGBAEditBox->onChangeCompleted = [&](const WString& text) { OnColorEditBoxRGBAChanged((String)text); };
        rgbaContainer->AddChild(mRGBAEditBox);

        // HEX
        auto hexContainer = mmake<Widget>();

        auto hexLabel = o2UI.CreateLabel("HEX");
        *hexLabel->layout = WidgetLayout::Based(BaseCorner::Left, Vec2F(100, 20));
        hexLabel->horAlign = HorAlign::Left;
        hexContainer->AddChild(hexLabel);

        mHEXEditBox = o2UI.CreateEditBox("singleline");
        *mHEXEditBox->layout = WidgetLayout::HorStretch(VerAlign::Middle, 100, 0, 20);
        mHEXEditBox->onChangeCompleted = [&](const WString& text) { OnColorEditBoxHEXChanged((String)text); };
        hexContainer->AddChild(mHEXEditBox);

        colorParamsArea->AddChild(hParamWidget);
        colorParamsArea->AddChild(sParamWidget);
        colorParamsArea->AddChild(lParamWidget);
        colorParamsArea->AddChild(rParamWidget);
        colorParamsArea->AddChild(gParamWidget);
        colorParamsArea->AddChild(bParamWidget);
        colorParamsArea->AddChild(aParamWidget);
        colorParamsArea->AddChild(rgbaContainer);
        colorParamsArea->AddChild(hexContainer);

        mWindow->AddChild(colorParamsArea);
    }

    void ColorPickerDlg::OnColorPickHandleMoved(const Input::Cursor& cursor)
    {
        auto rt = mColorPickAreaColor->GetRect();

        Vec2F anchorPos = (cursor.position - rt.LeftBottom())/rt.Size();
        anchorPos.x = Math::Clamp01(anchorPos.x);
        anchorPos.y = Math::Clamp01(anchorPos.y);

        mColorPickAreaHandle->layout->anchorMin = anchorPos;
        mColorPickAreaHandle->layout->anchorMax = anchorPos;

        mColorValue.SetHSL(mColorHProperty.progressBar->value, anchorPos.y, anchorPos.x);
        UpdateValues(ParameterType::Picker);
    }

    void ColorPickerDlg::UpdateColorPickHandle()
    {
        auto rt = mColorPickAreaColor->GetRect();
        float h, s, l; mColorValue.ToHSL(h, s, l);
        Vec2F anchorPos = Vec2F(l, s);
        anchorPos.x = Math::Clamp01(anchorPos.x);
        anchorPos.y = Math::Clamp01(anchorPos.y);

        mColorPickAreaHandle->layout->anchorMin = anchorPos;
        mColorPickAreaHandle->layout->anchorMax = anchorPos;
    }

    void ColorPickerDlg::UpdateColorEditBoxeRGBA()
    {
        String rgbaText = String::Format("%d, %d, %d, %d", mColorValue.r, mColorValue.g, mColorValue.b, mColorValue.a);
        mRGBAEditBox->text = (WString)rgbaText;
    }

    void ColorPickerDlg::UpdateColorEditBoxHEX()
    {
        String hexText = String::Format("#%02X%02X%02X%02X", mColorValue.r, mColorValue.g, mColorValue.b, mColorValue.a);
        mHEXEditBox->text = (WString)hexText;
    }

    void ColorPickerDlg::OnColorEditBoxRGBAChanged(const String& text)
    {
        Vector<String> splitText;

        // Split string by any non-digit character
        String buffer;
        for (int i = 0; i < text.Length(); i++)
        {
            if (text[i] >= '0' && text[i] <= '9')
                buffer += text[i];
            else
            {
                if (buffer.Length() > 0)
                    splitText.Add(buffer);

                buffer.Clear();
            }
        }

        // Get available values
        float r = splitText.Count() > 0 ? (float)splitText[0]/255.0f : 1.0f;
        float g = splitText.Count() > 1 ? (float)splitText[1]/255.0f : 1.0f;
        float b = splitText.Count() > 2 ? (float)splitText[2]/255.0f : 1.0f;
        float a = splitText.Count() > 3 ? (float)splitText[3]/255.0f : 1.0f;

        // Update color value
        mColorValue = Color4(r, g, b, a);
        UpdateValues(ParameterType::RGBABox);
    }

    void ColorPickerDlg::OnColorEditBoxHEXChanged(const String& text)
    {
        // Extract hex string
        String hexText = text.Length() > 0 && text[0] == '#' ? text.SubStr(1) : text;
        UInt32 hexValue = 0;

        // Convert hex string to integer
        for (int i = 0; i < hexText.Length(); i++)
        {
            hexValue <<= 4;
            if (hexText[i] >= '0' && hexText[i] <= '9')
                hexValue += hexText[i] - '0';
            else if (hexText[i] >= 'A' && hexText[i] <= 'F')
                hexValue += hexText[i] - 'A' + 10;
            else if (hexText[i] >= 'a' && hexText[i] <= 'f')
                hexValue += hexText[i] - 'a' + 10;
        }

        // Update color value
        mColorValue.r = (hexValue >> 24) & 0xFF;
        mColorValue.g = (hexValue >> 16) & 0xFF;
        mColorValue.b = (hexValue >> 8) & 0xFF;
        mColorValue.a = hexValue & 0xFF;

        UpdateValues(ParameterType::HEXBox);
    }

    void ColorPickerDlg::OnColorChanged(ParameterType changedParameter)
    {
        mOnChangedCallback(mColorValue);
        UpdateValues(changedParameter);
    }

    void ColorPickerDlg::UpdateValues(ParameterType changedParameter)
    {
        mColorSampleImage->GetImage()->SetColor(mColorValue);

        if (changedParameter != ParameterType::S && changedParameter != ParameterType::L && changedParameter != ParameterType::Picker)
            mColorHProperty.UpdateValue(mColorValue);

        if (changedParameter != ParameterType::H && changedParameter != ParameterType::L)
            mColorSProperty.UpdateValue(mColorValue);

        if (changedParameter != ParameterType::H && changedParameter != ParameterType::S)
            mColorLProperty.UpdateValue(mColorValue);

        mColorRProperty.UpdateValue(mColorValue);
        mColorGProperty.UpdateValue(mColorValue);
        mColorBProperty.UpdateValue(mColorValue);
        mColorAProperty.UpdateValue(mColorValue);

        if (changedParameter == ParameterType::H || changedParameter == ParameterType::S || changedParameter == ParameterType::L)
            mColorValue.SetHSL(mColorHProperty.progressBar->value, mColorSProperty.progressBar->value, mColorLProperty.progressBar->value);

        UpdateColorPickBitmap();

        if (changedParameter != ParameterType::Picker)
            UpdateColorPickHandle();

        if (changedParameter != ParameterType::RGBABox)
            UpdateColorEditBoxeRGBA();

        if (changedParameter != ParameterType::HEXBox)
            UpdateColorEditBoxHEX();
    }

    void ColorPickerDlg::UpdateColorPickBitmap()
    {
        float h = mColorHProperty.progressBar->value;
        Vec2I sz = mColorPickAreaBitmap->GetSize();

        for (int i = 0; i < sz.x; i++)
        {
            for (int j = 0; j < sz.y; j++)
            {
                Color4 c; c.SetHSL(h, (float)i/(float)sz.x, (float)j/(float)sz.y);
                UInt cc = c.ABGR();
                memcpy(mColorPickAreaBitmap->GetData() + 4*i*sz.x + 4*j, &cc, 4);
            }
        }

        mColorPickAreaTexture->SetData(*mColorPickAreaBitmap);
    }

    void ColorPickerDlg::OnCursorPressedOutside()
    {
        mOnChangedCallback(mColorValue);
        mWindow->Hide();
    }

    Ref<Widget> ColorPickerDlg::ColorProperty::Initialize(const String& name, ParameterType type, const Ref<ColorPickerDlg>& dialog,
                                                          const Function<void(float value, Color4& color)>& setValue,
                                                          const Function<float(const Color4& value)>& getValue,
                                                          const Function<Color4(int x)>& updateBitmap)
    {
        this->dialog = dialog;
        this->type = type;
        this->setValue = setValue;
        this->getValue = getValue;
        this->updateBitmap = updateBitmap;

        auto resLayout = mmake<Widget>();
        nameLabel = o2UI.CreateLabel(name);
        *nameLabel->layout = WidgetLayout::Based(BaseCorner::Left, Vec2F(100, 20));
        nameLabel->horAlign = HorAlign::Left;
        resLayout->AddChild(nameLabel);

        progressBar = o2UI.CreateWidget<HorizontalProgress>("wide");
        *progressBar->layout = WidgetLayout::HorStretch(VerAlign::Middle, 100, 50, 20);
        progressBar->onChangeByUser = [&](float v) { OnProgressBarChanged(v); };

        bitmap = mmake<Bitmap>(PixelFormat::R8G8B8A8, Vec2F(256, 256));
        progressBarTexture = TextureRef(*bitmap);
        progressBar->AddLayer("color", mmake<Sprite>(progressBarTexture, RectI(0, 0, 256, 256)), Layout::BothStretch(1, 1, 1, 1), 0.5f);

        auto backSprite = mmake<Sprite>(dialog->mChessBackTexture, RectI(0, 0, 20, 20));
        backSprite->mode = SpriteMode::Tiled;
        progressBar->AddLayer("colorBack", backSprite, Layout::BothStretch(1, 1, 1, 1), 0.4f);

        resLayout->AddChild(progressBar);

        editBox = o2UI.CreateEditBox("singleline");
        *editBox->layout = WidgetLayout::Based(BaseCorner::Right, Vec2F(45, 20));
        editBox->SetFilterInteger();

        editBox->onChangeCompleted = [&](const WString& text) { progressBar->SetValue(((float)text/255.0f)); };

        resLayout->AddChild(editBox);

        return resLayout;
    }

    void ColorPickerDlg::ColorProperty::UpdateValue(const Color4& value)
    {
        if (mChanging)
            return;

        progressBar->SetValue(getValue(value));
        editBox->text = (WString)(int)(getValue(value)*255.0f);

        UpdateBitmap();
    }

    void ColorPickerDlg::ColorProperty::OnProgressBarChanged(float value)
    {
        mChanging = true;

        if (auto dialog = this->dialog.Lock()) {
            setValue(value, dialog->mColorValue);
            editBox->text = (WString)(int)(value*255.0f);
            dialog->OnColorChanged(type);
        }

        mChanging = false;
    }

    void ColorPickerDlg::ColorProperty::UpdateBitmap()
    {
        int bpp = 4;
        UInt8* c1 = bitmap->GetData();
        int h = bitmap->GetSize().y;
        int w = bitmap->GetSize().x;

        for (int i = 0; i < 256; i++)
            *(ULong*)(c1 + bpp*i) = updateBitmap(i).ABGR();

        for (int i = 1; i < h; i++)
            memcpy(c1 + bpp*w*i, c1, bpp*w);

        progressBarTexture->SetData(*bitmap);
    }

}
// --- META ---

ENUM_META(Editor::ColorPickerDlg::ParameterType)
{
    ENUM_ENTRY(A);
    ENUM_ENTRY(B);
    ENUM_ENTRY(G);
    ENUM_ENTRY(General);
    ENUM_ENTRY(H);
    ENUM_ENTRY(HEXBox);
    ENUM_ENTRY(L);
    ENUM_ENTRY(Picker);
    ENUM_ENTRY(R);
    ENUM_ENTRY(RGBABox);
    ENUM_ENTRY(S);
}
END_ENUM_META;
// --- END META ---
