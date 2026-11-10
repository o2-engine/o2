#include "o2Editor/stdafx.h"
#include "UiDocUiml.h"

#include <cmath>
#include <optional>

#include "o2Editor/Pipeline/UiDoc/UiDocUimlText.h"
#include "o2Editor/Pipeline/UiDoc/UiDocFp.h"

namespace Editor::UiDoc
{
    using namespace UimlText;

    // --- tokens --------------------------------------------------------------------------------------

    // s.replace(/\\(["\\n])/g, ...): \" -> ", \\ -> \, \n -> newline
    static std::u32string Unescape(const std::u32string& s)
    {
        std::u32string out;
        for (size_t i = 0; i < s.size(); i++)
        {
            if (s[i] == '\\' && i + 1 < s.size() && (s[i + 1] == '"' || s[i + 1] == '\\' || s[i + 1] == 'n'))
            {
                out.push_back(s[i + 1] == 'n' ? U'\n' : s[i + 1]);
                i++;
                continue;
            }
            out.push_back(s[i]);
        }
        return out;
    }

    Vector<UimlToken> TokenizeUiml(const String& text)
    {
        Vector<UimlToken> out;
        std::u32string line = Utf8ToCodePoints(text);
        size_t i = 0, n = line.size();

        auto readQuoted = [&]() {
            char32_t q = line[i];
            i++;
            std::u32string s;
            while (i < n && line[i] != q)
            {
                if (line[i] == '\\' && i + 1 < n)
                {
                    s.push_back(line[i]);
                    s.push_back(line[i + 1]);
                    i += 2;
                    continue;
                }
                s.push_back(line[i++]);
            }
            i++; // closing quote (or end of line)
            return CodePointsToUtf8(Unescape(s));
        };

        while (i < n)
        {
            while (i < n && IsJsSpace(line[i]))
                i++;
            if (i >= n)
                break;
            if (line[i] == '#')
                break; // a comment to the end of the line

            if (line[i] == '"' || line[i] == 0x201C || line[i] == 0xAB)
            {
                if (line[i] != '"')
                {
                    char32_t close = line[i] == 0x201C ? 0x201D : 0xBB;
                    i++;
                    size_t end = line.find(close, i);
                    std::u32string s = line.substr(i, (end == std::u32string::npos ? n : end) - i);
                    i = end == std::u32string::npos ? n : end + 1;
                    out.Add(UimlToken{ "", CodePointsToUtf8(s), true });
                    continue;
                }
                out.Add(UimlToken{ "", readQuoted(), true });
                continue;
            }

            std::u32string word;
            while (i < n && !IsJsSpace(line[i]) && line[i] != '=')
                word.push_back(line[i++]);

            if (i < n && line[i] == '=')
            {
                i++;
                if (i < n && line[i] == '"')
                    out.Add(UimlToken{ CodePointsToUtf8(word), readQuoted(), true });
                else
                {
                    std::u32string v;
                    while (i < n && !IsJsSpace(line[i]))
                        v.push_back(line[i++]);
                    out.Add(UimlToken{ CodePointsToUtf8(word), CodePointsToUtf8(v), false });
                }
            }
            else
                out.Add(UimlToken{ "", CodePointsToUtf8(word), false });
        }
        return out;
    }

    // A key alias (width -> w...) by the key's lower case; the key as written when it has none
    String UimlText::AliasKey(const String& key)
    {
        static const std::pair<const char*, const char*> aliases[] = {
            { "width", "w" }, { "height", "h" }, { "padding", "pad" }, { "spacing", "gap" }, { "columns", "cols" }, { "label", "text" },
            { "textalign", "textAlign" }, { "fontsize", "fontSize" }, { "textcolor", "textColor" }, { "gapy", "gapY" },
            { "cellw", "cellW" }, { "cellh", "cellH" }, { "text-align", "textAlign" }, { "font-size", "fontSize" },
            { "text-color", "textColor" },
        };
        String lower = JsToLower(key);
        for (auto& a : aliases)
        {
            if (lower == a.first)
                return a.second;
        }
        return key;
    }

    // Splits on any of the code points
    Vector<String> UimlText::SplitOn(const String& text, const std::u32string& separators)
    {
        Vector<String> out;
        std::u32string part;
        for (char32_t c : Utf8ToCodePoints(text))
        {
            char32_t lc = c >= 'A' && c <= 'Z' ? c + 32 : c;
            if (separators.find(lc) != std::u32string::npos)
            {
                out.Add(CodePointsToUtf8(part));
                part.clear();
            }
            else
                part.push_back(c);
        }
        out.Add(CodePointsToUtf8(part));
        return out;
    }

