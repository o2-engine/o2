#include "o2Editor/stdafx.h"
#include "UiLayoutWidgets.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>

namespace Editor::UiLayout
{
    namespace PyValue
    {
        bool Float(const Json* v, double& out)
        {
            if (!v)
                return false;
            if (v->IsNumber())
            {
                out = v->Num();
                return std::isfinite(out);
            }
            if (v->IsBool())
            {
                out = v->Bool() ? 1.0 : 0.0;
                return true;
            }
            if (v->IsString())
            {
                String text = v->Str().Trimed(" \t\r\n");
                if (text.IsEmpty())
                    return false;
                char* end = nullptr;
                out = std::strtod(text.Data(), &end);
                return end && *end == '\0' && std::isfinite(out);
            }
            return false;
        }

        int ToInt(const Json* v, int def /*= 0*/)
        {
            double d = 0;
            return Float(v, d) ? (int)std::nearbyint(d) : def;
        }

        double Num(const Json* parent, const char* key, double def /*= 0.0*/)
        {
            double d = 0;
            return Float(parent ? parent->Get(key) : nullptr, d) ? d : def;
        }

        String Str(const Json* parent, const char* key)
        {
            auto v = parent ? parent->Get(key) : nullptr;
            return v && v->IsString() ? v->Str() : String();
        }

        bool Truthy(const Json* v)
        {
            if (!v || v->IsNull())
                return false;
            if (v->IsBool())
                return v->Bool();
            if (v->IsNumber())
                return v->Num() != 0;
            if (v->IsString())
                return !v->Str().IsEmpty();
            return v->Count() > 0;
        }
    }

    using namespace PyValue;

    // --- values the way the skill's Python reads them ----------------------------------------------------------

    namespace
    {
        int FloorDiv(int a, int b)
        {
            int q = a/b;
            return (a % b != 0 && ((a < 0) != (b < 0))) ? q - 1 : q;
        }

        void SizeOf(const Json& elem, int& w, int& h)
        {
            const Json* size = elem.Get("size");
            const Json* s = size && size->IsObject() ? size : nullptr;
            w = ToInt(s ? s->Get("width") : nullptr);
            h = ToInt(s ? s->Get("height") : nullptr);
        }

        void PairOf(const Json& elem, const char* key, int& x, int& y)
        {
            const Json* pair = elem.Get(key);
            const Json* p = pair && pair->IsObject() ? pair : nullptr;
            x = ToInt(p ? p->Get("x") : nullptr);
            y = ToInt(p ? p->Get("y") : nullptr);
        }

        void PaddingOf(const Json& layout, int& x, int& y)
        {
            const Json* pad = layout.Get("padding");
            x = y = 0;
            if (pad && (pad->IsNumber() || pad->IsBool()))
                x = y = ToInt(pad);
            else if (pad && pad->IsObject())
            {
                x = ToInt(pad->Get("x"));
                y = ToInt(pad->Get("y"));
            }
        }

        // layout._align_axis; align is the member (null: not given)
        int AlignAxis(int parentExtent, int elemExtent, const Json* align, int explicitPos)
        {
            String a = align && align->IsString() ? align->Str() : String();
            if (a == "start" || a == "left" || a == "top")
                return 0;
            if (a == "center" || a == "middle")
                return FloorDiv(parentExtent - elemExtent, 2);
            if (a == "end" || a == "right" || a == "bottom")
                return parentExtent - elemExtent;
            return explicitPos;
        }

        // A member with a default when it is missing, as dict.get(key, default)
        const Json* GetOr(const Json& obj, const char* key, const Json& def)
        {
            const Json* v = obj.Get(key);
            return v ? v : &def;
        }

        Vector<int> DistributePixels(int total, int slots)
        {
            Vector<int> out;
            for (int i = 0; i < slots; i++)
                out.Add(FloorDiv((i + 1)*total, slots) - FloorDiv(i*total, slots));
            return out;
        }

        bool LayoutType(const Json* layout, String& type, String* error)
        {
            type = layout && layout->IsObject() ? Str(layout, "type") : String();
            if (type == "row" || type == "column" || type == "grid")
                return true;
            if (error)
                *error = "layout.type must be explicitly set to row or column or grid";
            return false;
        }

