#include "o2/stdafx.h"
#include "SvgParser.h"

#include "3rdPartyLibs/pugixml/pugixml.hpp"
#include "o2/Utils/Math/Math.h"

#include <cctype>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <map>
#include <string>

namespace o2
{
    namespace
    {
        const double kPi = 3.14159265358979323846;

        struct NamedColor
        {
            const char* name;
            unsigned    rgb;
        };

        const NamedColor kNamedColors[] =
        {
            { "aliceblue", 0xF0F8FF }, { "antiquewhite", 0xFAEBD7 }, { "aqua", 0x00FFFF }, { "aquamarine", 0x7FFFD4 },
            { "azure", 0xF0FFFF }, { "beige", 0xF5F5DC }, { "bisque", 0xFFE4C4 }, { "black", 0x000000 },
            { "blanchedalmond", 0xFFEBCD }, { "blue", 0x0000FF }, { "blueviolet", 0x8A2BE2 }, { "brown", 0xA52A2A },
            { "burlywood", 0xDEB887 }, { "cadetblue", 0x5F9EA0 }, { "chartreuse", 0x7FFF00 },
            { "chocolate", 0xD2691E }, { "coral", 0xFF7F50 }, { "cornflowerblue", 0x6495ED },
            { "cornsilk", 0xFFF8DC }, { "crimson", 0xDC143C }, { "cyan", 0x00FFFF }, { "darkblue", 0x00008B },
            { "darkcyan", 0x008B8B }, { "darkgoldenrod", 0xB8860B }, { "darkgray", 0xA9A9A9 },
            { "darkgreen", 0x006400 }, { "darkgrey", 0xA9A9A9 }, { "darkkhaki", 0xBDB76B },
            { "darkmagenta", 0x8B008B }, { "darkolivegreen", 0x556B2F }, { "darkorange", 0xFF8C00 },
            { "darkorchid", 0x9932CC }, { "darkred", 0x8B0000 }, { "darksalmon", 0xE9967A },
            { "darkseagreen", 0x8FBC8F }, { "darkslateblue", 0x483D8B }, { "darkslategray", 0x2F4F4F },
            { "darkslategrey", 0x2F4F4F }, { "darkturquoise", 0x00CED1 }, { "darkviolet", 0x9400D3 },
            { "deeppink", 0xFF1493 }, { "deepskyblue", 0x00BFFF }, { "dimgray", 0x696969 }, { "dimgrey", 0x696969 },
            { "dodgerblue", 0x1E90FF }, { "firebrick", 0xB22222 }, { "floralwhite", 0xFFFAF0 },
            { "forestgreen", 0x228B22 }, { "fuchsia", 0xFF00FF }, { "gainsboro", 0xDCDCDC },
            { "ghostwhite", 0xF8F8FF }, { "gold", 0xFFD700 }, { "goldenrod", 0xDAA520 }, { "gray", 0x808080 },
            { "green", 0x008000 }, { "greenyellow", 0xADFF2F }, { "grey", 0x808080 }, { "honeydew", 0xF0FFF0 },
            { "hotpink", 0xFF69B4 }, { "indianred", 0xCD5C5C }, { "indigo", 0x4B0082 }, { "ivory", 0xFFFFF0 },
            { "khaki", 0xF0E68C }, { "lavender", 0xE6E6FA }, { "lavenderblush", 0xFFF0F5 }, { "lawngreen", 0x7CFC00 },
            { "lemonchiffon", 0xFFFACD }, { "lightblue", 0xADD8E6 }, { "lightcoral", 0xF08080 },
            { "lightcyan", 0xE0FFFF }, { "lightgoldenrodyellow", 0xFAFAD2 }, { "lightgray", 0xD3D3D3 },
            { "lightgreen", 0x90EE90 }, { "lightgrey", 0xD3D3D3 }, { "lightpink", 0xFFB6C1 },
            { "lightsalmon", 0xFFA07A }, { "lightseagreen", 0x20B2AA }, { "lightskyblue", 0x87CEFA },
            { "lightslategray", 0x778899 }, { "lightslategrey", 0x778899 }, { "lightsteelblue", 0xB0C4DE },
            { "lightyellow", 0xFFFFE0 }, { "lime", 0x00FF00 }, { "limegreen", 0x32CD32 }, { "linen", 0xFAF0E6 },
            { "magenta", 0xFF00FF }, { "maroon", 0x800000 }, { "mediumaquamarine", 0x66CDAA },
            { "mediumblue", 0x0000CD }, { "mediumorchid", 0xBA55D3 }, { "mediumpurple", 0x9370DB },
            { "mediumseagreen", 0x3CB371 }, { "mediumslateblue", 0x7B68EE }, { "mediumspringgreen", 0x00FA9A },
            { "mediumturquoise", 0x48D1CC }, { "mediumvioletred", 0xC71585 }, { "midnightblue", 0x191970 },
            { "mintcream", 0xF5FFFA }, { "mistyrose", 0xFFE4E1 }, { "moccasin", 0xFFE4B5 },
            { "navajowhite", 0xFFDEAD }, { "navy", 0x000080 }, { "oldlace", 0xFDF5E6 }, { "olive", 0x808000 },
            { "olivedrab", 0x6B8E23 }, { "orange", 0xFFA500 }, { "orangered", 0xFF4500 }, { "orchid", 0xDA70D6 },
            { "palegoldenrod", 0xEEE8AA }, { "palegreen", 0x98FB98 }, { "paleturquoise", 0xAFEEEE },
            { "palevioletred", 0xDB7093 }, { "papayawhip", 0xFFEFD5 }, { "peachpuff", 0xFFDAB9 },
            { "peru", 0xCD853F }, { "pink", 0xFFC0CB }, { "plum", 0xDDA0DD }, { "powderblue", 0xB0E0E6 },
            { "purple", 0x800080 }, { "rebeccapurple", 0x663399 }, { "red", 0xFF0000 }, { "rosybrown", 0xBC8F8F },
            { "royalblue", 0x4169E1 }, { "saddlebrown", 0x8B4513 }, { "salmon", 0xFA8072 },
            { "sandybrown", 0xF4A460 }, { "seagreen", 0x2E8B57 }, { "seashell", 0xFFF5EE }, { "sienna", 0xA0522D },
            { "silver", 0xC0C0C0 }, { "skyblue", 0x87CEEB }, { "slateblue", 0x6A5ACD }, { "slategray", 0x708090 },
            { "slategrey", 0x708090 }, { "snow", 0xFFFAFA }, { "springgreen", 0x00FF7F }, { "steelblue", 0x4682B4 },
            { "tan", 0xD2B48C }, { "teal", 0x008080 }, { "thistle", 0xD8BFD8 }, { "tomato", 0xFF6347 },
            { "turquoise", 0x40E0D0 }, { "violet", 0xEE82EE }, { "wheat", 0xF5DEB3 }, { "white", 0xFFFFFF },
            { "whitesmoke", 0xF5F5F5 }, { "yellow", 0xFFFF00 }, { "yellowgreen", 0x9ACD32 }
        };

        enum class PaintKind { None, Color, Reference };

