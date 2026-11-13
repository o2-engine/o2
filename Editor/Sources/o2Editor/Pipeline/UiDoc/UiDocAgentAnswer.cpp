#include "o2Editor/stdafx.h"
#include "UiDocAgent.h"

#include <algorithm>
#include <cmath>
#include <cstring>

#include "o2Editor/Pipeline/UiDoc/UiDocUiml.h"
#include "o2Editor/Pipeline/UiDoc/UiDocFp.h"

// The agent's answer (the second half of AssetsLine shared/uiDoc/agent.ts): @done / @markup, the @style / @asset kit
// additions, then the fenced edits or screen
namespace Editor::UiDoc
{
    // --- the answer --------------------------------------------------------------------------------------

    namespace
    {
        // A fenced block of the answer: its language, body and byte range
        struct AnswerBlock
        {
            String lang;
            String body;
            size_t start = 0, end = 0;
        };

        bool IsWordChar(char c)
        {
            return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_';
        }

        // Byte offsets where a line starts in multi-line mode (after \n, \r, U+2028, U+2029)
        std::vector<size_t> LineStarts(const std::string& s)
        {
            std::vector<size_t> out = { 0 };
            for (size_t i = 0; i < s.size(); i++)
            {
                if (s[i] == '\n' || s[i] == '\r')
                    out.push_back(i + 1);
                else if ((unsigned char)s[i] == 0xE2 && i + 2 < s.size() && (unsigned char)s[i + 1] == 0x80 &&
                         ((unsigned char)s[i + 2] == 0xA8 || (unsigned char)s[i + 2] == 0xA9))
                    out.push_back(i + 3);
            }
            return out;
        }

        // The JavaScript whitespace code point starting at i, its byte length; 0 when there is none
        size_t SpaceAt(const std::string& s, size_t i)
        {
            if (i >= s.size())
                return 0;
            unsigned char c = (unsigned char)s[i];
            int len = c < 0x80 ? 1 : (c >> 5) == 6 ? 2 : (c >> 4) == 14 ? 3 : 4;
            if (i + len > s.size())
                return 0;
            std::u32string cps = Utf8ToCodePoints(String(s.substr(i, len)));
            return cps.size() == 1 && IsJsSpace(cps[0]) ? len : 0;
        }

        // /^@\w+/m: where the first such line starts, npos when none
        size_t FindCommandLine(const std::string& s)
        {
            for (size_t at : LineStarts(s))
            {
                if (at + 1 < s.size() && s[at] == '@' && IsWordChar(s[at + 1]))
                    return at;
            }
            return std::string::npos;
        }

        // /^screen\s+\S+/m: where the first such line starts, npos when none
        size_t FindScreenLine(const std::string& s)
        {
            for (size_t at : LineStarts(s))
            {
                if (s.compare(at, 6, "screen") != 0)
                    continue;
                size_t i = at + 6, spaces = 0;
                while (size_t len = SpaceAt(s, i))
                {
                    i += len;
                    spaces++;
                }
                if (spaces > 0 && i < s.size())
                    return at;
            }
            return std::string::npos;
        }

        // /^\s*@\w+/ on a body
        bool StartsWithCommand(const std::string& s)
        {
            size_t i = 0;
            while (size_t len = SpaceAt(s, i))
                i += len;
            return i + 1 < s.size() && s[i] == '@' && IsWordChar(s[i + 1]);
        }

        // text.replace(/\n{3,}/g, '\n\n')
        std::string CollapseBlankLines(const std::string& s)
        {
            std::string out;
            size_t run = 0;
            for (char c : s)
            {
                if (c == '\n')
                {
                    run++;
                    continue;
                }
                out.append(run >= 3 ? 2 : run, '\n');
                run = 0;
                out += c;
            }
            out.append(run >= 3 ? 2 : run, '\n');
            return out;
        }

        // A line of a text and the terminator after it (\n, \r, U+2028, U+2029, or none for the last)
        struct TextLine
        {
            std::string content, end;
        };

        std::vector<TextLine> SplitLines(const std::string& s)
        {
            std::vector<TextLine> out;
            size_t from = 0;
            for (size_t i = 0; i < s.size(); i++)
            {
                size_t len = 0;
                if (s[i] == '\n' || s[i] == '\r')
                    len = 1;
                else if ((unsigned char)s[i] == 0xE2 && i + 2 < s.size() && (unsigned char)s[i + 1] == 0x80 &&
                         ((unsigned char)s[i + 2] == 0xA8 || (unsigned char)s[i + 2] == 0xA9))
                    len = 3;
                if (!len)
                    continue;
                out.push_back(TextLine{ s.substr(from, i - from), s.substr(i, len) });
                i += len - 1;
                from = i + 1;
            }
            out.push_back(TextLine{ s.substr(from), "" });
            return out;
        }

