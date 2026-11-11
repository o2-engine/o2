#include "o2Editor/stdafx.h"
#include "UiUnityInternal.h"

#include <cmath>
#include <cstdio>
#include <regex>

#include "o2Editor/Pipeline/UiDoc/UiDocFp.h"

namespace Editor::UiDoc::Unity
{
    // --- float32 and the vectors --------------------------------------------------------------------

    V2 Add(const V2& a, const V2& b) { return MakeV2(a.x + b.x, a.y + b.y); }
    V2 Sub(const V2& a, const V2& b) { return MakeV2(a.x - b.x, a.y - b.y); }
    V2 Scale(const V2& a, const V2& b) { return MakeV2(a.x * b.x, a.y * b.y); }
    bool Eq2(const V2& a, const V2& b) { return a.x == b.x && a.y == b.y; }
    bool IsZero4(const V4& b) { return b.x == 0 && b.y == 0 && b.z == 0 && b.w == 0; }

    bool Eq4(const V4& a, const V4& b)
    {
        double dx = a.x - b.x, dy = a.y - b.y, dz = a.z - b.z, dw = a.w - b.w;
        return dx * dx + dy * dy + dz * dz + dw * dw < 1e-10;
    }

    double RoundEven(double x)
    {
        double r = JsRound(x);
        return std::fabs(std::fmod(x, 1.0)) == 0.5 && std::fmod(r, 2.0) != 0 ? r - 1 : r;
    }

    V2 Round2(const V2& a)
    {
        return MakeV2(RoundEven(a.x), RoundEven(a.y));
    }

    bool Approximately(double a, double b)
    {
        return std::fabs(b - a) < std::max(1e-6 * std::max(std::fabs(a), std::fabs(b)), 1.401298e-45 * 8);
    }

    double Repeat(double t, double length)
    {
        return std::min(std::max(F(t - std::floor(F(t / length)) * length), 0.0), length);
    }

    // --- colours (ColorUtility.TryParseHtmlString) --------------------------------------------------

    Col ParseColor(const String& value)
    {
        static const std::pair<const char*, const char*> named[] = {
            { "red", "ff0000" }, { "cyan", "00ffff" }, { "blue", "0000ff" }, { "darkblue", "0000a0" }, { "lightblue", "add8e6" },
            { "purple", "800080" }, { "yellow", "ffff00" }, { "lime", "00ff00" }, { "fuchsia", "ff00ff" }, { "white", "ffffff" },
            { "silver", "c0c0c0" }, { "grey", "808080" }, { "gray", "808080" }, { "black", "000000" }, { "orange", "ffa500" },
            { "brown", "a52a2a" }, { "maroon", "800000" }, { "green", "008000" }, { "olive", "808000" }, { "navy", "000080" },
            { "teal", "008080" }, { "aqua", "00ffff" }, { "magenta", "ff00ff" },
        };

        bool found = false;
        std::string hex;
        if (StartsWith(value, "#"))
        {
            hex = value.SubStr(1).Data();
            found = true;
        }
        else
        {
            String lower = JsToLower(value);
            for (auto& n : named)
            {
                if (lower == n.first)
                {
                    hex = n.second;
                    found = true;
                }
            }
        }

        bool valid = found && !hex.empty() && (hex.size() == 3 || hex.size() == 4 || hex.size() == 6 || hex.size() == 8);
        for (char c : hex)
            valid = valid && std::isxdigit((unsigned char)c);
        if (!valid)
            throw ExportError(("Invalid color: " + value).Data());

        if (hex.size() <= 4)
        {
            std::string doubled;
            for (char c : hex)
            {
                doubled += c;
                doubled += c;
            }
            hex = doubled;
        }
        if (hex.size() == 6)
            hex += "ff";

        auto channel = [&](int i) { return F(std::stoi(hex.substr(i * 2, 2), nullptr, 16) / 255.0); };
        return Col{ channel(0), channel(1), channel(2), channel(3) };
    }

    // --- reading the structure ----------------------------------------------------------------------

    static bool IsObj(const Json* v)
    {
        return v && v->IsObject();
    }

