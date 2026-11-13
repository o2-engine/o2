#pragma once

#include <optional>

#include "o2Editor/Pipeline/UiDoc/UiDocJs.h"

// ------------------------------------------------------------------------------------------------
// Text style: a data type of its own (port type `textStyle`), port of AssetsLine shared/textStyle.ts.
// A style is a small JSON object - font, size, a flat or gradient fill and a list of layer effects
// (outlines, depth, shadows, glows, inner shadows, a bevel: what a Photoshop layer style is made of) -
// produced by the Text style node (or read off a picture by the AI text style node) and collected into
// a UI kit, where the AI UI layout applies it to its labels. `fill` and `effects` are what a style is;
// `gradient`, `stroke` and `shadow` are mirrors (the gradient's last colour, the first outline, the
// first shadow or glow) for whoever draws only those. Text widths are estimated from per-font averages,
// never measured, so every editor wraps and shrinks a label the same way
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

    // ------------------------------
    // Mirror: the first outline of a style
    // ------------------------------
    struct TextStroke
    {
        String color = "#000000";
        double width = 2;
    };

    // ----------------------------------------------
    // Mirror: the first drop shadow (or glow) of a style
    // ----------------------------------------------
    struct TextShadow
    {
        String color = "#000000";
        double x = 0, y = 4, blur = 4, opacity = 0.5;
    };

    // ---------------------------
    // A stop of a gradient fill
    // ---------------------------
    struct GradientStop
    {
        String color;
        double at = 0; // 0..1 along the gradient
    };

    // ----------------------------------------------------------------------------------------------
    // A linear gradient over the letters: angle 90 = top to bottom, 0 = left to right, degrees clockwise.
    // Two stops at one place make a hard edge (a glossy band)
    // ----------------------------------------------------------------------------------------------
    struct TextGradient
    {
        double               angle = 90;
        Vector<GradientStop> stops;
    };

    // The kinds of layer effects: stroke, extrude, shadow, glow, innerShadow, bevel
    const Vector<String>& TextEffectTypes();

    // At most this many effects and gradient stops
    constexpr int TEXT_EFFECTS_MAX = 8;
    constexpr int GRADIENT_STOPS_MAX = 6;

    // ------------------------------------------------------------------------------------------------
    // One layer effect; which fields mean something depends on the type (JSON keys in the web's order):
    // stroke {color, width, opacity} - an outline outside the letters, several nest from the letters out;
    // extrude {color, x, y} - solid depth; shadow {color, x, y, blur, spread, opacity}; glow {color, size,
    // opacity}; innerShadow {color, x, y, blur, opacity}; bevel {highlight, shade, depth, soft, opacity}.
    // `off` keeps it in the list without drawing it
    // ------------------------------------------------------------------------------------------------
    struct TextEffect
    {
        String type = "stroke";
        String color = "#000000";
        double width = 0;
        double x = 0, y = 0;
        double blur = 0;
        double spread = 0;
        double size = 0;
        double opacity = 1;
        String highlight = "#FFFFFF";
        String shade = "#000000";
        double depth = 0, soft = 0;
        bool   off = false;
    };

    // The effect as JSON, keys in the web's order for its type
    Json TextEffectToJson(const TextEffect& effect);

    // -------------------------------------------------------------------------------------
    // A complete text style; JSON key order as normalizeTextStyle builds it (name, role, font...)
    // -------------------------------------------------------------------------------------
    struct TextStyle
    {
        String                      name = "body";  // Name the layout refers to (style=title)
        String                      role = "body";  // title | heading | body | button | caption | number | label
        String                      font = "Roboto";
        double                      weight = 400;
        double                      size = 32;
        String                      color = "#FFFFFF";   // Fill colour, #RRGGBB (a gradient's first colour)
        std::optional<TextGradient> fill;                // Gradient fill; none = flat color
        Vector<TextEffect>          effects;             // Layer effects
        std::optional<String>       gradient;            // Mirror: the last colour of fill
        std::optional<TextStroke>   stroke;              // Mirror: the first live outline
        std::optional<TextShadow>   shadow;              // Mirror: the first live shadow or glow
        double                      letterSpacing = 0;   // Extra space between letters, px
        double                      lineHeight = 1.2;    // Line step as a multiple of the size
        bool                        uppercase = false;
        String                      align = "center";    // left | center | right
        std::optional<String>       description;         // What the style is for, for the model
        Json                        extracted;           // AI text style only: {name, role, text?} the model answered (null = none)
    };

    // The default style (body, Roboto 400 32, white, centred)
    TextStyle TextStyleDefault();

    // A name the layout can refer to: letters, digits, _ and - (spaces become _), def when nothing is left
    String CleanStyleName(const Json* value, const String& def = "style");

    // One effect as stored or written by a model (aliases like outline / dropShadow / emboss taken) -> cleaned; none
    // when it is not one or draws nothing
    std::optional<TextEffect> NormalizeEffect(const Json& raw);

    // A list of effects cleaned, at most TEXT_EFFECTS_MAX
    Vector<TextEffect> NormalizeEffects(const Json* raw);

    // A gradient with at least two colours, its stops in order; none for anything else (a flat fill)
    std::optional<TextGradient> NormalizeGradient(const Json* raw);

    // The gradient as JSON {angle, stops: [{color, at}]}
    Json TextGradientToJson(const TextGradient& gradient);

    // The effects that are drawn (not switched off)
    Vector<TextEffect> LiveEffects(const TextStyle& style);

    // Any stored or model-written object -> a complete style. fill / effects, when the object has them, are what it
    // says; without them its gradient / stroke / shadow (an older style, a model answering the short way) are read
    TextStyle NormalizeTextStyle(const Json& raw, const String& fallbackName = "body");

    // Fields written over a style by a model or an older editor: fill / effects taken as they are; the short color /
    // gradient / stroke / shadow replace the fill, the outlines and the shadows, the rest of the effects stays. The
    // name is kept
    TextStyle MergeTextStyle(const TextStyle& current, const Json& raw);

    // How far the drawn style reaches outside the letters, px at the style's size
    struct TextReach
    {
        double left = 0, top = 0, right = 0, bottom = 0;
    };
    TextReach EffectsReach(const TextStyle& style);

    // The style at another scale: its size, spacing and every pixel size of its effects times k
    TextStyle ScaleTextStyle(const TextStyle& style, double k);

    // The look of a style in a few words, for a model's list of styles
    String StyleLook(const TextStyle& style);

    // A new effect of a kind with sizes that suit the style (the editor's "add effect")
    TextEffect DefaultEffect(const String& type, const TextStyle& style);

    // The style a Text style node's config describes (its config IS the style)
    TextStyle TextStyleFromConfig(const Json& config);

    // The style a Text style node's o2 config describes
    TextStyle TextStyleFromConfig(const DataValue& config);

    // The style as JSON, keys in the web's order (JSON.stringify(style)); extracted last when the style has it
    Json TextStyleToJson(const TextStyle& style);

    // The style's JSON members written into a node config (the Text style node's config IS the style): fill, effects
    // and their mirrors together
    void WriteStyleLook(DataValue& config, const TextStyle& style);

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

    // --- AI ------------------------------------------------------------------------------------------

    // The Text style node's default model for the AI button, also the AI text style node's
    extern const char* const TEXT_STYLE_AI_DEFAULT_MODEL;

    // Sizes of a style are pixels on a screen this wide
    constexpr double TEXT_STYLE_SCREEN_W = 1080;

    // How a model writes the fill and the effects of a style (JSON fields); px: what its pixel numbers are measured in
    Vector<String> StyleLookSpec(const String& px);

    // The prompt of the Text style node's AI button: the font catalogue, the current style and the request
    String BuildStylePrompt(const TextStyle& current, const String& request, int references);

    // The JSON object in a model's answer (code fences and words around it dropped); false when there is none
    bool AnswerObject(const String& answer, Json& out);

    // The model's answer -> the new style (the name is kept, fields the answer leaves out stay); false when it holds no
    // JSON object
    bool ParseStyleAnswer(const String& answer, const TextStyle& current, TextStyle& out);

    // ------------------------------------------------------
    // What the AI text style node asks a vision model about
    // ------------------------------------------------------
    struct StyleExtractRequest
    {
        double width = 0, height = 0; // The picture as the model gets it, px
        String hint;                  // Which lettering to take
        String matte;                 // The transparent areas shown as this flat colour ("" = none)
        double fullWidth = 0;         // The picture is a part of a larger one: that one's width at the same scale (0 = none)
    };

    // The prompt that reads the style of lettering in a picture
    String BuildStyleExtractPrompt(const StyleExtractRequest& request);

    // A name / role set over an extracted style (JSON; empty or unknown = what the model answered); the style keeps
    // what the model answered in `extracted`
    Json ApplyStyleOverrides(const Json& style, const String& name, const String& role);

    // The model's answer to BuildStyleExtractPrompt -> the style JSON (with `extracted`), its measurements brought from
    // the picture's pixels to the styles' scale. "none": the model found no lettering; "": no style in the answer
    String ParseStyleExtractAnswer(const String& answer, double pictureWidth, Json& out, const String& name = "",
                                   const String& role = "");
}
