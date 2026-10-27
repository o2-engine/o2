#include "o2Editor/stdafx.h"
#include "UiLayoutParse.h"

#include <cmath>
#include <cstring>
#include <functional>
#include <set>

namespace Editor::UiLayout
{
    bool JsTruthy(const Json* value)
    {
        if (!value)
            return false;

        switch (value->GetKind())
        {
            case Json::Kind::Null: return false;
            case Json::Kind::Bool: return value->Bool();
            case Json::Kind::Number: return value->Num() != 0.0 && !std::isnan(value->Num());
            case Json::Kind::String: return !value->Str().IsEmpty();
            default: return true;
        }
    }

    String JsText(const Json* value)
    {
        if (!value || value->IsNull())
            return "";

        return value->ToJsString();
    }

    static bool IsTrimSpace(unsigned char c)
    {
        return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f' || c == '\v';
    }

    String JsTrim(const String& text)
    {
        int begin = 0, end = text.Length();
        while (begin < end && IsTrimSpace((unsigned char)text[begin]))
            begin++;
        while (end > begin && IsTrimSpace((unsigned char)text[end - 1]))
            end--;

        // U+00A0 and U+FEFF are whitespace for String#trim too
        auto startsWith = [&](int at, const char* bytes, int n) { return at + n <= end && std::memcmp(text.Data() + at, bytes, n) == 0; };
        auto endsWith = [&](int at, const char* bytes, int n) { return at - n >= begin && std::memcmp(text.Data() + at - n, bytes, n) == 0; };
        for (bool changed = true; changed;)
        {
            changed = false;
            if (startsWith(begin, "\xC2\xA0", 2)) { begin += 2; changed = true; }
            else if (startsWith(begin, "\xEF\xBB\xBF", 3)) { begin += 3; changed = true; }
            while (begin < end && IsTrimSpace((unsigned char)text[begin])) { begin++; changed = true; }
        }
        for (bool changed = true; changed;)
        {
            changed = false;
            if (endsWith(end, "\xC2\xA0", 2)) { end -= 2; changed = true; }
            else if (endsWith(end, "\xEF\xBB\xBF", 3)) { end -= 3; changed = true; }
            while (end > begin && IsTrimSpace((unsigned char)text[end - 1])) { end--; changed = true; }
        }

        return text.SubStr(begin, end);
    }

    bool JsonOf(const String& text, Json& out)
    {
        String raw = JsTrim(text);

        int fence = raw.Find("```");
        if (fence >= 0)
        {
            int body = fence + 3;
            if (raw.Length() >= body + 4 && raw.SubStr(body, body + 4).ToLowerCase() == "json")
                body += 4;
            while (body < raw.Length() && IsTrimSpace((unsigned char)raw[body]))
                body++;

            int close = raw.Find("```", body);
            if (close >= 0)
                raw = JsTrim(raw.SubStr(body, close));
        }

        int start = -1;
        for (int i = 0; i < raw.Length(); i++)
        {
            if (raw[i] == '{' || raw[i] == '[')
            {
                start = i;
                break;
            }
        }
        if (start > 0)
            raw = raw.SubStr(start);

        int end = Math::Max(raw.FindLast("}"), raw.FindLast("]"));
        if (end >= 0)
            raw = raw.SubStr(0, end + 1);

        return Json::Parse(raw, out);
    }

    bool BoxFrom2d(const Json* value, double w, double h, Box& out)
    {
        if (!value || !value->IsArray() || value->Count() != 4)
            return false;

        double v[4];
        for (int i = 0; i < 4; i++)
        {
            v[i] = value->At(i)->ToJsNumber();
            if (!std::isfinite(v[i]))
                return false;
        }

        double y0 = v[0], x0 = v[1], y1 = v[2], x1 = v[3];
        if (y1 < y0) std::swap(y0, y1);
        if (x1 < x0) std::swap(x0, x1);

        Box box = ClampBox(Box(x0/1000*w, y0/1000*h, (x1 - x0)/1000*w, (y1 - y0)/1000*h), w, h);
        if (box.w < 1 || box.h < 1)
            return false;

        out = box;
        return true;
    }

    String ColorOf(const Json* value)
    {
        Rgb rgb;
        return ParseHex(JsText(value), rgb) ? HexOf(rgb) : String();
    }

    static bool IsOneOf(const Json* value, const Vector<String>& options)
    {
        return value && value->IsString() && options.Contains(value->Str());
    }