        std::string JoinLines(const std::vector<TextLine>& lines)
        {
            std::string out;
            for (auto& l : lines)
                out += l.content + l.end;
            return out;
        }

        // The command word after "@" at i (ASCII letters, case-insensitively one of the words) followed by a non-word
        // character or the end; its length, 0 when there is none
        size_t CommandAt(const std::string& s, size_t i, std::initializer_list<const char*> words)
        {
            if (i >= s.size() || s[i] != '@')
                return 0;
            for (const char* w : words)
            {
                size_t n = strlen(w);
                if (i + 1 + n > s.size())
                    continue;
                bool same = true;
                for (size_t k = 0; k < n && same; k++)
                    same = tolower((unsigned char)s[i + 1 + k]) == w[k];
                if (same && (i + 1 + n == s.size() || !IsWordChar(s[i + 1 + n])))
                    return 1 + n;
            }
            return 0;
        }

        // /^[ \t]*`?@(done|markup)\b`?[ \t.]*$/i on a line: "done", "markup" or ""
        std::string FlowWord(const std::string& line)
        {
            size_t i = 0;
            while (i < line.size() && (line[i] == ' ' || line[i] == '\t'))
                i++;
            if (i < line.size() && line[i] == '`')
                i++;
            size_t len = CommandAt(line, i, { "done", "markup" });
            if (!len)
                return "";
            std::string word = (len == 5) ? "done" : "markup";
            i += len;
            if (i < line.size() && line[i] == '`')
                i++;
            while (i < line.size() && (line[i] == ' ' || line[i] == '\t' || line[i] == '.'))
                i++;
            return i == line.size() ? word : "";
        }

        // /^[ \t]*@(style|asset)\b/i on a line
        bool IsKitLine(const std::string& line)
        {
            size_t i = 0;
            while (i < line.size() && (line[i] == ' ' || line[i] == '\t'))
                i++;
            return CommandAt(line, i, { "style", "asset" }) > 0;
        }

        // Number(v) of a text, NaN for nothing
        double TextNumber(const String& v)
        {
            return Json(v).ToJsNumber();
        }

        // v.split(/[,x×*]/i).map((p) => Number(p.trim()))
        std::vector<double> Numbers(const String& v)
        {
            std::vector<double> out;
            std::string cur;
            const std::string& s = v;
            for (size_t i = 0; i <= s.size(); i++)
            {
                bool cut = i == s.size() || s[i] == ',' || s[i] == 'x' || s[i] == 'X' || s[i] == '*';
                size_t skip = 1;
                if (!cut && (unsigned char)s[i] == 0xC3 && i + 1 < s.size() && (unsigned char)s[i + 1] == 0x97)
                {
                    cut = true;
                    skip = 2;
                }
                if (!cut)
                {
                    cur += s[i];
                    continue;
                }
                out.push_back(TextNumber(JsTrim(String(cur))));
                cur.clear();
                i += skip - 1;
            }
            return out;
        }

        // /^-?\d+(\.\d+)?(deg|°)?$/i
        bool IsAngle(const String& text)
        {
            std::string s = text;
            size_t i = 0;
            if (i < s.size() && s[i] == '-')
                i++;
            size_t digits = 0;
            while (i < s.size() && isdigit((unsigned char)s[i]))
                i++, digits++;
            if (!digits)
                return false;
            if (i < s.size() && s[i] == '.')
            {
                size_t frac = 0;
                i++;
                while (i < s.size() && isdigit((unsigned char)s[i]))
                    i++, frac++;
                if (!frac)
                    return false;
            }
            std::string rest = s.substr(i);
            std::string lower;
            for (char c : rest)
                lower += (char)tolower((unsigned char)c);
            return lower.empty() || lower == "deg" || rest == "\xC2\xB0";
        }
    }

    Json AssetRequestToJson(const UiAssetRequest& r)
    {
        Json out = Json::Object();
        out["name"] = r.name;
        out["type"] = r.type;
        out["assetType"] = r.assetType;
        out["w"] = r.w;
        out["h"] = r.h;
        if (r.box)
        {
            Json box = Json::Object();
            box["x"] = r.box->x;
            box["y"] = r.box->y;
            box["w"] = r.box->w;
            box["h"] = r.box->h;
            out["box"] = box;
        }
        if (!r.purpose.IsEmpty())
            out["purpose"] = r.purpose;
        out["describe"] = r.describe;
        return out;
    }