        // layout._layout_group: the positions of a layout's children inside the parent of the size
        bool LayoutGroup(const Json& parent, const Json& layout, int pw, int ph, Vector<Pair<int, int>>& out, String* error)
        {
            String type;
            if (!LayoutType(&layout, type, error))
                return false;

            const Json* children = parent.Get("children");
            if (!children || !children->IsArray() || children->Items().IsEmpty())
                return true;

            auto& items = children->Items();
            int padX, padY;
            PaddingOf(layout, padX, padY);
            int innerW = Math::Max(0, pw - 2*padX), innerH = Math::Max(0, ph - 2*padY);
            static const Json start("start");

            if (type == "grid")
            {
                const Json* columnsJson = layout.Get("columns");
                double columnsNum = columnsJson && columnsJson->IsNumber() ? columnsJson->Num() : 0;
                if (columnsNum <= 0 || columnsNum != std::floor(columnsNum))
                {
                    if (error)
                        *error = "grid columns must be a positive integer";
                    return false;
                }

                int columns = (int)columnsNum;
                const Json* cell = layout.Get("cellSize");
                int cw = ToInt(cell ? cell->Get("width") : nullptr), ch = ToInt(cell ? cell->Get("height") : nullptr);
                if (cw <= 0 || ch <= 0)
                {
                    if (error)
                        *error = "grid cellSize must be positive";
                    return false;
                }

                const Json* spacing = layout.Get("spacing");
                if (spacing && !spacing->IsObject())
                {
                    if (error)
                        *error = "grid spacing must be {x,y}";
                    return false;
                }

                int gx = ToInt(spacing ? spacing->Get("x") : nullptr), gy = ToInt(spacing ? spacing->Get("y") : nullptr);
                int n = items.Count();
                int usedColumns = Math::Min(columns, n);
                int rows = (n + columns - 1)/columns;
                int width = usedColumns*cw + Math::Max(0, usedColumns - 1)*gx;
                int height = rows*ch + Math::Max(0, rows - 1)*gy;
                int x = padX + AlignAxis(innerW, width, GetOr(layout, "align", start), 0);
                int y = padY + AlignAxis(innerH, height, GetOr(layout, "vAlign", start), 0);
                for (int i = 0; i < n; i++)
                {
                    int ox, oy;
                    PairOf(items[i], "offset", ox, oy);
                    out.Add(Pair<int, int>(x + (i % columns)*(cw + gx) + ox, y + (i/columns)*(ch + gy) + oy));
                }
                return true;
            }

            bool row = type == "row";
            int mainExtent = row ? innerW : innerH, crossExtent = row ? innerH : innerW;
            int n = items.Count();
            Vector<int> sizesMain, sizesCross;
            int totalMain = 0;
            for (auto& c : items)
            {
                int w, h;
                SizeOf(c, w, h);
                sizesMain.Add(row ? w : h);
                sizesCross.Add(row ? h : w);
                totalMain += row ? w : h;
            }
            int leftover = Math::Max(0, mainExtent - totalMain);

            static const Json zero(0);
            const Json* spacingCfg = GetOr(layout, "spacing", zero);
            const Json* alignJson = GetOr(layout, "align", start);
            const Json* vAlign = GetOr(layout, "vAlign", start);
            String align = alignJson->IsString() ? alignJson->Str() : String();
            bool even = spacingCfg->IsString() && spacingCfg->Str() == "even";

            String distribution = align == "space-between" || align == "space-around" || align == "space-evenly" ? align
                : even ? String("space-evenly") : String();
            int lead = 0;
            Vector<int> gaps;
            if (distribution == "space-between")
                gaps = DistributePixels(leftover, n - 1);
            else if (distribution == "space-around")
            {
                Vector<int> half = DistributePixels(leftover, 2*n);
                lead = half[0];
                for (int i = 0; i < n - 1; i++)
                    gaps.Add(half[2*i + 1] + half[2*i + 2]);
            }
            else if (distribution == "space-evenly")
            {
                Vector<int> all = DistributePixels(leftover, n + 1);
                lead = all[0];
                for (int i = 1; i < n; i++)
                    gaps.Add(all[i]);
            }
            else
            {
                int fixed = ToInt(spacingCfg);
                int totalWithGaps = totalMain + fixed*Math::Max(0, n - 1);
                if (align == "center" || align == "middle")
                    lead = Math::Max(0, FloorDiv(mainExtent - totalWithGaps, 2));
                else if (align == "end" || align == "right" || align == "bottom")
                    lead = Math::Max(0, mainExtent - totalWithGaps);
                for (int i = 0; i < n - 1; i++)
                    gaps.Add(fixed);
            }

            int cursor = lead;
            for (int i = 0; i < n; i++)
            {
                auto& c = items[i];
                int cross = AlignAxis(crossExtent, sizesCross[i], vAlign, 0);
                int x = row ? padX + cursor : padX + cross;
                int y = row ? padY + cross : padY + cursor;
                if (const Json* childCross = c.Get(row ? "vAlign" : "align"); childCross && !childCross->IsNull())
                {
                    if (row)
                        y = padY + AlignAxis(crossExtent, sizesCross[i], childCross, cross);
                    else
                        x = padX + AlignAxis(crossExtent, sizesCross[i], childCross, cross);
                }

                int ox, oy;
                PairOf(c, "offset", ox, oy);
                out.Add(Pair<int, int>(x + ox, y + oy));
                cursor += sizesMain[i];
                if (i < n - 1)
                    cursor += gaps[i];
            }
            return true;
        }

