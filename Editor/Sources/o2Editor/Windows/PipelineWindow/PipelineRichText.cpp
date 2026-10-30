#include "o2Editor/stdafx.h"
#include "PipelineRichText.h"

#include "o2/Scene/UI/WidgetLayout.h"
#include "o2Editor/Windows/PipelineWindow/PipelineControls.h"
#include "o2Editor/Windows/PipelineWindow/PipelineTheme.h"

namespace Editor
{
    PipelineRichText::PipelineRichText(RefCounter* refCounter):
        Widget(refCounter)
    {}

    void PipelineRichText::SetMarkdown(const String& text)
    {
        Parse(text);
        mLaidOutWidth = -1.0f;
        UpdateSelfTransform();
    }

    void PipelineRichText::SetStyle(float size, float lineHeight, const Color4& color)
    {
        mSize = size;
        mLineHeight = lineHeight;
        mColor = color;
        mLaidOutWidth = -1.0f;
    }

    void PipelineRichText::Parse(const String& text)
    {
        mParagraphs.Clear();

        // Paragraphs are split by blank lines; a single line break stays a space, as in rendered Markdown
        Vector<String> paragraphs;
        String current;
        int i = 0;
        while (i <= text.Length())
        {
            bool end = i == text.Length();
            if (end || (text[i] == '\n' && i + 1 < text.Length() && text[i + 1] == '\n'))
            {
                if (!current.Trimed(" \n\t\r").IsEmpty())
                    paragraphs.Add(current);
                current.Clear();
                while (i < text.Length() && text[i] == '\n')
                    i++;
                if (end)
                    break;
                continue;
            }
            current += text[i] == '\n' ? ' ' : text[i];
            i++;
        }

        for (auto& paragraph : paragraphs)
        {
            String p = paragraph.Trimed(" \t\r");
            // A heading keeps its words, not its marks
            while (p.StartsWith("#"))
                p = p.SubStr(1).Trimed(" ");

            // Each pair of ** or __ marks turns the strong face on and off
            bool strong = false;
            Vector<Run> fixedRuns;
            Run part;
            for (int c = 0; c < p.Length(); c++)
            {
                if ((p[c] == '*' || p[c] == '_') && c + 1 < p.Length() && p[c + 1] == p[c])
                {
                    if (!part.text.IsEmpty())
                        fixedRuns.Add(part);
                    strong = !strong;
                    part = Run();
                    part.strong = strong;
                    c++;
                    continue;
                }
                part.strong = strong;
                part.text += p[c];
            }
            if (!part.text.IsEmpty())
                fixedRuns.Add(part);
            mParagraphs.Add(fixedRuns);
        }
    }

    void PipelineRichText::Layout(float width)
    {
        mPieces.Clear();
        mLaidOutWidth = width;

        auto regular = PipelineControls::MakeThemeText("", mSize, 400, mColor);
        auto strong = PipelineControls::MakeThemeText("", mSize, 600, mColor);
        auto measure = [&](const String& s, bool isStrong)
        {
            auto& probe = isStrong ? strong : regular;
            auto font = probe->GetFont();
            return font ? Text::GetTextSize(s, font, probe->height, Vec2F(), HorAlign::Left, VerAlign::Top, false).x : s.Length() * mSize * 0.5f;
        };

        float y = 0.0f;
        const float paragraphGap = mSize * 0.5f;
        for (int pi = 0; pi < mParagraphs.Count(); pi++)
        {
            if (pi > 0)
                y += paragraphGap;

            float x = 0.0f;
            int lineStart = mPieces.Count();
            auto place = [&](const String& word, bool isStrong, float w)
            {
                // Words of one style on one line join into a single piece
                if (mPieces.Count() > lineStart)
                {
                    auto& last = mPieces.Last();
                    if (last.strong == isStrong && Math::Equals(last.offset.y, y))
                    {
                        last.drawable->text = last.drawable->GetText() + (WString)word;
                        last.width += w;
                        x += w;
                        return;
                    }
                }

                Piece piece;
                piece.drawable = PipelineControls::MakeThemeText(word, mSize, isStrong ? 600 : 400, mColor);
                piece.drawable->dotsEngings = false;
                piece.drawable->horAlign = HorAlign::Left;
                piece.drawable->verAlign = VerAlign::Middle;
                piece.offset = Vec2F(x, y);
                piece.width = w;
                piece.strong = isStrong;
                mPieces.Add(piece);
                x += w;
            };

            for (auto& run : mParagraphs[pi])
            {
                // Words with their trailing spaces
                int c = 0;
                while (c < run.text.Length())
                {
                    int start = c;
                    while (c < run.text.Length() && run.text[c] != ' ')
                        c++;
                    while (c < run.text.Length() && run.text[c] == ' ')
                        c++;
                    String word = run.text.SubStr(start, c);
                    float w = measure(word, run.strong);
                    float visible = measure(word.TrimedEnd(" "), run.strong);
                    if (x > 0.0f && x + visible > width + 0.5f)
                    {
                        y += mLineHeight;
                        x = 0.0f;
                        lineStart = mPieces.Count();
                        if (word.Trimed(" ").IsEmpty())
                            continue;
                    }
                    place(word, run.strong, w);
                }
            }
            y += mLineHeight;
        }
        mContentHeight = y;
    }

    float PipelineRichText::GetContentHeight(float width)
    {
        if (!Math::Equals(width, mLaidOutWidth))
            Layout(width);
        return mContentHeight;
    }

    void PipelineRichText::UpdateSelfTransform()
    {
        Widget::UpdateSelfTransform();
        float width = layout->GetWidth();
        if (width > 0.0f && !Math::Equals(width, mLaidOutWidth))
            Layout(width);

        // The scroll area reads the content size from the layout: it is as tall as the laid out lines
        if (width > 0.0f && !Math::Equals(layout->GetHeight(), mContentHeight))
        {
            *layout = WidgetLayout::HorStretch(VerAlign::Top, 0, 0, mContentHeight, 0);
            Widget::UpdateSelfTransform();
        }
    }

    void PipelineRichText::Draw()
    {
        if (!mResEnabledInHierarchy || mIsClipped)
            return;

        // A text mesh is rebuilt on every change of its alignment or transparency and moved on every change of its rect: only
        // what changed is set, as all the texts of a graph are drawn zoomed far out
        RectF rect = layout->GetWorldRect();
        float transparency = GetResTransparency();
        for (auto& piece : mPieces)
        {
            float top = rect.top - piece.offset.y;
            RectF pieceRect(rect.left + piece.offset.x, top, rect.left + piece.offset.x + piece.width + 2.0f, top - mLineHeight);
            if (piece.placed != pieceRect)
            {
                piece.drawable->SetRect(pieceRect);
                piece.placed = pieceRect;
            }
            if (!Math::Equals(piece.drawable->GetTransparency(), transparency))
                piece.drawable->transparency = transparency;
            piece.drawable->Draw();
        }
        OnDrawn();
    }
}
// --- META ---

DECLARE_CLASS(Editor::PipelineRichText, Editor__PipelineRichText);
// --- END META ---