    static const Json* Member(const Json* obj, const char* key)
    {
        return obj && obj->IsObject() ? obj->Get(key) : nullptr;
    }

    String TextOf(const Json* obj, const char* key, const String& fallback /*= ""*/)
    {
        const Json* v = Member(obj, key);
        if (!v)
            return fallback;
        return v->IsString() ? v->Str() : v->Dump();
    }

    double NumOf(const Json* obj, const char* key, double fallback /*= 0*/)
    {
        const Json* v = Member(obj, key);
        if (!v || v->IsNull())
        {
            if (std::isnan(fallback))
                throw ExportError(std::string(key) + " is required");
            return fallback;
        }
        if (!v->IsNumber() || !std::isfinite(v->Num()))
            throw ExportError(std::string(key) + " must be a number");
        return v->Num();
    }

    bool BoolOf(const Json* obj, const char* key, bool fallback /*= false*/)
    {
        const Json* v = Member(obj, key);
        if (!v || v->IsNull())
            return fallback;
        if (!v->IsBool())
            throw ExportError(std::string(key) + " must be a boolean");
        return v->Bool();
    }

    V2 PairOf(const Json* obj)
    {
        return MakeV2(NumOf(obj, "x"), NumOf(obj, "y"));
    }

    V2 SizeOf(const Json* obj, const String& label)
    {
        V2 r = MakeV2(NumOf(obj, "width", NAN), NumOf(obj, "height", NAN));
        if (!(r.x > 0) || !(r.y > 0))
            throw ExportError((label + ": size must be positive").Data());
        return Round2(r);
    }

    Vector<const Json*> ChildrenOf(const Json& node)
    {
        Vector<const Json*> out;
        const Json* c = Member(&node, "children");
        if (!c || c->IsNull())
            return out;
        if (!c->IsArray())
            throw ExportError("children must be an array");
        for (auto& item : c->Items())
        {
            if (!item.IsObject())
                throw ExportError("child must be an object");
            out.Add(&item);
        }
        return out;
    }

    bool HasSlice(const Json* value)
    {
        if (!value || value->IsNull())
            return false;
        if (value->IsBool())
            return value->Bool();
        if (value->IsNumber())
            return value->Num() != 0;
        return true;
    }

    String ParentPath(const String& path)
    {
        int slash = path.FindLast("/");
        return slash < 0 ? path.SubStr(0, Math::Max(0, path.Length() - 1)) : path.SubStr(0, slash);
    }

    static bool OneOf(const String& v, std::initializer_list<const char*> words)
    {
        for (auto w : words)
        {
            if (v == w)
                return true;
        }
        return false;
    }

    double FactorF(const String& v)
    {
        return OneOf(v, { "center", "middle" }) ? 0.5 : OneOf(v, { "end", "right", "bottom" }) ? 1 : 0;
    }

    int FactorI(const String& v)
    {
        return OneOf(v, { "center", "middle" }) ? 1 : OneOf(v, { "end", "right", "bottom" }) ? 2 : 0;
    }

    static double AlignOf(double parent, double size, const String& alignment, double fallback)
    {
        if (OneOf(alignment, { "start", "left", "top" }))
            return 0;
        if (OneOf(alignment, { "center", "middle" }))
            return F(std::floor((parent - size) / 2));
        if (OneOf(alignment, { "end", "right", "bottom" }))
            return F(parent - size);
        return fallback;
    }

    static V2 AnchorOf(const Json& node)
    {
        const Json* a = Member(&node, "anchor");
        String h = TextOf(a, "horizontal"), v = TextOf(a, "vertical");
        return MakeV2(h == "right" ? 1 : h == "center" ? 0.5 : 0, v == "bottom" ? 0 : v == "middle" ? 0.5 : 1);
    }

    static bool IsDistribution(const String& mode)
    {
        return OneOf(mode, { "space-between", "space-around", "space-evenly" });
    }

    // --- .NET numeric formats of a progress label --------------------------------------------------

