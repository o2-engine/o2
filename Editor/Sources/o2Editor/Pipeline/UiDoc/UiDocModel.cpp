#include "o2Editor/stdafx.h"
#include "UiDocModel.h"

#include <cmath>

#include "o2Editor/Pipeline/UiDoc/UiDocFp.h"

namespace Editor::UiDoc
{
    const Vector<String>& UiNodeTypes()
    {
        static const Vector<String> types = { "screen", "group", "panel", "background", "image", "icon", "button", "text",
                                              "progress", "toggle", "slider", "input", "tab", "badge", "divider", "slot" };
        return types;
    }

    const Vector<String>& NodeKeys()
    {
        static const Vector<String> keys = { "anchor", "x", "y", "w", "h", "margin", "grow",
                                             "layout", "gap", "gapY", "pad", "align", "justify", "cols", "cellW", "cellH",
                                             "kit", "variant", "style", "color", "opacity", "visible", "fit",
                                             "textAlign", "fontSize", "textColor", "value", "on", "placeholder", "note", "text" };
        return keys;
    }

    // --- generic access to the optional keys -----------------------------------------------------

    namespace
    {
        // The member an optional key is stored in: exactly one pointer is set
        struct FieldRef
        {
            std::optional<String>* s = nullptr;
            std::optional<double>* d = nullptr;
            std::optional<Box4>*   b = nullptr;
            std::optional<bool>*   f = nullptr;
        };

        FieldRef FieldOf(UiNode& n, const String& key)
        {
            FieldRef r;
            const std::string& k = key;
            if (k == "anchor") r.s = &n.anchor;
            else if (k == "x") r.d = &n.x;
            else if (k == "y") r.d = &n.y;
            else if (k == "w") r.d = &n.w;
            else if (k == "h") r.d = &n.h;
            else if (k == "margin") r.b = &n.margin;
            else if (k == "grow") r.d = &n.grow;
            else if (k == "layout") r.s = &n.layout;
            else if (k == "gap") r.d = &n.gap;
            else if (k == "gapY") r.d = &n.gapY;
            else if (k == "pad") r.b = &n.pad;
            else if (k == "align") r.s = &n.align;
            else if (k == "justify") r.s = &n.justify;
            else if (k == "cols") r.d = &n.cols;
            else if (k == "cellW") r.d = &n.cellW;
            else if (k == "cellH") r.d = &n.cellH;
            else if (k == "kit") r.s = &n.kit;
            else if (k == "variant") r.s = &n.variant;
            else if (k == "style") r.s = &n.style;
            else if (k == "color") r.s = &n.color;
            else if (k == "opacity") r.d = &n.opacity;
            else if (k == "visible") r.f = &n.visible;
            else if (k == "fit") r.s = &n.fit;
            else if (k == "text") r.s = &n.text;
            else if (k == "textAlign") r.s = &n.textAlign;
            else if (k == "fontSize") r.d = &n.fontSize;
            else if (k == "textColor") r.s = &n.textColor;
            else if (k == "value") r.d = &n.value;
            else if (k == "on") r.f = &n.on;
            else if (k == "placeholder") r.s = &n.placeholder;
            else if (k == "note") r.s = &n.note;
            return r;
        }

        Json Box4ToJson(const Box4& b)
        {
            Json out = Json::Array();
            for (double v : b)
                out.Add(Json(v));
            return out;
        }
    }

    bool UiNode::Has(const String& key) const
    {
        if (key == "children")
            return hasChildren;

        FieldRef r = FieldOf(const_cast<UiNode&>(*this), key);
        return (r.s && r.s->has_value()) || (r.d && r.d->has_value()) || (r.b && r.b->has_value()) || (r.f && r.f->has_value());
    }

    Json UiNode::Get(const String& key) const
    {
        FieldRef r = FieldOf(const_cast<UiNode&>(*this), key);
        if (r.s && *r.s) return Json(**r.s);
        if (r.d && *r.d) return Json(**r.d);
        if (r.b && *r.b) return Box4ToJson(**r.b);
        if (r.f && *r.f) return Json(**r.f);
        return Json();
    }

    void UiNode::Set(const String& key, const Json& cleaned)
    {
        FieldRef r = FieldOf(*this, key);
        if (r.s)
            *r.s = cleaned.ToJsString();
        else if (r.d)
            *r.d = cleaned.ToJsNumber();
        else if (r.f)
            *r.f = cleaned.IsBool() ? cleaned.Bool() : JsTruthy(&cleaned);
        else if (r.b)
        {
            Box4 box{ 0, 0, 0, 0 };
            for (int i = 0; i < 4 && i < cleaned.Count(); i++)
                box[i] = cleaned.At(i)->ToJsNumber();
            *r.b = box;
        }
        else
            return;

        if (!keys.Contains(key))
            keys.Add(key);
    }

