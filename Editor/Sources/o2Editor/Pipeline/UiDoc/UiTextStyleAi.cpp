#include "o2Editor/stdafx.h"
#include "UiTextStyle.h"

#include <cmath>

#include "o2Editor/Pipeline/UiDoc/UiDocFp.h"

// The AI side of shared/textStyle.ts: the Text style node's request prompt, the AI text style node's
// prompt and the reading of both answers
namespace Editor::UiDoc
{
    static const char* const DASH = "\xE2\x80\x94";
    static const char* const EN_DASH = "\xE2\x80\x93";

    static String FontLines()
    {
        Vector<String> fonts;
        for (auto& f : UiFonts())
        {
            Vector<String> weights;
            for (auto& face : f.faces)
                weights.Add(JsNum(face.weight));
            fonts.Add("- " + f.family + " " + DASH + " weights " + Join(weights, "/") + " " + DASH + " " + f.look +
                      (f.cyrillic ? ", has Cyrillic" : ""));
        }
        return Join(fonts, "\n");
    }

    Vector<String> StyleLookSpec(const String& px)
    {
        return {
            String("\"color\": \"#RRGGBB\" ") + DASH + " the fill when it is one flat colour (with a gradient: its first colour);",
            String("\"fill\": null for a flat fill, or a gradient {\"angle\": <degrees: 90 = top to bottom, 0 = left to right>, "
                   "\"stops\": [{\"color\": \"#RRGGBB\", \"at\": <0..1>}, ...]} ") + DASH + " 2 to " + JsNum(GRADIENT_STOPS_MAX) +
                " stops from the start of the gradient to its end; two stops at the same \"at\" make a hard edge (a glossy band "
                "across the letters);",
            String("\"effects\": [...] ") + DASH + " layer effects as in a Photoshop layer style, any number of each kind, at most " +
                JsNum(TEXT_EFFECTS_MAX) + " in all, [] for plain letters; sizes are " + px + ":",
            String("  {\"type\": \"stroke\", \"color\": \"#RRGGBB\", \"width\": <px>, \"opacity\": <0..1>} ") + DASH +
                " an outline outside the letters. Several strokes nest: the first is next to the letters, each next one goes "
                "around the previous (a thin light line, then a thick dark contour);",
            String("  {\"type\": \"extrude\", \"color\": \"#RRGGBB\", \"x\": <px>, \"y\": <px>} ") + DASH +
                " solid 3D depth: the outlined letters repeated towards the offset " + DASH +
                " the thick dark \"lip\" under cartoon titles and button labels;",
            String("  {\"type\": \"shadow\", \"color\": \"#RRGGBB\", \"x\": <px>, \"y\": <px>, \"blur\": <px>, \"spread\": <px>, "
                   "\"opacity\": <0..1>} ") + DASH + " a drop shadow of the whole outlined shape; blur 0 = a hard offset copy;",
            String("  {\"type\": \"glow\", \"color\": \"#RRGGBB\", \"size\": <px>, \"opacity\": <0..1>} ") + DASH +
                " a soft halo around the shape;",
            String("  {\"type\": \"innerShadow\", \"color\": \"#RRGGBB\", \"x\": <px>, \"y\": <px>, \"blur\": <px>, \"opacity\": "
                   "<0..1>} ") + DASH + " a shade inside the letters along their edge: y > 0 darkens (or, with a light colour, "
                "lights) their top, y < 0 their bottom; x 0 and y 0 = an inner glow;",
            String("  {\"type\": \"bevel\", \"highlight\": \"#RRGGBB\", \"shade\": \"#RRGGBB\", \"depth\": <px>, \"soft\": <px>, "
                   "\"opacity\": <0..1>} ") + DASH + " a light edge inside the top of the letters and a dark one inside the "
                "bottom: embossed, volumetric letters.",
        };
    }

