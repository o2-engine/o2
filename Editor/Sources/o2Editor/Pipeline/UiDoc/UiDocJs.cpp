#include "o2Editor/stdafx.h"
#include "UiDocJs.h"

#include <chrono>
#include <cmath>
#include <random>

#include "o2Editor/Pipeline/UiDoc/UiDocFp.h"

namespace Editor::UiDoc
{
    double JsRound(double v)
    {
        if (!std::isfinite(v))
            return v;

        double f = std::floor(v);
        double r = v - f >= 0.5 ? f + 1.0 : f;
        // Math.round(-0.2) is -0
        return r == 0.0 && std::signbit(v) ? -0.0 : r;
    }

    String JsNum(double v)
    {
        return Json::FormatNumber(v);
    }

    double JsNumber(const Json* v)
    {
        return v ? v->ToJsNumber() : NAN;
    }

    String JsString(const Json* v)
    {
        return v ? v->ToJsString() : String("undefined");
    }

    String JsStringOr(const Json* v, const String& def /*= ""*/)
    {
        return JsNullish(v) ? def : v->ToJsString();
    }

    bool JsTruthy(const Json* v)
    {
        if (!v)
            return false;

        switch (v->GetKind())
        {
            case Json::Kind::Null: return false;
            case Json::Kind::Bool: return v->Bool();
            case Json::Kind::Number: return v->Num() != 0.0 && !std::isnan(v->Num());
            case Json::Kind::String: return !v->Str().IsEmpty();
            default: return true;
        }
    }

    bool JsNullish(const Json* v)
    {
        return !v || v->IsNull();
    }

    bool JsIsObjectLike(const Json* v)
    {
        return v && (v->IsObject() || v->IsArray());
    }

    const Json* JsGet(const Json& obj, const char* key)
    {
        return obj.IsObject() ? obj.Get(String(key)) : nullptr;
    }

    const Json* JsGet(const Json* obj, const char* key)
    {
        return obj ? JsGet(*obj, key) : nullptr;
    }

    bool JsStrictEqual(const Json* a, const Json* b)
    {
        if (!a || !b)
            return !a && !b;
        if (a->GetKind() != b->GetKind())
            return false;
        switch (a->GetKind())
        {
            case Json::Kind::Null: return true;
            case Json::Kind::Bool: return a->Bool() == b->Bool();
            case Json::Kind::Number: return a->Num() == b->Num();
            case Json::Kind::String: return a->Str() == b->Str();
            default: return false;
        }
    }

    Json JsonStrings(const Vector<String>& items)
    {
        Json out = Json::Array();
        for (auto& item : items)
            out.Add(Json(item));
        return out;
    }

    Vector<String> JsonToStrings(const Json* v)
    {
        Vector<String> out;
        if (v && v->IsArray())
        {
            for (auto& item : v->Items())
                out.Add(item.ToJsString());
        }
        return out;
    }

    // --- text --------------------------------------------------------------------------------

    std::u32string Utf8ToCodePoints(const String& text)
    {
        std::u32string out;
        const unsigned char* s = (const unsigned char*)text.Data();
        int n = text.Length();
        int i = 0;
        while (i < n)
        {
            unsigned char c = s[i];
            int extra = c < 0x80 ? 0 : (c >> 5) == 6 ? 1 : (c >> 4) == 14 ? 2 : (c >> 3) == 30 ? 3 : -1;
            if (extra < 0 || (extra > 0 && i + extra >= n))
            {
                out.push_back(0xFFFD);
                i++;
                continue;
            }

            char32_t cp = extra == 0 ? c : extra == 1 ? (c & 0x1F) : extra == 2 ? (c & 0x0F) : (c & 0x07);
            bool ok = true;
            for (int k = 1; k <= extra; k++)
            {
                if ((s[i + k] & 0xC0) != 0x80)
                {
                    ok = false;
                    break;
                }
                cp = (cp << 6) | (s[i + k] & 0x3F);
            }

            if (!ok)
            {
                out.push_back(0xFFFD);
                i++;
                continue;
            }

            out.push_back(cp);
            i += extra + 1;
        }
        return out;
    }