        // layout._resolve_alignment
        void ResolveAlignment(const Json& elem, int pw, int ph, int& x, int& y)
        {
            int ew, eh, px, py, ox, oy;
            SizeOf(elem, ew, eh);
            PairOf(elem, "position", px, py);
            PairOf(elem, "offset", ox, oy);
            const Json* ax = elem.Get("align");
            const Json* ay = elem.Get("vAlign");
            x = (ax && !ax->IsNull() ? AlignAxis(pw, ew, ax, px) : px) + ox;
            y = (ay && !ay->IsNull() ? AlignAxis(ph, eh, ay, py) : py) + oy;
        }

        // ui_components.responsive_box
        bool ResponsiveBox(const Json& spec, int width, int height, const Json* safeArea, Box& out, String* error)
        {
            bool safe = Truthy(spec.Get("safeArea"));
            double left = safe ? Num(safeArea, "left") : 0, top = safe ? Num(safeArea, "top") : 0;
            double w = width - (left + (safe ? Num(safeArea, "right") : 0));
            double h = height - (top + (safe ? Num(safeArea, "bottom") : 0));
            const Json* mn = spec.Get("min");
            const Json* mx = spec.Get("max");
            const Json* omn = spec.Get("offsetMin");
            const Json* omx = spec.Get("offsetMax");
            double x = std::nearbyint(left + w*Num(mn, "x") + Num(omn, "x"));
            double y = std::nearbyint(top + h*Num(mn, "y") + Num(omn, "y"));
            double right = std::nearbyint(left + w*Num(mx, "x") + Num(omx, "x"));
            double bottom = std::nearbyint(top + h*Num(mx, "y") + Num(omx, "y"));
            if (right <= x || bottom <= y)
            {
                if (error)
                    *error = "responsive constraints produce a non-positive size";
                return false;
            }
            out = Box(x, y, right - x, bottom - y);
            return true;
        }