    String BuildStylePrompt(const TextStyle& current, const String& request, int references)
    {
        // The style as a model is shown it: without its name and the mirrors of its fill and effects
        Json rest = TextStyleToJson(current);
        for (const char* key : { "name", "gradient", "stroke", "shadow", "extracted" })
            rest.Remove(key);

        Vector<String> lines = {
            "You design text styles for the labels of game and app interfaces (titles, buttons, counters, body text). A style is "
            "drawn by an SVG renderer: one font, a flat or gradient fill, and a stack of layer effects " + String(DASH) +
            " outlines, solid depth, drop shadows, glows, inner shadows, a bevel.",
            "",
            "Fonts you may use (family " + String(DASH) + " weights " + DASH + " look):",
            FontLines(),
            "",
            "Answer with JSON only, an object with these fields:",
            "\"role\": \"title|heading|body|button|caption|number|label\", \"font\": \"<family from the list>\", \"weight\": <one of "
            "that font's weights>, \"size\": <px on a 1080-px-wide screen>,",
        };
        lines.Add(StyleLookSpec("px at that size"));
        lines.Add("\"letterSpacing\": <px>, \"lineHeight\": <multiple of size>, \"uppercase\": true|false, \"align\": "
                  "\"left|center|right\", \"description\": \"<one sentence: where the style is used>\"");
        lines.Add("");
        lines.Add("Rules: keep what the request does not ask to change " + String(DASH) + " \"effects\" is the whole list, so repeat "
                  "the effects that stay; effects scale with the size (a 100 px title takes a 6" + EN_DASH + "10 px outline); build "
                  "rich looks out of several effects (a casual-game title: gradient fill, a bevel or a light inner shadow at the "
                  "top, a dark outline, depth under it, a soft shadow); pick a font with Cyrillic when the labels may be Russian; "
                  "readable contrast.");
        if (references)
        {
            lines.Add("");
            lines.Add(JsNum(references) + " reference picture(s) are attached: match their lettering " + DASH + " fill, every "
                      "outline, depth, shadows, glow, weight, case " + DASH + " and pick the closest font.");
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

    bool AnswerObject(const String& answer, Json& out)
    {
        std::string s = StripFences(answer);
        size_t a = s.find('{'), b = s.rfind('}');
        if (a == std::string::npos || b == std::string::npos || b <= a)
            return false;

        Json raw;
        if (!Json::Parse(String(s.substr(a, b - a + 1)), raw) || !raw.IsObject())
            return false;

        out = raw;
        return true;
    }

    bool ParseStyleAnswer(const String& answer, const TextStyle& current, TextStyle& out)
    {
        Json raw;
        if (!AnswerObject(answer, raw))
            return false;

        out = MergeTextStyle(current, raw);
        return true;
    }

    String BuildStyleExtractPrompt(const StyleExtractRequest& req)
    {
        String hint = JsTrim(req.hint);
        Vector<String> lines = {
            "You read the text style of lettering in a picture " + String(DASH) + " a game or app screen, a mockup, a banner, a "
            "button, a logo " + DASH + " so that labels like it can be drawn again by an SVG renderer: one font, a flat or gradient "
            "fill, and a stack of layer effects " + DASH + " outlines, solid depth, drop shadows, glows, inner shadows, a bevel.",
            "",
            "The picture is " + JsNum(JsRound(req.width)) + "x" + JsNum(JsRound(req.height)) + " px." +
                (!req.matte.IsEmpty() ? " Its transparent areas are shown as flat " + req.matte + ": that colour is not part of "
                                        "the lettering." : String()),
            !hint.IsEmpty()
                ? String("Which lettering to take is said in <lettering> below; when nothing in the picture fits it, take the most "
                         "prominent lettering.")
                : String("When the picture has several kinds of lettering, take the most prominent one (the title or the main label)."),
            "",
            "Fonts you may use (family " + String(DASH) + " weights " + DASH + " look):",
            FontLines(),
            "Pick the one whose letters are closest " + String(DASH) + " proportions, weight, roundness, slant, serifs; one with "
            "Cyrillic when the lettering is Cyrillic.",
            "",
            "Measure in the picture's own pixels (they are converted to the layout's scale afterwards):",
            "- capHeight: the height of a capital letter or a digit of that lettering " + String(DASH) + " the letter's fill, without "
            "its outline and shadow;",
            req.fullWidth > 0 && req.fullWidth > req.width*1.02
                ? "- screenWidth: the width of the whole screen the lettering belongs to, in the same pixels. This picture is a part "
                  "cut out of a larger one, " + JsNum(JsRound(req.fullWidth)) + " px wide at this scale: answer " +
                  JsNum(JsRound(req.fullWidth)) + " when that larger picture is a whole screen; when it is itself only a piece of "
                  "one (a button, a banner, a logo), how wide the screen would be;"
                : "- screenWidth: the width of the whole screen the lettering belongs to, in the same pixels " + String(DASH) +
                  " the picture's width when it shows a whole screen; when it is a piece cut out of one (a button, a banner, a "
                  "logo), how wide the screen it was cut from would be;",
            "- the sizes of the effects " + String(DASH) + " an outline's width (how far it reaches outside what it goes around), a "
            "shadow's or a depth's offset, a blur's softness; letterSpacing: the extra gap between letters compared with the "
            "font's normal spacing (0 when it looks normal).",
            "",
            "Answer with JSON only, an object with these fields:",
            "\"text\": \"<the words of that lettering as written, the first 30 characters>\", \"name\": \"<short snake_case name: "
            "title_gold, button_label, counter>\", \"role\": \"title|heading|body|button|caption|number|label\", \"font\": "
            "\"<family from the list>\", \"weight\": <one of that font's weights>, \"capHeight\": <px>, \"screenWidth\": <px>,",
        };
        lines.Add(StyleLookSpec("pixels of the picture"));
        lines.Add("\"letterSpacing\": <px>, \"uppercase\": true|false, \"description\": \"<one sentence: what the lettering looks "
                  "like and where such a style is used>\"");
        lines.Add("");
        lines.Add("Rules: the colours of the letters themselves, not of what is behind them; look at the letters from the inside "
                  "out and list every layer you see " + String(DASH) + " the fill (a gradient when it fades or has a lighter band), "
                  "light or dark edges inside the letters (bevel, inner shadow), each outline in turn, the solid depth under the "
                  "outline, then shadows and glow; do not invent effects the lettering does not have; uppercase is true when the "
                  "lettering is all capitals; when the picture has no lettering at all, answer {\"none\": true}.");
        if (!hint.IsEmpty())
        {
            lines.Add("");
            lines.Add("<lettering>");
            lines.Add(hint);
            lines.Add("</lettering>");
        }
        return Join(lines, "\n");
    }

    Json ApplyStyleOverrides(const Json& style, const String& nameIn, const String& roleIn)
    {
        Json base = Json::Object();
        const Json* extracted = style.Get("extracted");
        if (!JsNullish(extracted))
            base = *extracted;
        else
        {
            base["name"] = style.Get("name") ? *style.Get("name") : Json();
            base["role"] = style.Get("role") ? *style.Get("role") : Json();
        }

        String name = JsTrim(nameIn);
        String role = JsTrim(roleIn);
        Json out = style;
        const Json* baseName = base.Get("name");
        Json typed(name);
        out["name"] = !name.IsEmpty() ? Json(CleanStyleName(&typed, JsStringOr(baseName))) : (baseName ? *baseName : Json());
        const Json* baseRole = base.Get("role");
        out["role"] = TextRoles().Contains(role) ? Json(role) : (baseRole ? *baseRole : Json());
        out["extracted"] = base;
        return out;
    }

    String ParseStyleExtractAnswer(const String& answer, double pictureWidth, Json& out, const String& name /*= ""*/,
                                   const String& role /*= ""*/)
    {
        Json raw;
        if (!AnswerObject(answer, raw))
            return "";

        const Json* none = raw.Get("none");
        if (none && none->IsBool() && none->Bool())
            return "none";

        double cap = JsNumber(raw.Get("capHeight"));
        bool measured = std::isfinite(cap) && cap > 0;
        if (!measured && !(JsNumber(raw.Get("size")) > 0))
            return "";
        if (!JsTruthy(raw.Get("font")) && !JsTruthy(raw.Get("color")))
            return "";

        double screen = JsNumber(raw.Get("screenWidth"));
        double k = measured ? TEXT_STYLE_SCREEN_W/(std::isfinite(screen) && screen >= 16 ? screen : std::max(16.0, pictureWidth)) : 1;
        auto px = [&](const Json* v, double step = 1) {
            double n = JsNumber(v);
            return std::isfinite(n) ? JsRound(n*k/step)*step : 0.0;
        };

        static const char* const PX[] = { "width", "x", "y", "blur", "spread", "size", "depth", "soft" };
        auto scaled = [&](const Json& e) -> Json {
            if (!e.IsObject())
                return e;
            Json o = e;
            for (const char* key : PX)
            {
                const Json* v = e.Get(key);
                if (v && std::isfinite(v->ToJsNumber()))
                    o[key] = JsRound(v->ToJsNumber()*k*2)/2;
            }
            return o;
        };

        Json style = Json::Object();
        auto copy = [&](const char* key) {
            if (const Json* v = raw.Get(key))
                style[key] = *v;
        };
        copy("name");
        String roleAnswered = JsString(raw.Get("role"));
        String r = TextRoles().Contains(roleAnswered) ? roleAnswered : String("label");
        style["role"] = r;
        copy("font");
        copy("weight");
        if (measured)
            style["size"] = cap/TEXT_METRICS.cap*k;
        else
            copy("size");
        copy("color");
        if (raw.Has("fill"))
            style["fill"] = *raw.Get("fill");
        else
            style["gradient"] = JsTruthy(raw.Get("gradient")) ? *raw.Get("gradient") : Json();

        const Json* effects = raw.Get("effects");
        if (effects && effects->IsArray())
        {
            Json list = Json::Array();
            for (auto& e : effects->Items())
            {
                Json o = scaled(e);
                if (o.IsObject())
                {
                    String type = JsToLower(JsString(o.Get("type")));
                    bool outline = ((std::string)type).find("stroke") != std::string::npos ||
                        ((std::string)type).find("outline") != std::string::npos;
                    double w = JsNumber(o.Get("width"));
                    if (outline && w > 0)
                        o["width"] = std::max(1.0, w);
                }
                list.Add(o);
            }
            style["effects"] = list;
        }
        else
        {
            const Json* st = raw.Get("stroke");
            st = st && st->IsObject() ? st : nullptr;
            const Json* sh = raw.Get("shadow");
            sh = sh && sh->IsObject() ? sh : nullptr;
            if (st && JsNumber(st->Get("width")) > 0)
            {
                Json s = Json::Object();
                if (const Json* c = st->Get("color"))
                    s["color"] = *c;
                s["width"] = std::max(1.0, px(st->Get("width")));
                style["stroke"] = s;
            }
            else
                style["stroke"] = Json();

            if (sh)
            {
                Json s = Json::Object();
                if (const Json* c = sh->Get("color"))
                    s["color"] = *c;
                s["x"] = px(sh->Get("x"));
                s["y"] = px(sh->Get("y"));
                s["blur"] = std::max(0.0, px(sh->Get("blur")));
                const Json* opacity = sh->Get("opacity");
                s["opacity"] = JsNullish(opacity) ? Json(0.5) : *opacity;
                style["shadow"] = s;
            }
            else
                style["shadow"] = Json();
        }
        style["letterSpacing"] = px(raw.Get("letterSpacing"), 0.5);
        const Json* upper = raw.Get("uppercase");
        style["uppercase"] = upper && upper->IsBool() && upper->Bool();
        copy("description");

        TextStyle normalized = NormalizeTextStyle(style, r);
        Json result = TextStyleToJson(normalized);
        Json extracted = Json::Object();
        extracted["name"] = normalized.name;
        extracted["role"] = normalized.role;
        String text = JsSlice(JsTrim(ReplaceSpaceRuns(JsStringOr(raw.Get("text")), " ")), 0, 40);
        if (!text.IsEmpty())
            extracted["text"] = text;
        result["extracted"] = extracted;
        out = ApplyStyleOverrides(result, name, role);
        return "style";
    }
}
