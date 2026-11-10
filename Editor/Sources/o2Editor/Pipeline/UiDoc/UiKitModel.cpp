#include "o2Editor/stdafx.h"
#include "UiKitModel.h"

#include <cmath>
#include <set>

#include "o2Editor/Pipeline/UiDoc/UiDocFp.h"

namespace Editor::UiDoc
{
    const char* const UI_KIT_AI_DEFAULT_MODEL = "gpt-6.1-sol";
    const char* const UI_KIT_AI_DEFAULT_IMAGE_MODEL = "openai/gpt-5-image-mini";

    const Vector<String>& UiElementTypes()
    {
        static const Vector<String> types = { "background", "panel", "frame", "button", "icon", "image", "progressBar",
                                              "progressFill", "toggle", "slider", "sliderHandle", "input", "tab", "badge",
                                              "divider", "header", "slot", "other" };
        return types;
    }

    String UiElementHint(const String& type)
    {
        static const std::pair<const char*, const char*> hints[] = {
            { "background", "full-screen backdrop behind everything" },
            { "panel", "stretchable surface that holds content (window, card, popup body)" },
            { "frame", "border / outline drawn around content" },
            { "button", "pressable button surface (the label is drawn on top)" },
            { "icon", "small symbol kept at its aspect (coin, gear, heart, close X)" },
            { "image", "illustration or picture kept at its aspect" },
            { "progressBar", "track (background) of a progress / health / xp bar" },
            { "progressFill", "fill of a progress bar, cut to the value" },
            { "toggle", "on/off switch or checkbox" },
            { "slider", "slider track" },
            { "sliderHandle", "slider knob" },
            { "input", "text field box" },
            { "tab", "tab header" },
            { "badge", "small counter / notification bubble" },
            { "divider", "separator line" },
            { "header", "title ribbon or banner" },
            { "slot", "item cell of an inventory or a grid" },
            { "other", "anything else" },
        };
        for (auto& h : hints)
        {
            if (type == h.first)
                return h.second;
        }
        return "";
    }

    const Vector<String>& UiAssetTypes()
    {
        static const Vector<String> types = { "simple", "sliced", "tiled" };
        return types;
    }

    const Vector<String>& KitConfigUiOnly()
    {
        static const Vector<String> keys = { "kitSelected", "kitTab", "kitRemoveBg", "kitModel", "kitImageModel" };
        return keys;
    }

    Json NineToJson(const UiNine& nine)
    {
        Json out = Json::Object();
        out["left"] = nine.left;
        out["top"] = nine.top;
        out["right"] = nine.right;
        out["bottom"] = nine.bottom;
        return out;
    }

    Json KitElementToJson(const UiKitElement& e)
    {
        Json out = Json::Object();
        out["name"] = e.name;
        out["type"] = e.type;
        out["assetType"] = e.assetType;
        out["width"] = e.width;
        out["height"] = e.height;
        out["png"] = e.png;
        if (!e.source.IsNull())
            out["source"] = e.source;
        if (e.port)
            out["port"] = *e.port;
        if (e.dpi)
            out["dpi"] = *e.dpi;
        if (e.dpiFrom)
            out["dpiFrom"] = *e.dpiFrom;
        if (e.variant)
            out["variant"] = *e.variant;
        if (e.description)
            out["description"] = *e.description;
        if (e.nineSlice)
            out["nineSlice"] = NineToJson(*e.nineSlice);
        return out;
    }

    Json KitToJson(const UiKit& kit)
    {
        Json out = Json::Object();
        out["version"] = 1;
        out["name"] = kit.name;
        Json elements = Json::Array();
        for (auto& e : kit.elements)
            elements.Add(KitElementToJson(e));
        out["elements"] = elements;
        Json styles = Json::Array();
        for (auto& s : kit.textStyles)
            styles.Add(TextStyleToJson(s));
        out["textStyles"] = styles;
        return out;
    }

