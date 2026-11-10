#pragma once

#include <optional>

#include "o2Editor/Pipeline/UiDoc/UiDocJs.h"

// ------------------------------------------------------------------------------------------------
// Text style: a data type of its own (port type `textStyle`), port of AssetsLine shared/textStyle.ts.
// A style is a small JSON object - font, size, fill, outline, shadow - produced by the Text style node
// and collected into a UI kit, where the AI UI layout applies it to its labels. Fonts come from a fixed
// catalogue of open fonts. Text widths are estimated from per-font averages, never measured, so every
// editor wraps and shrinks a label the same way
// ------------------------------------------------------------------------------------------------
namespace Editor::UiDoc
{
    // ---------------------------------
    // One face (weight) of a font family
    // ---------------------------------
    struct UiFontFace
    {
        int    weight = 400;
        String file;
    };

    // ------------------------------------
    // A font of the catalogue: its faces and looks
    // ------------------------------------
    struct UiFont
    {
        String             family;   // Family name as fontconfig and CSS know it
        Vector<UiFontFace> faces;
        bool               cyrillic = false; // Has Cyrillic glyphs
        String             look;     // Short look note for the menu and the model
        String             license;  // Licence file next to the fonts
    };

    // The font catalogue (Roboto first: the fallback)
    const Vector<UiFont>& UiFonts();

    // The default font family
    extern const char* const DEFAULT_FONT;

    // The font of a family name (any case, trimmed), Roboto when it is not in the catalogue
    const UiFont& FontOf(const String& family);

    // The face the font has closest to the asked weight (a single-face font always gives its one)
    const UiFontFace& FaceOf(const String& family, double weight);

    // What a label's text is for: title, heading, body, button, caption, number, label
    const Vector<String>& TextRoles();

    // ------------------
    // Outline of a style
    // ------------------
    struct TextStroke
    {
        String color = "#000000";
        double width = 2;
    };

    // --------------------
    // Drop shadow of a style
    // --------------------
    struct TextShadow
    {
        String color = "#000000";
        double x = 0, y = 4, blur = 4, opacity = 0.5;
    };

    // -------------------------------------------------------------------------------------
    // A complete text style; JSON key order as normalizeTextStyle builds it (name, role, font...)
    // -------------------------------------------------------------------------------------
    struct TextStyle
    {
        String                    name = "body";  // Name the layout refers to (style=title)
        String                    role = "body";  // title | heading | body | button | caption | number | label
        String                    font = "Roboto";
        double                    weight = 400;
        double                    size = 32;
        String                    color = "#FFFFFF";   // Fill colour, #RRGGBB
        std::optional<String>     gradient;            // Bottom colour of a vertical gradient from color; none = flat
        std::optional<TextStroke> stroke;
        std::optional<TextShadow> shadow;
        double                    letterSpacing = 0;   // Extra space between letters, px
        double                    lineHeight = 1.2;    // Line step as a multiple of the size
        bool                      uppercase = false;
        String                    align = "center";    // left | center | right
        std::optional<String>     description;         // What the style is for, for the model
    };

    // The default style (body, Roboto 400 32, white, centred)
    TextStyle TextStyleDefault();

    // A name the layout can refer to: letters, digits, _ and - (spaces become _), def when nothing is left
    String CleanStyleName(const Json* value, const String& def = "style");

    // Any stored or model-written object -> a complete style
    TextStyle NormalizeTextStyle(const Json& raw, const String& fallbackName = "body");

    // The style a Text style node's config describes (its config IS the style)
    TextStyle TextStyleFromConfig(const Json& config);

    // The style a Text style node's o2 config describes
    TextStyle TextStyleFromConfig(const DataValue& config);

    // The style as JSON, keys in the web's order (JSON.stringify(style))
    Json TextStyleToJson(const TextStyle& style);

    // Font files a set of styles needs (for exports), in first-use order
    Vector<String> FontFilesOf(const Vector<TextStyle>& styles);

    // Estimated width of a one-line text in the style (per-font average advances; uppercase applied)
    double EstimateTextWidth(const String& text, const String& font, double weight, double size, double letterSpacing,
                             bool uppercase);

    // Estimated width of a one-line text in the style at its own size
    double EstimateTextWidth(const String& text, const TextStyle& style);

    // Ascent above the baseline and the visible cap height, as shares of the size
    struct TextMetrics
    {
        double cap = 0.71, ascent = 0.93, descent = 0.24;
    };

    // The metrics every renderer places text by
    constexpr TextMetrics TEXT_METRICS{};

    // The Text style node's default model for the AI button
    extern const char* const TEXT_STYLE_AI_DEFAULT_MODEL;

    // The prompt of the Text style node's the AI button: the font catalogue, the current style and the request
    String BuildStylePrompt(const TextStyle& current, const String& request, int references);

    // The model's answer -> the new style (the name is kept, fields the answer leaves out stay); false when it holds no
    // JSON object
    bool ParseStyleAnswer(const String& answer, const TextStyle& current, TextStyle& out);
}