    void UiNode::Unset(const String& key)
    {
        if (key == "children")
        {
            ClearChildren();
            return;
        }

        FieldRef r = FieldOf(*this, key);
        if (r.s) r.s->reset();
        else if (r.d) r.d->reset();
        else if (r.b) r.b->reset();
        else if (r.f) r.f->reset();
        keys.Remove(key);
    }

    void UiNode::SetChildren(const std::vector<UiNode>& kids)
    {
        children = kids;
        hasChildren = true;
        if (!keys.Contains("children"))
            keys.Add("children");
    }

    void UiNode::ClearChildren()
    {
        children.clear();
        hasChildren = false;
        keys.Remove("children");
    }

    UiDoc EmptyDoc(const String& name /*= "Screen"*/, double width /*= 1080*/, double height /*= 1920*/,
                   std::optional<double> dpi /*= std::nullopt*/)
    {
        UiDoc doc;
        doc.name = name;
        doc.width = width;
        doc.height = height;
        if (dpi)
            doc.dpi = CleanDpi(*dpi);
        // The web's empty screen has an empty children list ({id: "root", type: "screen", children: []})
        doc.root.SetChildren({});
        return doc;
    }

    UiSize DefaultSize(const String& type)
    {
        static const std::pair<const char*, UiSize> sizes[] = {
            { "screen", { 1080, 1920 } }, { "group", { 400, 300 } }, { "panel", { 600, 400 } }, { "background", { 1080, 1920 } },
            { "image", { 300, 300 } }, { "icon", { 72, 72 } }, { "button", { 360, 110 } }, { "text", { 400, 56 } },
            { "progress", { 480, 44 } }, { "toggle", { 120, 64 } }, { "slider", { 480, 48 } }, { "input", { 560, 96 } },
            { "tab", { 220, 88 } }, { "badge", { 52, 52 } }, { "divider", { 600, 6 } }, { "slot", { 160, 160 } },
        };
        for (auto& s : sizes)
        {
            if (type == s.first)
                return s.second;
        }
        return UiSize{ 400, 300 };
    }

    // --- anchors -----------------------------------------------------------------------------------

    Anchor ParseAnchor(const String& raw)
    {
        // String(raw).trim().toLowerCase().replace(/[\s_]+/g, '-')
        String lowered = JsToLower(JsTrim(raw));
        std::string s;
        bool inRun = false;
        for (char32_t c : Utf8ToCodePoints(lowered))
        {
            if (IsJsSpace(c) || c == '_')
            {
                if (!inRun)
                    s += '-';
                inRun = true;
                continue;
            }
            inRun = false;
            s += CodePointToUtf8(c).Data();
        }

        if (s.empty()) return Anchor{ "left", "top" };
        if (s == "stretch" || s == "fill" || s == "stretch-stretch") return Anchor{ "stretch", "stretch" };
        if (s == "center" || s == "centre" || s == "middle" || s == "middle-center") return Anchor{ "center", "middle" };
        if (s == "h-stretch" || s == "stretch-h" || s == "hstretch") return Anchor{ "stretch", "middle" };
        if (s == "v-stretch" || s == "stretch-v" || s == "vstretch") return Anchor{ "center", "stretch" };

        String h, v;
        Vector<String> pending;
        for (auto& p : JsSplit(String(s), "-"))
        {
            if (p.IsEmpty())
                continue;
            if (p == "top" || p == "bottom") v = p;
            else if (p == "middle") v = "middle";
            else if (p == "left" || p == "right") h = p;
            else pending.Add(p);
        }

        for (auto& p : pending)
        {
            if (p == "center" || p == "centre")
            {
                if (h.IsEmpty()) h = "center";
                else if (v.IsEmpty()) v = "middle";
            }
            else if (p == "stretch" || p == "fill")
            {
                // top-stretch: the edge is given, the other axis stretches
                if (!v.IsEmpty() && h.IsEmpty()) h = "stretch";
                else if (!h.IsEmpty() && v.IsEmpty()) v = "stretch";
                else if (h.IsEmpty()) h = "stretch";
                else if (v.IsEmpty()) v = "stretch";
            }
        }

        Anchor a;
        a.h = !h.IsEmpty() ? h : !v.IsEmpty() ? String("center") : String("left");
        a.v = !v.IsEmpty() ? v : !h.IsEmpty() ? String("middle") : String("top");
        return a;
    }