        struct PaintSpec
        {
            PaintKind   kind = PaintKind::None;
            Color4      color = Color4::Black();
            std::string reference;
            bool        hasFallback = false;
        };

        struct Style
        {
            PaintSpec      fill;
            PaintSpec      stroke;
            VectorFillRule fillRule = VectorFillRule::NonZero;
            float          fillOpacity = 1.0f;
            float          strokeOpacity = 1.0f;
            float          strokeWidth = 1.0f;
            VectorLineCap  strokeCap = VectorLineCap::Butt;
            VectorLineJoin strokeJoin = VectorLineJoin::Miter;
            float          strokeMiterLimit = 4.0f;
            float          opacity = 1.0f;
            Color4         color = Color4::Black();
            bool           visible = true;
            bool           displayed = true;

            Style()
            {
                fill.kind = PaintKind::Color;
            }
        };

        std::string Narrow(const pugi::char_t* text)
        {
            std::string res;
            for (; *text; text++)
                res += (*text > 0 && *text < 128) ? (char)*text : '?';

            return res;
        }

        std::wstring Wide(const char* text)
        {
            std::wstring res;
            for (; *text; text++)
                res += (wchar_t)*text;

            return res;
        }

        std::string LocalName(const pugi::xml_node& node)
        {
            std::string name = Narrow(node.name());
            size_t colon = name.find(':');
            return colon == std::string::npos ? name : name.substr(colon + 1);
        }

        bool HasAttribute(const pugi::xml_node& node, const char* name)
        {
            return !node.attribute(Wide(name).c_str()).empty();
        }

        std::string Attribute(const pugi::xml_node& node, const char* name)
        {
            return Narrow(node.attribute(Wide(name).c_str()).value());
        }

        std::string Trim(const std::string& text)
        {
            size_t begin = text.find_first_not_of(" \t\r\n");
            if (begin == std::string::npos)
                return std::string();

            size_t end = text.find_last_not_of(" \t\r\n");
            return text.substr(begin, end - begin + 1);
        }

        std::string Lower(std::string text)
        {
            for (char& ch : text)
                ch = (char)tolower((unsigned char)ch);

            return text;
        }

        bool IsDigit(char ch)
        {
            return ch >= '0' && ch <= '9';
        }

        void SkipSeparators(const char*& cursor)
        {
            while (*cursor == ' ' || *cursor == '\t' || *cursor == '\r' || *cursor == '\n' || *cursor == ',')
                cursor++;
        }

        // SVG number grammar: sign, digits, fraction, exponent; stops at the first char out of the number
        bool ParseNumber(const char*& cursor, double& value)
        {
            SkipSeparators(cursor);

            const char* begin = cursor;
            const char* end = cursor;
            if (*end == '+' || *end == '-')
                end++;

            bool hasDigits = false;
            while (IsDigit(*end))
            {
                end++;
                hasDigits = true;
            }

            if (*end == '.')
            {
                end++;
                while (IsDigit(*end))
                {
                    end++;
                    hasDigits = true;
                }
            }

            if (!hasDigits)
                return false;

            if (*end == 'e' || *end == 'E')
            {
                const char* exponent = end + 1;
                if (*exponent == '+' || *exponent == '-')
                    exponent++;

                if (IsDigit(*exponent))
                {
                    while (IsDigit(*exponent))
                        exponent++;

                    end = exponent;
                }
            }

            const char* digit = begin;
            double sign = 1.0;
            if (*digit == '+' || *digit == '-')
                sign = *digit++ == '-' ? -1.0 : 1.0;

            double mantissa = 0.0;
            int exponent = 0;
            for (; IsDigit(*digit); digit++)
                mantissa = mantissa*10.0 + (*digit - '0');

            if (*digit == '.')
            {
                for (digit++; IsDigit(*digit); digit++, exponent--)
                    mantissa = mantissa*10.0 + (*digit - '0');
            }

            if ((*digit == 'e' || *digit == 'E') && digit < end)
            {
                digit++;
                int exponentSign = 1;
                if (*digit == '+' || *digit == '-')
                    exponentSign = *digit++ == '-' ? -1 : 1;

                int power = 0;
                for (; IsDigit(*digit) && power < 10000; digit++)
                    power = power*10 + (*digit - '0');

                exponent += power*exponentSign;
            }

            double res = sign*mantissa*pow(10.0, (double)exponent);
            if (!std::isfinite(res))
                return false;

            value = res;
            cursor = end;
            return true;
        }

        enum class LengthUnit { User, Percent, Unknown };

        // Number with optional unit in user units; percents are returned as fraction
        bool ParseLength(const std::string& text, float& value, bool unitsAsPixels = false,
                         LengthUnit* resUnit = nullptr)
        {
            const char* cursor = text.c_str();
            double number = 0;
            if (!ParseNumber(cursor, number))
                return false;

            std::string unit = Lower(Trim(cursor));
            LengthUnit kind = LengthUnit::User;

            if (unit == "%")
            {
                number /= 100.0;
                kind = LengthUnit::Percent;
            }
            else if (unit == "pt" || unit == "pc" || unit == "mm" || unit == "cm" || unit == "in")
            {
                if (!unitsAsPixels)
                {
                    number *= unit == "pt" ? 96.0/72.0 : unit == "pc" ? 16.0 : unit == "mm" ? 96.0/25.4 :
                        unit == "cm" ? 96.0/2.54 : 96.0;
                }
            }
            else if (!unit.empty() && unit != "px")
                kind = LengthUnit::Unknown;

            if (resUnit)
                *resUnit = kind;

            if (!std::isfinite((float)number))
                return false;

            value = (float)number;
            return true;
        }

        double HueChannel(double p, double q, double t)
        {
            t = t - floor(t);
            if (t < 1.0/6.0)
                return p + (q - p)*6.0*t;

            if (t < 0.5)
                return q;

            return t < 2.0/3.0 ? p + (q - p)*(2.0/3.0 - t)*6.0 : p;
        }

        int ToChannel(double value)
        {
            return (int)floor(Math::Clamp(value, 0.0, 255.0) + 0.5);
        }