    // The exact decimal expansion of a non-negative double: digits before and after the point
    static void Expand(double x, std::string& whole, std::string& fraction)
    {
        static thread_local std::vector<char> buffer(1500);
        snprintf(buffer.data(), buffer.size(), "%.1100f", x);
        std::string s = buffer.data();
        size_t dot = s.find('.');
        whole = s.substr(0, dot);
        fraction = dot == std::string::npos ? "" : s.substr(dot + 1);
    }

    // Adds one to the last digit of a digit string; true when it carried past the first
    static bool Increment(std::string& digits)
    {
        for (int i = (int)digits.size() - 1; i >= 0; i--)
        {
            if (digits[i] == '9')
                digits[i] = '0';
            else
            {
                digits[i]++;
                return false;
            }
        }
        digits.insert(digits.begin(), '1');
        return true;
    }

    // Number.prototype.toFixed of a non-negative value (ties to the larger)
    static std::string ToFixed(double x, int digits)
    {
        std::string whole, fraction;
        Expand(x, whole, fraction);
        std::string kept = whole + fraction.substr(0, digits);
        if ((int)fraction.size() > digits && fraction[digits] >= '5')
            Increment(kept);
        std::string intPart = kept.substr(0, kept.size() - digits);
        return digits > 0 ? intPart + "." + kept.substr(kept.size() - digits) : intPart;
    }

    // The significant digits (count of them, ties to the larger) and the decimal exponent of a positive value
    static void Significant(double x, int count, std::string& digits, int& exponent)
    {
        std::string whole, fraction;
        Expand(x, whole, fraction);
        std::string all = whole + fraction;
        size_t first = all.find_first_not_of('0');
        if (first == std::string::npos)
        {
            digits = std::string(count, '0');
            exponent = 0;
            return;
        }
        exponent = (int)whole.size() - 1 - (int)first;
        digits = all.substr(first, count);
        while ((int)digits.size() < count)
            digits += '0';
        if (first + count < all.size() && all[first + count] >= '5' && Increment(digits))
        {
            digits.pop_back();
            exponent++;
        }
    }

    // Number.prototype.toExponential
    static std::string ToExponential(double n, int fractionDigits)
    {
        std::string sign = n < 0 ? "-" : "";
        std::string digits;
        int exponent = 0;
        if (n == 0)
            digits = std::string(fractionDigits + 1, '0');
        else
            Significant(std::fabs(n), fractionDigits + 1, digits, exponent);
        std::string mantissa = digits.substr(0, 1) + (fractionDigits > 0 ? "." + digits.substr(1) : "");
        return sign + mantissa + "e" + (exponent < 0 ? "-" : "+") + std::to_string(std::abs(exponent));
    }

    std::string ToPrecision(double n, int precision)
    {
        if (n == 0)
            return precision > 1 ? "0." + std::string(precision - 1, '0') : "0";

        std::string sign = n < 0 ? "-" : "";
        std::string digits;
        int e = 0;
        Significant(std::fabs(n), precision, digits, e);
        if (e < -6 || e >= precision)
        {
            std::string mantissa = digits.substr(0, 1) + (precision > 1 ? "." + digits.substr(1) : "");
            return sign + mantissa + "e" + (e < 0 ? "-" : "+") + std::to_string(std::abs(e));
        }
        if (e >= 0)
        {
            std::string whole = digits.substr(0, e + 1), rest = digits.substr(e + 1);
            return sign + whole + (rest.empty() ? "" : "." + rest);
        }
        return sign + "0." + std::string(-e - 1, '0') + digits;
    }

    static std::string StripZeros(std::string s)
    {
        if (s.find('.') == std::string::npos)
            return s;
        while (!s.empty() && s.back() == '0')
            s.pop_back();
        if (!s.empty() && s.back() == '.')
            s.pop_back();
        return s;
    }

    static std::string Pad(int value, int width)
    {
        std::string s = std::to_string(value);
        while ((int)s.size() < width)
            s = "0" + s;
        return s;
    }

    static std::string DotnetF(double n, int digits)
    {
        std::string s = ToFixed(std::fabs(n), digits);
        return (n < 0 && std::stod(s) != 0 ? "-" : "") + s;
    }