    Anchor ParseAnchor(const std::optional<String>& raw)
    {
        return ParseAnchor(raw ? *raw : String());
    }

    String AnchorName(const Anchor& a)
    {
        if (a.h == "stretch" && a.v == "stretch") return "stretch";
        if (a.h == "center" && a.v == "middle") return "center";
        return a.v + "-" + a.h;
    }

    const Vector<String>& AnchorPresets()
    {
        static const Vector<String> presets = {
            "top-left", "top-center", "top-right", "middle-left", "center", "middle-right", "bottom-left", "bottom-center",
            "bottom-right", "top-stretch", "middle-stretch", "bottom-stretch", "stretch-left", "stretch-center", "stretch-right",
            "stretch",
        };
        return presets;
    }

    // --- cleaning ------------------------------------------------------------------------------------

    bool ResolveType(const String& word, String& type, String& layout)
    {
        static const std::pair<const char*, std::pair<const char*, const char*>> aliases[] = {
            { "row", { "group", "row" } }, { "hbox", { "group", "row" } },
            { "column", { "group", "column" } }, { "col", { "group", "column" } }, { "vbox", { "group", "column" } },
            { "grid", { "group", "grid" } },
            { "container", { "group", "" } }, { "box", { "group", "" } }, { "div", { "group", "" } },
            { "label", { "text", "" } }, { "title", { "text", "" } }, { "bg", { "background", "" } }, { "backdrop", { "background", "" } },
            { "picture", { "image", "" } }, { "img", { "image", "" } }, { "bar", { "progress", "" } }, { "progressbar", { "progress", "" } },
            { "checkbox", { "toggle", "" } }, { "switch", { "toggle", "" } }, { "field", { "input", "" } }, { "textfield", { "input", "" } },
            { "cell", { "slot", "" } }, { "card", { "panel", "" } }, { "window", { "panel", "" } }, { "popup", { "panel", "" } },
            { "frame", { "panel", "" } }, { "separator", { "divider", "" } }, { "line", { "divider", "" } }, { "btn", { "button", "" } },
        };

        String w = JsToLower(JsTrim(word));
        if (UiNodeTypes().Contains(w))
        {
            type = w;
            layout = "";
            return true;
        }

        for (auto& a : aliases)
        {
            if (w == a.first)
            {
                type = a.second.first;
                layout = a.second.second;
                return true;
            }
        }
        return false;
    }

    static bool IsHexText(const String& s, int from, int count)
    {
        if (s.Length() != from + count)
            return false;
        for (int i = from; i < from + count; i++)
        {
            char c = s[i];
            if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F')))
                return false;
        }
        return true;
    }

    bool CleanColor(const Json* value, String& out)
    {
        String s = JsTrim(JsStringOr(value));
        if (s.Length() == 4 && s[0] == '#' && IsHexText(s, 1, 3))
            s = String("#") + s[1] + s[1] + s[2] + s[2] + s[3] + s[3];
        if (IsHexText(s, 0, 6))
            s = "#" + s;
        if (s.Length() == 7 && s[0] == '#' && IsHexText(s, 1, 6))
        {
            out = JsToUpper(s);
            return true;
        }
        return false;
    }

    String CleanId(const String& value)
    {
        String spaced = ReplaceSpaceRuns(JsTrim(value), "_");
        std::string out;
        for (char c : (const std::string&)spaced)
        {
            if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_' || c == '-')
                out += c;
        }
        return String(out.substr(0, 64));
    }

    String CleanId(const Json* value)
    {
        return CleanId(JsStringOr(value));
    }

    // fin: undefined / null / '' -> none, else Number(v) when finite
    static std::optional<double> Fin(const Json* v)
    {
        if (JsNullish(v) || (v->IsString() && v->Str().IsEmpty()))
            return std::nullopt;
        double n = v->ToJsNumber();
        return std::isfinite(n) ? std::optional<double>(n) : std::nullopt;
    }

    static double R2(double n)
    {
        return JsRound(n*100.0)/100.0;
    }

    // Numbers of a text split by spaces / commas
    static std::vector<double> NumbersOf(const String& text)
    {
        std::vector<double> out;
        std::string part;
        auto flush = [&]() {
            if (!part.empty())
                out.push_back(Json(String(part)).ToJsNumber());
            part.clear();
        };
        for (char c : (const std::string&)text)
        {
            if (c == ' ' || c == ',')
                flush();
            else
                part += c;
        }
        flush();
        return out;
    }