    String CleanKitName(const Json* value, const String& fallback)
    {
        String spaced = ReplaceSpaceRuns(JsTrim(JsStringOr(value)), "_");
        std::string s;
        for (char c : (const std::string&)spaced)
        {
            if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_' || c == '-')
                s += c;
        }

        size_t a = 0, b = s.size();
        while (a < b && (s[a] == '-' || s[a] == '_'))
            a++;
        while (b > a && (s[b - 1] == '-' || s[b - 1] == '_'))
            b--;
        s = s.substr(a, b - a);
        return s.empty() ? fallback : String(s);
    }

    String GuessElementType(const String& name)
    {
        const std::string n = JsToLower(name);
        auto has = [&](std::initializer_list<const char*> words) {
            for (const char* w : words)
            {
                if (n.find(w) != std::string::npos)
                    return true;
            }
            return false;
        };

        if (has({ "bg", "background", "backdrop", "\xD1\x84\xD0\xBE\xD0\xBD" })) return "background";
        if (has({ "btn", "button", "\xD0\xBA\xD0\xBD\xD0\xBE\xD0\xBF" })) return "button";
        if (has({ "fill" })) return "progressFill";
        if (has({ "progress", "bar", "health", "hp", "xp" })) return "progressBar";
        if (has({ "panel", "window", "popup", "card", "dialog", "\xD0\xBF\xD0\xB0\xD0\xBD\xD0\xB5\xD0\xBB",
                  "\xD0\xBE\xD0\xBA\xD0\xBD\xD0\xBE" })) return "panel";
        if (has({ "frame", "border", "\xD1\x80\xD0\xB0\xD0\xBC\xD0\xBA" })) return "frame";
        if (has({ "icon", "ico_", "coin", "gem", "heart", "star", "\xD0\xB8\xD0\xBA\xD0\xBE\xD0\xBD\xD0\xBA" })) return "icon";
        if (has({ "toggle", "checkbox", "switch" })) return "toggle";
        if (has({ "handle", "knob" })) return "sliderHandle";
        if (has({ "slider" })) return "slider";
        if (has({ "input", "field" })) return "input";
        if (has({ "tab" })) return "tab";
        if (has({ "badge" })) return "badge";
        if (has({ "divider", "separator", "line" })) return "divider";
        if (has({ "header", "ribbon", "banner", "title" })) return "header";
        if (has({ "slot", "cell", "tile" })) return "slot";
        return "other";
    }

    String DefaultAssetType(const String& type)
    {
        static const Vector<String> simple = { "background", "icon", "image", "sliderHandle", "badge", "other" };
        return simple.Contains(type) ? "simple" : "sliced";
    }

    std::optional<UiNine> NormalizeNine(const Json* value)
    {
        if (!JsTruthy(value) || !JsIsObjectLike(value))
            return std::nullopt;

        auto n = [&](const char* key) {
            double d = JsNumber(JsGet(value, key));
            return std::max(0.0, JsRound(std::isnan(d) || d == 0 ? 0.0 : d));
        };
        UiNine out{ n("left"), n("top"), n("right"), n("bottom") };
        if (out.left + out.top + out.right + out.bottom > 0)
            return out;
        return std::nullopt;
    }

