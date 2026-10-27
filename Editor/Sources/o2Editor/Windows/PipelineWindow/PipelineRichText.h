#pragma once

#include "o2/Render/Text.h"
#include "o2/Scene/UI/Widget.h"

using namespace o2;

namespace Editor
{
    // -----------------------------------------------------------------------------------------
    // Text result as the AssetsLine card reads it: Markdown paragraphs 0.5 em apart, **strong**
    // in the semibold face, words wrapped to the width at a fixed line height
    // -----------------------------------------------------------------------------------------
    class PipelineRichText : public Widget
    {
    public:
        // Default constructor
        explicit PipelineRichText(RefCounter* refCounter);

        // Sets the Markdown text
        void SetMarkdown(const String& text);

        // Sets the size in pixels, the line height and the colour
        void SetStyle(float size, float lineHeight, const Color4& color);

        // Returns the height the text takes at the width
        float GetContentHeight(float width);

        // Lays the words out for the current width and fits the widget's height to them
        void UpdateSelfTransform() override;

        // Draws the laid out runs
        void Draw() override;

        SERIALIZABLE(PipelineRichText);

    protected:
        struct Run
        {
            String text;
            bool   strong = false;

            bool operator==(const Run& other) const { return text == other.text && strong == other.strong; }
        };

        struct Piece
        {
            Ref<Text> drawable;
            Vec2F     offset; // From the widget's top-left, y down
            float     width = 0.0f;
            bool      strong = false;

            bool operator==(const Piece& other) const { return drawable == other.drawable; }
        };

        Vector<Vector<Run>> mParagraphs;       // Paragraphs of runs
        Vector<Piece>       mPieces;           // Laid out pieces for mLaidOutWidth
        float               mLaidOutWidth = -1.0f;
        float               mContentHeight = 0.0f;
        float               mSize = 11.5f;
        float               mLineHeight = 17.0f;
        Color4              mColor = Color4::White();

    protected:
        // Breaks the text into paragraphs and their runs
        void Parse(const String& text);

        // Lays the runs out into lines at the width
        void Layout(float width);
    };
}
// --- META ---

CLASS_BASES_META(Editor::PipelineRichText)
{
    BASE_CLASS(o2::Widget);
}
END_META;
CLASS_FIELDS_META(Editor::PipelineRichText)
{
    FIELD().PROTECTED().NAME(mParagraphs);
    FIELD().PROTECTED().NAME(mPieces);
    FIELD().PROTECTED().DEFAULT_VALUE(-1.0f).NAME(mLaidOutWidth);
    FIELD().PROTECTED().DEFAULT_VALUE(0.0f).NAME(mContentHeight);
    FIELD().PROTECTED().DEFAULT_VALUE(11.5f).NAME(mSize);
    FIELD().PROTECTED().DEFAULT_VALUE(17.0f).NAME(mLineHeight);
    FIELD().PROTECTED().DEFAULT_VALUE(Color4::White()).NAME(mColor);
}
END_META;
CLASS_METHODS_META(Editor::PipelineRichText)
{

    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*);
    FUNCTION().PUBLIC().SIGNATURE(void, SetMarkdown, const String&);
    FUNCTION().PUBLIC().SIGNATURE(void, SetStyle, float, float, const Color4&);
    FUNCTION().PUBLIC().SIGNATURE(float, GetContentHeight, float);
    FUNCTION().PUBLIC().SIGNATURE(void, UpdateSelfTransform);
    FUNCTION().PUBLIC().SIGNATURE(void, Draw);
    FUNCTION().PROTECTED().SIGNATURE(void, Parse, const String&);
    FUNCTION().PROTECTED().SIGNATURE(void, Layout, float);
}
END_META;
// --- END META ---