    std::optional<Box4> ParseBox4(const Json* v)
    {
        std::vector<double> arr;
        if (!v)
            return std::nullopt;
        if (v->IsArray())
        {
            for (auto& item : v->Items())
                arr.push_back(item.ToJsNumber());
        }
        else if (v->IsNumber())
            arr.push_back(v->Num());
        else if (v->IsString())
            arr = NumbersOf(v->Str());
        else if (v->IsObject())
        {
            auto pick = [&](std::initializer_list<const char*> names) {
                for (const char* name : names)
                {
                    const Json* x = JsGet(v, name);
                    if (!JsNullish(x))
                        return x->ToJsNumber();
                }
                return 0.0;
            };
            arr = { pick({ "top", "t", "y" }), pick({ "right", "r", "x" }), pick({ "bottom", "b", "y" }), pick({ "left", "l", "x" }) };
        }
        else
            return std::nullopt;

        if (arr.empty())
            return std::nullopt;
        for (double n : arr)
        {
            if (!std::isfinite(n))
                return std::nullopt;
        }

        double a = arr[0];
        double b = arr.size() > 1 ? arr[1] : a;
        double c = arr.size() > 2 ? arr[2] : a;
        double d = arr.size() > 3 ? arr[3] : b;
        Box4 out{ R2(a), R2(b), R2(c), R2(d) };
        if (out[0] == 0 && out[1] == 0 && out[2] == 0 && out[3] == 0)
            return std::nullopt;
        return out;
    }

    // left/top/right/bottom/middle/space-*/fill -> the align / justify words
    static String AlignAlias(const String& s)
    {
        if (s == "left" || s == "top") return "start";
        if (s == "right" || s == "bottom") return "end";
        if (s == "middle") return "center";
        if (s == "space-between") return "between";
        if (s == "space-around") return "around";
        if (s == "space-evenly") return "evenly";
        if (s == "fill") return "stretch";
        return s;
    }

    bool CleanProp(const String& key, const Json* v, Json& out)
    {
        const std::string& k = key;
        auto lowerText = [&]() { return JsToLower(JsStringOr(v)); };

        if (k == "anchor")
        {
            if (JsNullish(v) || (v->IsString() && v->Str().IsEmpty()))
                return false;
            out = Json(AnchorName(ParseAnchor(v->ToJsString())));
            return true;
        }
        if (k == "x" || k == "y" || k == "gap" || k == "gapY")
        {
            auto n = Fin(v);
            if (!n) return false;
            out = Json(R2(*n));
            return true;
        }
        if (k == "w" || k == "h" || k == "cellW" || k == "cellH")
        {
            auto n = Fin(v);
            if (!n) return false;
            out = Json(R2(std::max(0.0, *n)));
            return true;
        }
        if (k == "grow")
        {
            auto n = Fin(v);
            if (!n || *n <= 0) return false;
            out = Json(R2(*n));
            return true;
        }
        if (k == "margin" || k == "pad")
        {
            auto b = ParseBox4(v);
            if (!b) return false;
            out = Box4ToJson(*b);
            return true;
        }
        if (k == "layout")
        {
            String s = lowerText();
            if (s != "row" && s != "column" && s != "grid") return false;
            out = Json(s);
            return true;
        }
        if (k == "align")
        {
            String s = AlignAlias(lowerText());
            if (s != "start" && s != "center" && s != "end" && s != "stretch") return false;
            out = Json(s);
            return true;
        }
        if (k == "justify")
        {
            String s = AlignAlias(lowerText());
            if (s != "start" && s != "center" && s != "end" && s != "between" && s != "around" && s != "evenly") return false;
            out = Json(s);
            return true;
        }
        if (k == "cols")
        {
            auto n = Fin(v);
            if (!n) return false;
            out = Json(std::max(1.0, std::min(64.0, JsRound(*n))));
            return true;
        }
        if (k == "kit" || k == "style")
        {
            String id = CleanId(v);
            if (id.IsEmpty()) return false;
            out = Json(id);
            return true;
        }
        if (k == "variant")
        {
            String s = JsSlice(JsTrim(JsStringOr(v)), 0, 40);
            if (s.IsEmpty()) return false;
            out = Json(s);
            return true;
        }
        if (k == "color" || k == "textColor")
        {
            String c;
            if (!CleanColor(v, c)) return false;
            out = Json(c);
            return true;
        }
        if (k == "opacity")
        {
            auto n = Fin(v);
            if (!n) return false;
            out = Json(std::max(0.0, std::min(1.0, R2(*n))));
            return true;
        }
        if (k == "value")
        {
            auto n = Fin(v);
            if (!n) return false;
            out = Json(std::max(0.0, std::min(1.0, JsRound(*n*1000.0)/1000.0)));
            return true;
        }
        if (k == "visible" || k == "on")
        {
            if (JsNullish(v)) return false;
            bool on = (v->IsBool() && v->Bool()) || (v->IsNumber() && v->Num() == 1) ||
                (v->IsString() && (v->Str() == "true" || v->Str() == "1" || v->Str() == "yes"));
            out = Json(on);
            return true;
        }
        if (k == "fit")
        {
            String s = lowerText();
            if (s != "contain" && s != "cover" && s != "stretch") return false;
            out = Json(s);
            return true;
        }
        if (k == "textAlign")
        {
            String s = lowerText();
            if (s != "left" && s != "center" && s != "right") return false;
            out = Json(s);
            return true;
        }
        if (k == "fontSize")
        {
            auto n = Fin(v);
            if (!n) return false;
            out = Json(std::max(4.0, std::min(400.0, JsRound(*n))));
            return true;
        }
        if (k == "text" || k == "placeholder")
        {
            if (JsNullish(v)) return false;
            out = Json(JsSlice(v->ToJsString(), 0, 2000));
            return true;
        }
        if (k == "note")
        {
            String s = JsSlice(JsTrim(JsStringOr(v)), 0, 300);
            if (s.IsEmpty()) return false;
            out = Json(s);
            return true;
        }
        return false;
    }