    static std::optional<Json> NumberAt(const Vector<String>& parts, int i)
    {
        if (i >= parts.Count())
            return std::nullopt;
        return Json(Json(parts[i]).ToJsNumber());
    }

    // A key=value from the text -> node fields (several for the shorthands)
    UimlProps UimlText::PropsOf(const String& key, const String& value)
    {
        String k = AliasKey(key);
        UimlProps out;
        auto add = [&](const char* name, const std::optional<Json>& v) { out.Add(Pair<String, std::optional<Json>>(name, v)); };
        auto pair = [&]() { return SplitOn(value, U"x\u00D7,*"); };

        if (k == "size")
        {
            auto p = pair();
            add("w", NumberAt(p, 0));
            add("h", NumberAt(p, 1));
        }
        else if (k == "pos" || k == "position" || k == "offset")
        {
            auto p = pair();
            add("x", NumberAt(p, 0));
            add("y", NumberAt(p, 1));
        }
        else if (k == "cell")
        {
            auto p = pair();
            add("cellW", NumberAt(p, 0));
            add("cellH", p.Count() > 1 ? NumberAt(p, 1) : NumberAt(p, 0));
        }
        else if (k == "gap")
        {
            auto parts = JsSplit(value, ",");
            if (parts.Count() > 1)
            {
                add("gap", NumberAt(parts, 0));
                add("gapY", NumberAt(parts, 1));
            }
            else
                add("gap", NumberAt(parts, 0));
        }
        else if (k == "margin" || k == "pad")
        {
            Json arr = Json::Array();
            for (auto& part : SplitOn(value, U" ,"))
            {
                if (!part.IsEmpty())
                    arr.Add(Json(Json(part).ToJsNumber()));
            }
            out.Add(Pair<String, std::optional<Json>>(k, arr));
        }
        else
            out.Add(Pair<String, std::optional<Json>>(k, Json(value)));
        return out;
    }

    // Object.assign(target, props) with undefined values dropping the key
    void UimlText::AssignProps(Json& target, const UimlProps& props)
    {
        for (auto& p : props)
        {
            if (p.second)
                target[p.first] = *p.second;
            else
                target.Remove(p.first);
        }
    }

    // --- parsing a tree --------------------------------------------------------------------------------

    namespace
    {
        // A block read from a line with the blocks indented under it
        struct RawNode
        {
            Json                 props;
            std::vector<RawNode> kids;

            Json ToJson() const
            {
                Json out = props;
                if (!kids.empty())
                {
                    Json list = Json::Array();
                    for (auto& k : kids)
                        list.Add(k.ToJson());
                    out["children"] = list;
                }
                return out;
            }
        };
    }

    Vector<UimlLine> UimlText::LinesOf(const String& src)
    {
        Vector<UimlLine> out;
        std::string normalized;
        const std::string& s = src;
        for (size_t i = 0; i < s.size(); i++)
        {
            if (s[i] == '\r')
            {
                normalized += '\n';
                if (i + 1 < s.size() && s[i + 1] == '\n')
                    i++;
            }
            else
                normalized += s[i];
        }

        int no = 0;
        for (auto& raw : JsSplit(String(normalized), "\n"))
        {
            no++;
            String expanded = raw;
            expanded.ReplaceAll("\t", "  ");
            String text = JsTrim(expanded);
            if (text.IsEmpty() || StartsWith(text, "#") || StartsWith(text, "```"))
                continue;

            int indent = 0;
            for (char32_t c : Utf8ToCodePoints(expanded))
            {
                if (!IsJsSpace(c))
                    break;
                indent++;
            }
            out.Add(UimlLine{ indent, text, no });
        }
        return out;
    }

    // /^\d+\s*[xx]\s*\d+$/i: the two numbers
    bool UimlText::MatchDims(const String& text, double& w, double& h)
    {
        std::u32string s = Utf8ToCodePoints(text);
        size_t i = 0, n = s.size();
        auto digits = [&](double& out) {
            size_t start = i;
            while (i < n && s[i] >= '0' && s[i] <= '9')
                i++;
            if (i == start)
                return false;
            out = Json(CodePointsToUtf8(s.substr(start, i - start))).ToJsNumber();
            return true;
        };
        if (!digits(w))
            return false;
        while (i < n && IsJsSpace(s[i]))
            i++;
        if (i >= n || !(s[i] == 'x' || s[i] == 'X' || s[i] == 0xD7))
            return false;
        i++;
        while (i < n && IsJsSpace(s[i]))
            i++;
        return digits(h) && i == n;
    }

