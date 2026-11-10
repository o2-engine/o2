#include "o2Editor/stdafx.h"
#include "UiTextStyle.h"

#include <cmath>

#include "o2Editor/Pipeline/UiDoc/UiDocFp.h"

namespace Editor::UiDoc
{
    const char* const DEFAULT_FONT = "Roboto";
    const char* const TEXT_STYLE_AI_DEFAULT_MODEL = "gpt-6.1-sol";

    const Vector<UiFont>& UiFonts()
    {
        static const Vector<UiFont> fonts = {
            { "Roboto", { { 400, "Roboto-Regular.ttf" }, { 500, "Roboto-Medium.ttf" }, { 700, "Roboto-Bold.ttf" } }, true,
              "neutral sans", "LICENSE-Roboto.txt" },
            { "Fira Sans", { { 700, "FiraSans-Bold.ttf" }, { 900, "FiraSans-Black.ttf" } }, true, "strong humanist sans",
              "LICENSE-FiraSans.txt" },
            { "Play", { { 400, "Play-Regular.ttf" }, { 700, "Play-Bold.ttf" } }, true, "techy squared sans", "LICENSE-Play.txt" },
            { "Russo One", { { 400, "RussoOne-Regular.ttf" } }, true, "heavy sporty display", "LICENSE-RussoOne.txt" },
            { "Montserrat Alternates", { { 900, "MontserratAlternates-Black.ttf" } }, true, "rounded geometric black",
              "LICENSE-MontserratAlternates.txt" },
            { "Rubik Mono One", { { 400, "RubikMonoOne-Regular.ttf" } }, true, "chunky wide caps", "LICENSE-RubikMonoOne.txt" },
            { "Lobster", { { 400, "Lobster-Regular.ttf" } }, true, "bold script", "LICENSE-Lobster.txt" },
            { "Amatic SC", { { 700, "AmaticSC-Bold.ttf" } }, true, "hand-drawn narrow caps", "LICENSE-AmaticSC.txt" },
            { "Press Start 2P", { { 400, "PressStart2P-Regular.ttf" } }, true, "pixel / retro", "LICENSE-PressStart2P.txt" },
            { "Lilita One", { { 400, "LilitaOne-Regular.ttf" } }, false, "casual game display (Latin only)", "LICENSE-LilitaOne.txt" },
            { "Luckiest Guy", { { 400, "LuckiestGuy-Regular.ttf" } }, false, "cartoon caps (Latin only)", "LICENSE-LuckiestGuy.txt" },
            { "Bangers", { { 400, "Bangers-Regular.ttf" } }, false, "comic caps (Latin only)", "LICENSE-Bangers.txt" },
        };
        return fonts;
    }

    const UiFont& FontOf(const String& family)
    {
        String f = JsToLower(JsTrim(family));
        for (auto& font : UiFonts())
        {
            if (JsToLower(font.family) == f)
                return font;
        }
        return UiFonts()[0];
    }

    const UiFontFace& FaceOf(const String& family, double weight)
    {
        const UiFont& font = FontOf(family);
        const UiFontFace* best = &font.faces[0];
        for (auto& face : font.faces)
        {
            if (std::fabs(face.weight - weight) < std::fabs(best->weight - weight))
                best = &face;
        }
        return *best;
    }

    const Vector<String>& TextRoles()
    {
        static const Vector<String> roles = { "title", "heading", "body", "button", "caption", "number", "label" };
        return roles;
    }

    TextStyle TextStyleDefault()
    {
        return TextStyle();
    }