        bool ParseColorText(const std::string& source, Color4& color)
        {
            std::string text = Lower(Trim(source));
            if (text.empty())
                return false;

            if (text[0] == '#')
            {
                std::string hex = text.substr(1);
                for (char ch : hex)
                {
                    if (!isxdigit((unsigned char)ch))
                        return false;
                }

                unsigned long value = strtoul(hex.c_str(), nullptr, 16);
                if (hex.size() == 3 || hex.size() == 4)
                {
                    int shift = hex.size() == 4 ? 4 : 0;
                    int r = (value >> (8 + shift)) & 0xF, g = (value >> (4 + shift)) & 0xF, b = (value >> shift) & 0xF;
                    int a = hex.size() == 4 ? (int)(value & 0xF) : 0xF;
                    color = Color4(r*17, g*17, b*17, a*17);
                    return true;
                }

                if (hex.size() == 6)
                {
                    color = Color4((int)((value >> 16) & 0xFF), (int)((value >> 8) & 0xFF), (int)(value & 0xFF), 255);
                    return true;
                }

                if (hex.size() == 8)
                {
                    color = Color4((int)((value >> 24) & 0xFF), (int)((value >> 16) & 0xFF),
                                   (int)((value >> 8) & 0xFF), (int)(value & 0xFF));
                    return true;
                }

                return false;
            }

            if (text.compare(0, 3, "rgb") == 0)
            {
                size_t open = text.find('(');
                if (open == std::string::npos)
                    return false;

                const char* cursor = text.c_str() + open + 1;
                int channels[4] = { 0, 0, 0, 255 };
                int count = 0;
                for (; count < 4; count++)
                {
                    double value = 0;
                    if (!ParseNumber(cursor, value))
                        break;

                    bool percent = *cursor == '%';
                    if (percent)
                        cursor++;

                    if (count == 3)
                        value = percent ? value*255.0/100.0 : value*255.0;
                    else if (percent)
                        value = value*255.0/100.0;

                    channels[count] = ToChannel(value);

                    SkipSeparators(cursor);
                    if (*cursor == '/')
                        cursor++;
                }

                if (count < 3)
                    return false;

                color = Color4(channels[0], channels[1], channels[2], channels[3]);
                return true;
            }

            if (text.compare(0, 3, "hsl") == 0)
            {
                size_t open = text.find('(');
                if (open == std::string::npos)
                    return false;

                const char* cursor = text.c_str() + open + 1;
                double values[4] = { 0, 0, 0, 1 };
                int count = 0;
                for (; count < 4; count++)
                {
                    if (!ParseNumber(cursor, values[count]))
                        break;

                    bool percent = *cursor == '%';
                    if (percent)
                        cursor++;

                    if (count > 0 && (percent || count < 3))
                        values[count] /= 100.0;

                    while (isalpha((unsigned char)*cursor))
                        cursor++;

                    SkipSeparators(cursor);
                    if (*cursor == '/')
                        cursor++;
                }

                if (count < 3)
                    return false;

                double hue = values[0]/360.0, saturation = Math::Clamp(values[1], 0.0, 1.0);
                double lightness = Math::Clamp(values[2], 0.0, 1.0);
                double q = lightness < 0.5 ? lightness*(1.0 + saturation) :
                    lightness + saturation - lightness*saturation;
                double p = 2.0*lightness - q;

                color = Color4(ToChannel(HueChannel(p, q, hue + 1.0/3.0)*255.0), ToChannel(HueChannel(p, q, hue)*255.0),
                               ToChannel(HueChannel(p, q, hue - 1.0/3.0)*255.0), ToChannel(values[3]*255.0));
                return true;
            }

            if (text == "transparent")
            {
                color = Color4(0, 0, 0, 0);
                return true;
            }

            if (text == "currentcolor")
            {
                color = Color4::Black();
                return true;
            }

            for (const NamedColor& named : kNamedColors)
            {
                if (text == named.name)
                {
                    color = Color4((int)((named.rgb >> 16) & 0xFF), (int)((named.rgb >> 8) & 0xFF),
                                   (int)(named.rgb & 0xFF), 255);
                    return true;
                }
            }

            return false;
        }

        Basis MatrixBasis(double a, double b, double c, double d, double e, double f)
        {
            return Basis(Vec2F((float)e, (float)f), Vec2F((float)a, (float)b), Vec2F((float)c, (float)d));
        }

        bool ParseTransformText(const std::string& text, Basis& transform)
        {
            transform = Basis::Identity();

            const char* cursor = text.c_str();
            while (true)
            {
                SkipSeparators(cursor);
                if (!*cursor)
                    return true;

                const char* nameBegin = cursor;
                while (isalpha((unsigned char)*cursor))
                    cursor++;

                std::string name(nameBegin, cursor);
                SkipSeparators(cursor);
                if (*cursor != '(')
                    return false;

                cursor++;

                double args[6];
                int count = 0;
                while (count < 6 && ParseNumber(cursor, args[count]))
                    count++;

                SkipSeparators(cursor);
                if (*cursor != ')')
                    return false;

                cursor++;

                Basis local;
                if (name == "matrix" && count == 6)
                    local = MatrixBasis(args[0], args[1], args[2], args[3], args[4], args[5]);
                else if (name == "translate" && count >= 1)
                    local = MatrixBasis(1, 0, 0, 1, args[0], count > 1 ? args[1] : 0.0);
                else if (name == "scale" && count >= 1)
                    local = MatrixBasis(args[0], 0, 0, count > 1 ? args[1] : args[0], 0, 0);
                else if (name == "rotate" && (count == 1 || count == 3))
                {
                    double angle = args[0]*kPi/180.0;
                    double cs = cos(angle), sn = sin(angle);
                    double cx = count == 3 ? args[1] : 0.0, cy = count == 3 ? args[2] : 0.0;
                    local = MatrixBasis(cs, sn, -sn, cs, cx - cs*cx + sn*cy, cy - sn*cx - cs*cy);
                }
                else if (name == "skewX" && count == 1)
                    local = MatrixBasis(1, 0, tan(args[0]*kPi/180.0), 1, 0, 0);
                else if (name == "skewY" && count == 1)
                    local = MatrixBasis(1, tan(args[0]*kPi/180.0), 0, 1, 0, 0);
                else
                    return false;

                transform = local*transform;
            }
        }

        struct PathBuilder
        {
            Vector<VectorSubPath>& subPaths;

            Vec2F current;
            Vec2F subPathStart;
            Vec2F lastControl;
            char  lastCommand = 0;
            bool  open = false;

            PathBuilder(Vector<VectorSubPath>& subPaths):
                subPaths(subPaths)
            {}

            void MoveTo(const Vec2F& point)
            {
                current = subPathStart = point;
                open = false;
            }

            VectorSubPath& Current()
            {
                if (!open)
                {
                    subPaths.Add(VectorSubPath());
                    subPaths.back().start = current;
                    subPathStart = current;
                    open = true;
                }

                return subPaths.back();
            }

            void LineTo(const Vec2F& point)
            {
                Current().LineTo(point);
                current = point;
            }

            void CubicTo(const Vec2F& control1, const Vec2F& control2, const Vec2F& point)
            {
                Current().CubicTo(control1, control2, point);
                lastControl = control2;
                current = point;
            }

            void QuadTo(const Vec2F& control, const Vec2F& point)
            {
                Vec2F from = current;
                Current().CubicTo(from + (control - from)*(2.0f/3.0f), point + (control - point)*(2.0f/3.0f), point);
                lastControl = control;
                current = point;
            }

            void Close()
            {
                if (open)
                    subPaths.back().closed = true;
                else if (lastCommand == 'M')
                    Current().closed = true;

                current = subPathStart;
                open = false;
            }

