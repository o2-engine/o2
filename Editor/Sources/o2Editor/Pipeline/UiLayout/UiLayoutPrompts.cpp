#include "o2Editor/stdafx.h"
#include "UiLayoutPrompts.h"

#include "o2Editor/Pipeline/UiLayout/UiLayoutParse.h"

namespace Editor::UiLayout::Prompts
{
    static String Join(const Vector<String>& lines, const String& separator)
    {
        String out;
        for (int i = 0; i < lines.Count(); i++)
        {
            if (i > 0)
                out += separator;
            out += lines[i];
        }
        return out;
    }

    String Analyze(int w, int h, const String& hint)
    {
        Vector<String> lines = {
            "You are a senior game UI engineer. The image is a mockup of ONE game UI screen, " + (String)w + " x " + (String)h +
                " pixels. Rebuild it as a hierarchy of engine UI elements so it can be assembled from cut sprites, text labels and layout groups.",
            "",
            "Return ONLY JSON:",
            "{",
            " \"background\": {\"kind\": \"image\" | \"color\", \"color\": \"#RRGGBB\", \"describe\": \"<what is behind the UI>\"},",
            " \"looks\": {\"<look key>\": \"<short visual description of that artwork WITHOUT any text, letters or icons drawn on top of it>\"},",
            " \"elements\": [ <element>, ... ]",
            "}",
            "",
            "element = {",
            " \"id\": \"<unique snake_case id>\",",
            " \"parent\": \"<id of the enclosing element, or empty for top level>\",",
            " \"kind\": \"panel\" | \"image\" | \"button\" | \"input\" | \"text\" | \"progress\" | \"group\" | \"grid\",",
            " \"box_2d\": [ymin, xmin, ymax, xmax],   // integers 0..1000 of the image height and width, tight around the visible element including its outline and shadow",
            " \"look\": \"<look key>\",                  // panel / image / button / input / progress: the artwork the element is drawn with",
            " \"text\": \"<exact text>\", \"color\": \"#RRGGBB\", \"weight\": \"regular\" | \"medium\" | \"bold\", \"align\": \"left\" | \"center\" | \"right\",  // text only; add \"stroke\": \"#RRGGBB\" when the glyphs have an outline",
            " \"value\": 0.0-1.0,                      // progress only: the visible fill ratio",
            " \"rows\": n, \"cols\": n, \"cells\": [ {\"look\": \"<look key>\", \"texts\": [\"<main text>\", \"<small text>\"]} | null ]   // grid only: every cell row by row, null for an empty cell",
            "}",
            "",
            "Rules:",
            "- kinds: panel = a stretchable surface or frame other elements sit on (it will be nine-sliced); image = a fixed piece of artwork (icon, tile, decoration, character); button = a pressable control drawn with its own look, its icon or label is a child; input = an empty text field; text = a label rendered by the engine; progress = a bar drawn with the look of its empty track, with children: an image for the fill (its own look, box = the visible filled part) and a text for its label; group = an invisible container; grid = a regular grid of identical-size cells (a board, an inventory, rows of letter slots) described by rows, cols and cells instead of separate elements.",
            "- A look is ONE piece of artwork at ONE size: every element drawn with the same artwork apart from the text or icon on top of it shares it (all cream tiles of one size share one look; tiles of another color or another size have their own). Name looks by what they are: tile_cream, tile_orange, button_green, panel_brown.",
            "- Every visible piece of text is its own text element (letters on separate tiles too), except inside a grid, whose cells carry their texts; never bake text into a look. Icons that sit on a button are separate image children of the button.",
            "- Children must be inside their parent. List parents before children, top to bottom, left to right.",
            "- Be exhaustive: every panel, tile, slot, button, icon, bar and label on the screen."
        };

        String notes = JsTrim(hint);
        if (!notes.IsEmpty())
        {
            lines.Add("");
            lines.Add("Notes from the user about this screen: " + notes);
        }

        return Join(lines, "\n");
    }

    String Cut(const String& describe, const Vector<String>& erase, const String& keyName, const String& keyHex)
    {
        Vector<String> parts = {
            "This image is a crop of a game UI mockup. Keep ONLY " + describe +
                " \xE2\x80\x94 the element that fills the middle of the image \xE2\x80\x94 exactly as it is: the same shape, size, position, colours, outline and shadow. Do not move, resize, recolor or redraw it."
        };

        if (!erase.IsEmpty())
        {
            parts.Add("Remove " + Join(erase, ", ") +
                      " drawn on top of it and restore its plain surface underneath, continuing its own colour, texture and shading exactly.");
        }

        parts.Add("Replace EVERYTHING else \xE2\x80\x94 the background and any other elements around it \xE2\x80\x94 with a flat, solid " + keyName +
                  " (" + keyHex + ") background.");
        parts.Add("Do not add anything. Output only the image.");
        return Join(parts, " ");
    }

    String Background(const String& describe, bool strict)
    {
        String prompt = "This is a game UI mockup. Remove EVERY UI element from it \xE2\x80\x94 panels, frames, tiles, buttons, icons, bars and all text \xE2\x80\x94 and show only the background behind them" +
            (describe.IsEmpty() ? String() : " (" + describe + ")") + ", continued naturally where the elements were." +
            " Keep the image size, the colours, the lighting and the texture of the background exactly. Output only the image.";
        if (strict)
            prompt += " The letters, digits and words printed on the tiles, buttons and panels belong to those elements: remove them too. No letter, digit, icon or piece of an element may remain \xE2\x80\x94 only the bare background.";
        return prompt;
    }
}