    UiKit ParseKit(const String& json)
    {
        UiKit kit;
        if (json.IsEmpty())
            return kit;

        Json raw;
        if (!Json::Parse(json, raw))
            return kit;

        std::set<std::string> seen;
        const Json* elements = JsGet(raw, "elements");
        if (elements && elements->IsArray())
        {
            for (auto& e : elements->Items())
            {
                const Json* png = JsGet(e, "png");
                if (!JsTruthy(&e) || !png || !png->IsString() || png->Str().IsEmpty())
                    continue;

                UiKitElement el;
                String name = CleanKitName(JsGet(e, "name"), "element_" + JsNum(kit.elements.Count() + 1));
                while (seen.count(JsToLower(name)))
                    name += "_2";
                seen.insert(JsToLower(name));
                el.name = name;

                const Json* type = JsGet(e, "type");
                el.type = type && type->IsString() && UiElementTypes().Contains(type->Str()) ? type->Str() : GuessElementType(name);
                const Json* assetType = JsGet(e, "assetType");
                el.assetType = assetType && assetType->IsString() && UiAssetTypes().Contains(assetType->Str()) ? assetType->Str()
                                                                                                               : DefaultAssetType(el.type);

                auto size = [&](const char* key) {
                    double d = JsNumber(JsGet(e, key));
                    return std::max(1.0, JsRound(std::isnan(d) || d == 0 ? 1.0 : d));
                };
                el.width = size("width");
                el.height = size("height");
                el.png = png->Str();

                if (JsTruthy(JsGet(e, "variant")))
                    el.variant = JsSlice(JsString(JsGet(e, "variant")), 0, 40);
                if (JsTruthy(JsGet(e, "description")))
                    el.description = JsSlice(JsString(JsGet(e, "description")), 0, 300);

                auto nine = NormalizeNine(JsGet(e, "nineSlice"));
                if (nine && el.assetType != "simple")
                    el.nineSlice = nine;

                const Json* source = JsGet(e, "source");
                if (JsTruthy(source) && JsIsObjectLike(source))
                    el.source = *source;

                const Json* port = JsGet(e, "port");
                if (port && port->IsString())
                    el.port = port->Str();

                if (auto dpi = CleanDpi(JsGet(e, "dpi")))
                    el.dpi = dpi;

                const Json* dpiFrom = JsGet(e, "dpiFrom");
                if (dpiFrom && dpiFrom->IsString() && (dpiFrom->Str() == "item" || dpiFrom->Str() == "input" || dpiFrom->Str() == "base"))
                    el.dpiFrom = dpiFrom->Str();

                kit.elements.Add(el);
            }
        }

        std::set<std::string> styleSeen;
        const Json* styles = JsGet(raw, "textStyles");
        if (styles && styles->IsArray())
        {
            for (auto& s : styles->Items())
            {
                TextStyle st = NormalizeTextStyle(s, "style_" + JsNum(kit.textStyles.Count() + 1));
                while (styleSeen.count(JsToLower(st.name)))
                    st.name += "_2";
                styleSeen.insert(JsToLower(st.name));
                kit.textStyles.Add(st);
            }
        }

        const Json* name = JsGet(raw, "name");
        String kitName = JsSlice(JsNullish(name) ? String("kit") : name->ToJsString(), 0, 80);
        kit.name = kitName.IsEmpty() ? String("kit") : kitName;
        return kit;
    }

    Vector<Pair<String, UiKitItem>> KitItems(const Json& config)
    {
        Vector<Pair<String, UiKitItem>> out;
        const Json* items = JsGet(config, "items");
        if (!items || !items->IsObject())
            return out;

        for (auto& member : items->Members())
        {
            const Json& v = member.second;
            UiKitItem item;
            item.raw = v;
            auto str = [&](const char* key) { const Json* x = JsGet(v, key); return JsTruthy(x) ? x->ToJsString() : String(); };
            item.name = str("name");
            item.type = str("type");
            item.assetType = str("assetType");
            item.variant = str("variant");
            item.description = str("description");
            if (const Json* nine = JsGet(v, "nineSlice"))
                item.nineSlice = *nine;
            item.bg = str("bg");
            const Json* trim = JsGet(v, "trim");
            item.trim = !(trim && trim->IsBool() && !trim->Bool());
            const Json* optimize = JsGet(v, "optimize");
            item.optimize = !(optimize && optimize->IsBool() && !optimize->Bool());
            if (const Json* prepared = JsGet(v, "prepared"); prepared && prepared->IsObject())
            {
                item.preparedUploadId = JsStringOr(JsGet(prepared, "uploadId"));
                item.preparedFrom = JsStringOr(JsGet(prepared, "from"));
            }
            item.ai = JsTruthy(JsGet(v, "ai"));
            item.nineFrom = str("nineFrom");
            item.dpi = CleanDpi(JsGet(v, "dpi"));
            out.Add(Pair<String, UiKitItem>(member.first, item));
        }
        return out;
    }

