#pragma once

#include "o2/Scene/UI/Widget.h"
#include "o2/Utils/Math/Color.h"
#include "o2/Utils/Math/Rect.h"

using namespace o2;

namespace o2
{
    class Button;
    class EditBox;
    class Label;
    class Toggle;
}

namespace Editor
{
    // -----------------------------------------------------------------------------------------------------------------
    // Controls of the composer card, AssetsLine's compact composer look (ComposerBody.tsx, styles.css "composer card"):
    // 3 px corners, 20 px boxes, 9.5-11 px text; every colour from the theme palette
    // -----------------------------------------------------------------------------------------------------------------
    namespace PipelineComposerTheme
    {
        constexpr float boxRadius = 3.0f;  // Fields, buttons and rows of the composer
        constexpr float boxHeight = 20.0f; // Number fields and toolbar buttons

        // Returns the width of a text in the theme font or the mono font
        float TextWidth(const String& text, float size, bool mono = false, int weight = 400);

        // Creates a caption of the toolbar groups ("Canvas", "Zoom"): 9.5 px in the quietest text, as wide as the text and 1
        Ref<Label> MakeCaption(const String& text);

        // Creates a plain label of the size and colour, as wide as its text
        Ref<Label> MakeText(const String& text, float size, const Color4& color);

        // Restyles an edit box as a composer field: field fill, border token, 3 px corners, the font at the size
        void SkinField(const Ref<EditBox>& edit, float size, bool mono);

        // Creates a number field: mono 11 px, the width given, 20 tall
        Ref<EditBox> MakeNumberField(const String& text, float width);

        // Creates a toolbar button (.cmp-zoom button, .cmp-dl button): bg-3 fill, border token, the dim text, an optional icon
        // before the caption; the width follows the content when width is 0
        Ref<Button> MakeToolButton(const String& caption, const String& icon, float fontSize, float width = 0.0f);

        // Creates an icon action of a layer row (.cmp-row-actions button): 18x20, the dim icon, a bg-2 wash on hover
        Ref<Button> MakeRowAction(const String& icon, float iconSize, float width = 18.0f, float height = 20.0f);

        // Creates a checkbox with its caption (.cmp-bgs): a 15 px box, the accent with a check while on, the caption 4 px
        // after it in 10.5 px dim text; width gets the width the two take
        Ref<Toggle> MakeCheck(const String& caption, bool value, float& width);

        // Creates an icon toggle (flip, aspect lock): bg-3 fill and border, the accent icon and border while on
        Ref<Toggle> MakeIconToggle(const String& icon, bool value, float width = 22.0f);

        // Draws a dashed rectangle outline one pixel wide inside the rectangle
        void DashedFrame(const RectF& rect, const Color4& color, float dash = 3.0f, float gap = 2.0f);

        // Draws squares of the second colour over the first in a checker of the cell size from the top-left
        void Checker(const RectF& rect, float cell, const Color4& first, const Color4& second);
    }
}