    Ref<UiEl> ElementOf(const Json& raw, double w, double h)
    {
        if (!raw.IsObject())
            return nullptr;

        String kind = JsText(raw.Get("kind")).ToLowerCase();
        if (!uiKinds.Contains(kind))
            return nullptr;

        const Json* boxValue = raw.Get("box_2d");
        if (!boxValue || boxValue->IsNull())
            boxValue = raw.Get("box");

        Box box;
        if (!BoxFrom2d(boxValue, w, h, box))
            return nullptr;

        auto el = mmake<UiEl>();
        el->kind = kind;
        el->box = box;
        el->rough = box;
        el->measured = "analysis";
        if (JsTruthy(raw.Get("look")))
            el->look = SafeKey(JsText(raw.Get("look")), "look");

        if (kind == "text")
        {
            String text = JsTrim(JsText(raw.Get("text")));
            if (text.IsEmpty())
                return nullptr;

            el->text = text;
            String color = ColorOf(raw.Get("color"));
            el->color = color.IsEmpty() ? String("#000000") : color;
            el->weight = IsOneOf(raw.Get("weight"), { "regular", "medium", "bold" }) ? raw.Get("weight")->Str() : String("regular");
            el->align = IsOneOf(raw.Get("align"), { "left", "center", "right" }) ? raw.Get("align")->Str() : String("center");
            String stroke = ColorOf(raw.Get("stroke"));
            if (!stroke.IsEmpty())
                el->stroke = stroke;
        }

        if (kind == "progress")
        {
            const Json* v = raw.Get("value");
            double value = v ? v->ToJsNumber() : NAN;
            el->value = std::isfinite(value) ? Math::Max(0.0, Math::Min(1.0, value)) : 0.5;
        }

        if (kind == "grid")
        {
            auto count = [&](const char* key)
            {
                const Json* v = raw.Get(key);
                double n = v ? v->ToJsNumber() : NAN;
                if (std::isnan(n) || n == 0.0)
                    n = 1.0;
                return (int)Math::Max(1.0, Math::Min(64.0, JsRound(n)));
            };
            el->rows = count("rows");
            el->cols = count("cols");
        }

        return el;
    }

    static void ExpandGrid(UiDoc& doc, const Ref<UiEl>& grid, const Json* cells, const std::function<String(const String&)>& unique)
    {
        int rows = grid->rows.value_or(1), cols = grid->cols.value_or(1);
        double cw = grid->box.w/cols, ch = grid->box.h/rows;
        for (int r = 0; r < rows; r++)
        {
            for (int c = 0; c < cols; c++)
            {
                const Json* cell = cells ? cells->At(r*cols + c) : nullptr;
                if (!cell || !(cell->IsObject() || cell->IsArray()))
                    continue;

                String look = JsTruthy(cell->Get("look")) ? SafeKey(JsText(cell->Get("look")), "look") : String();
                String id = unique(grid->id + "_r" + (String)r + "c" + (String)c);

                auto image = mmake<UiEl>();
                image->id = id;
                image->parent = grid->id;
                image->kind = "image";
                image->look = look;
                image->box = Box(grid->box.x + c*cw, grid->box.y + r*ch, cw, ch);
                image->cell = UiCell{ grid->id, r, c };
                image->measured = "analysis";
                doc.els.Add(image);

                Vector<String> texts;
                const Json* list = cell->Get("texts");
                if (list && list->IsArray())
                {
                    for (auto& t : list->Items())
                    {
                        String text = JsTrim(JsText(&t));
                        if (!text.IsEmpty())
                            texts.Add(text);
                    }
                }

                for (int i = 0; i < texts.Count(); i++)
                {
                    auto text = mmake<UiEl>();
                    text->id = unique(id + "_t" + (String)i);
                    text->parent = id;
                    text->kind = "text";
                    text->text = texts[i];
                    text->box = Box(grid->box.x + c*cw, grid->box.y + r*ch, cw, ch);
                    if (JsTruthy(cell->Get("color")) && i == 0)
                        text->color = ColorOf(cell->Get("color"));
                    text->weight = i == 0 ? "bold" : "medium";
                    text->align = "center";
                    text->slot = i;
                    text->measured = "analysis";
                    doc.els.Add(text);
                }
            }
        }
    }

