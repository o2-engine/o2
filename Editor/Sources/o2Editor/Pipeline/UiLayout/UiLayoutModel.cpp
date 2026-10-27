#include "o2Editor/stdafx.h"
#include "UiLayoutModel.h"

namespace Editor::UiLayout
{
    const Vector<String> uiKinds = { "panel", "image", "button", "text", "progress", "group", "grid", "input" };

    Ref<UiEl> UiEl::Clone() const
    {
        auto copy = mmake<UiEl>();
        copy->id = id;
        copy->parent = parent;
        copy->kind = kind;
        copy->box = box;
        copy->look = look;
        copy->text = text;
        copy->color = color;
        copy->weight = weight;
        copy->align = align;
        copy->stroke = stroke;
        copy->strokeWidth = strokeWidth;
        copy->fontSize = fontSize;
        copy->ink = ink;
        copy->value = value;
        copy->rows = rows;
        copy->cols = cols;
        copy->cell = cell;
        copy->slot = slot;
        copy->rough = rough;
        copy->measured = measured;
        return copy;
    }

    UiDoc UiDoc::Clone() const
    {
        UiDoc copy;
        copy.w = w;
        copy.h = h;
        copy.background = background;
        copy.looks = looks;
        for (auto& e : els)
            copy.els.Add(e->Clone());
        return copy;
    }

    Ref<UiEl> UiDoc::ById(const String& id) const
    {
        for (auto& e : els)
        {
            if (e->id == id)
                return e;
        }
        return nullptr;
    }

    int UiDoc::IndexOf(const Ref<UiEl>& e) const
    {
        for (int i = 0; i < els.Count(); i++)
        {
            if (els[i] == e)
                return i;
        }
        return -1;
    }

    Vector<Ref<UiEl>> UiDoc::ChildrenOf(const String& id) const
    {
        Vector<Ref<UiEl>> out;
        for (auto& e : els)
        {
            if (e->parent == id)
                out.Add(e);
        }
        return out;
    }

    Vector<Ref<UiEl>> UiDoc::DescendantsOf(const String& id) const
    {
        Vector<Ref<UiEl>> out;
        Function<void(const String&)> walk;
        walk = [&](const String& pid)
        {
            for (auto& e : els)
            {
                if (e->parent == pid)
                {
                    out.Add(e);
                    walk(e->id);
                }
            }
        };
        walk(id);
        return out;
    }

    const UiEl* UiDoc::Find(const String& id) const
    {
        for (auto& e : els)
        {
            if (e->id == id)
                return e.Get();
        }
        return nullptr;
    }

    void UiDoc::CollectDescendantIds(const String& id, std::set<std::string>& out) const
    {
        for (auto& e : els)
        {
            if (e->parent == id && !out.count(e->id.Data()))
            {
                out.insert(e->id.Data());
                CollectDescendantIds(e->id, out);
            }
        }
    }

    bool HasLook(const UiEl& e)
    {
        return !e.look.IsEmpty() && (e.kind == "panel" || e.kind == "image" || e.kind == "button" || e.kind == "input" || e.kind == "progress");
    }

    bool IsStretchy(const UiEl& e)
    {
        return e.kind == "panel" || e.kind == "input" || e.kind == "progress";
    }

    String SafeKey(const String& s, const String& fallback /*= "el"*/)
    {
        String key;
        bool gap = false;
        for (int i = 0; i < s.Length(); i++)
        {
            char c = s[i];
            if (c >= 'A' && c <= 'Z')
                c = (char)(c - 'A' + 'a');

            if ((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9'))
            {
                if (gap)
                    key += '_';
                key += c;
                gap = false;
            }
            else
                gap = true;
        }

        if (gap && !key.IsEmpty())
            key += '_';

        int begin = 0, end = key.Length();
        while (begin < end && key[begin] == '_')
            begin++;
        while (end > begin && key[end - 1] == '_')
            end--;
        key = key.SubStr(begin, end);

        if (key.Length() > 48)
            key = key.SubStr(0, 48);

        return key.IsEmpty() ? fallback : key;
    }
}