        bool Walk(const Json& elem, const String& path, int ax, int ay, int pw, int ph, const Json* canvas,
                  Vector<UiResolvedNode>& out, String* error)
        {
            const Json* children = elem.Get("children");
            if (!children || !children->IsArray())
                return true;

            Vector<Pair<int, int>> positions;
            bool hasLayout = elem.Has("layout");
            if (hasLayout && !LayoutGroup(elem, *elem.Get("layout"), pw, ph, positions, error))
                return false;

            const Json* scroll = elem.Get("scroll");
            for (int index = 0; index < children->Items().Count(); index++)
            {
                auto& child = children->Items()[index];
                int cw, ch, rx = 0, ry = 0;
                SizeOf(child, cw, ch);
                if (const Json* responsive = child.Get("responsive"))
                {
                    Box b;
                    if (!ResponsiveBox(*responsive, pw, ph, canvas ? canvas->Get("safeArea") : nullptr, b, error))
                        return false;
                    rx = (int)b.x; ry = (int)b.y; cw = (int)b.w; ch = (int)b.h;
                }
                else if (hasLayout && index < positions.Count())
                {
                    rx = positions[index].first;
                    ry = positions[index].second;
                }
                else
                    ResolveAlignment(child, pw, ph, rx, ry);

                String name = child.Get("name") ? child.Get("name")->ToJsString() : String("?");
                if (Truthy(scroll) && scroll->IsObject() && child.Get("name") && scroll->Get("content") &&
                    child.Get("name")->ToJsString() == scroll->Get("content")->ToJsString())
                {
                    if (rx != 0 || ry != 0)
                    {
                        if (error)
                            *error = "scroll content must start at viewport (0, 0)";
                        return false;
                    }

                    int extentX = Math::Max(0, cw - pw), extentY = Math::Max(0, ch - ph);
                    int dx, dy;
                    PairOf(*scroll, "offset", dx, dy);
                    rx -= Math::Min(extentX, Math::Max(0, dx));
                    ry -= Math::Min(extentY, Math::Max(0, dy));
                }

                String childPath = path + "/" + name;
                out.Add({ childPath, Box(rx, ry, cw, ch), Box(ax + rx, ay + ry, cw, ch) });
                if (!Walk(child, childPath, ax + rx, ay + ry, cw, ch, canvas, out, error))
                    return false;
            }
            return true;
        }
    }

    bool ResolveStructure(const Json& structure, Vector<UiResolvedNode>& out, String* error /*= nullptr*/)
    {
        out.Clear();
        static const Json empty = Json::Object();
        const Json* root = structure.Get("root");
        if (!root)
            root = &empty;
        const Json* canvas = structure.Get("canvas");

        int rw, rh, rx, ry;
        SizeOf(*root, rw, rh);
        if (!rw)
            rw = ToInt(canvas ? canvas->Get("width") : nullptr);
        if (!rh)
            rh = ToInt(canvas ? canvas->Get("height") : nullptr);
        PairOf(*root, "position", rx, ry);
        out.Add({ "root", Box(rx, ry, rw, rh), Box(rx, ry, rw, rh) });
        return Walk(*root, "root", rx, ry, rw, rh, canvas, out, error);
    }

    // --- the snapshot ------------------------------------------------------------------------------------------

    static Json* Target(Json& owner, const String& relative)
    {
        if (relative == ".")
            return &owner;

        Json* node = &owner;
        for (auto& part : relative.Split("/"))
        {
            Json* children = node->Get("children");
            if (!children || !children->IsArray())
                return nullptr;

            Json* next = nullptr;
            for (auto& c : children->Items())
            {
                if (c.Get("name") && c.Get("name")->ToJsString() == part)
                {
                    next = &c;
                    break;
                }
            }
            if (!next)
                return nullptr;
            node = next;
        }
        return node;
    }

    static void ApplyStates(Json& node)
    {
        if (const Json* state = node.Get("state"); state && state->IsObject())
        {
            String current = Str(state, "current");
            const Json* variants = state->Get("variants");
            const Json* patches = variants ? variants->Get(current) : nullptr;
            if (patches && patches->IsObject())
            {
                Json copy = *patches;
                for (auto& patch : copy.Members())
                {
                    Json* target = Target(node, patch.first);
                    if (!target || !patch.second.IsObject())
                        continue;
                    for (auto& field : patch.second.Members())
                        (*target)[field.first] = field.second;
                }
            }
        }

        if (Json* children = node.Get("children"); children && children->IsArray())
        {
            for (auto& c : children->Items())
                ApplyStates(c);
        }
    }

    static void ApplyProgressLabels(Json& node)
    {
        if (const Json* progress = node.Get("progress"); progress && progress->IsObject() && progress->Has("label"))
        {
            double low = Num(progress, "min", 0), high = Num(progress, "max", 1), value = Num(progress, "value", 0);
            String format = progress->Has("format") ? Str(progress, "format") : String("{percent:.0f}%");
            Json* label = Target(node, Str(progress, "label"));
            String text;
            if (label && high > low && FormatProgress(format, value, low, high, text))
                (*label)["text"] = text;
        }

        if (Json* children = node.Get("children"); children && children->IsArray())
        {
            for (auto& c : children->Items())
                ApplyProgressLabels(c);
        }
    }