            // Elliptical arc by SVG endpoint parameterization, converted to cubics of at most 90 degrees
            void ArcTo(double rx, double ry, double rotation, bool largeArc, bool sweep, const Vec2F& point)
            {
                double x1 = current.x, y1 = current.y, x2 = point.x, y2 = point.y;
                rx = fabs(rx);
                ry = fabs(ry);

                if (x1 == x2 && y1 == y2)
                    return;

                if (rx < 1e-9 || ry < 1e-9)
                {
                    LineTo(point);
                    return;
                }

                double phi = rotation*kPi/180.0;
                double cs = cos(phi), sn = sin(phi);

                double dx = (x1 - x2)*0.5, dy = (y1 - y2)*0.5;
                double x1p = cs*dx + sn*dy, y1p = -sn*dx + cs*dy;

                double lambda = x1p*x1p/(rx*rx) + y1p*y1p/(ry*ry);
                if (lambda > 1.0)
                {
                    rx *= sqrt(lambda);
                    ry *= sqrt(lambda);
                }

                double numerator = rx*rx*ry*ry - rx*rx*y1p*y1p - ry*ry*x1p*x1p;
                double denominator = rx*rx*y1p*y1p + ry*ry*x1p*x1p;
                double factor = denominator > 0.0 ? sqrt(fmax(numerator/denominator, 0.0)) : 0.0;
                if (largeArc == sweep)
                    factor = -factor;

                double cxp = factor*rx*y1p/ry, cyp = -factor*ry*x1p/rx;
                double cx = cs*cxp - sn*cyp + (x1 + x2)*0.5, cy = sn*cxp + cs*cyp + (y1 + y2)*0.5;

                double startAngle = atan2((y1p - cyp)/ry, (x1p - cxp)/rx);
                double endAngle = atan2((-y1p - cyp)/ry, (-x1p - cxp)/rx);
                double delta = endAngle - startAngle;
                if (sweep && delta < 0.0)
                    delta += 2.0*kPi;
                else if (!sweep && delta > 0.0)
                    delta -= 2.0*kPi;

                int parts = Math::Max(1, (int)ceil(fabs(delta)/(kPi*0.5) - 1e-9));
                double step = delta/parts;
                double handle = 4.0/3.0*tan(step*0.25);

                auto pointAt = [&](double angle, double hx, double hy)
                {
                    double px = rx*(cos(angle) + hx), py = ry*(sin(angle) + hy);
                    return Vec2F((float)(cs*px - sn*py + cx), (float)(sn*px + cs*py + cy));
                };

                for (int i = 0; i < parts; i++)
                {
                    double a1 = startAngle + step*i, a2 = a1 + step;
                    Vec2F control1 = pointAt(a1, -handle*sin(a1), handle*cos(a1));
                    Vec2F control2 = pointAt(a2, handle*sin(a2), -handle*cos(a2));
                    CubicTo(control1, control2, i == parts - 1 ? point : pointAt(a2, 0, 0));
                }
            }
        };

        bool ParseFlag(const char*& cursor, bool& flag)
        {
            SkipSeparators(cursor);
            if (*cursor != '0' && *cursor != '1')
                return false;

            flag = *cursor == '1';
            cursor++;
            return true;
        }

        bool ParsePathText(const std::string& data, Vector<VectorSubPath>& subPaths)
        {
            PathBuilder builder(subPaths);

            const char* cursor = data.c_str();
            char command = 0;
            bool first = true;

            while (true)
            {
                SkipSeparators(cursor);
                if (!*cursor)
                    return true;

                bool explicitCommand = isalpha((unsigned char)*cursor) != 0;
                if (explicitCommand)
                    command = *cursor++;
                else if (command == 0 || command == 'Z' || command == 'z')
                    return false;
                else if (command == 'M')
                    command = 'L';
                else if (command == 'm')
                    command = 'l';

                if (first && command != 'M' && command != 'm')
                    return false;

                first = false;

                bool relative = command >= 'a' && command <= 'z';
                Vec2F base = relative ? builder.current : Vec2F();
                char upper = (char)toupper(command);

                double v[7];
                auto read = [&](int count)
                {
                    for (int i = 0; i < count; i++)
                    {
                        if (!ParseNumber(cursor, v[i]))
                            return false;
                    }

                    return true;
                };

                auto point = [&](int idx) { return Vec2F((float)v[idx], (float)v[idx + 1]) + base; };

                if (upper == 'M')
                {
                    if (!read(2))
                        return false;

                    builder.MoveTo(point(0));
                }
                else if (upper == 'L')
                {
                    if (!read(2))
                        return false;

                    builder.LineTo(point(0));
                }
                else if (upper == 'H')
                {
                    if (!read(1))
                        return false;

                    builder.LineTo(Vec2F((float)v[0] + base.x, builder.current.y));
                }
                else if (upper == 'V')
                {
                    if (!read(1))
                        return false;

                    builder.LineTo(Vec2F(builder.current.x, (float)v[0] + base.y));
                }
                else if (upper == 'C')
                {
                    if (!read(6))
                        return false;

                    builder.CubicTo(point(0), point(2), point(4));
                }
                else if (upper == 'S')
                {
                    if (!read(4))
                        return false;

                    bool smooth = builder.lastCommand == 'C' || builder.lastCommand == 'S';
                    Vec2F control1 = smooth ? builder.current*2.0f - builder.lastControl : builder.current;
                    builder.CubicTo(control1, point(0), point(2));
                }
                else if (upper == 'Q')
                {
                    if (!read(4))
                        return false;

                    builder.QuadTo(point(0), point(2));
                }
                else if (upper == 'T')
                {
                    if (!read(2))
                        return false;

                    bool smooth = builder.lastCommand == 'Q' || builder.lastCommand == 'T';
                    Vec2F control = smooth ? builder.current*2.0f - builder.lastControl : builder.current;
                    builder.QuadTo(control, point(0));
                }
                else if (upper == 'A')
                {
                    bool largeArc = false, sweep = false;
                    if (!read(3) || !ParseFlag(cursor, largeArc) || !ParseFlag(cursor, sweep))
                        return false;

                    double rx = v[0], ry = v[1], rotation = v[2];
                    if (!read(2))
                        return false;

                    builder.ArcTo(rx, ry, rotation, largeArc, sweep, point(0));
                }
                else if (upper == 'Z')
                    builder.Close();
                else
                    return false;

                builder.lastCommand = upper;
            }
        }

        // Mapping of the view box into the viewport by preserveAspectRatio
        Basis ViewportTransform(const Vec2F& position, const Vec2F& size, const Vec2F& boxOrigin, const Vec2F& boxSize,
                                const std::string& aspectRatio)
        {
            std::string mode = Lower(Trim(aspectRatio));
            Vec2F scale(size.x/boxSize.x, size.y/boxSize.y);
            Vec2F offset;

            if (mode.compare(0, 4, "none") != 0)
            {
                float uniform = mode.find("slice") != std::string::npos ? Math::Max(scale.x, scale.y) :
                    Math::Min(scale.x, scale.y);

                auto has = [&](const char* part) { return mode.find(part) != std::string::npos; };
                Vec2F align(has("xmin") ? 0.0f : has("xmax") ? 1.0f : 0.5f,
                            has("ymin") ? 0.0f : has("ymax") ? 1.0f : 0.5f);

                offset = Vec2F((size.x - boxSize.x*uniform)*align.x, (size.y - boxSize.y*uniform)*align.y);
                scale = Vec2F(uniform, uniform);
            }

            Vec2F origin(position.x - boxOrigin.x*scale.x + offset.x, position.y - boxOrigin.y*scale.y + offset.y);
            return Basis(origin, Vec2F(scale.x, 0.0f), Vec2F(0.0f, scale.y));
        }