    String KitSummary(const UiKit& kit, std::optional<double> layoutDpi /*= std::nullopt*/)
    {
        Vector<String> lines;
        if (kit.elements.IsEmpty() && kit.textStyles.IsEmpty())
            return "(the UI kit is empty \xE2\x80\x94 draw with line art only)";

        if (!kit.elements.IsEmpty())
        {
            lines.Add("Graphics (kit=<name>; sizes in screen design pixels):");
            for (auto& e : kit.elements)
            {
                double k = (layoutDpi ? *layoutDpi : UI_BASE_DPI)/GraphicDpi(e.dpi);
                auto r = [&](double v) { return JsNum(JsRound(v*k)); };
                String nine = e.nineSlice ? " nine-slice corners " + r(e.nineSlice->left) + "," + r(e.nineSlice->top) + "," +
                                                r(e.nineSlice->right) + "," + r(e.nineSlice->bottom)
                                          : String();
                String own = std::fabs(k - 1) > 0.01 ? " (picture " + JsNum(e.width) + "x" + JsNum(e.height) + " px at " +
                                                            JsNum(GraphicDpi(e.dpi)) + " dpi)"
                                                      : String();
                lines.Add("- " + e.name + ": " + e.type + (e.variant && !e.variant->IsEmpty() ? "/" + *e.variant : String()) + ", " +
                          e.assetType + ", " + r(e.width) + "x" + r(e.height) + own + nine +
                          (e.description && !e.description->IsEmpty() ? " \xE2\x80\x94 " + *e.description : String()));
            }
        }

        if (!kit.textStyles.IsEmpty())
        {
            lines.Add("Text styles (style=<name>):");
            for (auto& s : kit.textStyles)
            {
                Vector<String> fx;
                if (s.stroke)
                    fx.Add("outline " + JsNum(s.stroke->width) + "px " + s.stroke->color);
                if (s.shadow)
                    fx.Add("shadow");
                if (s.gradient)
                    fx.Add("gradient to " + *s.gradient);
                if (s.uppercase)
                    fx.Add("UPPERCASE");
                String fxText = Join(fx, ", ");
                lines.Add("- " + s.name + ": " + s.role + ", " + s.font + " " + JsNum(s.weight) + " " + JsNum(s.size) + "px " + s.color +
                          (fxText.IsEmpty() ? String() : ", " + fxText) +
                          (s.description && !s.description->IsEmpty() ? " \xE2\x80\x94 " + *s.description : String()));
            }
        }
        return Join(lines, "\n");
    }

    const UiKitElement* KitElement(const UiKit* kit, const String& name)
    {
        if (!kit || name.IsEmpty())
            return nullptr;

        String n = JsToLower(name);
        for (auto& e : kit->elements)
        {
            if (JsToLower(e.name) == n)
                return &e;
        }
        return nullptr;
    }

    const TextStyle* KitStyle(const UiKit* kit, const String& name)
    {
        if (!kit || name.IsEmpty())
            return nullptr;

        String n = JsToLower(name);
        for (auto& s : kit->textStyles)
        {
            if (JsToLower(s.name) == n)
                return &s;
        }
        return nullptr;
    }

