#pragma once

#include "o2/Render/Font.h"
#include "o2/Scene/UI/Widget.h"

using namespace o2;

namespace o2
{
    class Text;
}

namespace Editor
{
    // ----------------------------------------------------------------------------------------------
    // Sample of a font: a line of text for each size, the size before it, from the top of the widget
    // ----------------------------------------------------------------------------------------------
    class FontSampleView: public Widget
    {
    public:
        // Default constructor
        FontSampleView(RefCounter* refCounter);

        // Copy-constructor
        FontSampleView(RefCounter* refCounter, const FontSampleView& other);

        // Copy-operator
        FontSampleView& operator=(const FontSampleView& other);

        // Sets the font of the lines
        void SetFont(const Ref<Font>& font);

        // Returns the font of the lines
        const Ref<Font>& GetFont() const;

        // Sets the sizes of the lines, from the top
        void SetSizes(const Vector<int>& sizes);

        // Returns the sizes of the lines
        const Vector<int>& GetSizes() const;

        // Sets the sample text of the lines
        void SetSampleText(const WString& text);

        // Returns the sample text
        const WString& GetSampleText() const;

        // Returns the texts of the lines
        const Vector<Ref<Text>>& GetLines() const;

        // Returns the height the lines take
        float GetContentHeight() const;

        SERIALIZABLE(FontSampleView);
        CLONEABLE_REF(FontSampleView);

    protected:
        Ref<Font>         mFont;   // Font of the lines
        Vector<int>       mSizes;  // Sizes of the lines
        WString           mSample; // Sample text
        Vector<Ref<Text>> mLines;  // Texts of the lines

    protected:
        // Builds the lines
        void Rebuild();
    };
}
// --- META ---

CLASS_BASES_META(Editor::FontSampleView)
{
    BASE_CLASS(o2::Widget);
}
END_META;
CLASS_FIELDS_META(Editor::FontSampleView)
{
    FIELD().PROTECTED().NAME(mFont);
    FIELD().PROTECTED().NAME(mSizes);
    FIELD().PROTECTED().NAME(mSample);
    FIELD().PROTECTED().NAME(mLines);
}
END_META;
CLASS_METHODS_META(Editor::FontSampleView)
{

    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*);
    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*, const FontSampleView&);
    FUNCTION().PUBLIC().SIGNATURE(void, SetFont, const Ref<Font>&);
    FUNCTION().PUBLIC().SIGNATURE(const Ref<Font>&, GetFont);
    FUNCTION().PUBLIC().SIGNATURE(void, SetSizes, const Vector<int>&);
    FUNCTION().PUBLIC().SIGNATURE(const Vector<int>&, GetSizes);
    FUNCTION().PUBLIC().SIGNATURE(void, SetSampleText, const WString&);
    FUNCTION().PUBLIC().SIGNATURE(const WString&, GetSampleText);
    FUNCTION().PUBLIC().SIGNATURE(const Vector<Ref<Text>>&, GetLines);
    FUNCTION().PUBLIC().SIGNATURE(float, GetContentHeight);
    FUNCTION().PROTECTED().SIGNATURE(void, Rebuild);
}
END_META;
// --- END META ---