        class Document
        {
        public:
            VectorImage&           image;
            Vector<String>&        warnings;
            const SvgParseOptions& options;

            std::map<std::string, pugi::xml_node>  ids;
            std::map<std::string, std::string>     classRules;
            std::vector<pugi::xml_node>            usedTargets;

            Vec2F viewSize; // Size of the nearest view box, reference of percent lengths

            Document(VectorImage& image, Vector<String>& warnings, const SvgParseOptions& options):
                image(image), warnings(warnings), options(options)
            {}

            enum class LengthAxis { X, Y, Diagonal };

            float Length(const std::string& text, float defaultValue, LengthAxis axis)
            {
                float value = defaultValue;
                LengthUnit unit = LengthUnit::User;
                if (!ParseLength(text, value, options.unitsAsPixels, &unit))
                    return defaultValue;

                if (unit == LengthUnit::Unknown)
                    Warn("unsupported unit in length '" + Trim(text) + "'");
                else if (unit == LengthUnit::Percent)
                {
                    value *= axis == LengthAxis::X ? viewSize.x : axis == LengthAxis::Y ? viewSize.y :
                        Math::Sqrt((viewSize.x*viewSize.x + viewSize.y*viewSize.y)*0.5f);
                }

                return value;
            }

            float LengthAttribute(const pugi::xml_node& node, const char* name, LengthAxis axis)
            {
                return Length(Attribute(node, name), 0.0f, axis);
            }

            Vec2F PointAttribute(const pugi::xml_node& node, const char* nameX, const char* nameY)
            {
                return Vec2F(LengthAttribute(node, nameX, LengthAxis::X), LengthAttribute(node, nameY, LengthAxis::Y));
            }

            void Warn(const std::string& text)
            {
                String warning(text.c_str());
                if (!warnings.Contains(warning))
                    warnings.Add(warning);
            }

            void Collect(const pugi::xml_node& node)
            {
                for (pugi::xml_node child : node)
                {
                    if (child.type() != pugi::node_element)
                        continue;

                    std::string id = Attribute(child, "id");
                    if (!id.empty())
                        ids[id] = child;

                    if (LocalName(child) == "style")
                        CollectClassRules(Narrow(child.text().get()));

                    Collect(child);
                }
            }

            void CollectClassRules(const std::string& source)
            {
                std::string css;
                for (size_t i = 0; i < source.size();)
                {
                    size_t comment = source.find("/*", i);
                    css += source.substr(i, comment == std::string::npos ? comment : comment - i);
                    size_t end = comment == std::string::npos ? comment : source.find("*/", comment + 2);
                    i = end == std::string::npos ? source.size() : end + 2;
                }

                size_t cursor = 0;
                while (true)
                {
                    size_t open = css.find('{', cursor);
                    size_t close = open == std::string::npos ? open : css.find('}', open);
                    if (close == std::string::npos)
                        return;

                    std::string selectors = css.substr(cursor, open - cursor);
                    size_t rule = selectors.find('@');
                    if (rule != std::string::npos)
                    {
                        size_t statementEnd = selectors.find(';', rule);
                        if (statementEnd != std::string::npos)
                        {
                            cursor += statementEnd + 1;
                            continue;
                        }

                        int depth = 1;
                        for (close = open + 1; close < css.size() && depth > 0; close++)
                            depth += css[close] == '{' ? 1 : css[close] == '}' ? -1 : 0;

                        Warn("unsupported css rule '" + Trim(selectors.substr(rule)) + "'");
                        cursor = close;
                        continue;
                    }

                    std::string declarations = css.substr(open + 1, close - open - 1);
                    cursor = close + 1;

                    size_t begin = 0;
                    while (begin <= selectors.size())
                    {
                        size_t comma = selectors.find(',', begin);
                        if (comma == std::string::npos)
                            comma = selectors.size();

                        std::string selector = Trim(selectors.substr(begin, comma - begin));
                        if (selector.size() > 1 && selector[0] == '.')
                            classRules[selector.substr(1)] += declarations + ";";
                        else if (!selector.empty())
                            Warn("unsupported css selector '" + selector + "'");

                        begin = comma + 1;
                    }
                }
            }

            void ApplyPaint(PaintSpec& paint, const std::string& value, const char* property,
                            const Color4& currentColor)
            {
                std::string text = Trim(value);
                if (text == "none")
                {
                    paint = PaintSpec();
                    return;
                }

                if (Lower(text) == "currentcolor")
                {
                    paint = PaintSpec();
                    paint.kind = PaintKind::Color;
                    paint.color = currentColor;
                    return;
                }

                if (text.compare(0, 4, "url(") == 0)
                {
                    size_t close = text.find(')');
                    if (close == std::string::npos)
                        return;

                    std::string reference = Trim(text.substr(4, close - 4));
                    if (!reference.empty() && (reference[0] == '"' || reference[0] == '\''))
                        reference = reference.substr(1, reference.size() - 2);

                    if (!reference.empty() && reference[0] == '#')
                        reference = reference.substr(1);

                    paint.kind = PaintKind::Reference;
                    paint.reference = reference;

                    std::string fallback = Trim(text.substr(close + 1));
                    paint.hasFallback = !fallback.empty() && fallback != "none" &&
                        ParseColorText(fallback, paint.color);
                    return;
                }

                Color4 color;
                if (!ParseColorText(text, color))
                {
                    Warn(std::string("unknown ") + property + " color '" + text + "'");
                    return;
                }

                paint.kind = PaintKind::Color;
                paint.color = color;
                paint.reference.clear();
            }

            void ApplyOpacity(float& target, const std::string& value)
            {
                float opacity = 1.0f;
                if (ParseLength(value, opacity))
                    target = Math::Clamp01(opacity);
            }

            void ApplyProperty(Style& style, float& elementOpacity, const std::string& name, const std::string& source)
            {
                std::string value = Trim(source);
                if (value.empty() || value == "inherit")
                    return;

                if (name == "color")
                {
                    if (!ParseColorText(value, style.color))
                        Warn("unknown color '" + value + "'");
                }
                else if (name == "fill")
                    ApplyPaint(style.fill, value, "fill", style.color);
                else if (name == "stroke")
                    ApplyPaint(style.stroke, value, "stroke", style.color);
                else if (name == "fill-opacity")
                    ApplyOpacity(style.fillOpacity, value);
                else if (name == "stroke-opacity")
                    ApplyOpacity(style.strokeOpacity, value);
                else if (name == "opacity")
                    ApplyOpacity(elementOpacity, value);
                else if (name == "fill-rule")
                    style.fillRule = value == "evenodd" ? VectorFillRule::EvenOdd : VectorFillRule::NonZero;
                else if (name == "stroke-width")
                    style.strokeWidth = Length(value, style.strokeWidth, LengthAxis::Diagonal);
                else if (name == "stroke-miterlimit")
                    ParseLength(value, style.strokeMiterLimit);
                else if (name == "stroke-linecap")
                    style.strokeCap = value == "round" ? VectorLineCap::Round :
                                      value == "square" ? VectorLineCap::Square : VectorLineCap::Butt;
                else if (name == "stroke-linejoin")
                    style.strokeJoin = value == "round" ? VectorLineJoin::Round :
                                       value == "bevel" ? VectorLineJoin::Bevel : VectorLineJoin::Miter;
                else if (name == "display")
                    style.displayed = value != "none";
                else if (name == "visibility")
                    style.visible = value != "hidden" && value != "collapse";
                else if (name == "stroke-dasharray" && value != "none")
                    Warn("unsupported property 'stroke-dasharray'");
                else if ((name == "clip-path" || name == "mask" || name == "filter") && value != "none")
                    Warn("unsupported property '" + name + "'");
            }