    String CodePointToUtf8(char32_t cp)
    {
        std::string out;
        if (cp < 0x80)
            out += (char)cp;
        else if (cp < 0x800)
        {
            out += (char)(0xC0 | (cp >> 6));
            out += (char)(0x80 | (cp & 0x3F));
        }
        else if (cp < 0x10000)
        {
            out += (char)(0xE0 | (cp >> 12));
            out += (char)(0x80 | ((cp >> 6) & 0x3F));
            out += (char)(0x80 | (cp & 0x3F));
        }
        else
        {
            out += (char)(0xF0 | (cp >> 18));
            out += (char)(0x80 | ((cp >> 12) & 0x3F));
            out += (char)(0x80 | ((cp >> 6) & 0x3F));
            out += (char)(0x80 | (cp & 0x3F));
        }
        return String(out);
    }

    String CodePointsToUtf8(const std::u32string& cps)
    {
        std::string out;
        out.reserve(cps.size());
        for (char32_t cp : cps)
            out += CodePointToUtf8(cp);
        return String(out);
    }

    int Utf16Length(const String& text)
    {
        int n = 0;
        for (char32_t cp : Utf8ToCodePoints(text))
            n += cp >= 0x10000 ? 2 : 1;
        return n;
    }

    String JsSlice(const String& text, int start, int end /*= INT32_MAX*/)
    {
        std::u32string cps = Utf8ToCodePoints(text);
        int len = 0;
        for (char32_t cp : cps)
            len += cp >= 0x10000 ? 2 : 1;

        auto clampIndex = [&](int i) { return i < 0 ? std::max(0, len + i) : std::min(len, i); };
        int from = clampIndex(start), to = clampIndex(end);
        if (to <= from)
            return String();

        std::u32string out;
        int unit = 0;
        for (char32_t cp : cps)
        {
            int w = cp >= 0x10000 ? 2 : 1;
            if (unit >= from && unit + w <= to)
                out.push_back(cp);
            unit += w;
            if (unit >= to)
                break;
        }
        return CodePointsToUtf8(out);
    }

    bool IsJsSpace(char32_t c)
    {
        return (c >= 0x09 && c <= 0x0D) || c == 0x20 || c == 0xA0 || c == 0x1680 || (c >= 0x2000 && c <= 0x200A) ||
            c == 0x2028 || c == 0x2029 || c == 0x202F || c == 0x205F || c == 0x3000 || c == 0xFEFF;
    }

    static String TrimImpl(const String& text, bool start, bool end)
    {
        std::u32string cps = Utf8ToCodePoints(text);
        size_t a = 0, b = cps.size();
        if (start)
        {
            while (a < b && IsJsSpace(cps[a]))
                a++;
        }
        if (end)
        {
            while (b > a && IsJsSpace(cps[b - 1]))
                b--;
        }
        return CodePointsToUtf8(cps.substr(a, b - a));
    }

    String JsTrim(const String& text) { return TrimImpl(text, true, true); }
    String JsTrimStart(const String& text) { return TrimImpl(text, true, false); }
    String JsTrimEnd(const String& text) { return TrimImpl(text, false, true); }

    // Upper-case letter -> its lower-case one, 0 when the code point has no simple lower case
    static char32_t LowerOf(char32_t c)
    {
        if (c >= 'A' && c <= 'Z') return c + 32;
        if (c < 0xC0) return 0;
        if (c <= 0xDE) return c == 0xD7 ? 0 : c + 32;
        if (c >= 0x100 && c <= 0x137) return (c & 1) == 0 && c != 0x130 ? c + 1 : 0;
        if (c >= 0x139 && c <= 0x148) return (c & 1) == 1 ? c + 1 : 0;
        if (c >= 0x14A && c <= 0x177) return (c & 1) == 0 ? c + 1 : 0;
        if (c == 0x178) return 0xFF;
        if (c >= 0x179 && c <= 0x17E) return (c & 1) == 1 ? c + 1 : 0;
        if (c == 0x386) return 0x3AC;
        if (c >= 0x388 && c <= 0x38A) return c + 37;
        if (c == 0x38C) return 0x3CC;
        if (c == 0x38E || c == 0x38F) return c + 63;
        if (c >= 0x391 && c <= 0x3AB) return c == 0x3A2 ? 0 : c + 32;
        if (c >= 0x400 && c <= 0x40F) return c + 80;
        if (c >= 0x410 && c <= 0x42F) return c + 32;
        if (c >= 0x460 && c <= 0x481) return (c & 1) == 0 ? c + 1 : 0;
        if (c >= 0x48A && c <= 0x4BF) return (c & 1) == 0 ? c + 1 : 0;
        if (c == 0x4C0) return 0x4CF;
        if (c >= 0x4C1 && c <= 0x4CE) return (c & 1) == 1 ? c + 1 : 0;
        if (c >= 0x4D0 && c <= 0x52F) return (c & 1) == 0 ? c + 1 : 0;
        if (c >= 0x531 && c <= 0x556) return c + 48;
        if (c >= 0x1E00 && c <= 0x1E95) return (c & 1) == 0 ? c + 1 : 0;
        if (c >= 0x1EA0 && c <= 0x1EFF) return (c & 1) == 0 ? c + 1 : 0;
        if (c >= 0xFF21 && c <= 0xFF3A) return c + 32;
        return 0;
    }

