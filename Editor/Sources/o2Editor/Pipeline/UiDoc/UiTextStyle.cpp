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

    const Vector<String>& TextEffectTypes()
    {
        static const Vector<String> types = { "stroke", "extrude", "shadow", "glow", "innerShadow", "bevel" };
        return types;
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

    static double Half(double v)
    {
        return JsRound(v*2)/2;
    }

    // The value when it is an object (not an array, not null)
    static const Json* Rec(const Json* v)
    {
        return v && v->IsObject() ? v : nullptr;
    }

    // a ?? b for members
    static const Json* Either(const Json* a, const Json* b)
    {
        return JsNullish(a) ? b : a;
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

    // Names a model may give an effect -> ours
    static String EffectAlias(const String& raw)
    {
        std::string key;
        for (char c : (const std::string&)JsToLower(raw))
        {
            if ((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9'))
                key += c;
        }

        static const std::pair<const char*, const char*> aliases[] = {
            { "stroke", "stroke" }, { "outline", "stroke" }, { "border", "stroke" }, { "contour", "stroke" },
            { "extrude", "extrude" }, { "extrusion", "extrude" }, { "depth", "extrude" }, { "3d", "extrude" }, { "lip", "extrude" },
            { "shadow", "shadow" }, { "dropshadow", "shadow" }, { "outershadow", "shadow" },
            { "glow", "glow" }, { "outerglow", "glow" }, { "halo", "glow" },
            { "innershadow", "innerShadow" }, { "inner", "innerShadow" }, { "innerglow", "innerShadow" }, { "inset", "innerShadow" },
            { "bevel", "bevel" }, { "emboss", "bevel" }, { "bevelemboss", "bevel" },
        };
        for (auto& a : aliases)
        {
            if (key == a.first)
                return a.second;
        }
        return "";
    }

    std::optional<TextEffect> NormalizeEffect(const Json& raw)
    {
        const Json* r = Rec(&raw);
        if (!r)
            return std::nullopt;

        String type = EffectAlias(JsStringOr(Either(JsGet(r, "type"), JsGet(r, "kind"))));
        if (type.IsEmpty())
            return std::nullopt;

        TextEffect e;
        e.type = type;
        const Json* off = JsGet(r, "off");
        const Json* enabled = JsGet(r, "enabled");
        e.off = (off && off->IsBool() && off->Bool()) || (enabled && enabled->IsBool() && !enabled->Bool());
        e.opacity = Num(JsGet(r, "opacity"), type == "stroke" ? 1 : type == "bevel" ? 0.6 : 0.5, 0, 1);
        if (type != "extrude" && e.opacity <= 0)
            return std::nullopt;

        if (type == "stroke")
        {
            e.width = Half(Num(Either(JsGet(r, "width"), JsGet(r, "size")), 2, 0, 60));
            e.color = Hex(JsGet(r, "color"), "#000000");
            return e.width > 0 ? std::optional<TextEffect>(e) : std::nullopt;
        }
        if (type == "extrude")
        {
            e.x = Half(Num(JsGet(r, "x"), 0, -80, 80));
            e.y = Half(Num(Either(JsGet(r, "y"), JsGet(r, "depth")), 4, -80, 80));
            e.color = Hex(JsGet(r, "color"), "#000000");
            return e.x != 0 || e.y != 0 ? std::optional<TextEffect>(e) : std::nullopt;
        }
        if (type == "shadow")
        {
            e.color = Hex(JsGet(r, "color"), "#000000");
            e.x = Half(Num(JsGet(r, "x"), 0, -120, 120));
            e.y = Half(Num(JsGet(r, "y"), 4, -120, 120));
            e.blur = Half(Num(JsGet(r, "blur"), 4, 0, 120));
            e.spread = Half(Num(JsGet(r, "spread"), 0, 0, 60));
            return e;
        }
        if (type == "glow")
        {
            e.size = Half(Num(Either(JsGet(r, "size"), JsGet(r, "blur")), 8, 0, 120));
            e.color = Hex(JsGet(r, "color"), "#FFFFFF");
            return e.size > 0 ? std::optional<TextEffect>(e) : std::nullopt;
        }
        if (type == "innerShadow")
        {
            e.color = Hex(JsGet(r, "color"), "#000000");
            e.x = Half(Num(JsGet(r, "x"), 0, -40, 40));
            e.y = Half(Num(JsGet(r, "y"), 0, -40, 40));
            e.blur = Half(Num(Either(JsGet(r, "blur"), JsGet(r, "size")), 3, 0, 60));
            return e;
        }

        e.depth = Half(Num(Either(JsGet(r, "depth"), JsGet(r, "size")), 2, 0, 40));
        if (!(e.depth > 0))
            return std::nullopt;
        e.highlight = Hex(JsGet(r, "highlight"), "#FFFFFF");
        e.shade = Hex(Either(JsGet(r, "shade"), JsGet(r, "shadow")), "#000000");
        e.soft = Half(Num(Either(JsGet(r, "soft"), JsGet(r, "blur")), 1, 0, 40));
        return e;
    }

    Vector<TextEffect> NormalizeEffects(const Json* raw)
    {
        Vector<TextEffect> out;
        if (!raw || !raw->IsArray())
            return out;

        for (auto& item : raw->Items())
        {
            auto fx = NormalizeEffect(item);
            if (fx && out.Count() < TEXT_EFFECTS_MAX)
                out.Add(*fx);
        }
        return out;
    }

    Json TextEffectToJson(const TextEffect& e)
    {
        Json out = Json::Object();
        out["type"] = e.type;
        if (e.type == "bevel")
        {
            out["highlight"] = e.highlight;
            out["shade"] = e.shade;
            out["depth"] = e.depth;
            out["soft"] = e.soft;
            out["opacity"] = e.opacity;
        }
        else
        {
            out["color"] = e.color;
            if (e.type == "stroke")
                out["width"] = e.width;
            else if (e.type == "glow")
                out["size"] = e.size;
            else
            {
                out["x"] = e.x;
                out["y"] = e.y;
                if (e.type == "shadow" || e.type == "innerShadow")
                    out["blur"] = e.blur;
                if (e.type == "shadow")
                    out["spread"] = e.spread;
            }
            if (e.type != "extrude")
                out["opacity"] = e.opacity;
        }
        if (e.off)
            out["off"] = true;
        return out;
    }

    std::optional<TextGradient> NormalizeGradient(const Json* raw)
    {
        const Json* r = Rec(raw);
        if (!r)
            return std::nullopt;

        const Json* list = JsGet(r, "stops");
        if (!list || !list->IsArray())
            list = JsGet(r, "colors");
        if (!list || !list->IsArray())
            return std::nullopt;

        TextGradient g;
        int count = std::min(list->Count(), GRADIENT_STOPS_MAX);
        for (int i = 0; i < count; i++)
        {
            const Json& s = list->Items()[i];
            const Json* o = Rec(&s);
            String color = Hex(o ? JsGet(o, "color") : &s, "");
            if (color.IsEmpty())
                continue;

            double even = count > 1 ? (double)i/(count - 1) : 0;
            const Json* at = o ? Either(Either(JsGet(o, "at"), JsGet(o, "pos")), JsGet(o, "offset")) : nullptr;
            double v = o ? Num(at, even, 0, 1) : even;
            g.stops.Add(GradientStop{ color, JsRound(v*1000)/1000 });
        }
        if (g.stops.Count() < 2)
            return std::nullopt;

        for (int i = 1; i < g.stops.Count(); i++)
            g.stops[i].at = std::max(g.stops[i].at, g.stops[i - 1].at);

        double angle = std::fmod(JsRound(Num(JsGet(r, "angle"), 90, -3600, 3600)), 360.0);
        if (angle < 0)
            angle += 360;
        g.angle = angle + 0.0;
        return g;
    }

    Json TextGradientToJson(const TextGradient& gradient)
    {
        Json out = Json::Object();
        out["angle"] = gradient.angle;
        Json stops = Json::Array();
        for (auto& s : gradient.stops)
        {
            Json stop = Json::Object();
            stop["color"] = s.color;
            stop["at"] = s.at;
            stops.Add(stop);
        }
        out["stops"] = stops;
        return out;
    }

    Vector<TextEffect> LiveEffects(const TextStyle& style)
    {
        return style.effects.FindAll([](const TextEffect& e) { return !e.off; });
    }

    TextStyle NormalizeTextStyle(const Json& raw, const String& fallbackName /*= "body"*/)
    {
        const Json empty = Json::Object();
        const Json& r = raw.IsObject() ? raw : (raw.IsArray() ? raw : empty);
        TextStyle d;
        TextStyle out;

        const Json* fontValue = JsGet(r, "font");
        out.font = FontOf(JsNullish(fontValue) ? String(d.font) : fontValue->ToJsString()).family;

        const Json* roleValue = JsGet(r, "role");
        out.role = TextRoles().Contains(JsString(roleValue)) ? JsString(roleValue) : d.role;

        const Json* effects = JsGet(r, "effects");
        if (effects && effects->IsArray())
            out.effects = NormalizeEffects(effects);
        else
        {
            Json list = Json::Array();
            const Json* st = Rec(JsGet(r, "stroke"));
            if (st && JsNumber(JsGet(st, "width")) > 0)
            {
                Json e = Json::Object();
                e["type"] = "stroke";
                if (const Json* c = JsGet(st, "color"))
                    e["color"] = *c;
                e["width"] = Num(JsGet(st, "width"), 2, 0, 40);
                list.Add(e);
            }
            if (const Json* sh = Rec(JsGet(r, "shadow")))
            {
                Json e = Json::Object();
                e["type"] = "shadow";
                if (const Json* c = JsGet(sh, "color"))
                    e["color"] = *c;
                e["x"] = Num(JsGet(sh, "x"), 0, -80, 80);
                e["y"] = Num(JsGet(sh, "y"), 4, -80, 80);
                e["blur"] = Num(JsGet(sh, "blur"), 4, 0, 60);
                const Json* opacity = JsGet(sh, "opacity");
                e["opacity"] = JsNullish(opacity) ? Json(0.5) : *opacity;
                list.Add(e);
            }
            out.effects = NormalizeEffects(&list);
        }

        out.color = Hex(JsGet(r, "color"), d.color);
        if (r.IsObject() && r.Has("fill"))
            out.fill = NormalizeGradient(JsGet(r, "fill"));
        else
        {
            const Json* gradient = JsGet(r, "gradient");
            String to = JsTruthy(gradient) ? Hex(gradient, "") : String();
            if (!to.IsEmpty())
                out.fill = TextGradient{ 90, { GradientStop{ out.color, 0 }, GradientStop{ to, 1 } } };
        }
        if (out.fill)
            out.color = out.fill->stops[0].color;

        for (auto& e : out.effects)
        {
            if (!e.off && e.type == "stroke")
            {
                out.stroke = TextStroke{ e.color, e.width };
                break;
            }
        }
        for (auto& e : out.effects)
        {
            if (!e.off && e.type == "shadow")
            {
                out.shadow = TextShadow{ e.color, e.x, e.y, e.blur, e.opacity };
                break;
            }
            if (!e.off && e.type == "glow")
            {
                out.shadow = TextShadow{ e.color, 0, 0, e.size, e.opacity };
                break;
            }
        }
        if (out.fill)
            out.gradient = out.fill->stops.Last().color;

        const Json* alignValue = JsGet(r, "align");
        String align = alignValue && alignValue->IsString() ? alignValue->Str() : String();
        out.align = align == "left" || align == "right" ? align : String("center");

        out.name = CleanStyleName(JsGet(r, "name"), fallbackName);
        out.weight = FaceOf(out.font, Num(JsGet(r, "weight"), d.weight, 100, 900)).weight;
        out.size = JsRound(Num(JsGet(r, "size"), d.size, 6, 400));
        out.letterSpacing = Num(JsGet(r, "letterSpacing"), 0, -20, 80);
        out.lineHeight = Num(JsGet(r, "lineHeight"), d.lineHeight, 0.6, 3);
        out.uppercase = JsTruthy(JsGet(r, "uppercase"));

        String desc = JsTrim(JsStringOr(JsGet(r, "description")));
        if (!desc.IsEmpty())
            out.description = JsSlice(desc, 0, 300);

        return out;
    }

    TextStyle MergeTextStyle(const TextStyle& current, const Json& raw)
    {
        Json merged = TextStyleToJson(current);
        merged.Remove("extracted");
        for (auto& member : raw.Members())
            merged[member.first] = member.second;
        merged["name"] = current.name;

        if (!(raw.Get("effects") && raw.Get("effects")->IsArray()))
        {
            Vector<Json> fx;
            for (auto& e : current.effects)
                fx.Add(TextEffectToJson(e));

            auto typeOf = [](const Json& e) { return JsStringOr(JsGet(e, "type")); };
            if (raw.Has("stroke"))
            {
                const Json* st = Rec(raw.Get("stroke"));
                Vector<Json> next;
                if (st && JsNumber(JsGet(st, "width")) > 0)
                {
                    Json e = Json::Object();
                    e["type"] = "stroke";
                    for (auto& m : st->Members())
                        e[m.first] = m.second;
                    next.Add(e);
                }
                for (auto& e : fx)
                {
                    if (typeOf(e) != "stroke")
                        next.Add(e);
                }
                fx = next;
            }
            if (raw.Has("shadow"))
            {
                const Json* sh = Rec(raw.Get("shadow"));
                Vector<Json> next;
                for (auto& e : fx)
                {
                    if (typeOf(e) != "shadow" && typeOf(e) != "glow")
                        next.Add(e);
                }
                if (sh)
                {
                    Json e = Json::Object();
                    e["type"] = "shadow";
                    for (auto& m : sh->Members())
                        e[m.first] = m.second;
                    next.Add(e);
                }
                fx = next;
            }

            Json list = Json::Array();
            for (auto& e : fx)
                list.Add(e);
            merged["effects"] = list;
        }

        if (!raw.Has("fill"))
        {
            if (raw.Has("gradient") || (raw.Has("color") && !current.fill))
                merged.Remove("fill");
            else if (raw.Has("color"))
                merged["fill"] = Json();
        }
        return NormalizeTextStyle(merged, current.name);
    }

    TextReach EffectsReach(const TextStyle& style)
    {
        auto fx = LiveEffects(style);
        double outline = 0;
        for (auto& e : fx)
            outline += e.type == "stroke" ? e.width : 0;

        double l = outline, t = outline, r = outline, b = outline;
        auto reach = [&](double x, double y, double more) {
            l = std::max(l, outline + more - x);
            r = std::max(r, outline + more + x);
            t = std::max(t, outline + more - y);
            b = std::max(b, outline + more + y);
        };

        double ex = 0, ey = 0;
        for (auto& e : fx)
        {
            if (e.type != "extrude")
                continue;
            reach(e.x, e.y, 0);
            if (std::fabs(e.x) + std::fabs(e.y) > std::fabs(ex) + std::fabs(ey))
            {
                ex = e.x;
                ey = e.y;
            }
        }
        for (auto& e : fx)
        {
            if (e.type == "shadow")
            {
                reach(e.x, e.y, e.spread + e.blur*1.5);
                reach(e.x + ex, e.y + ey, e.spread + e.blur*1.5);
            }
            else if (e.type == "glow")
            {
                reach(0, 0, e.size*1.75);
                reach(ex, ey, e.size*1.75);
            }
        }
        return TextReach{ std::ceil(l), std::ceil(t), std::ceil(r), std::ceil(b) };
    }

    TextStyle ScaleTextStyle(const TextStyle& style, double k)
    {
        if (!(k > 0) || std::fabs(k - 1) < 0.001)
            return style;

        auto px = [&](double v) { return JsRound(v*k*2)/2; };
        Json effects = Json::Array();
        for (auto e : style.effects)
        {
            if (e.type == "stroke")
                e.width = std::max(0.5, px(e.width));
            else if (e.type == "extrude")
            {
                e.x = px(e.x);
                e.y = px(e.y);
            }
            else if (e.type == "shadow")
            {
                e.x = px(e.x);
                e.y = px(e.y);
                e.blur = px(e.blur);
                e.spread = px(e.spread);
            }
            else if (e.type == "glow")
                e.size = std::max(0.5, px(e.size));
            else if (e.type == "innerShadow")
            {
                e.x = px(e.x);
                e.y = px(e.y);
                e.blur = px(e.blur);
            }
            else
            {
                e.depth = std::max(0.5, px(e.depth));
                e.soft = px(e.soft);
            }
            effects.Add(TextEffectToJson(e));
        }

        Json raw = TextStyleToJson(style);
        raw.Remove("extracted");
        raw["size"] = style.size*k;
        raw["letterSpacing"] = style.letterSpacing*k;
        raw["effects"] = effects;
        return NormalizeTextStyle(raw, style.name);
    }

    String StyleLook(const TextStyle& s)
    {
        Vector<String> parts;
        if (s.fill)
        {
            Vector<String> colors;
            for (auto& stop : s.fill->stops)
                colors.Add(stop.color);
            parts.Add("gradient " + Join(colors, "\xE2\x86\x92") +
                      (s.fill->angle != 90 ? " at " + JsNum(s.fill->angle) + "\xC2\xB0" : String()));
        }
        for (auto& e : LiveEffects(s))
        {
            if (e.type == "stroke")
                parts.Add("outline " + JsNum(e.width) + "px " + e.color);
            else if (e.type == "extrude")
                parts.Add("depth " + JsNum(JsRound(std::hypot(e.x, e.y))) + "px " + e.color);
            else if (e.type == "shadow")
                parts.Add(e.blur > 0 ? "soft shadow" : "hard shadow");
            else if (e.type == "glow")
                parts.Add("glow " + e.color);
            else if (e.type == "innerShadow")
                parts.Add("inner shadow");
            else
                parts.Add("bevel");
        }
        if (s.uppercase)
            parts.Add("UPPERCASE");
        return Join(parts, ", ");
    }

    TextEffect DefaultEffect(const String& type, const TextStyle& style)
    {
        auto u = [&](double k, double min = 1) { return std::max(min, JsRound(style.size*k)); };
        TextEffect e;
        e.type = type;
        if (type == "stroke")
        {
            bool second = style.effects.Any([](const TextEffect& x) { return x.type == "stroke"; });
            e.color = second ? "#FFFFFF" : "#000000";
            e.width = u(0.06, 2);
            e.opacity = 1;
        }
        else if (type == "extrude")
        {
            e.color = "#000000";
            e.x = 0;
            e.y = u(0.08, 2);
        }
        else if (type == "shadow")
        {
            e.color = "#000000";
            e.x = 0;
            e.y = u(0.06, 2);
            e.blur = u(0.08, 2);
            e.spread = 0;
            e.opacity = 0.6;
        }
        else if (type == "glow")
        {
            e.color = "#FFFFFF";
            e.size = u(0.15, 3);
            e.opacity = 0.7;
        }
        else if (type == "innerShadow")
        {
            e.color = "#000000";
            e.x = 0;
            e.y = -u(0.04);
            e.blur = u(0.04);
            e.opacity = 0.45;
        }
        else
        {
            e.type = "bevel";
            e.highlight = "#FFFFFF";
            e.shade = "#000000";
            e.depth = u(0.035);
            e.soft = u(0.02);
            e.opacity = 0.55;
        }
        return e;
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
        out["fill"] = style.fill ? TextGradientToJson(*style.fill) : Json();
        Json effects = Json::Array();
        for (auto& e : style.effects)
            effects.Add(TextEffectToJson(e));
        out["effects"] = effects;
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
        if (!style.extracted.IsNull())
            out["extracted"] = style.extracted;
        return out;
    }

    void WriteStyleLook(DataValue& config, const TextStyle& style)
    {
        Json json = TextStyleToJson(style);
        for (const char* key : { "color", "fill", "effects", "gradient", "stroke", "shadow" })
            SetConfigJson(config, key, *json.Get(key));
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
}