            void ApplyDeclarations(Style& style, float& elementOpacity, const std::string& declarations)
            {
                size_t begin = 0;
                while (begin < declarations.size())
                {
                    size_t end = declarations.find(';', begin);
                    if (end == std::string::npos)
                        end = declarations.size();

                    size_t colon = declarations.find(':', begin);
                    if (colon != std::string::npos && colon < end)
                    {
                        std::string value = declarations.substr(colon + 1, end - colon - 1);
                        size_t important = Lower(value).find("!important");
                        if (important != std::string::npos)
                            value.erase(important);

                        std::string name = Lower(Trim(declarations.substr(begin, colon - begin)));
                        ApplyProperty(style, elementOpacity, name, value);
                    }

                    begin = end + 1;
                }
            }

            // Presentation attributes, then class rules, then style attribute; opacity is multiplied into children
            Style ResolveStyle(const pugi::xml_node& node, const Style& parent)
            {
                static const char* properties[] =
                {
                    "color", "fill", "stroke", "fill-opacity", "stroke-opacity", "opacity", "fill-rule", "stroke-width",
                    "stroke-miterlimit", "stroke-linecap", "stroke-linejoin", "display", "visibility",
                    "stroke-dasharray", "clip-path", "mask", "filter"
                };

                Style style = parent;
                float elementOpacity = 1.0f;

                for (const char* property : properties)
                {
                    if (HasAttribute(node, property))
                        ApplyProperty(style, elementOpacity, property, Attribute(node, property));
                }

                std::string classes = Attribute(node, "class");
                size_t begin = 0;
                while (begin < classes.size())
                {
                    size_t end = classes.find_first_of(" \t\r\n", begin);
                    if (end == std::string::npos)
                        end = classes.size();

                    auto rule = classRules.find(classes.substr(begin, end - begin));
                    if (rule != classRules.end())
                        ApplyDeclarations(style, elementOpacity, rule->second);

                    begin = end + 1;
                }

                ApplyDeclarations(style, elementOpacity, Attribute(node, "style"));

                style.opacity = parent.opacity*elementOpacity;
                return style;
            }

            pugi::xml_node Referenced(const pugi::xml_node& node)
            {
                std::string href = Attribute(node, "href");
                if (href.empty())
                    href = Attribute(node, "xlink:href");

                if (href.size() < 2 || href[0] != '#')
                    return pugi::xml_node();

                auto found = ids.find(href.substr(1));
                return found != ids.end() ? found->second : pugi::xml_node();
            }

            // Attribute of gradient or of the nearest gradient in its href chain
            bool GradientAttribute(pugi::xml_node node, const char* name, std::string& value)
            {
                for (int depth = 0; depth < 16 && node; depth++)
                {
                    if (HasAttribute(node, name))
                    {
                        value = Attribute(node, name);
                        return true;
                    }

                    node = Referenced(node);
                }

                return false;
            }

            float GradientCoordinate(const pugi::xml_node& node, const char* name, float defaultFraction,
                                     float userRange, bool boundingBoxUnits)
            {
                float value = defaultFraction;
                bool isPercent = true;

                std::string text;
                if (GradientAttribute(node, name, text))
                {
                    LengthUnit unit = LengthUnit::User;
                    ParseLength(text, value, options.unitsAsPixels, &unit);
                    isPercent = unit == LengthUnit::Percent;
                }

                if (isPercent && !boundingBoxUnits)
                    value *= userRange;

                return value;
            }

            void ReadStops(pugi::xml_node node, Vector<VectorGradientStop>& stops)
            {
                for (int depth = 0; depth < 16 && node; depth++)
                {
                    for (pugi::xml_node child : node)
                    {
                        if (child.type() != pugi::node_element || LocalName(child) != "stop")
                            continue;

                        std::string colorText = Attribute(child, "stop-color");
                        std::string opacityText = Attribute(child, "stop-opacity");

                        std::string declarations = Attribute(child, "style");
                        size_t begin = 0;
                        while (begin < declarations.size())
                        {
                            size_t end = declarations.find(';', begin);
                            if (end == std::string::npos)
                                end = declarations.size();

                            size_t colon = declarations.find(':', begin);
                            if (colon != std::string::npos && colon < end)
                            {
                                std::string name = Lower(Trim(declarations.substr(begin, colon - begin)));
                                std::string value = declarations.substr(colon + 1, end - colon - 1);
                                if (name == "stop-color")
                                    colorText = value;
                                else if (name == "stop-opacity")
                                    opacityText = value;
                            }

                            begin = end + 1;
                        }

                        VectorGradientStop stop;
                        stop.color = Color4::Black();
                        if (!colorText.empty() && !ParseColorText(colorText, stop.color))
                            Warn("unknown stop color '" + Trim(colorText) + "'");

                        float opacity = 1.0f;
                        ParseLength(opacityText, opacity);
                        stop.color.a = Math::RoundToInt((float)stop.color.a*Math::Clamp01(opacity));

                        ParseLength(Attribute(child, "offset"), stop.offset);
                        stop.offset = Math::Clamp01(stop.offset);
                        if (!stops.empty())
                            stop.offset = Math::Max(stop.offset, stops.back().offset);

                        stops.Add(stop);
                    }

                    if (!stops.empty())
                        return;

                    node = Referenced(node);
                }
            }

