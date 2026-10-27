#include "o2Editor/stdafx.h"
#include "UiLayoutJson.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace Editor::UiLayout
{
    Json Json::Array()
    {
        Json json;
        json.mKind = Kind::Array;
        return json;
    }

    Json Json::Object()
    {
        Json json;
        json.mKind = Kind::Object;
        return json;
    }

    const Json* Json::Get(const String& key) const
    {
        if (mKind != Kind::Object)
            return nullptr;

        for (auto& member : mMembers)
        {
            if (member.first == key)
                return &member.second;
        }
        return nullptr;
    }

    Json* Json::Get(const String& key)
    {
        return const_cast<Json*>(static_cast<const Json*>(this)->Get(key));
    }

    Json& Json::operator[](const String& key)
    {
        if (mKind != Kind::Object)
        {
            *this = Object();
        }

        if (auto existing = Get(key))
            return *existing;

        mMembers.Add(Pair<String, Json>(key, Json()));
        return mMembers.Last().second;
    }

    void Json::Remove(const String& key)
    {
        mMembers.RemoveAll([&](const Pair<String, Json>& member) { return member.first == key; });
    }

    Json& Json::Add(const Json& value)
    {
        if (mKind != Kind::Array)
            *this = Array();

        mItems.Add(value);
        return mItems.Last();
    }

    int Json::Count() const
    {
        return mKind == Kind::Array ? mItems.Count() : mKind == Kind::Object ? mMembers.Count() : 0;
    }

    const Json* Json::At(int index) const
    {
        return mKind == Kind::Array && index >= 0 && index < mItems.Count() ? &mItems[index] : nullptr;
    }

    String Json::FormatNumber(double value)
    {
        if (std::isnan(value))
            return "NaN";

        if (std::isinf(value))
            return value > 0 ? "Infinity" : "-Infinity";

        if (value == 0.0)
            return "0";

        char buffer[64];
        String digits;
        int exponent = 0;
        bool negative = value < 0;
        double magnitude = std::fabs(value);
        for (int precision = 1; precision <= 17; precision++)
        {
            snprintf(buffer, sizeof(buffer), "%.*e", precision - 1, magnitude);
            if (std::strtod(buffer, nullptr) != magnitude && precision < 17)
                continue;

            char* e = std::strchr(buffer, 'e');
            exponent = std::atoi(e + 1);
            *e = 0;
            for (char* c = buffer; *c; c++)
            {
                if (*c != '.')
                    digits += *c;
            }
            break;
        }

        while (digits.Length() > 1 && digits[digits.Length() - 1] == '0')
            digits = digits.SubStr(0, digits.Length() - 1);

        int k = digits.Length(), n = exponent + 1;
        String result;
        if (k <= n && n <= 21)
        {
            result = digits;
            for (int i = 0; i < n - k; i++)
                result += '0';
        }
        else if (0 < n && n <= 21)
            result = digits.SubStr(0, n) + "." + digits.SubStr(n);
        else if (-6 < n && n <= 0)
        {
            result = "0.";
            for (int i = 0; i < -n; i++)
                result += '0';
            result += digits;
        }
        else
        {
            result = digits.SubStr(0, 1);
            if (k > 1)
                result += "." + digits.SubStr(1);
            result += String("e") + (n - 1 >= 0 ? "+" : "-") + (String)std::abs(n - 1);
        }

        return negative ? "-" + result : result;
    }

    String Json::ToJsString() const
    {
        switch (mKind)
        {
            case Kind::Null: return "null";
            case Kind::Bool: return mBool ? "true" : "false";
            case Kind::Number: return FormatNumber(mNumber);
            case Kind::String: return mString;
            case Kind::Array:
            {
                String joined;
                for (int i = 0; i < mItems.Count(); i++)
                {
                    if (i > 0)
                        joined += ",";
                    if (!mItems[i].IsNull())
                        joined += mItems[i].ToJsString();
                }
                return joined;
            }
            default: return "[object Object]";
        }
    }

    static bool IsJsSpace(char c)
    {
        return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f' || c == '\v';
    }

    double Json::ToJsNumber() const
    {
        switch (mKind)
        {
            case Kind::Null: return 0.0;
            case Kind::Bool: return mBool ? 1.0 : 0.0;
            case Kind::Number: return mNumber;
            case Kind::Object: return NAN;
            case Kind::Array:
            {
                if (mItems.IsEmpty())
                    return 0.0;
                if (mItems.Count() > 1)
                    return NAN;
                return Json(mItems[0].ToJsString()).ToJsNumber();
            }
            default: break;
        }

        const char* begin = mString.Data();
        const char* end = begin + mString.Length();
        while (begin < end && IsJsSpace(*begin))
            begin++;
        while (end > begin && IsJsSpace(*(end - 1)))
            end--;

        String text = String(std::string(begin, end));
        if (text.IsEmpty())
            return 0.0;

        if (text == "Infinity" || text == "+Infinity")
            return INFINITY;
        if (text == "-Infinity")
            return -INFINITY;

        char* parsedEnd = nullptr;
        if (text.Length() > 2 && text[0] == '0' && (text[1] == 'x' || text[1] == 'X'))
        {
            double value = (double)std::strtoull(text.Data() + 2, &parsedEnd, 16);
            return parsedEnd == text.Data() + text.Length() ? value : NAN;
        }

        for (int i = 0; i < text.Length(); i++)
        {
            char c = text[i];
            if (!((c >= '0' && c <= '9') || c == '.' || c == 'e' || c == 'E' || c == '+' || c == '-'))
                return NAN;
        }

        double value = std::strtod(text.Data(), &parsedEnd);
        return parsedEnd == text.Data() + text.Length() ? value : NAN;
    }

    static void WriteString(String& out, const String& value)
    {
        static const char* hex = "0123456789abcdef";
        out += '"';
        for (int i = 0; i < value.Length(); i++)
        {
            unsigned char c = (unsigned char)value[i];
            switch (c)
            {
                case '"': out += "\\\""; break;
                case '\\': out += "\\\\"; break;
                case '\b': out += "\\b"; break;
                case '\f': out += "\\f"; break;
                case '\n': out += "\\n"; break;
                case '\r': out += "\\r"; break;
                case '\t': out += "\\t"; break;
                default:
                    if (c < 0x20)
                    {
                        out += "\\u00";
                        out += hex[c >> 4];
                        out += hex[c & 15];
                    }
                    else
                        out += (char)c;
            }
        }
        out += '"';
    }

    static void WriteIndent(String& out, int indent, int depth)
    {
        out += '\n';
        for (int i = 0; i < indent*depth; i++)
            out += ' ';
    }

    void Json::Write(String& out, int indent, int depth) const
    {
        switch (mKind)
        {
            case Kind::Null: out += "null"; return;
            case Kind::Bool: out += mBool ? "true" : "false"; return;
            case Kind::Number: out += std::isfinite(mNumber) ? FormatNumber(mNumber) : String("null"); return;
            case Kind::String: WriteString(out, mString); return;
            case Kind::Array:
            {
                if (mItems.IsEmpty())
                {
                    out += "[]";
                    return;
                }

                out += '[';
                for (int i = 0; i < mItems.Count(); i++)
                {
                    if (i > 0)
                        out += ',';
                    if (indent > 0)
                        WriteIndent(out, indent, depth + 1);
                    mItems[i].Write(out, indent, depth + 1);
                }
                if (indent > 0)
                    WriteIndent(out, indent, depth);
                out += ']';
                return;
            }
            case Kind::Object:
            {
                if (mMembers.IsEmpty())
                {
                    out += "{}";
                    return;
                }

                out += '{';
                for (int i = 0; i < mMembers.Count(); i++)
                {
                    if (i > 0)
                        out += ',';
                    if (indent > 0)
                        WriteIndent(out, indent, depth + 1);
                    WriteString(out, mMembers[i].first);
                    out += indent > 0 ? ": " : ":";
                    mMembers[i].second.Write(out, indent, depth + 1);
                }
                if (indent > 0)
                    WriteIndent(out, indent, depth);
                out += '}';
                return;
            }
        }
    }

    String Json::Dump(int indent /*= 0*/) const
    {
        String out;
        Write(out, indent, 0);
        return out;
    }

    namespace
    {
        struct Parser
        {
            const char* p;
            const char* end;
            String      error;

            void SkipSpace()
            {
                while (p < end && (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r'))
                    p++;
            }

            bool Fail(const String& message)
            {
                if (error.IsEmpty())
                    error = message;
                return false;
            }

            static void AppendUtf8(String& out, unsigned int code)
            {
                if (code < 0x80)
                    out += (char)code;
                else if (code < 0x800)
                {
                    out += (char)(0xC0 | (code >> 6));
                    out += (char)(0x80 | (code & 0x3F));
                }
                else if (code < 0x10000)
                {
                    out += (char)(0xE0 | (code >> 12));
                    out += (char)(0x80 | ((code >> 6) & 0x3F));
                    out += (char)(0x80 | (code & 0x3F));
                }
                else
                {
                    out += (char)(0xF0 | (code >> 18));
                    out += (char)(0x80 | ((code >> 12) & 0x3F));
                    out += (char)(0x80 | ((code >> 6) & 0x3F));
                    out += (char)(0x80 | (code & 0x3F));
                }
            }

            bool ReadHex4(unsigned int& code)
            {
                if (end - p < 4)
                    return Fail("bad \\u escape");

                code = 0;
                for (int i = 0; i < 4; i++)
                {
                    char c = *p++;
                    code <<= 4;
                    if (c >= '0' && c <= '9') code |= c - '0';
                    else if (c >= 'a' && c <= 'f') code |= c - 'a' + 10;
                    else if (c >= 'A' && c <= 'F') code |= c - 'A' + 10;
                    else return Fail("bad \\u escape");
                }
                return true;
            }

            bool ReadString(String& out)
            {
                p++;
                const char* runStart = p;
                while (p < end)
                {
                    char c = *p;
                    if (c == '"')
                    {
                        out += String(std::string(runStart, p));
                        p++;
                        return true;
                    }

                    if ((unsigned char)c < 0x20)
                        return Fail("control character in a string");

                    if (c != '\\')
                    {
                        p++;
                        continue;
                    }

                    out += String(std::string(runStart, p));
                    p++;
                    if (p >= end)
                        return Fail("unterminated string");

                    char e = *p++;
                    switch (e)
                    {
                        case '"': out += '"'; break;
                        case '\\': out += '\\'; break;
                        case '/': out += '/'; break;
                        case 'b': out += '\b'; break;
                        case 'f': out += '\f'; break;
                        case 'n': out += '\n'; break;
                        case 'r': out += '\r'; break;
                        case 't': out += '\t'; break;
                        case 'u':
                        {
                            unsigned int code = 0;
                            if (!ReadHex4(code))
                                return false;

                            if (code >= 0xD800 && code < 0xDC00 && end - p >= 6 && p[0] == '\\' && p[1] == 'u')
                            {
                                const char* save = p;
                                p += 2;
                                unsigned int low = 0;
                                if (ReadHex4(low) && low >= 0xDC00 && low < 0xE000)
                                    code = 0x10000 + ((code - 0xD800) << 10) + (low - 0xDC00);
                                else
                                    p = save;
                            }
                            AppendUtf8(out, code);
                            break;
                        }
                        default: return Fail("bad escape");
                    }
                    runStart = p;
                }
                return Fail("unterminated string");
            }

            bool ReadNumber(Json& out)
            {
                const char* start = p;
                if (p < end && *p == '-')
                    p++;
                if (p >= end || !(*p >= '0' && *p <= '9'))
                    return Fail("bad number");
                if (*p == '0')
                    p++;
                else
                {
                    while (p < end && *p >= '0' && *p <= '9')
                        p++;
                }
                if (p < end && *p == '.')
                {
                    p++;
                    if (p >= end || !(*p >= '0' && *p <= '9'))
                        return Fail("bad number");
                    while (p < end && *p >= '0' && *p <= '9')
                        p++;
                }
                if (p < end && (*p == 'e' || *p == 'E'))
                {
                    p++;
                    if (p < end && (*p == '+' || *p == '-'))
                        p++;
                    if (p >= end || !(*p >= '0' && *p <= '9'))
                        return Fail("bad number");
                    while (p < end && *p >= '0' && *p <= '9')
                        p++;
                }

                String text = String(std::string(start, p));
                out = Json(std::strtod(text.Data(), nullptr));
                return true;
            }

            bool ReadValue(Json& out, int depth)
            {
                if (depth > 512)
                    return Fail("too deep");

                SkipSpace();
                if (p >= end)
                    return Fail("unexpected end");

                char c = *p;
                if (c == '{')
                {
                    p++;
                    out = Json::Object();
                    SkipSpace();
                    if (p < end && *p == '}')
                    {
                        p++;
                        return true;
                    }

                    for (;;)
                    {
                        SkipSpace();
                        if (p >= end || *p != '"')
                            return Fail("expected a key");

                        String key;
                        if (!ReadString(key))
                            return false;

                        SkipSpace();
                        if (p >= end || *p != ':')
                            return Fail("expected ':'");
                        p++;

                        Json value;
                        if (!ReadValue(value, depth + 1))
                            return false;

                        out[key] = value;
                        SkipSpace();
                        if (p < end && *p == ',')
                        {
                            p++;
                            continue;
                        }
                        if (p < end && *p == '}')
                        {
                            p++;
                            return true;
                        }
                        return Fail("expected ',' or '}'");
                    }
                }

                if (c == '[')
                {
                    p++;
                    out = Json::Array();
                    SkipSpace();
                    if (p < end && *p == ']')
                    {
                        p++;
                        return true;
                    }

                    for (;;)
                    {
                        Json value;
                        if (!ReadValue(value, depth + 1))
                            return false;

                        out.Add(value);
                        SkipSpace();
                        if (p < end && *p == ',')
                        {
                            p++;
                            continue;
                        }
                        if (p < end && *p == ']')
                        {
                            p++;
                            return true;
                        }
                        return Fail("expected ',' or ']'");
                    }
                }

                if (c == '"')
                {
                    String text;
                    if (!ReadString(text))
                        return false;
                    out = Json(text);
                    return true;
                }

                auto word = [&](const char* w) { size_t n = std::strlen(w); return (size_t)(end - p) >= n && std::strncmp(p, w, n) == 0; };
                if (word("true")) { p += 4; out = Json(true); return true; }
                if (word("false")) { p += 5; out = Json(false); return true; }
                if (word("null")) { p += 4; out = Json(); return true; }

                return ReadNumber(out);
            }
        };
    }

    bool Json::Parse(const String& text, Json& out, String* error /*= nullptr*/)
    {
        Parser parser{ text.Data(), text.Data() + text.Length() };
        Json value;
        bool ok = parser.ReadValue(value, 0);
        if (ok)
        {
            parser.SkipSpace();
            if (parser.p != parser.end)
                ok = parser.Fail("unexpected text after the value");
        }

        if (!ok)
        {
            if (error)
                *error = parser.error;
            return false;
        }

        out = value;
        return true;
    }
}
// --- META ---

ENUM_META(Editor::UiLayout::Json::Kind, Editor__UiLayout__Json__Kind)
{
    ENUM_ENTRY(Array);
    ENUM_ENTRY(Bool);
    ENUM_ENTRY(Null);
    ENUM_ENTRY(Number);
    ENUM_ENTRY(Object);
    ENUM_ENTRY(String);
}
END_ENUM_META;
// --- END META ---