    // An unused id from the base: the base itself, else base_2, base_3...
    static String FreshId(UiIdBook& book, const String& base)
    {
        String id = CleanId(base);
        if (id.IsEmpty())
            id = "el";
        if (!book.Has(id))
        {
            book.used[id] = true;
            return id;
        }

        int n = book.counters.find(id) != book.counters.end() ? book.counters[id] : 1;
        for (;;)
        {
            n++;
            String cand = id + "_" + JsNum(n);
            if (!book.Has(cand))
            {
                book.counters[id] = n;
                book.used[cand] = true;
                return cand;
            }
        }
    }

    // <type><n> for a block without an id
    static String AutoId(UiIdBook& book, const String& type)
    {
        String key = "#" + type;
        int n = (book.counters.find(key) != book.counters.end() ? book.counters[key] : 0) + 1;
        book.counters[key] = n;
        return FreshId(book, type + JsNum(n));
    }

    // Matches /^\s*(-?[\d.]+)\s*[xx,* ]\s*(-?[\d.]+)\s*$/i, the two numbers' texts
    static bool MatchSizeText(const String& text, String& first, String& second)
    {
        std::u32string s = Utf8ToCodePoints(text);
        size_t i = 0, n = s.size();
        auto skipSpace = [&]() { while (i < n && IsJsSpace(s[i])) i++; };
        auto number = [&](String& out) {
            size_t start = i;
            if (i < n && s[i] == '-')
                i++;
            size_t digits = i;
            while (i < n && ((s[i] >= '0' && s[i] <= '9') || s[i] == '.'))
                i++;
            if (i == digits)
                return false;
            out = CodePointsToUtf8(s.substr(start, i - start));
            return true;
        };

        skipSpace();
        if (!number(first))
            return false;

        size_t runStart = i;
        skipSpace();
        bool spaceInRun = false;
        for (size_t k = runStart; k < i; k++)
            spaceInRun = spaceInRun || s[k] == ' ';

        if (i < n && (s[i] == 'x' || s[i] == 'X' || s[i] == 0xD7 || s[i] == ',' || s[i] == '*'))
        {
            i++;
            skipSpace();
        }
        else if (!spaceInRun)
            return false;

        if (!number(second))
            return false;
        skipSpace();
        return i == n;
    }