    Json SnapshotStructure(const Json& structure)
    {
        Json scene = structure;
        if (Json* root = scene.Get("root"))
        {
            ApplyStates(*root);
            ApplyProgressLabels(*root);
        }
        return scene;
    }

    // Python's str() of a float: the shortest form, ".0" on whole numbers
    static String PyFloatText(double v)
    {
        String text = Json::FormatNumber(v);
        if (!text.Contains(".") && !text.Contains("e") && std::isfinite(v))
            text += ".0";
        return text;
    }

    bool FormatProgress(const String& format, double value, double min, double max, String& out)
    {
        out = "";
        double percent = (value - min)/(max - min)*100;
        for (int i = 0; i < format.Length(); i++)
        {
            char c = format[i];
            if (c == '}' && i + 1 < format.Length() && format[i + 1] == '}')
            {
                out += '}';
                i++;
                continue;
            }
            if (c != '{')
            {
                out += c;
                continue;
            }
            if (i + 1 < format.Length() && format[i + 1] == '{')
            {
                out += '{';
                i++;
                continue;
            }

            int end = format.Find("}", i);
            if (end < 0)
                return false;

            String field = format.SubStr(i + 1, end), spec;
            int colon = field.Find(":");
            if (colon >= 0)
            {
                spec = field.SubStr(colon + 1);
                field = field.SubStr(0, colon);
            }

            double v = 0;
            bool isFloat = true;
            if (field == "value") { v = value; isFloat = value != std::floor(value); }
            else if (field == "min") { v = min; isFloat = min != std::floor(min); }
            else if (field == "max") { v = max; isFloat = max != std::floor(max); }
            else if (field == "percent") v = percent;
            else
                return false;

            char buffer[64];
            if (spec.IsEmpty())
                out += isFloat ? PyFloatText(v) : Json::FormatNumber(v);
            else if (spec.StartsWith(".") && HasSuffix(spec, "f") && spec.Length() >= 3)
            {
                int digits = atoi(spec.SubStr(1, spec.Length() - 1).Data());
                snprintf(buffer, sizeof(buffer), "%.*f", Math::Clamp(digits, 0, 20), v);
                out += buffer;
            }
            else if (spec == "d" && !isFloat)
            {
                snprintf(buffer, sizeof(buffer), "%lld", (long long)v);
                out += buffer;
            }
            else
                return false;
            i = end;
        }
        return true;
    }

    std::optional<NineSlice> NineSliceOf(const Json* value, const UiSpriteInfo* sprite)
    {
        if (!value || value->IsNull() || (value->IsBool() && !value->Bool()))
            return std::nullopt;

        NineSlice nine;
        if ((value->IsBool() && value->Bool()) || (value->IsString() && (value->Str() == "auto" || value->Str() == "meta")))
        {
            if (sprite && sprite->nine)
                return sprite->nine;
            if (!sprite || sprite->w <= 0 || sprite->h <= 0)
                return std::nullopt;
            int m = Math::Max(4, Math::Min(60, Math::Min(sprite->w, sprite->h)/4));
            nine.left = nine.top = nine.right = nine.bottom = m;
            return nine;
        }

        if (value->IsNumber())
        {
            nine.left = nine.top = nine.right = nine.bottom = (int)value->Num();
            return nine;
        }

        if (value->IsObject())
        {
            nine.left = (int)Num(value, "left");
            nine.top = (int)Num(value, "top");
            nine.right = (int)Num(value, "right");
            nine.bottom = (int)Num(value, "bottom");
            return nine;
        }
        return std::nullopt;
    }

    String FontFileOf(const String& weight)
    {
        if (weight == "bold")
            return "Roboto-Bold.ttf";
        if (weight == "medium")
            return "Roboto-Medium.ttf";
        return "Roboto-Regular.ttf";
    }

    String WeightOfFont(const String& file)
    {
        String f = file.ToLowerCase();
        if (f.Contains("bold") || f.Contains("black"))
            return "bold";
        if (f.Contains("medium"))
            return "medium";
        return "regular";
    }

    int HeightPoints(double fontSize)
    {
        return Math::Max(1, (int)JsRound(fontSize*72.0/92.0));
    }

}