            VectorPaint ResolvePaint(const PaintSpec& spec, const VectorShape& localShape, const Basis& transform)
            {
                if (spec.kind == PaintKind::None)
                    return VectorPaint();

                if (spec.kind == PaintKind::Color)
                    return VectorPaint::Solid(spec.color);

                VectorPaint fallback = spec.hasFallback ? VectorPaint::Solid(spec.color) : VectorPaint();

                auto found = ids.find(spec.reference);
                if (found == ids.end())
                {
                    Warn("paint reference '#" + spec.reference + "' is not found");
                    return fallback;
                }

                pugi::xml_node node = found->second;
                std::string kind = LocalName(node);
                if (kind != "linearGradient" && kind != "radialGradient")
                {
                    Warn("unsupported paint server <" + kind + ">");
                    return fallback;
                }

                VectorPaint paint;
                ReadStops(node, paint.stops);
                if (paint.stops.empty())
                    return VectorPaint();

                if (paint.stops.size() == 1)
                    return VectorPaint::Solid(paint.stops[0].color);

                std::string text;
                bool boundingBoxUnits = !GradientAttribute(node, "gradientUnits", text) ||
                    Trim(text) != "userSpaceOnUse";

                if (GradientAttribute(node, "spreadMethod", text) && Trim(text) != "pad")
                    Warn("unsupported spreadMethod '" + Trim(text) + "', pad is used");

                Basis gradientTransform;
                if (GradientAttribute(node, "gradientTransform", text) && !ParseTransformText(text, gradientTransform))
                    Warn("invalid gradientTransform '" + text + "'");

                float viewDiagonal = Math::Sqrt((viewSize.x*viewSize.x + viewSize.y*viewSize.y)*0.5f);

                if (kind == "linearGradient")
                {
                    paint.type = VectorPaintType::LinearGradient;
                    paint.begin.x = GradientCoordinate(node, "x1", 0.0f, viewSize.x, boundingBoxUnits);
                    paint.begin.y = GradientCoordinate(node, "y1", 0.0f, viewSize.y, boundingBoxUnits);
                    paint.end.x = GradientCoordinate(node, "x2", 1.0f, viewSize.x, boundingBoxUnits);
                    paint.end.y = GradientCoordinate(node, "y2", 0.0f, viewSize.y, boundingBoxUnits);
                }
                else
                {
                    paint.type = VectorPaintType::RadialGradient;
                    paint.center.x = GradientCoordinate(node, "cx", 0.5f, viewSize.x, boundingBoxUnits);
                    paint.center.y = GradientCoordinate(node, "cy", 0.5f, viewSize.y, boundingBoxUnits);
                    paint.radius = GradientCoordinate(node, "r", 0.5f, viewDiagonal, boundingBoxUnits);
                    paint.focal.x = GradientCoordinate(node, "fx", paint.center.x, viewSize.x, boundingBoxUnits);
                    paint.focal.y = GradientCoordinate(node, "fy", paint.center.y, viewSize.y, boundingBoxUnits);

                    std::string focalText;
                    if (!GradientAttribute(node, "fx", focalText))
                        paint.focal.x = paint.center.x;

                    if (!GradientAttribute(node, "fy", focalText))
                        paint.focal.y = paint.center.y;

                    if (paint.radius <= 0.0f)
                        return VectorPaint::Solid(paint.stops.back().color);
                }

                paint.transform = gradientTransform;
                if (boundingBoxUnits)
                {
                    Vec2F min, max;
                    if (!localShape.GetBounds(min, max) || max.x - min.x <= 0.0f || max.y - min.y <= 0.0f)
                        return VectorPaint();

                    Basis bounds(min, Vec2F(max.x - min.x, 0.0f), Vec2F(0.0f, max.y - min.y));
                    paint.transform = paint.transform*bounds;
                }

                paint.transform = paint.transform*transform;
                return paint;
            }

            void ReadPoints(const std::string& text, Vector<Vec2F>& points)
            {
                const char* cursor = text.c_str();
                double x = 0, y = 0;
                while (ParseNumber(cursor, x) && ParseNumber(cursor, y))
                    points.Add(Vec2F((float)x, (float)y));
            }

            bool BuildGeometry(const pugi::xml_node& node, const std::string& name, VectorShape& shape, bool& fillable)
            {
                fillable = true;

                if (name == "path")
                {
                    if (!ParsePathText(Attribute(node, "d"), shape.subPaths))
                        Warn("path data is cut at a syntax error");
                }
                else if (name == "rect")
                {
                    Vec2F size = PointAttribute(node, "width", "height");
                    if (size.x <= 0.0f || size.y <= 0.0f)
                        return false;

                    float parsed = 0.0f;
                    bool hasRx = ParseLength(Attribute(node, "rx"), parsed);
                    bool hasRy = ParseLength(Attribute(node, "ry"), parsed);
                    Vec2F radius = PointAttribute(node, "rx", "ry");
                    if (hasRx && !hasRy)
                        radius.y = radius.x;
                    else if (hasRy && !hasRx)
                        radius.x = radius.y;

                    shape.AddRect(PointAttribute(node, "x", "y"), size, radius);
                }
                else if (name == "circle")
                {
                    float radius = LengthAttribute(node, "r", LengthAxis::Diagonal);
                    if (radius <= 0.0f)
                        return false;

                    shape.AddEllipse(PointAttribute(node, "cx", "cy"), Vec2F(radius, radius));
                }
                else if (name == "ellipse")
                {
                    Vec2F radius = PointAttribute(node, "rx", "ry");
                    if (radius.x <= 0.0f || radius.y <= 0.0f)
                        return false;

                    shape.AddEllipse(PointAttribute(node, "cx", "cy"), radius);
                }
                else if (name == "line")
                {
                    fillable = false;
                    shape.AddPolyline({ PointAttribute(node, "x1", "y1"), PointAttribute(node, "x2", "y2") }, false);
                }
                else
                {
                    Vector<Vec2F> points;
                    ReadPoints(Attribute(node, "points"), points);
                    shape.AddPolyline(points, name == "polygon");
                }

                return !shape.subPaths.empty();
            }

            void AddShape(const pugi::xml_node& node, const std::string& name, const Style& style,
                          const Basis& transform)
            {
                VectorShape shape;
                bool fillable = true;
                if (!BuildGeometry(node, name, shape, fillable))
                    return;

                shape.fill = fillable ? ResolvePaint(style.fill, shape, transform) : VectorPaint();
                shape.stroke = style.strokeWidth > 0.0f ? ResolvePaint(style.stroke, shape, transform) : VectorPaint();
                shape.fillRule = style.fillRule;
                shape.fillOpacity = style.fillOpacity;
                shape.strokeWidth = style.strokeWidth;
                shape.strokeCap = style.strokeCap;
                shape.strokeJoin = style.strokeJoin;
                shape.strokeMiterLimit = style.strokeMiterLimit;
                shape.strokeOpacity = style.strokeOpacity;
                shape.opacity = style.opacity;

                if (shape.fill.IsNone() && shape.stroke.IsNone())
                    return;

                float scaleX = transform.xv.Length(), scaleY = transform.yv.Length();
                if (!shape.stroke.IsNone() && (Math::Abs(scaleX - scaleY) > 0.01f*Math::Max(scaleX, scaleY) ||
                                               Math::Abs(transform.xv.Dot(transform.yv)) > 0.01f*scaleX*scaleY))
                {
                    Warn("stroke under non-uniform scale has uniform width");
                }

                Basis fillTransform = shape.fill.transform, strokeTransform = shape.stroke.transform;
                shape.Transform(transform);
                shape.fill.transform = fillTransform;
                shape.stroke.transform = strokeTransform;

                image.shapes.Add(shape);
            }

            bool ReadViewBox(const pugi::xml_node& node, Vec2F& origin, Vec2F& size)
            {
                double box[4] = { 0, 0, 0, 0 };
                std::string text = Attribute(node, "viewBox");
                const char* cursor = text.c_str();
                if (!ParseNumber(cursor, box[0]) || !ParseNumber(cursor, box[1]) || !ParseNumber(cursor, box[2]) ||
                    !ParseNumber(cursor, box[3]) || box[2] <= 0.0 || box[3] <= 0.0)
                {
                    return false;
                }

                origin = Vec2F((float)box[0], (float)box[1]);
                size = Vec2F((float)box[2], (float)box[3]);
                return true;
            }