    UiNode NormalizeNode(const Json& raw, UiIdBook& book, bool isRoot /*= false*/)
    {
        const Json empty = Json::Object();
        const Json& r = raw.IsObject() ? raw : empty;

        String type = "screen", impliedLayout;
        if (!isRoot)
        {
            if (!ResolveType(JsStringOr(JsGet(r, "type")), type, impliedLayout))
            {
                type = "group";
                impliedLayout = "";
            }
            if (type == "screen")
                type = "group";
        }

        UiNode out;
        out.type = type;
        if (isRoot)
        {
            book.used["root"] = true;
            out.id = "root";
        }
        else
        {
            const Json* idValue = JsGet(r, "id");
            if (JsNullish(idValue))
                idValue = JsGet(r, "name");
            out.id = !CleanId(idValue).IsEmpty() ? FreshId(book, JsString(idValue)) : AutoId(book, type);
        }

        Json src = r;
        if (!impliedLayout.IsEmpty() && !src.Has("layout"))
            src["layout"] = impliedLayout;

        // Friendly spellings a model (or a person) may use (`a ??= b`; values copied before src may grow)
        auto assignIfNullish = [&](const char* key, Json value) {
            if (JsNullish(JsGet(src, key)))
                src[key] = value;
        };
        auto copyOf = [&](const char* key) { return Json(*src.Get(key)); };
        if (src.Has("size") && (!src.Has("w") || !src.Has("h")))
        {
            const Json size = copyOf("size");
            String a, b;
            if (MatchSizeText(size.ToJsString(), a, b))
            {
                assignIfNullish("w", Json(Json(a).ToJsNumber()));
                assignIfNullish("h", Json(Json(b).ToJsNumber()));
            }
            else if (size.IsObject() || size.IsArray())
            {
                auto pick = [&](const char* k1, const char* k2) {
                    const Json* x = JsGet(size, k1);
                    return JsNullish(x) ? JsGet(size, k2) : x;
                };
                // A missing one assigns undefined, which cleans the same as leaving it
                if (const Json* w = pick("width", "w"); !JsNullish(w))
                    assignIfNullish("w", *w);
                if (const Json* h = pick("height", "h"); !JsNullish(h))
                    assignIfNullish("h", *h);
            }
        }
        if (src.Has("width")) assignIfNullish("w", copyOf("width"));
        if (src.Has("height")) assignIfNullish("h", copyOf("height"));
        if (src.Has("padding")) assignIfNullish("pad", copyOf("padding"));
        if (src.Has("spacing")) assignIfNullish("gap", copyOf("spacing"));
        if (src.Has("columns")) assignIfNullish("cols", copyOf("columns"));
        if (src.Has("label") && !src.Has("text"))
        {
            Json label = copyOf("label");
            src["text"] = label;
        }

        static const Vector<String> rootSkipped = { "anchor", "x", "y", "w", "h", "margin", "grow" };
        for (auto& key : NodeKeys())
        {
            if (isRoot && rootSkipped.Contains(key))
                continue;
            Json cleaned;
            if (CleanProp(key, src.Get(key), cleaned))
                out.Set(key, cleaned);
        }

        if (!out.layout || *out.layout != "grid")
        {
            out.Unset("cols");
            out.Unset("cellW");
            out.Unset("cellH");
            out.Unset("gapY");
        }
        if (!out.layout)
        {
            out.Unset("gap");
            out.Unset("pad");
            out.Unset("justify");
            out.Unset("align");
        }

        const Json* kids = JsGet(r, "children");
        if (kids && kids->IsArray() && kids->Count() > 0)
        {
            std::vector<UiNode> children;
            for (int i = 0; i < kids->Count() && i < 400; i++)
                children.push_back(NormalizeNode(*kids->At(i), book));
            out.SetChildren(children);
        }
        return out;
    }

    UiNode NormalizeNode(const Json& raw)
    {
        UiIdBook book;
        return NormalizeNode(raw, book);
    }

    UiDoc NormalizeDoc(const Json& raw)
    {
        const Json empty = Json::Object();
        const Json& r = raw.IsObject() ? raw : empty;
        UiDoc doc;
        String name = CleanId(JsGet(r, "name"));
        doc.name = name.IsEmpty() ? String("Screen") : name;

        auto side = [&](const char* key, double def) {
            double n = JsNumber(JsGet(r, key));
            return std::max(64.0, std::min(8192.0, JsRound(std::isnan(n) || n == 0 ? def : n)));
        };
        doc.width = side("width", DEFAULT_DOC_WIDTH);
        doc.height = side("height", DEFAULT_DOC_HEIGHT);

        UiIdBook book;
        const Json* root = JsGet(r, "root");
        if (JsNullish(root))
        {
            Json fallback = Json::Object();
            fallback["children"] = Json::Array();
            doc.root = NormalizeNode(fallback, book, true);
        }
        else
            doc.root = NormalizeNode(*root, book, true);

        doc.dpi = CleanDpi(JsGet(r, "dpi"));
        return doc;
    }
}