    // Words that say what a block IS, not which one: they match every graphic of a kind
    static bool IsGenericWord(const String& w)
    {
        static const std::set<std::string> generic = {
            "icon", "icons", "ico", "btn", "button", "buttons", "img", "image", "picture", "pic", "panel", "frame", "bg", "background",
            "label", "text", "title", "slot", "cell", "badge", "item", "box", "container", "group", "row", "column", "col", "grid",
            "bar", "fill", "track", "tab", "inset", "symbol", "small", "big", "large", "main", "primary", "secondary", "default",
            "element", "kit", "ui", "the", "and",
        };
        return generic.count(w) > 0;
    }

    // s.toLowerCase().split(/[^a-z0-9 cyrillic a-ya yo]+/) without short, generic and all-digit words; each word as code points
    static Vector<std::u32string> Words(const String& s)
    {
        Vector<std::u32string> out;
        std::u32string word;
        auto flush = [&]() {
            if (word.size() >= 3)
            {
                bool digits = true;
                for (char32_t c : word)
                    digits = digits && c >= '0' && c <= '9';
                if (!digits && !IsGenericWord(CodePointsToUtf8(word)))
                    out.Add(word);
            }
            word.clear();
        };

        for (char32_t c : Utf8ToCodePoints(JsToLower(s)))
        {
            bool keep = (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || (c >= 0x430 && c <= 0x44F) || c == 0x451;
            if (keep)
                word.push_back(c);
            else
                flush();
        }
        flush();
        return out;
    }

    // Equal, or one starts with the other and both are at least 4 long
    static bool Like(const std::u32string& x, const std::u32string& y)
    {
        if (x == y)
            return true;
        if (std::min(x.size(), y.size()) < 4)
            return false;
        return x.compare(0, y.size(), y) == 0 || y.compare(0, x.size(), x) == 0;
    }

    int NameScore(const String& id, const String& elementName, const String& elementDescription /*= ""*/)
    {
        auto a = Words(id);
        if (a.IsEmpty())
            return 0;

        auto b = Words(elementName);
        auto d = Words(elementDescription);
        int score = 0;
        for (auto& w : a)
        {
            if (b.Any([&](const std::u32string& x) { return Like(w, x); }))
                score += 2;
            else if (d.Any([&](const std::u32string& x) { return Like(w, x); }))
                score += 1;
        }
        return score;
    }

    int NameScore(const String& id, const UiKitElement& element)
    {
        return NameScore(id, element.name, element.description ? *element.description : String());
    }

    const UiKitElement* KitElementOfType(const UiKit* kit, const Vector<String>& types, const String& variant /*= ""*/,
                                         const String& hint /*= ""*/)
    {
        if (!kit)
            return nullptr;

        const String* first = nullptr;
        for (auto& t : types)
        {
            if (kit->elements.Any([&](const UiKitElement& e) { return e.type == t; }))
            {
                first = &t;
                break;
            }
        }
        if (!first)
            return nullptr;

        Vector<const UiKitElement*> pool;
        for (auto& e : kit->elements)
        {
            if (e.type == *first)
                pool.Add(&e);
        }

        String v = JsToLower(variant);
        if (!v.IsEmpty())
        {
            for (auto e : pool)
            {
                if ((e->variant && JsToLower(*e->variant) == v) || ((std::string)JsToLower(e->name)).find(v) != std::string::npos)
                    return e;
            }
        }

        if (!hint.IsEmpty())
        {
            const UiKitElement* best = nullptr;
            int bestScore = 0;
            for (auto e : pool)
            {
                int sc = NameScore(hint, *e);
                if (sc > bestScore)
                {
                    best = e;
                    bestScore = sc;
                }
            }
            if (best)
                return best;
        }

        for (auto e : pool)
        {
            if (!e->variant || e->variant->IsEmpty())
                return e;
            String lv = JsToLower(*e->variant);
            const std::string& s = lv;
            if (s.find("primary") != std::string::npos || s.find("normal") != std::string::npos || s.find("default") != std::string::npos)
                return e;
        }
        return pool[0];
    }

    String KitFileName(const UiKitElement& element)
    {
        return element.name + ".png";
    }
}