    // A block line -> its raw fields (not normalised yet); false when the first word is no type
    static bool NodeOfLine(const String& text, Json& node)
    {
        auto toks = TokenizeUiml(text);
        if (toks.IsEmpty() || !toks[0].key.IsEmpty() || toks[0].quoted)
            return false;

        String type, layout;
        String first = JsToLower(toks[0].value);
        if (!ResolveType(toks[0].value, type, layout) && first != "screen")
            return false;

        node = Json::Object();
        node["type"] = first;
        int start = 1;
        if (toks.Count() > 1 && toks[1].key.IsEmpty() && !toks[1].quoted)
        {
            node["id"] = toks[1].value;
            start = 2;
        }

        Vector<String> texts;
        for (int t = start; t < toks.Count(); t++)
        {
            auto& tk = toks[t];
            double w, h;
            if (!tk.key.IsEmpty())
                AssignProps(node, PropsOf(tk.key, tk.value));
            else if (tk.quoted)
                texts.Add(tk.value);
            else if (JsToLower(tk.value) == "hidden")
                node["visible"] = false;
            else if (MatchDims(tk.value, w, h))
            {
                node["w"] = w;
                node["h"] = h;
            }
        }
        if (!texts.IsEmpty())
            node["text"] = Join(texts, " ");
        return true;
    }

    // Indented lines -> a forest of raw blocks
    static std::vector<RawNode> TreeOf(const Vector<UimlLine>& lines, Vector<String>& errors)
    {
        std::vector<RawNode> roots;
        std::vector<std::pair<int, RawNode*>> stack;
        for (auto& ln : lines)
        {
            Json node;
            if (!NodeOfLine(ln.text, node))
            {
                errors.Add("line " + JsNum(ln.no) + ": \"" + JsSlice(ln.text, 0, 60) +
                           "\" is not a block (a line starts with its type)");
                continue;
            }

            while (!stack.empty() && stack.back().first >= ln.indent)
                stack.pop_back();

            // Only the top of the stack grows: the deeper entries were popped, so no pointer on the stack moves
            RawNode* added;
            if (!stack.empty())
            {
                stack.back().second->kids.push_back(RawNode{ node, {} });
                added = &stack.back().second->kids.back();
            }
            else
            {
                roots.push_back(RawNode{ node, {} });
                added = &roots.back();
            }
            stack.push_back({ ln.indent, added });
        }
        return roots;
    }

    UimlParseResult ParseUiml(const String& src, const UiDoc* base /*= nullptr*/)
    {
        UimlParseResult result;
        auto roots = TreeOf(LinesOf(src), result.errors);
        bool isScreen = roots.size() == 1 && JsString(roots[0].props.Get("type")) == "screen";
        UiDoc fallback = base ? EmptyDoc(base->name, base->width, base->height, base->dpi) : EmptyDoc();

        if (!isScreen)
        {
            Json raw = Json::Object();
            raw["name"] = fallback.name;
            raw["width"] = fallback.width;
            raw["height"] = fallback.height;
            if (fallback.dpi)
                raw["dpi"] = *fallback.dpi;
            Json root = Json::Object();
            Json kids = Json::Array();
            for (auto& r : roots)
                kids.Add(r.ToJson());
            root["children"] = kids;
            raw["root"] = root;
            result.doc = NormalizeDoc(raw);
            return result;
        }

        Json screen = roots[0].ToJson();
        String name = CleanId(screen.Get("id"));
        double width = JsNumber(screen.Get("w")), height = JsNumber(screen.Get("h")), dpi = JsNumber(screen.Get("dpi"));
        screen.Remove("dpi");
        screen["id"] = "root";

        Json raw = Json::Object();
        raw["name"] = name.IsEmpty() ? fallback.name : name;
        raw["width"] = std::isnan(width) || width == 0 ? fallback.width : width;
        raw["height"] = std::isnan(height) || height == 0 ? fallback.height : height;
        if (!(std::isnan(dpi) || dpi == 0))
            raw["dpi"] = dpi;
        else if (fallback.dpi)
            raw["dpi"] = *fallback.dpi;
        raw["root"] = screen;
        result.doc = NormalizeDoc(raw);
        return result;
    }