    static std::string DotnetN(double n, int digits)
    {
        std::string fixed = DotnetF(std::fabs(n), digits);
        size_t dot = fixed.find('.');
        std::string i = fixed.substr(0, dot), grouped;
        for (int k = 0; k < (int)i.size(); k++)
        {
            if (k > 0 && ((int)i.size() - k) % 3 == 0)
                grouped += ',';
            grouped += i[k];
        }
        return (n < 0 && std::stod(fixed) != 0 ? "-" : "") + grouped + (dot == std::string::npos ? "" : fixed.substr(dot));
    }

    static std::string DotnetE(double n, int digits)
    {
        std::string s = ToExponential(n, digits);
        size_t e = s.find('e');
        int exp = std::stoi(s.substr(e + 1));
        return s.substr(0, e) + "E" + (exp < 0 ? "-" : "+") + Pad(std::abs(exp), 3);
    }

    static std::string DotnetG(double n, int precision)
    {
        if (n == 0)
            return "0";
        int p = std::max(1, precision);
        int exp = (int)std::floor(std::log10(std::fabs(std::stod(ToPrecision(n, p)))));
        if (exp >= -5 && exp < p)
        {
            std::string s = ToPrecision(n, p);
            if (s.find('e') != std::string::npos)
                s = Json::FormatNumber(std::stod(s)).Data();
            return StripZeros(s);
        }
        std::string s = ToExponential(n, p - 1);
        size_t e = s.find('e');
        int ex = std::stoi(s.substr(e + 1));
        return StripZeros(s.substr(0, e)) + "E" + (ex < 0 ? "-" : "+") + Pad(std::abs(ex), 2);
    }

    static String FormatProgress(const Json& spec)
    {
        double min = NumOf(&spec, "min"), max = NumOf(&spec, "max", 1), value = NumOf(&spec, "value");
        String format = TextOf(&spec, "format", "{percent:.0f}%");

        std::string escaped;
        std::string src = format.Data();
        for (size_t i = 0; i < src.size(); i++)
        {
            if (i + 1 < src.size() && src[i] == '{' && src[i + 1] == '{')
            {
                escaped += '\x01';
                i++;
            }
            else if (i + 1 < src.size() && src[i] == '}' && src[i + 1] == '}')
            {
                escaped += '\x02';
                i++;
            }
            else
                escaped += src[i];
        }

        static const std::regex field("\\{(value|min|max|percent)(?::([^{}]*))?\\}");
        static const std::regex spec_("^(,)?(?:\\.(\\d{1,2}))?([fFgGeE%])$");
        std::string formatted;
        size_t last = 0;
        for (auto it = std::sregex_iterator(escaped.begin(), escaped.end(), field); it != std::sregex_iterator(); ++it)
        {
            auto& m = *it;
            formatted += escaped.substr(last, m.position(0) - last);
            last = m.position(0) + m.length(0);

            std::string key = m[1].str();
            double n = key == "min" ? min : key == "max" ? max : key == "percent" ? ((value - min) / (max - min)) * 100 : value;
            std::string fmt = m[2].matched ? m[2].str() : "";
            if (fmt.empty())
            {
                formatted += DotnetG(n, 15);
                continue;
            }

            std::smatch f;
            if (!std::regex_match(fmt, f, spec_))
                throw ExportError("Unsupported progress numeric format: " + fmt);
            char code = (char)std::toupper((unsigned char)f[3].str()[0]);
            int precision = f[2].matched ? std::stoi(f[2].str()) : 6;
            if (code == '%')
                formatted += DotnetF(n * 100, precision) + "%";
            else
            {
                if (f[1].matched && code == 'F')
                    code = 'N';
                formatted += code == 'F' ? DotnetF(n, precision) : code == 'N' ? DotnetN(n, precision) :
                    code == 'E' ? DotnetE(n, precision) : DotnetG(n, precision);
            }
        }
        formatted += escaped.substr(last);

        if (formatted.find('{') != std::string::npos || formatted.find('}') != std::string::npos)
            throw ExportError(("Unsupported progress format: " + format).Data());
        for (auto& c : formatted)
        {
            if (c == '\x01')
                c = '{';
            else if (c == '\x02')
                c = '}';
        }
        return String(formatted.c_str());
    }