    // #RGB / #RRGGBB (trimmed) -> #RRGGBB upper case, def for anything else
    static String Hex(const Json* v, const String& def)
    {
        String s = JsTrim(JsStringOr(v));
        auto isHex = [](char c) { return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'); };
        if (s.Length() == 4 && s[0] == '#' && isHex(s[1]) && isHex(s[2]) && isHex(s[3]))
            s = String("#") + s[1] + s[1] + s[2] + s[2] + s[3] + s[3];

        if (s.Length() != 7 || s[0] != '#')
            return def;
        for (int i = 1; i < 7; i++)
        {
            if (!isHex(s[i]))
                return def;
        }
        return JsToUpper(s);
    }

    // Number(v) clamped to [lo, hi], d when it is not a number
    static double Num(const Json* v, double d, double lo, double hi)
    {
        double n = JsNumber(v);
        return std::isfinite(n) ? std::max(lo, std::min(hi, n)) : d;
    }

    // Keeps letters, digits, _ and -
    static String KeepNameChars(const String& text)
    {
        std::string out;
        for (char c : (const std::string&)text)
        {
            if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_' || c == '-')
                out += c;
        }
        return String(out);
    }

    String CleanStyleName(const Json* value, const String& def /*= "style"*/)
    {
        String s = KeepNameChars(ReplaceSpaceRuns(JsTrim(JsStringOr(value)), "_"));
        return s.IsEmpty() ? def : s;
    }

    TextStyle NormalizeTextStyle(const Json& raw, const String& fallbackName /*= "body"*/)
    {
        const Json empty = Json::Object();
        const Json& r = raw.IsObject() ? raw : empty;
        TextStyle d;
        TextStyle out;

        const Json* fontValue = JsGet(r, "font");
        out.font = FontOf(JsNullish(fontValue) ? String(d.font) : fontValue->ToJsString()).family;

        const Json* roleValue = JsGet(r, "role");
        out.role = TextRoles().Contains(JsString(roleValue)) ? JsString(roleValue) : d.role;

        const Json* st = JsGet(r, "stroke");
        if (JsTruthy(st) && JsIsObjectLike(st) && JsNumber(JsGet(st, "width")) > 0)
            out.stroke = TextStroke{ Hex(JsGet(st, "color"), "#000000"), Num(JsGet(st, "width"), 2, 0, 40) };

        const Json* sh = JsGet(r, "shadow");
        if (JsTruthy(sh) && JsIsObjectLike(sh))
        {
            TextShadow shadow{ Hex(JsGet(sh, "color"), "#000000"), Num(JsGet(sh, "x"), 0, -80, 80), Num(JsGet(sh, "y"), 4, -80, 80),
                               Num(JsGet(sh, "blur"), 4, 0, 60), Num(JsGet(sh, "opacity"), 0.5, 0, 1) };
            if (shadow.opacity > 0)
                out.shadow = shadow;
        }

        const Json* alignValue = JsGet(r, "align");
        String align = alignValue && alignValue->IsString() ? alignValue->Str() : String();
        out.align = align == "left" || align == "right" ? align : String("center");

        out.name = CleanStyleName(JsGet(r, "name"), fallbackName);
        out.weight = FaceOf(out.font, Num(JsGet(r, "weight"), d.weight, 100, 900)).weight;
        out.size = JsRound(Num(JsGet(r, "size"), d.size, 6, 400));
        out.color = Hex(JsGet(r, "color"), d.color);

        const Json* gradient = JsGet(r, "gradient");
        if (JsTruthy(gradient))
        {
            String g = Hex(gradient, "");
            if (!g.IsEmpty())
                out.gradient = g;
        }

        out.letterSpacing = Num(JsGet(r, "letterSpacing"), 0, -20, 80);
        out.lineHeight = Num(JsGet(r, "lineHeight"), d.lineHeight, 0.6, 3);
        out.uppercase = JsTruthy(JsGet(r, "uppercase"));

        String desc = JsTrim(JsStringOr(JsGet(r, "description")));
        if (!desc.IsEmpty())
            out.description = JsSlice(desc, 0, 300);

        return out;
    }

    TextStyle TextStyleFromConfig(const Json& config)
    {
        return NormalizeTextStyle(config, CleanStyleName(JsGet(config, "name"), "body"));
    }

    TextStyle TextStyleFromConfig(const DataValue& config)
    {
        return TextStyleFromConfig(FromDataValue(config));
    }

    Json TextStyleToJson(const TextStyle& style)
    {
        Json out = Json::Object();
        out["name"] = style.name;
        out["role"] = style.role;
        out["font"] = style.font;
        out["weight"] = style.weight;
        out["size"] = style.size;
        out["color"] = style.color;
        out["gradient"] = style.gradient ? Json(*style.gradient) : Json();
        if (style.stroke)
        {
            Json stroke = Json::Object();
            stroke["color"] = style.stroke->color;
            stroke["width"] = style.stroke->width;
            out["stroke"] = stroke;
        }
        else
            out["stroke"] = Json();

        if (style.shadow)
        {
            Json shadow = Json::Object();
            shadow["color"] = style.shadow->color;
            shadow["x"] = style.shadow->x;
            shadow["y"] = style.shadow->y;
            shadow["blur"] = style.shadow->blur;
            shadow["opacity"] = style.shadow->opacity;
            out["shadow"] = shadow;
        }
        else
            out["shadow"] = Json();

        out["letterSpacing"] = style.letterSpacing;
        out["lineHeight"] = style.lineHeight;
        out["uppercase"] = style.uppercase;
        out["align"] = style.align;
        if (style.description)
            out["description"] = *style.description;
        return out;
    }

    Vector<String> FontFilesOf(const Vector<TextStyle>& styles)
    {
        Vector<String> out;
        for (auto& s : styles)
        {
            String file = FaceOf(s.font, s.weight).file;
            if (!out.Contains(file))
                out.Add(file);
        }
        return out;
    }

    // Average advance of a lowercase-mixed glyph, as a share of the size
    static double Advance(const String& family)
    {
        static const std::pair<const char*, double> advances[] = {
            { "Roboto", 0.53 }, { "Fira Sans", 0.55 }, { "Play", 0.55 }, { "Russo One", 0.62 }, { "Montserrat Alternates", 0.68 },
            { "Rubik Mono One", 0.86 }, { "Lobster", 0.47 }, { "Amatic SC", 0.36 }, { "Press Start 2P", 1.0 }, { "Lilita One", 0.5 },
            { "Luckiest Guy", 0.62 }, { "Bangers", 0.45 },
        };
        for (auto& a : advances)
        {
            if (family == a.first)
                return a.second;
        }
        return 0.55;
    }

    double EstimateTextWidth(const String& text, const String& font, double weight, double size, double letterSpacing,
                             bool uppercase)
    {
        double base = Advance(FontOf(font).family);
        double bold = weight >= 700 ? 1.06 : weight >= 500 ? 1.03 : 1;
        double units = 0;
        for (char32_t ch : Utf8ToCodePoints(uppercase ? JsToUpper(text) : text))
        {
            if (ch == ' ')
                units += 0.55;
            else if ((ch >= 'A' && ch <= 'Z') || (ch >= 0x410 && ch <= 0x42F) || ch == 0x401 || (ch >= '0' && ch <= '9'))
                units += 1.22;
            else if (ch == 'i' || ch == 'l' || ch == 'j' || ch == 'I' || ch == '.' || ch == ',' || ch == ':' || ch == ';' ||
                     ch == '\'' || ch == '!' || ch == '|')
                units += 0.5;
            else if (ch == 'm' || ch == 'w' || ch == 'M' || ch == 'W' || ch == 0x428 || ch == 0x429 || ch == 0x416 || ch == 0x42E ||
                     ch == 0x43C || ch == 0x448 || ch == 0x449 || ch == 0x436 || ch == 0x44E)
                units += 1.5;
            else
                units += 1;
        }
        return units*base*bold*size + std::max(0, Utf16Length(text) - 1)*letterSpacing;
    }

    double EstimateTextWidth(const String& text, const TextStyle& style)
    {
        return EstimateTextWidth(text, style.font, style.weight, style.size, style.letterSpacing, style.uppercase);
    }

    String BuildStylePrompt(const TextStyle& current, const String& request, int references)
    {
        Vector<String> fonts;
        for (auto& f : UiFonts())
        {
            Vector<String> weights;
            for (auto& face : f.faces)
                weights.Add(JsNum(face.weight));
            fonts.Add("- " + f.family + " \xE2\x80\x94 weights " + Join(weights, "/") + " \xE2\x80\x94 " + f.look +
                      (f.cyrillic ? ", has Cyrillic" : ""));
        }

        Json rest = TextStyleToJson(current);
        rest.Remove("name");

        Vector<String> lines = {
            "You design text styles for the labels of game and app interfaces (titles, buttons, counters, body text). A style is "
            "drawn by an SVG renderer: one font, a flat fill or a vertical gradient, an outline drawn under the fill, a drop shadow.",
            "",
            "Fonts you may use (family \xE2\x80\x94 weights \xE2\x80\x94 look):",
            Join(fonts, "\n"),
            "",
            "Answer with JSON only:",
            "{\"role\": \"title|heading|body|button|caption|number|label\", \"font\": \"<family from the list>\", \"weight\": <one of "
            "that font's weights>, \"size\": <px on a 1080-px-wide screen>, \"color\": \"#RRGGBB\", \"gradient\": \"#RRGGBB\" or "
            "null (vertical, from color at the top to this at the bottom), \"stroke\": {\"color\": \"#RRGGBB\", \"width\": <px>} or "
            "null, \"shadow\": {\"color\": \"#RRGGBB\", \"x\": <px>, \"y\": <px>, \"blur\": <px>, \"opacity\": <0..1>} or null, "
            "\"letterSpacing\": <px>, \"lineHeight\": <multiple of size>, \"uppercase\": true|false, \"align\": "
            "\"left|center|right\", \"description\": \"<one sentence: where the style is used>\"}",
            "",
            "Rules: keep what the request does not ask to change; outlines and shadows scale with the size (a 100 px title takes a "
            "6\xE2\x80\x93" "10 px outline); pick a font with Cyrillic when the labels may be Russian; readable contrast.",
        };
        if (references)
        {
            lines.Add("");
            lines.Add(JsNum(references) + " reference picture(s) are attached: match their lettering \xE2\x80\x94 colours, outline, "
                      "shadow, weight, case \xE2\x80\x94 and pick the closest font.");
        }
        lines.Add("");
        lines.Add("Current style:");
        lines.Add(rest.Dump());
        lines.Add("");
        lines.Add("<request>");
        lines.Add(JsTrim(request));
        lines.Add("</request>");
        return Join(lines, "\n");
    }

    // answer.replace(/```[a-z]*\n?|```/gi, '')
    static String StripFences(const String& answer)
    {
        const std::string& s = answer;
        std::string out;
        size_t i = 0;
        while (i < s.size())
        {
            if (s.compare(i, 3, "```") == 0)
            {
                i += 3;
                while (i < s.size() && ((s[i] >= 'a' && s[i] <= 'z') || (s[i] >= 'A' && s[i] <= 'Z')))
                    i++;
                if (i < s.size() && s[i] == '\n')
                    i++;
                continue;
            }
            out += s[i++];
        }
        return String(out);
    }

    bool ParseStyleAnswer(const String& answer, const TextStyle& current, TextStyle& out)
    {
        std::string s = StripFences(answer);
        size_t a = s.find('{'), b = s.rfind('}');
        if (a == std::string::npos || b == std::string::npos || b <= a)
            return false;

        Json raw;
        if (!Json::Parse(String(s.substr(a, b - a + 1)), raw) || !raw.IsObject())
            return false;

        Json merged = TextStyleToJson(current);
        for (auto& member : raw.Members())
            merged[member.first] = member.second;
        merged["name"] = current.name;

        out = NormalizeTextStyle(merged, current.name);
        return true;
    }
}