    std::vector<UiNode> ParseUimlNodes(const String& src, UiIdBook& book, Vector<String>& errors)
    {
        book.counters.clear();
        std::vector<UiNode> out;
        for (auto& r : TreeOf(LinesOf(src), errors))
            out.push_back(NormalizeNode(r.ToJson(), book));
        return out;
    }

    // --- writing --------------------------------------------------------------------------------------

    static String Num(double n)
    {
        return JsNum(JsRound(n*100.0)/100.0);
    }

    String UimlText::Quote(const String& s)
    {
        std::string out = "\"";
        for (char c : (const std::string&)s)
        {
            if (c == '\\') out += "\\\\";
            else if (c == '"') out += "\\\"";
            else if (c == '\n') out += "\\n";
            else out += c;
        }
        out += "\"";
        return String(out);
    }

    static String Word(const String& s)
    {
        if (s.IsEmpty())
            return Quote(s);
        for (char32_t c : Utf8ToCodePoints(s))
        {
            if (IsJsSpace(c) || c == '"' || c == '=' || c == '#')
                return Quote(s);
        }
        return s;
    }

    static String Box4Text(const Box4& b)
    {
        double t = b[0], r = b[1], bo = b[2], l = b[3];
        if (t == r && r == bo && bo == l)
            return Num(t);
        if (t == bo && r == l)
            return Num(t) + "," + Num(r);
        return Num(t) + "," + Num(r) + "," + Num(bo) + "," + Num(l);
    }

    Vector<String> PropTokens(const UiNode& n, bool skipLayout /*= false*/)
    {
        Vector<String> out;
        Vector<String> done;
        for (auto& key : NodeKeys())
        {
            if (done.Contains(key) || key == "text" || !n.Has(key))
                continue;
            if (key == "layout" && skipLayout)
                continue;

            if ((key == "x" || key == "y") && n.x && n.y)
            {
                out.Add("pos=" + Num(*n.x) + "," + Num(*n.y));
                done.Add("x");
                done.Add("y");
                continue;
            }
            if ((key == "w" || key == "h") && n.w && n.h)
            {
                out.Add("size=" + Num(*n.w) + "x" + Num(*n.h));
                done.Add("w");
                done.Add("h");
                continue;
            }
            if ((key == "cellW" || key == "cellH") && n.cellW && n.cellH)
            {
                out.Add("cell=" + Num(*n.cellW) + "x" + Num(*n.cellH));
                done.Add("cellW");
                done.Add("cellH");
                continue;
            }
            if (key == "gap" && n.gapY)
            {
                out.Add("gap=" + Num(n.gap ? *n.gap : 0) + "," + Num(*n.gapY));
                done.Add("gapY");
                continue;
            }
            if (key == "gapY" && !n.gap)
            {
                out.Add("gap=0," + Num(*n.gapY));
                continue;
            }
            if (key == "margin")
            {
                out.Add("margin=" + Box4Text(*n.margin));
                continue;
            }
            if (key == "pad")
            {
                out.Add("pad=" + Box4Text(*n.pad));
                continue;
            }

            Json v = n.Get(key);
            if (v.IsNumber())
                out.Add(key + "=" + Num(v.Num()));
            else if (v.IsBool())
                out.Add(key + "=" + (v.Bool() ? "true" : "false"));
            else
                out.Add(key + "=" + Word(v.ToJsString()));
        }
        return out;
    }

    static String NodeLine(const UiNode& n)
    {
        String layoutWord = n.type == "group" && n.layout && !n.layout->IsEmpty() ? *n.layout : n.type;
        Vector<String> parts = { layoutWord, n.id };
        parts.Add(PropTokens(n, layoutWord != n.type));
        if (n.text)
            parts.Add(Quote(*n.text));
        return Join(parts, " ");
    }

    String NodeToUiml(const UiNode& n, int depth /*= 0*/)
    {
        Vector<String> lines = { Spaces(2*depth) + NodeLine(n) };
        for (auto& c : n.children)
            lines.Add(NodeToUiml(c, depth + 1));
        return Join(lines, "\n");
    }

    String DocToUiml(const UiDoc& doc)
    {
        Vector<String> head = { "screen", doc.name, JsNum(doc.width) + "x" + JsNum(doc.height) };
        if (doc.dpi && *doc.dpi != 0)
            head.Add("dpi=" + JsNum(*doc.dpi));
        head.Add(PropTokens(doc.root));

        Vector<String> lines = { Join(head, " ") };
        for (auto& c : doc.root.children)
            lines.Add(NodeToUiml(c, 1));
        return Join(lines, "\n") + "\n";
    }
}