    // --- the document ---------------------------------------------------------------------------------

    Doc::Doc(const Json& structure): json(structure)
    {
        if (!IsObj(Member(&json, "canvas")))
            throw ExportError("canvas must be an object");
        if (!IsObj(Member(&json, "root")))
            throw ExportError("root must be an object");
        ref = SizeOf(json.Get("canvas"), "canvas");
        Index(*json.Get("root"), "root", {});
    }

    void Doc::Index(const Json& node, const String& path, const std::vector<int>& address)
    {
        if (mAddress.count(path.Data()))
            throw ExportError(("Duplicate element path: " + path).Data());
        mAddress[path.Data()] = address;
        mOrder.Add(path);

        auto kids = ChildrenOf(node);
        for (int i = 0; i < kids.Count(); i++)
        {
            String name = TextOf(kids[i], "name");
            if (JsTrim(name).IsEmpty() || name.Contains("/") || name.Contains("\\") || name == "." || name == "..")
                throw ExportError((path + ": invalid child name").Data());
            std::vector<int> child = address;
            child.push_back(i);
            Index(*kids[i], path + "/" + name, child);
        }
    }

    Json& Doc::Node(const String& path)
    {
        auto it = mAddress.find(path.Data());
        if (it == mAddress.end())
            throw ExportError(("unknown element " + path).Data());

        Json* node = json.Get("root");
        for (int i : it->second)
        {
            Json* kids = node ? node->Get("children") : nullptr;
            node = kids && kids->IsArray() && i < kids->Count() ? &kids->Items()[i] : nullptr;
        }
        if (!node)
            throw ExportError(("unknown element " + path).Data());
        return *node;
    }

    Json& Doc::Relative(const String& owner, const String& rel)
    {
        String path = rel == "." ? owner : owner + "/" + rel;
        bool bad = rel.IsEmpty() || StartsWith(rel, "/") || !mAddress.count(path.Data());
        for (auto& part : JsSplit(rel, "/"))
            bad = bad || part == ".." || (part == "." && rel != ".");
        if (bad)
            throw ExportError((owner + ": unknown relative target " + rel).Data());
        return Node(path);
    }

    Doc Doc::Snapshot() const
    {
        Doc r(json);
        for (auto& path : r.mOrder)
        {
            const Json* state = r.Node(path).Get("state");
            if (!IsObj(state))
                continue;

            const Json* variants = state->Get("variants");
            if (!variants || variants->IsNull() || !variants->IsObject())
                throw ExportError((path + ": state variants must be an object").Data());
            const Json* patches = variants->Get(TextOf(state, "current"));
            if (!patches || patches->IsNull())
                throw ExportError((path + ": unknown state " + TextOf(state, "current")).Data());
            if (!patches->IsObject())
                continue;

            Json copy = *patches;
            for (auto& entry : copy.Members())
            {
                if (!entry.second.IsObject())
                    continue;
                Json& target = r.Relative(path, entry.first);
                for (auto& field : entry.second.Members())
                    target[field.first] = field.second;
            }
        }

        for (auto& path : r.mOrder)
        {
            const Json* found = r.Node(path).Get("progress");
            if (!IsObj(found))
                continue;
            Json spec = *found;
            const Json* label = spec.Get("label");
            if (label && !label->IsNull())
                r.Relative(path, TextOf(&spec, "label"))["text"] = FormatProgress(spec);
        }
        return r;
    }