    // One @style line -> the style (its effects in the order written: stroke= and shadow= may repeat)
    static TextStyle StyleOfLine(const String& name, const Vector<UimlToken>& toks, const Vector<Pair<String, String>>& keys,
                                 const Vector<String>& texts)
    {
        auto key = [&](const char* k) -> const String* {
            for (auto& kv : keys)
            {
                if (kv.first == k)
                    return &kv.second;
            }
            return nullptr;
        };

        Json effects = Json::Array();
        for (int t = 2; t < toks.Count(); t++)
        {
            String k = JsToLower(toks[t].key);
            if (k.IsEmpty())
                continue;
            Vector<String> v;
            for (auto& part : JsSplit(toks[t].value, ","))
                v.Add(JsTrim(part));
            String c;
            Json first(v[0]);
            if (!CleanColor(&first, c))
                continue;
            auto n = [&](int i, double d) {
                if (i >= v.Count() || v[i].IsEmpty())
                    return d;
                double x = TextNumber(v[i]);
                return std::isfinite(x) ? x : d;
            };
            Json e = Json::Object();
            if (k == "stroke" || k == "outline")
            {
                e["type"] = "stroke"; e["color"] = c; e["width"] = n(1, 2); e["opacity"] = n(2, 1);
            }
            else if (k == "extrude" || k == "depth")
            {
                e["type"] = "extrude"; e["color"] = c; e["x"] = n(1, 0); e["y"] = n(2, 4);
            }
            else if (k == "shadow")
            {
                e["type"] = "shadow"; e["color"] = c; e["x"] = n(1, 0); e["y"] = n(2, 4); e["blur"] = n(3, 4); e["opacity"] = n(4, 0.5);
                e["spread"] = n(5, 0);
            }
            else if (k == "glow")
            {
                e["type"] = "glow"; e["color"] = c; e["size"] = n(1, 8); e["opacity"] = n(2, 0.7);
            }
            else if (k == "inner" || k == "innershadow")
            {
                e["type"] = "innerShadow"; e["color"] = c; e["x"] = n(1, 0); e["y"] = n(2, 2); e["blur"] = n(3, 2); e["opacity"] = n(4, 0.5);
            }
            else if (k == "bevel")
            {
                String shade;
                Json second(v.Count() > 1 ? v[1] : String());
                if (!CleanColor(v.Count() > 1 ? &second : nullptr, shade))
                    shade = "#000000";
                e["type"] = "bevel"; e["highlight"] = c; e["shade"] = shade; e["depth"] = n(2, 2); e["opacity"] = n(3, 0.6);
            }
            else
                continue;
            effects.Add(e);
        }

        Json raw = Json::Object();
        raw["name"] = name;
        auto copy = [&](const char* to, const char* from) {
            if (const String* v = key(from))
                raw[to] = *v;
        };
        copy("role", "role");
        copy("font", "font");
        copy("weight", "weight");
        copy("size", "size");
        String color;
        bool hasColor = false;
        if (const String* v = key("color"))
        {
            Json cv(*v);
            hasColor = CleanColor(&cv, color);
            if (hasColor)
                raw["color"] = color;
        }

        Vector<String> gv;
        if (const String* g = key("gradient"))
        {
            for (auto& part : JsSplit(*g, ","))
            {
                String t = JsTrim(part);
                if (!t.IsEmpty())
                    gv.Add(t);
            }
        }
        Json stops = Json::Array();
        stops.Add(Json(hasColor ? color : String("#FFFFFF")));
        int more = 0;
        for (auto& x : gv)
        {
            Json xv(x);
            String c;
            if (CleanColor(&xv, c))
            {
                stops.Add(Json(c));
                more++;
            }
        }
        if (more)
        {
            Json fill = Json::Object();
            const String* turn = gv.Find([](const String& x) { return IsAngle(x); });
            fill["angle"] = turn ? std::stod((std::string)*turn) : 90.0;
            fill["stops"] = stops;
            raw["fill"] = fill;
        }
        else
            raw["fill"] = Json();
        raw["effects"] = effects;
        if (const String* v = key("spacing"))
            raw["letterSpacing"] = *v;
        else
            copy("letterSpacing", "letterspacing");
        copy("lineHeight", "lineheight");
        const String* upper = key("uppercase");
        String u = upper ? JsToLower(*upper) : String();
        raw["uppercase"] = u == "true" || u == "1" || u == "yes";
        copy("align", "align");
        String desc = Join(texts, " ");
        if (!desc.IsEmpty())
            raw["description"] = desc;
        else
            copy("description", "description");
        return NormalizeTextStyle(raw, name);
    }

