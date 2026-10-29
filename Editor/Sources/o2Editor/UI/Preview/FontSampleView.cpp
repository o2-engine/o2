#include "o2Editor/stdafx.h"
#include "FontSampleView.h"

#include "o2/Render/Text.h"
#include "o2/Scene/UI/WidgetLayer.h"
#include "o2Editor/UI/Style/EditorTheme.h"

namespace Editor
{
    namespace
    {
        const float lineSpacing = 1.4f;
    }

    FontSampleView::FontSampleView(RefCounter* refCounter):
        Widget(refCounter), mSizes({ 10, 12, 14, 18, 24, 32, 48 }),
        mSample("The quick brown fox jumps over the lazy dog 0123456789. Съешь же ещё этих мягких французских булок")
    {}

    FontSampleView::FontSampleView(RefCounter* refCounter, const FontSampleView& other):
        Widget(refCounter), mSizes(other.mSizes), mSample(other.mSample)
    {
        SetFont(other.mFont);
    }

    FontSampleView& FontSampleView::operator=(const FontSampleView& other)
    {
        mSizes = other.mSizes;
        mSample = other.mSample;
        SetFont(other.mFont);
        return *this;
    }

    void FontSampleView::SetFont(const Ref<Font>& font)
    {
        mFont = font;
        Rebuild();
    }

    const Ref<Font>& FontSampleView::GetFont() const
    {
        return mFont;
    }

    void FontSampleView::SetSizes(const Vector<int>& sizes)
    {
        mSizes = sizes;
        Rebuild();
    }

    const Vector<int>& FontSampleView::GetSizes() const
    {
        return mSizes;
    }

    void FontSampleView::SetSampleText(const WString& text)
    {
        mSample = text;
        Rebuild();
    }

    const WString& FontSampleView::GetSampleText() const
    {
        return mSample;
    }

    const Vector<Ref<Text>>& FontSampleView::GetLines() const
    {
        return mLines;
    }

    float FontSampleView::GetContentHeight() const
    {
        float height = 0.0f;
        for (int size : mSizes)
            height += (float)size*lineSpacing;

        return height;
    }

    void FontSampleView::Rebuild()
    {
        for (int i = 0; i < mLines.Count(); i++)
            RemoveLayer("line " + (String)i);

        mLines.Clear();

        if (!mFont)
            return;

        Color4 color = EditorTheme::Get().text;
        float top = 0.0f;
        for (int i = 0; i < mSizes.Count(); i++)
        {
            int size = mSizes[i];
            float height = (float)size*lineSpacing;

            auto text = mmake<Text>(mFont);
            text->SetHeight(size);
            text->SetText((WString)(String)size + WString("  ") + mSample);
            text->SetHorAlign(HorAlign::Left);
            text->SetVerAlign(VerAlign::Middle);
            text->SetWordWrap(false);
            text->SetDotsEngings(true);
            text->SetColor(color);

            AddLayer("line " + (String)i, text, Layout::HorStretch(VerAlign::Top, 4, 4, height, top));
            mLines.Add(text);
            top += height;
        }

        layout->minHeight = top;
    }
}
// --- META ---

DECLARE_CLASS(Editor::FontSampleView, Editor__FontSampleView);
// --- END META ---