    Vector<std::pair<String, Json>> Doc::Candidates()
    {
        Vector<std::pair<String, Json>> out;
        for (auto& path : mOrder)
            out.Add({ path, Node(path) });

        for (auto& path : mOrder)
        {
            const Json* state = Node(path).Get("state");
            const Json* variants = IsObj(state) ? state->Get("variants") : nullptr;
            if (!IsObj(variants))
                continue;

            Json all = *variants;
            for (auto& variant : all.Members())
            {
                if (!variant.second.IsObject())
                    continue;
                for (auto& patch : variant.second.Members())
                {
                    Json merged = Relative(path, patch.first);
                    if (patch.second.IsObject())
                    {
                        for (auto& field : patch.second.Members())
                            merged[field.first] = field.second;
                    }
                    out.Add({ path + "@" + variant.first + "/" + patch.first, merged });
                }
            }
        }
        return out;
    }

    // --- boxes (the importer's Resolve) ----------------------------------------------------------------

    static std::vector<V2> LayoutPositions(const Vector<const Json*>& kids, const Json& layout, const V2& parent)
    {
        bool row = TextOf(&layout, "type") == "row";
        V2 pad = IsObj(layout.Get("padding")) ? PairOf(layout.Get("padding")) : MakeV2(NumOf(&layout, "padding"), NumOf(&layout, "padding"));
        V2 inner = MakeV2(std::max(0.0, F(parent.x - F(pad.x * 2))), std::max(0.0, F(parent.y - F(pad.y * 2))));
        std::vector<V2> sizes;
        for (auto c : kids)
            sizes.push_back(SizeOf(c->Get("size"), "layout child"));

        int count = kids.Count();
        std::vector<V2> result;
        if (TextOf(&layout, "type") == "grid")
        {
            V2 cell = SizeOf(layout.Get("cellSize"), "grid cell");
            V2 spacing = PairOf(layout.Get("spacing"));
            double columns = std::trunc(NumOf(&layout, "columns"));
            double used = std::min(columns, (double)count);
            double rows = std::floor((count + columns - 1) / columns);
            V2 extent = MakeV2(used * cell.x + std::max(0.0, used - 1) * spacing.x, rows * cell.y + std::max(0.0, rows - 1) * spacing.y);
            V2 start = Add(pad, MakeV2(AlignOf(inner.x, extent.x, TextOf(&layout, "align", "start"), 0),
                                       AlignOf(inner.y, extent.y, TextOf(&layout, "vAlign", "start"), 0)));
            for (int i = 0; i < count; i++)
            {
                V2 at = MakeV2(std::fmod((double)i, columns) * (cell.x + spacing.x), std::floor(i / columns) * (cell.y + spacing.y));
                result.push_back(Add(Add(start, at), PairOf(kids[i]->Get("offset"))));
            }
            return result;
        }

        double main = row ? inner.x : inner.y;
        double total = 0;
        for (auto& s : sizes)
            total = F(total + (row ? s.x : s.y));
        double leftover = std::max(0.0, F(main - total));
        String distribution = TextOf(&layout, "align", "start");
        if (!IsDistribution(distribution) && TextOf(&layout, "spacing") == "even")
            distribution = "space-evenly";

        std::vector<double> gaps(std::max(0, count - 1), 0.0);
        double lead = 0;
        auto slot = [&](double index, double slots)
        {
            return slots <= 0 ? 0.0 : F(std::floor(((index + 1) * leftover) / slots) - std::floor((index * leftover) / slots));
        };
        if (distribution == "space-between")
        {
            for (int i = 0; i < (int)gaps.size(); i++)
                gaps[i] = slot(i, count - 1);
        }
        else if (distribution == "space-around")
        {
            lead = slot(0, 2 * count);
            for (int i = 0; i < (int)gaps.size(); i++)
                gaps[i] = F(slot(2 * i + 1, 2 * count) + slot(2 * i + 2, 2 * count));
        }
        else if (distribution == "space-evenly")
        {
            lead = slot(0, count + 1);
            for (int i = 0; i < (int)gaps.size(); i++)
                gaps[i] = slot(i + 1, count + 1);
        }
        else
        {
            double spacing = F(NumOf(&layout, "spacing"));
            for (auto& g : gaps)
                g = spacing;
            lead = std::max(0.0, AlignOf(main, F(total + spacing * std::max(0, count - 1)), distribution, 0));
        }

        for (int i = 0; i < count; i++)
        {
            double cross = AlignOf(row ? inner.y : inner.x, row ? sizes[i].y : sizes[i].x,
                                   TextOf(kids[i], row ? "vAlign" : "align", TextOf(&layout, "vAlign", "start")), 0);
            V2 at = row ? MakeV2(pad.x + lead, pad.y + cross) : MakeV2(pad.x + cross, pad.y + lead);
            result.push_back(Add(at, PairOf(kids[i]->Get("offset"))));
            lead = F(lead + (row ? sizes[i].x : sizes[i].y) + (i < (int)gaps.size() ? gaps[i] : 0));
        }
        return result;
    }

