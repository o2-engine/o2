#include "o2Editor/stdafx.h"
#include "ColorProperty.h"

#include "o2/Application/Application.h"
#include "o2/Scene/UI/WidgetLayout.h"
#include "o2/Scene/UI/Widgets/Image.h"
#include "o2Editor/Dialogs/ColorPickerDlg.h"
#include "o2Editor/Windows/SceneWindow/SceneEditScreen.h"

namespace Editor
{
    ColorProperty::ColorProperty(RefCounter* refCounter):
        TPropertyField<Color4>(refCounter)
    {}

    ColorProperty::ColorProperty(RefCounter* refCounter, const ColorProperty& other):
        TPropertyField<Color4>(refCounter, other)
    {
        InitializeControls();
    }

    ColorProperty& ColorProperty::operator=(const ColorProperty& other)
    {
        TPropertyField<Color4>::operator=(other);
        InitializeControls();
        return *this;
    }

    void ColorProperty::InitializeControls()
    {
        mEditBox = GetChildWidget("container/layout/box");
        if (mEditBox)
        {
            mEditBox->layout->minHeight = 10;

            Color4 color1(1.0f, 1.0f, 1.0f, 1.0f), color2(0.7f, 0.7f, 0.7f, 1.0f);
            Bitmap backLayerBitmap(PixelFormat::R8G8B8A8, Vec2I(20, 20));
            backLayerBitmap.Fill(color1);
            backLayerBitmap.FillRect(0, 10, 10, 0, color2);
            backLayerBitmap.FillRect(10, 20, 20, 10, color2);

            auto backImage = mmake<Image>();
            backImage->image = mmake<Sprite>(backLayerBitmap);
            backImage->GetImage()->mode = SpriteMode::Tiled;
            *backImage->layout = WidgetLayout::BothStretch(1, 1, 1, 1);
            mEditBox->AddChild(backImage);

            Bitmap colorLayerBitmap(PixelFormat::R8G8B8A8, Vec2I(20, 20));
            colorLayerBitmap.Fill(color1);
            mColorSprite = mmake<Image>();
            mColorSprite->image = mmake<Sprite>(colorLayerBitmap);
            *mColorSprite->layout = WidgetLayout::BothStretch(1, 1, 1, 1);
            mEditBox->AddChild(mColorSprite);

            mClickArea = mmake<CursorEventsArea>();
            mEditBox->onDraw += [&]() { mClickArea->OnDrawn(); };
            mClickArea->isUnderPoint = [&](const Vec2F& point) { return mEditBox->IsUnderPoint(point); };
            mClickArea->onCursorReleased = [&](const Input::Cursor& cursor) { if (mEditBox->IsUnderPoint(cursor.position)) OnClicked(); };
        }
    }

    void ColorProperty::UpdateValueView()
    {
        mColorSprite->GetImage()->SetColor(mCommonValue);
        mColorSprite->transparency = mCommonValue.AF();
    }

    void ColorProperty::OnClicked()
    {
        StoreValues(mBeforeChangeValues);

        ColorPickerDlg::Show(mCommonValue,
                             MakeFunction<TPropertyField<Color4>, void, const Color4&>(this, &ColorProperty::SetValue),
                             MakeFunction<IPropertyField, void>(this, &ColorProperty::CheckValueChangeCompleted));
    }
}
DECLARE_TEMPLATE_CLASS(Editor::TPropertyField<o2::Color4>);
DECLARE_TEMPLATE_CLASS(o2::LinkRef<Editor::ColorProperty>);
DECLARE_TEMPLATE_CLASS(o2::LinkRef<Editor::TPropertyField<o2::Color4>>);
// --- META ---

DECLARE_CLASS(Editor::ColorProperty, Editor__ColorProperty);
// --- END META ---