            void ProcessChildren(const pugi::xml_node& node, const Style& style, const Basis& transform, int depth)
            {
                for (pugi::xml_node child : node)
                {
                    if (child.type() == pugi::node_element)
                        ProcessElement(child, style, transform, depth);
                }
            }

            void ProcessElement(const pugi::xml_node& node, const Style& parentStyle, const Basis& parentTransform,
                                int depth, bool referenced = false)
            {
                static const char* shapeNames[] = { "path", "rect", "circle", "ellipse", "line", "polyline",
                                                    "polygon" };
                static const char* groupNames[] = { "g", "a", "svg", "switch", "use", "symbol" };
                static const char* silentNames[] = { "defs", "linearGradient", "radialGradient", "stop", "style",
                                                     "title", "desc", "metadata", "foreignObject", "clipPath", "mask" };

                const int maxDepth = 32;

                std::string name = LocalName(node);

                for (const char* silentName : silentNames)
                {
                    if (name == silentName)
                        return;
                }

                bool isShape = false, isGroup = false;
                for (const char* shapeName : shapeNames)
                    isShape = isShape || name == shapeName;

                for (const char* groupName : groupNames)
                    isGroup = isGroup || name == groupName;

                if (!isShape && !isGroup)
                {
                    Warn("unsupported element <" + name + ">");
                    return;
                }

                if (name == "symbol" && !referenced)
                    return;

                Style style = ResolveStyle(node, parentStyle);
                if (!style.displayed)
                    return;

                Basis transform = parentTransform;
                if (HasAttribute(node, "transform"))
                {
                    Basis local;
                    if (ParseTransformText(Attribute(node, "transform"), local))
                        transform = local*parentTransform;
                    else
                        Warn("invalid transform '" + Attribute(node, "transform") + "'");
                }

                if (isShape)
                {
                    if (style.visible)
                        AddShape(node, name, style, transform);

                    return;
                }

                if (depth >= maxDepth)
                {
                    Warn("elements are nested too deep");
                    return;
                }

                if (name == "use")
                {
                    pugi::xml_node target = Referenced(node);
                    if (!target)
                    {
                        std::string reference = Attribute(node, "href") + Attribute(node, "xlink:href");
                        Warn("use reference '" + reference + "' is not found");
                        return;
                    }

                    if (std::find(usedTargets.begin(), usedTargets.end(), target) != usedTargets.end())
                    {
                        Warn("use refers to itself");
                        return;
                    }

                    Basis offset(PointAttribute(node, "x", "y"), Vec2F(1.0f, 0.0f), Vec2F(0.0f, 1.0f));

                    usedTargets.push_back(target);
                    ProcessElement(target, style, offset*transform, depth + 1, true);
                    usedTargets.pop_back();
                    return;
                }

                if (name == "svg")
                {
                    Vec2F boxOrigin, boxSize;
                    Vec2F position = PointAttribute(node, "x", "y");
                    Vec2F size(Length(Attribute(node, "width"), viewSize.x, LengthAxis::X),
                               Length(Attribute(node, "height"), viewSize.y, LengthAxis::Y));

                    if (!ReadViewBox(node, boxOrigin, boxSize))
                        boxSize = size;

                    if (size.x <= 0.0f || size.y <= 0.0f)
                        return;

                    Vec2F outerViewSize = viewSize;
                    viewSize = boxSize;
                    Basis viewport = ViewportTransform(position, size, boxOrigin, boxSize,
                                                       Attribute(node, "preserveAspectRatio"));
                    ProcessChildren(node, style, viewport*transform, depth + 1);
                    viewSize = outerViewSize;
                    return;
                }

                ProcessChildren(node, style, transform, depth + 1);
            }

            void Run(const pugi::xml_node& root)
            {
                float width = 0.0f, height = 0.0f;
                LengthUnit widthUnit = LengthUnit::User, heightUnit = LengthUnit::User;
                bool hasWidth = ParseLength(Attribute(root, "width"), width, options.unitsAsPixels, &widthUnit) &&
                    widthUnit == LengthUnit::User && width > 0.0f;
                bool hasHeight = ParseLength(Attribute(root, "height"), height, options.unitsAsPixels, &heightUnit) &&
                    heightUnit == LengthUnit::User && height > 0.0f;

                Vec2F boxOrigin, boxSize;
                if (!ReadViewBox(root, boxOrigin, boxSize))
                    boxSize = Vec2F(hasWidth ? width : 100.0f, hasHeight ? height : 100.0f);

                if (!hasWidth && !hasHeight)
                {
                    width = boxSize.x;
                    height = boxSize.y;
                }
                else if (!hasWidth)
                    width = height*boxSize.x/boxSize.y;
                else if (!hasHeight)
                    height = width*boxSize.y/boxSize.x;

                image.size = Vec2F(width, height);
                image.viewBoxOrigin = boxOrigin;
                image.viewBoxSize = boxSize;
                viewSize = boxSize;

                Basis transform = ViewportTransform(Vec2F(), image.size, boxOrigin, boxSize,
                                                    Attribute(root, "preserveAspectRatio"));
                if (HasAttribute(root, "transform"))
                {
                    Basis local;
                    if (ParseTransformText(Attribute(root, "transform"), local))
                        transform = transform*local;
                    else
                        Warn("invalid transform '" + Attribute(root, "transform") + "'");
                }

                Collect(root);

                Style style = ResolveStyle(root, Style());
                if (style.displayed)
                    ProcessChildren(root, style, transform, 0);
            }
        };
    }

    bool SvgParser::Parse(const char* text, UInt size, VectorImage& image, String& error, Vector<String>& warnings,
                          const SvgParseOptions& options /*= SvgParseOptions()*/)
    {
        image.Clear();
        error.clear();
        warnings.Clear();

        pugi::xml_document document;
        pugi::xml_parse_result result = document.load_buffer(text, size, pugi::parse_default, pugi::encoding_auto);
        if (!result)
        {
            error = String("SVG is not a valid XML: ") + result.description();
            return false;
        }

        pugi::xml_node root;
        for (pugi::xml_node child : document)
        {
            if (child.type() == pugi::node_element && LocalName(child) == "svg")
                root = child;
        }

        if (!root)
        {
            error = "SVG has no <svg> root element";
            return false;
        }

        Document(image, warnings, options).Run(root);
        return true;
    }

    bool SvgParser::Parse(const String& text, VectorImage& image, String& error, Vector<String>& warnings,
                          const SvgParseOptions& options /*= SvgParseOptions()*/)
    {
        return Parse(text.Data(), (UInt)text.size(), image, error, warnings, options);
    }

    bool SvgParser::Parse(const String& text, VectorImage& image)
    {
        String error;
        Vector<String> warnings;
        return Parse(text, image, error, warnings);
    }

    bool SvgParser::ParsePathData(const String& data, Vector<VectorSubPath>& subPaths)
    {
        return ParsePathText(std::string(data.Data()), subPaths);
    }

    bool SvgParser::ParseColor(const String& text, Color4& color)
    {
        return ParseColorText(std::string(text.Data()), color);
    }

    bool SvgParser::ParseTransform(const String& text, Basis& transform)
    {
        return ParseTransformText(std::string(text.Data()), transform);
    }
}