    UiKitAdditions ParseKitAdditions(const String& answer)
    {
        UiKitAdditions out;
        Vector<Pair<String, TextStyle>> styles;
        Vector<Pair<String, UiAssetRequest>> assets;
        auto lines = SplitLines(answer);
        for (auto& line : lines)
        {
            if (!IsKitLine(line.content))
                continue;
            String text = JsSlice(JsTrim(String(line.content)), 1);
            line.content.clear();

            auto toks = TokenizeUiml(text);
            String cmd = toks.IsEmpty() ? String("undefined") : JsToLower(toks[0].value);
            String name = toks.Count() > 1 && toks[1].key.IsEmpty() && !toks[1].quoted ? toks[1].value : String();
            Vector<Pair<String, String>> keys;
            Vector<String> texts;
            for (int t = name.IsEmpty() ? 1 : 2; t < toks.Count(); t++)
            {
                if (!toks[t].key.IsEmpty())
                {
                    String k = JsToLower(toks[t].key);
                    int at = keys.IndexOf([&](const Pair<String, String>& p) { return p.first == k; });
                    if (at >= 0)
                        keys[at].second = toks[t].value;
                    else
                        keys.Add(Pair<String, String>(k, toks[t].value));
                }
                else if (toks[t].quoted)
                    texts.Add(toks[t].value);
            }
            auto key = [&](const char* k) -> const String* {
                for (auto& kv : keys)
                {
                    if (kv.first == k)
                        return &kv.second;
                }
                return nullptr;
            };

            if (name.IsEmpty())
            {
                out.errors.Add("@" + cmd + " needs a name");
                continue;
            }

            if (cmd == "style")
            {
                TextStyle st = StyleOfLine(name, toks, keys, texts);
                String k = JsToLower(st.name);
                int at = styles.IndexOf([&](const Pair<String, TextStyle>& p) { return p.first == k; });
                if (at >= 0)
                    styles[at].second = st;
                else
                    styles.Add(Pair<String, TextStyle>(k, st));
                continue;
            }

            Json nameJson(name);
            String clean = CleanKitName(&nameJson, "");
            if (clean.IsEmpty())
            {
                out.errors.Add("@asset: \"" + name + "\" is not a name");
                continue;
            }

            UiAssetRequest r;
            r.name = clean;
            const String* type = key("type");
            r.type = type && UiElementTypes().Contains(*type) ? *type : GuessElementType(clean);
            const String* at = key("asset");
            if (!at)
                at = key("assettype");
            r.assetType = at && UiAssetTypes().Contains(*at) ? *at : DefaultAssetType(r.type);
            auto size = Numbers(key("size") ? *key("size") : String());
            double w = size.size() > 0 ? JsRound(size[0]) : NAN, h = size.size() > 1 ? JsRound(size[1]) : NAN;
            auto orElse = [](double v, double d) { return std::isnan(v) || v == 0 ? d : v; };
            r.w = std::max(8.0, orElse(w, 256));
            r.h = std::max(8.0, orElse(h, orElse(w, 256)));
            if (const String* box = key("box"))
            {
                auto b = Numbers(*box);
                bool finite = b.size() == 4 && std::all_of(b.begin(), b.end(), [](double v) { return std::isfinite(v); });
                if (finite && b[2] > 1 && b[3] > 1)
                    r.box = UiRect(JsRound(b[0]), JsRound(b[1]), JsRound(b[2]), JsRound(b[3]));
            }
            if (const String* purpose = key("purpose"))
                r.purpose = JsSlice(*purpose, 0, 200);
            String describe = Join(texts, " ");
            if (describe.IsEmpty() && key("describe"))
                describe = *key("describe");
            if (describe.IsEmpty())
            {
                std::string spaced;
                bool run = false;
                for (char c : (const std::string&)clean)
                {
                    bool sep = c == '_' || c == '-';
                    if (sep && !run)
                        spaced += ' ';
                    else if (!sep)
                        spaced += c;
                    run = sep;
                }
                describe = String(spaced);
            }
            r.describe = JsSlice(describe, 0, 400);
            String lower = JsToLower(clean);
            int idx = assets.IndexOf([&](const Pair<String, UiAssetRequest>& p) { return p.first == lower; });
            if (idx >= 0)
                assets[idx].second = r;
            else
                assets.Add(Pair<String, UiAssetRequest>(lower, r));
        }

        out.rest = String(JoinLines(lines));
        for (auto& kv : styles)
            out.styles.Add(kv.second);
        for (auto& kv : assets)
            out.assets.Add(kv.second);
        return out;
    }