    std::map<std::string, UBox> Doc::Resolve()
    {
        std::map<std::string, UBox> result;
        Json root = Node("root");
        V2 rootSize = SizeOf(root.Get("size"), "root");
        V2 position = PairOf(root.Get("position"));
        if (Eq2(rootSize, ref) && Eq2(position, V2()))
            rootSize = ref;
        result["root"] = UBox{ position.x, position.y, rootSize.x, rootSize.y };
        ResolveChildren("root", result);
        return result;
    }

    void Doc::ResolveChildren(const String& path, std::map<std::string, UBox>& boxes)
    {
        Json parent = Node(path);
        const UBox& pb = boxes[path.Data()];
        V2 parentSize = MakeV2(pb.w, pb.h);
        auto kids = ChildrenOf(parent);
        const Json* group = IsObj(parent.Get("layout")) ? parent.Get("layout") : nullptr;
        std::vector<V2> positions;
        if (group)
            positions = LayoutPositions(kids, *group, parentSize);

        for (int i = 0; i < kids.Count(); i++)
        {
            const Json& node = *kids[i];
            String childPath = path + "/" + TextOf(&node, "name");
            V2 size = SizeOf(node.Get("size"), childPath);
            V2 pos = PairOf(node.Get("position"));
            V2 offset = PairOf(node.Get("offset"));
            const Json* responsive = IsObj(node.Get("responsive")) ? node.Get("responsive") : nullptr;
            if (responsive)
            {
                V2 start;
                V2 extent = parentSize;
                if (BoolOf(responsive, "safeArea"))
                {
                    const Json* safe = Member(Member(&json, "canvas"), "safeArea");
                    start = MakeV2(NumOf(safe, "left"), NumOf(safe, "top"));
                    extent = Sub(extent, Add(start, MakeV2(NumOf(safe, "right"), NumOf(safe, "bottom"))));
                }
                pos = Round2(Add(Add(start, Scale(extent, PairOf(responsive->Get("min")))), PairOf(responsive->Get("offsetMin"))));
                V2 end = Round2(Add(Add(start, Scale(extent, PairOf(responsive->Get("max")))), PairOf(responsive->Get("offsetMax"))));
                size = Sub(end, pos);
                if (size.x <= 0 || size.y <= 0)
                    throw ExportError((childPath + ": responsive size is non-positive").Data());
            }
            else if (group)
                pos = positions[i];
            else
            {
                V2 anchor = AnchorOf(node);
                double x = node.Has("align") ? AlignOf(parentSize.x, size.x, TextOf(&node, "align"), pos.x) : F(pos.x + F(0 * anchor.x));
                double y = node.Has("vAlign") ? AlignOf(parentSize.y, size.y, TextOf(&node, "vAlign"), pos.y) : F(pos.y + F(0 * F(1 - anchor.y)));
                pos = Add(MakeV2(x, y), offset);
            }

            const Json* scroll = IsObj(parent.Get("scroll")) ? parent.Get("scroll") : nullptr;
            if (scroll && TextOf(&node, "name") == TextOf(scroll, "content") && !Eq2(pos, V2()))
                throw ExportError((childPath + ": scroll content must start at (0,0)").Data());

            V2 p = Round2(pos), s = Round2(size);
            boxes[childPath.Data()] = UBox{ p.x, p.y, s.x, s.y };
            ResolveChildren(childPath, boxes);
        }
    }
}