    // Lower-case letter -> its upper-case one, 0 when it has none (sharp s and other expansions are handled by the caller)
    static char32_t UpperOf(char32_t c)
    {
        if (c >= 'a' && c <= 'z') return c - 32;
        if (c < 0xB5) return 0;
        if (c == 0xB5) return 0x39C;
        if (c >= 0xE0 && c <= 0xFE) return c == 0xF7 ? 0 : c - 32;
        if (c == 0xFF) return 0x178;
        if (c == 0x131) return 'I';
        if (c == 0x17F) return 'S';
        if (c >= 0x101 && c <= 0x137) return (c & 1) == 1 ? c - 1 : 0;
        if (c >= 0x13A && c <= 0x148) return (c & 1) == 0 ? c - 1 : 0;
        if (c >= 0x14B && c <= 0x177) return (c & 1) == 1 ? c - 1 : 0;
        if (c >= 0x17A && c <= 0x17E) return (c & 1) == 0 ? c - 1 : 0;
        if (c == 0x3AC) return 0x386;
        if (c >= 0x3AD && c <= 0x3AF) return c - 37;
        if (c == 0x3CC) return 0x38C;
        if (c == 0x3CD || c == 0x3CE) return c - 63;
        if (c == 0x3C2) return 0x3A3;
        if (c >= 0x3B1 && c <= 0x3CB) return c - 32;
        if (c >= 0x430 && c <= 0x44F) return c - 32;
        if (c >= 0x450 && c <= 0x45F) return c - 80;
        if (c >= 0x461 && c <= 0x481) return (c & 1) == 1 ? c - 1 : 0;
        if (c >= 0x48B && c <= 0x4BF) return (c & 1) == 1 ? c - 1 : 0;
        if (c == 0x4CF) return 0x4C0;
        if (c >= 0x4C2 && c <= 0x4CE) return (c & 1) == 0 ? c - 1 : 0;
        if (c >= 0x4D1 && c <= 0x52F) return (c & 1) == 1 ? c - 1 : 0;
        if (c >= 0x561 && c <= 0x586) return c - 48;
        if (c >= 0x1E01 && c <= 0x1E95) return (c & 1) == 1 ? c - 1 : 0;
        if (c >= 0x1EA1 && c <= 0x1EFF) return (c & 1) == 1 ? c - 1 : 0;
        if (c >= 0xFF41 && c <= 0xFF5A) return c - 32;
        return 0;
    }

    String JsToLower(const String& text)
    {
        std::u32string cps = Utf8ToCodePoints(text);
        for (auto& c : cps)
        {
            if (char32_t l = LowerOf(c))
                c = l;
        }
        return CodePointsToUtf8(cps);
    }

    String JsToUpper(const String& text)
    {
        std::u32string cps = Utf8ToCodePoints(text), out;
        out.reserve(cps.size());
        for (char32_t c : cps)
        {
            if (c == 0xDF)
            {
                out += U"SS";
                continue;
            }
            char32_t u = UpperOf(c);
            out.push_back(u ? u : c);
        }
        return CodePointsToUtf8(out);
    }