    bool ParseAnalysis(const String& text, int w, int h, UiDoc& out, String* error /*= nullptr*/)
    {
        Json parsed;
        if (!JsonOf(text, parsed))
        {
            if (error)
                *error = "the analysis answer is not JSON";
            return false;
        }

        const Json* items = parsed.IsArray() ? &parsed : parsed.Get("elements");
        UiDoc doc;
        doc.w = w;
        doc.h = h;

        if (const Json* looks = parsed.IsObject() ? parsed.Get("looks") : nullptr; looks && looks->IsObject())
        {
            for (auto& member : looks->Members())
                doc.looks.Set(SafeKey(member.first, "look"), JsTrim(JsText(&member.second)));
        }

        const Json* bg = parsed.IsObject() ? parsed.Get("background") : nullptr;
        const Json* bgKind = bg ? bg->Get("kind") : nullptr;
        doc.background.kind = bgKind && bgKind->IsString() && bgKind->Str() == "image" ? "image" : "color";
        String bgColor = ColorOf(bg ? bg->Get("color") : nullptr);
        doc.background.color = bgColor.IsEmpty() ? String("#808080") : bgColor;
        doc.background.describe = JsTrim(JsText(bg ? bg->Get("describe") : nullptr));

        std::set<std::string> used = { "root" };
        auto unique = [&](const String& base) -> String
        {
            String id = SafeKey(base);
            for (int i = 2; used.count(id); i++)
                id = SafeKey(base) + "_" + (String)i;
            used.insert(id);
            return id;
        };

        Map<String, String> rename;
        if (items && items->IsArray())
        {
            for (auto& raw : items->Items())
            {
                auto el = ElementOf(raw, w, h);
                if (!el)
                    continue;

                const Json* rawId = raw.Get("id");
                bool hasId = rawId && !rawId->IsNull();
                el->id = unique(hasId ? rawId->ToJsString() : el->kind);
                if (JsTruthy(rawId))
                    rename[rawId->ToJsString()] = el->id;

                String parentRaw = JsTrim(JsText(raw.Get("parent")));
                String parent;
                if (!parentRaw.IsEmpty())
                    rename.TryGetValue(parentRaw, parent);
                el->parent = parent;

                doc.els.Add(el);
                if (el->kind == "grid")
                {
                    const Json* cells = raw.Get("cells");
                    ExpandGrid(doc, el, cells && cells->IsArray() ? cells : nullptr, unique);
                }
            }
        }

        FixParents(doc);
        for (auto& e : doc.els)
        {
            if (e->look.IsEmpty())
                continue;

            const String* describe = doc.looks.Find(e->look);
            if (!describe || describe->IsEmpty())
                doc.looks.Set(e->look, e->look.ReplacedAll("_", " "));
        }

        out = doc;
        return true;
    }

    void FixParents(UiDoc& doc)
    {
        Map<String, int> ids;
        for (int i = 0; i < doc.els.Count(); i++)
            ids[doc.els[i]->id] = i;

        auto area = [](const Box& b) { return b.w*b.h; };
        auto containsMost = [&](const Box& outer, const Box& inner)
        {
            double x0 = Math::Max(outer.x, inner.x), y0 = Math::Max(outer.y, inner.y);
            double x1 = Math::Min(outer.x + outer.w, inner.x + inner.w), y1 = Math::Min(outer.y + outer.h, inner.y + inner.h);
            return x1 > x0 && y1 > y0 && ((x1 - x0)*(y1 - y0))/Math::Max(1.0, area(inner)) >= 0.7;
        };

        for (int i = 0; i < doc.els.Count(); i++)
        {
            auto& e = doc.els[i];
            if (e->cell)
                continue;

            int parentIndex = -1;
            if (!e->parent.IsEmpty())
                ids.TryGetValue(e->parent, parentIndex);

            Ref<UiEl> p = parentIndex >= 0 ? doc.els[parentIndex] : nullptr;
            if (p && parentIndex < i && containsMost(p->box, e->box) && p->kind != "text")
                continue;

            Ref<UiEl> best;
            for (int j = 0; j < i; j++)
            {
                auto& c = doc.els[j];
                if (c->kind == "text" || c == e || area(c->box) <= area(e->box))
                    continue;
                if (!containsMost(c->box, e->box))
                    continue;
                if (!best || area(c->box) < area(best->box))
                    best = c;
            }
            e->parent = best ? best->id : String();
        }
    }
}