    UiAgentAnswer ParseAgentAnswer(const String& full, const UiDoc& doc)
    {
        UiAgentAnswer result;
        auto flowLines = SplitLines(full);
        for (auto& line : flowLines)
        {
            std::string word = FlowWord(line.content);
            if (word.empty())
                continue;
            if (word == "done")
                result.done = true;
            else
                result.markup = true;
            line.content.clear();
        }
        UiKitAdditions kitAdds = ParseKitAdditions(String(JoinLines(flowLines)));
        String answerText = kitAdds.rest;
        const std::string& answer = answerText;
        std::vector<AnswerBlock> blocks;

        // /```([A-Za-z0-9_-]*)[^\n]*\n([\s\S]*?)(```|$)/g
        size_t pos = 0;
        for (;;)
        {
            size_t s = answer.find("```", pos);
            if (s == std::string::npos)
                break;
            size_t i = s + 3;
            while (i < answer.size() && (IsWordChar(answer[i]) || answer[i] == '-'))
                i++;
            String lang = JsToLower(String(answer.substr(s + 3, i - s - 3)));
            size_t nl = answer.find('\n', i);
            if (nl == std::string::npos)
                break;
            size_t bodyStart = nl + 1;
            size_t close = answer.find("```", bodyStart);
            size_t bodyEnd = close == std::string::npos ? answer.size() : close;
            size_t end = close == std::string::npos ? answer.size() : close + 3;
            blocks.push_back(AnswerBlock{ lang, String(answer.substr(bodyStart, bodyEnd - bodyStart)), s, end });
            pos = end;
        }

        std::string reply = answer;
        // Every fence is cut out of the reply, from the last one back (so the offsets of the ones before stay true); a fence
        // left empty by a lone @done / @markup is then no block
        for (auto it = blocks.rbegin(); it != blocks.rend(); ++it)
            reply = reply.substr(0, std::min(reply.size(), it->start)) + reply.substr(std::min(reply.size(), it->end));
        for (int i = (int)blocks.size() - 1; i >= 0; i--)
        {
            if (JsTrim(blocks[i].body).IsEmpty())
                blocks.erase(blocks.begin() + i);
        }
        reply = JsTrim(String(CollapseBlankLines(reply)));

        // No fences: a bare answer that is plainly edits or a screen
        if (blocks.empty())
        {
            std::string t = JsTrim(answerText);
            bool anyCommandLine = false;
            for (auto& line : JsSplit(String(t), "\n"))
                anyCommandLine = anyCommandLine || StartsWith(JsTrim(line), "@");

            size_t firstCmd = FindCommandLine(t);
            size_t screenAt = FindScreenLine(t);
            if (firstCmd != std::string::npos && anyCommandLine)
            {
                blocks.push_back(AnswerBlock{ "uiml-edit", String(t.substr(firstCmd)), 0, 0 });
                reply = JsTrim(String(t.substr(0, firstCmd)));
            }
            else if (screenAt != std::string::npos)
            {
                blocks.push_back(AnswerBlock{ "uiml", String(t.substr(screenAt)), 0, 0 });
                reply = JsTrim(String(t.substr(0, screenAt)));
            }
        }

        result.errors = kitAdds.errors;
        result.styles = kitAdds.styles;
        result.assets = kitAdds.assets;
        const AnswerBlock* block = nullptr;
        for (auto& b : blocks)
        {
            if (((const std::string&)b.lang).find("edit") != std::string::npos || StartsWithCommand(b.body))
            {
                block = &b;
                break;
            }
        }
        if (!block)
        {
            for (auto& b : blocks)
            {
                const std::string& l = b.lang;
                if (l.find("ui") != std::string::npos || l.find("text") != std::string::npos || l.empty())
                {
                    block = &b;
                    break;
                }
            }
        }

        if (block)
        {
            if (((const std::string&)block->lang).find("edit") != std::string::npos || StartsWithCommand(block->body))
            {
                UimlEditsResult res = ParseEdits(block->body, doc);
                result.ops = res.ops;
                result.errors.Add(res.errors);
            }
            else
            {
                UimlParseResult res = ParseUiml(block->body, &doc);
                result.ops.Add(MakeDocOp(res.doc));
                result.errors.Add(res.errors);
            }
        }

        // Report edits that will not apply, so the step says so
        if (!result.ops.IsEmpty())
            result.errors.Add(ApplyOps(doc, result.ops).errors);

        result.reply = !reply.empty() ? String(reply) : !result.ops.IsEmpty() ? String("Done.") : String();
        return result;
    }
}