    bool IsLetterOrNumber(char32_t c)
    {
        if (c < 0x80)
            return (c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');

        if (c <= 0xBF)
            return c == 0xAA || c == 0xB2 || c == 0xB3 || c == 0xB5 || c == 0xB9 || c == 0xBA || (c >= 0xBC && c <= 0xBE);
        if (c == 0xD7 || c == 0xF7)
            return false;
        if (c < 0x2B0)
            return true;
        if (c <= 0x2FF)
            return !((c >= 0x2C2 && c <= 0x2C5) || (c >= 0x2D2 && c <= 0x2DF) || (c >= 0x2E5 && c <= 0x2EB) || c == 0x2ED ||
                     c >= 0x2EF);
        if (c <= 0x36F)
            return false;
        if (c == 0x37E || c == 0x387 || c == 0x375 || c == 0x384 || c == 0x385)
            return false;
        if (c >= 0x483 && c <= 0x489)
            return false;
        if (c >= 0x591 && c <= 0x5C7)
            return false;
        if (c >= 0x600 && c <= 0x61F && !(c >= 0x620))
            return false;
        if (c >= 0x64B && c <= 0x65F)
            return false;
        if (c >= 0x2000 && c <= 0x206F)
            return false;
        if (c >= 0x2070 && c <= 0x209F)
            return !((c >= 0x207A && c <= 0x207E) || (c >= 0x208A && c <= 0x208E) || c >= 0x209D || c == 0x2072 || c == 0x2073);
        if (c >= 0x20A0 && c <= 0x20FF)
            return false;
        if (c >= 0x2100 && c <= 0x214F)
        {
            return c == 0x2102 || c == 0x2107 || (c >= 0x210A && c <= 0x2113) || c == 0x2115 || (c >= 0x2119 && c <= 0x211D) ||
                c == 0x2124 || c == 0x2126 || c == 0x2128 || (c >= 0x212A && c <= 0x212D) || (c >= 0x212F && c <= 0x2139) ||
                (c >= 0x213C && c <= 0x213F) || (c >= 0x2145 && c <= 0x2149) || c == 0x214E;
        }
        if (c >= 0x2150 && c <= 0x218F)
            return c <= 0x2189;
        if (c >= 0x2190 && c <= 0x245F)
            return false;
        if (c >= 0x2460 && c <= 0x24FF)
            return c <= 0x249B || c >= 0x24EA;
        if (c >= 0x2500 && c <= 0x2BFF)
            return c >= 0x2776 && c <= 0x2793;
        if (c >= 0x2CE5 && c <= 0x2CEA)
            return false;
        if (c >= 0x2E00 && c <= 0x2FFF)
            return false;
        if (c >= 0x3000 && c <= 0x303F)
        {
            return !((c <= 0x3004) || (c >= 0x3008 && c <= 0x3020) || (c >= 0x302A && c <= 0x3030) || c == 0x3036 ||
                     c == 0x3037 || c >= 0x303D);
        }
        if (c >= 0x3099 && c <= 0x309C)
            return false;
        if (c == 0x30A0 || c == 0x30FB)
            return false;
        if (c >= 0x3200 && c <= 0x33FF)
        {
            return (c >= 0x3220 && c <= 0x3229) || (c >= 0x3248 && c <= 0x324F) || (c >= 0x3251 && c <= 0x325F) ||
                (c >= 0x3280 && c <= 0x3289) || (c >= 0x32B1 && c <= 0x32BF);
        }
        if (c >= 0x4DC0 && c <= 0x4DFF)
            return false;
        if (c >= 0xA700 && c <= 0xA721)
            return false;
        if (c >= 0xD800 && c <= 0xF8FF)
            return false;
        if (c >= 0xFE00 && c <= 0xFE6F)
            return false;
        if (c == 0xFEFF)
            return false;
        if (c >= 0xFF00 && c <= 0xFFFF)
        {
            return (c >= 0xFF10 && c <= 0xFF19) || (c >= 0xFF21 && c <= 0xFF3A) || (c >= 0xFF41 && c <= 0xFF5A) ||
                (c >= 0xFF66 && c <= 0xFFDC);
        }
        if (c >= 0x1D000 && c <= 0x1D24F)
            return false;
        if (c >= 0x1F000 && c <= 0x1FAFF)
            return c >= 0x1F100 && c <= 0x1F10C;
        if (c >= 0x1FB00 && c <= 0x1FBFF)
            return c >= 0x1FBF0 && c <= 0x1FBF9;
        if (c >= 0xE0000)
            return false;
        return true;
    }

    Vector<String> SplitWords(const String& text)
    {
        Vector<String> out;
        std::u32string cps = Utf8ToCodePoints(text), word;
        for (char32_t c : cps)
        {
            if (IsJsSpace(c))
            {
                if (!word.empty())
                    out.Add(CodePointsToUtf8(word));
                word.clear();
            }
            else
                word.push_back(c);
        }
        if (!word.empty())
            out.Add(CodePointsToUtf8(word));
        return out;
    }

    String ReplaceSpaceRuns(const String& text, const String& with)
    {
        std::u32string cps = Utf8ToCodePoints(text);
        std::string out;
        bool inRun = false;
        for (char32_t c : cps)
        {
            if (IsJsSpace(c))
            {
                if (!inRun)
                    out += with.Data();
                inRun = true;
                continue;
            }
            inRun = false;
            out += CodePointToUtf8(c).Data();
        }
        return String(out);
    }

    Vector<String> JsSplit(const String& text, const String& separator)
    {
        Vector<String> out;
        const std::string& s = text;
        const std::string& sep = separator;
        if (sep.empty())
        {
            for (char32_t c : Utf8ToCodePoints(text))
                out.Add(CodePointToUtf8(c));
            return out;
        }

        size_t from = 0;
        for (;;)
        {
            size_t at = s.find(sep, from);
            if (at == std::string::npos)
            {
                out.Add(String(s.substr(from)));
                return out;
            }
            out.Add(String(s.substr(from, at - from)));
            from = at + sep.size();
        }
    }

    String Join(const Vector<String>& parts, const String& separator)
    {
        std::string out;
        for (int i = 0; i < parts.Count(); i++)
        {
            if (i > 0)
                out += separator.Data();
            out += parts[i].Data();
        }
        return String(out);
    }

    bool StartsWith(const String& text, const String& prefix)
    {
        const std::string& s = text;
        return s.compare(0, prefix.Length(), (const std::string&)prefix) == 0 && s.size() >= (size_t)prefix.Length();
    }

    String Spaces(int count)
    {
        return String(std::string(std::max(0, count), ' '));
    }

    // --- time --------------------------------------------------------------------------------

    Int64 NowMs()
    {
        using namespace std::chrono;
        return (Int64)duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
    }

    // Days since 1970-01-01 of a civil date (Howard Hinnant's algorithm)
    static Int64 DaysFromCivil(Int64 y, unsigned m, unsigned d)
    {
        y -= m <= 2;
        const Int64 era = (y >= 0 ? y : y - 399) / 400;
        const unsigned yoe = (unsigned)(y - era*400);
        const unsigned doy = (153*(m + (m > 2 ? -3 : 9)) + 2)/5 + d - 1;
        const unsigned doe = yoe*365 + yoe/4 - yoe/100 + doy;
        return era*146097 + (Int64)doe - 719468;
    }

    static void CivilFromDays(Int64 z, Int64& y, unsigned& m, unsigned& d)
    {
        z += 719468;
        const Int64 era = (z >= 0 ? z : z - 146096) / 146097;
        const unsigned doe = (unsigned)(z - era*146097);
        const unsigned yoe = (doe - doe/1460 + doe/36524 - doe/146096) / 365;
        y = (Int64)yoe + era*400;
        const unsigned doy = doe - (365*yoe + yoe/4 - yoe/100);
        const unsigned mp = (5*doy + 2)/153;
        d = doy - (153*mp + 2)/5 + 1;
        m = mp < 10 ? mp + 3 : mp - 9;
        y += m <= 2;
    }

    String IsoTime(double ms)
    {
        Int64 t = (Int64)std::floor(ms);
        Int64 days = t >= 0 ? t / 86400000 : -((-t + 86399999) / 86400000);
        Int64 rest = t - days*86400000;
        Int64 y;
        unsigned m, d;
        CivilFromDays(days, y, m, d);
        char buf[64];
        snprintf(buf, sizeof(buf), "%04lld-%02u-%02uT%02lld:%02lld:%02lld.%03lldZ", (long long)y, m, d, (long long)(rest/3600000),
                 (long long)(rest/60000 % 60), (long long)(rest/1000 % 60), (long long)(rest % 1000));
        return String(buf);
    }

    double ParseIsoTime(const String& text)
    {
        const std::string& s = text;
        size_t i = 0;
        auto digits = [&](int count, int& out) {
            if (i + count > s.size())
                return false;
            out = 0;
            for (int k = 0; k < count; k++)
            {
                char c = s[i + k];
                if (c < '0' || c > '9')
                    return false;
                out = out*10 + (c - '0');
            }
            i += count;
            return true;
        };

        int year, month = 1, day = 1, hour = 0, minute = 0, second = 0;
        double fraction = 0.0;
        if (!digits(4, year))
            return NAN;
        if (i < s.size() && s[i] == '-')
        {
            i++;
            if (!digits(2, month))
                return NAN;
            if (i < s.size() && s[i] == '-')
            {
                i++;
                if (!digits(2, day))
                    return NAN;
            }
        }

        double offsetMs = 0.0;
        if (i < s.size() && s[i] == 'T')
        {
            i++;
            if (!digits(2, hour) || i >= s.size() || s[i] != ':')
                return NAN;
            i++;
            if (!digits(2, minute))
                return NAN;
            if (i < s.size() && s[i] == ':')
            {
                i++;
                if (!digits(2, second))
                    return NAN;
                if (i < s.size() && s[i] == '.')
                {
                    i++;
                    size_t start = i;
                    double scale = 100.0;
                    while (i < s.size() && s[i] >= '0' && s[i] <= '9')
                    {
                        fraction += (s[i] - '0')*scale;
                        scale /= 10.0;
                        i++;
                    }
                    if (i == start)
                        return NAN;
                    fraction = std::floor(fraction);
                }
            }

            if (i < s.size() && s[i] == 'Z')
                i++;
            else if (i < s.size() && (s[i] == '+' || s[i] == '-'))
            {
                int sign = s[i] == '-' ? -1 : 1, oh = 0, om = 0;
                i++;
                if (!digits(2, oh) || i >= s.size() || s[i] != ':')
                    return NAN;
                i++;
                if (!digits(2, om))
                    return NAN;
                offsetMs = sign*(oh*3600000.0 + om*60000.0);
            }
        }

        if (i != s.size() || month < 1 || month > 12 || day < 1 || day > 31 || hour > 24 || minute > 59 || second > 59)
            return NAN;

        double days = (double)DaysFromCivil(year, (unsigned)month, (unsigned)day);
        return days*86400000.0 + hour*3600000.0 + minute*60000.0 + second*1000.0 + fraction - offsetMs;
    }

    String ToBase36(Int64 n)
    {
        static const char* digits = "0123456789abcdefghijklmnopqrstuvwxyz";
        if (n <= 0)
            return "0";

        std::string out;
        while (n > 0)
        {
            out.insert(out.begin(), digits[n % 36]);
            n /= 36;
        }
        return String(out);
    }

    String RandomBase36(int length)
    {
        static std::mt19937_64 rng(std::random_device{}());
        static const char* digits = "0123456789abcdefghijklmnopqrstuvwxyz";
        std::string out;
        for (int i = 0; i < length; i++)
            out += digits[rng() % 36];
        return String(out);
    }

    // --- o2 data -------------------------------------------------------------------------------

    Json FromDataValue(const DataValue& value)
    {
        if (value.IsObject())
        {
            Json out = Json::Object();
            for (auto it = value.BeginMember(); it != value.EndMember(); ++it)
                out.Members().Add(Pair<String, Json>(String(it->name.GetString()), FromDataValue(it->value)));
            return out;
        }

        if (value.IsArray())
        {
            Json out = Json::Array();
            for (auto& item : value)
                out.Add(FromDataValue(item));
            return out;
        }

        if (value.IsString())
            return Json(String(value.GetString()));
        if (value.IsBoolean())
            return Json((bool)value);
        if (value.IsNumber())
            return Json((double)value);
        return Json();
    }

    void ToDataValue(const Json& value, DataValue& out)
    {
        // SetObject / SetArray keep what an object or array already holds: start from nothing
        out.SetNull();
        switch (value.GetKind())
        {
            case Json::Kind::Object:
            {
                out.SetObject();
                for (auto& member : value.Members())
                    ToDataValue(member.second, out.AddMember(member.first.Data()));
                return;
            }
            case Json::Kind::Array:
            {
                out.SetArray();
                for (auto& item : value.Items())
                    ToDataValue(item, out.AddElement());
                return;
            }
            case Json::Kind::String: out = value.Str(); return;
            case Json::Kind::Bool: out = value.Bool(); return;
            case Json::Kind::Number:
            {
                double d = value.Num();
                if (!std::isfinite(d))
                    out.SetNull();
                else if (d == std::floor(d) && std::fabs(d) < 9.0e15)
                {
                    if (std::fabs(d) <= 2147483647.0)
                        out = (int)d;
                    else
                        out = (Int64)d;
                }
                else
                    out = d;
                return;
            }
            default: out.SetNull(); return;
        }
    }

    void SetConfigJson(DataValue& config, const char* key, const Json& value)
    {
        if (!config.IsObject())
        {
            config.SetNull();
            config.SetObject();
        }
        DataValue* member = config.FindMember(key);
        ToDataValue(value, member ? *member : config.AddMember(key));
    }
}
