#include "o2Editor/stdafx.h"
#include "UiDocSkin.h"

#include <cmath>
#include <set>

#include "o2Editor/Pipeline/UiDoc/UiDocFp.h"

namespace Editor::UiDoc
{
    const Vector<String>& TypeSkin(const String& nodeType)
    {
        static const std::pair<const char*, Vector<String>> skins[] = {
            { "background", { "background" } }, { "panel", { "panel", "frame" } }, { "button", { "button" } }, { "icon", { "icon" } },
            { "image", { "image", "icon" } }, { "progress", { "progressBar" } }, { "toggle", { "toggle" } },
            { "slider", { "slider", "progressBar" } }, { "input", { "input", "panel" } }, { "tab", { "tab", "button" } },
            { "badge", { "badge" } }, { "divider", { "divider" } }, { "slot", { "slot", "frame", "panel" } },
        };
        static const Vector<String> none;
        for (auto& s : skins)
        {
            if (nodeType == s.first)
                return s.second;
        }
        return none;
    }

    bool IsPlain(const UiNode& n)
    {
        return n.kit && JsToLower(*n.kit) == "none" && n.type != "text";
    }

    UiSkin SkinOf(const UiNode& n, const UiKit* kit)
    {
        UiSkin skin;
        // kit=none: a plain block - its colour, or nothing - that takes no graphic of its type either
        if (IsPlain(n))
        {
            skin.explicitKit = true;
            return skin;
        }
        String variant = n.variant ? *n.variant : String();
        if (n.kit && !n.kit->IsEmpty())
        {
            skin.el = KitElement(kit, *n.kit);
            skin.fill = n.type == "progress" ? KitElementOfType(kit, { "progressFill" }, variant, n.id) : nullptr;
            skin.handle = n.type == "slider" ? KitElementOfType(kit, { "sliderHandle" }, variant, n.id) : nullptr;
            skin.explicitKit = true;
            skin.missing = !skin.el;
            return skin;
        }

        const Vector<String>& types = TypeSkin(n.type);
        if (types.IsEmpty() || !kit)
        {
            skin.missing = !types.IsEmpty();
            return skin;
        }

        bool byName = n.type == "icon" || n.type == "image";
        skin.el = KitElementOfType(kit, types, variant, n.id);
        // An icon or a picture is only taken when its name (or description) fits the block's id
        if (byName && skin.el && NameScore(n.id, *skin.el) == 0)
            skin.el = nullptr;
        skin.fill = n.type == "progress" ? KitElementOfType(kit, { "progressFill" }, variant, n.id) : nullptr;
        skin.handle = n.type == "slider" ? KitElementOfType(kit, { "sliderHandle" }, variant, n.id) : nullptr;
        skin.missing = !skin.el;
        return skin;
    }

    String RoleOf(const UiNode& n, const UiNode* labelOf /*= nullptr*/)
    {
        if (labelOf)
        {
            if (labelOf->type == "button" || labelOf->type == "tab")
                return "button";
            if (labelOf->type == "badge")
                return "number";
            return "body";
        }

        const std::string id = JsToLower(n.id);
        auto has = [&](std::initializer_list<const char*> words) {
            for (const char* w : words)
            {
                if (id.find(w) != std::string::npos)
                    return true;
            }
            return false;
        };
        if (has({ "title", "logo", "headline" })) return "title";
        if (has({ "head", "caption_big", "section" })) return "heading";
        if (has({ "count", "score", "amount", "coin", "gem", "price", "value", "number", "level", "lvl", "timer", "time" }))
            return "number";
        if (has({ "hint", "caption", "note", "small", "sub" })) return "caption";
        if (has({ "label", "name" })) return "label";
        return "body";
    }

    TextStyle StyleOf(const UiNode& n, const UiKit* kit, const UiNode* labelOf /*= nullptr*/)
    {
        return StyleForRole(n, kit, RoleOf(n, labelOf));
    }

    TextStyle StyleForRole(const UiNode& n, const UiKit* kit, const String& role)
    {
        if (const TextStyle* named = KitStyle(kit, n.style ? *n.style : String()))
            return *named;

        if (kit)
        {
            auto find = [&](const String& r) -> const TextStyle* {
                for (auto& s : kit->textStyles)
                {
                    if (s.role == r)
                        return &s;
                }
                return nullptr;
            };
            const TextStyle* byRole = find(role);
            if (!byRole && role == "heading")
                byRole = find("title");
            if (!byRole)
                byRole = find("body");
            if (byRole)
                return *byRole;
        }

        TextStyle style = TextStyleDefault();
        style.role = role;
        style.size = role == "title" ? 72 : role == "heading" ? 52 : role == "button" ? 44 : role == "number" ? 40 : role == "caption" ? 26 : 32;
        style.weight = role == "body" || role == "caption" ? 400 : 700;
        return style;
    }

    // Words into lines that fit the width (estimated)
    static Vector<String> Wrap(const String& text, const TextStyle& style, double size, double width)
    {
        Vector<String> out;
        for (auto& para : JsSplit(text, "\n"))
        {
            Vector<String> words = SplitWords(para);
            if (words.IsEmpty())
            {
                out.Add("");
                continue;
            }

            String line = words[0];
            for (int i = 1; i < words.Count(); i++)
            {
                String cand = line + " " + words[i];
                if (EstimateTextWidth(cand, style.font, style.weight, size, style.letterSpacing, style.uppercase) <= width)
                    line = cand;
                else
                {
                    out.Add(line);
                    line = words[i];
                }
            }
            out.Add(line);
        }
        return out;
    }

    UiTextFit LayoutText(const String& rawText, double w, double h, const TextStyle& style, double givenSize /*= 0*/,
                         double padX /*= 0*/)
    {
        String text = style.uppercase ? JsToUpper(rawText) : rawText;
        double avail = std::max(1.0, w - 2*padX);
        double size = givenSize ? givenSize : style.size;
        double min = size*0.4;
        for (;;)
        {
            double lineH = size*style.lineHeight;
            Vector<String> lines = Wrap(text, style, size, avail);
            double widest = -INFINITY;
            for (auto& l : lines)
                widest = std::max(widest, EstimateTextWidth(l, style.font, style.weight, size, style.letterSpacing, style.uppercase));
            bool fitsH = lines.Count()*lineH <= h + size*0.25 || lines.Count() == 1;
            if ((widest <= avail && fitsH) || size <= min)
                return UiTextFit{ lines, size, lineH };
            size = std::max(min, size*0.92);
        }
    }

    bool IsGlyphText(const String& text)
    {
        std::u32string cps = Utf8ToCodePoints(text);
        if (cps.empty() || cps.size() > 3)
            return false;
        for (char32_t c : cps)
        {
            if (IsLetterOrNumber(c) || IsJsSpace(c))
                return false;
        }
        return true;
    }

    UiKitReport KitReport(const UiDoc& doc, const UiKit* kit)
    {
        UiKitReport report;
        std::set<std::string> used;
        if (doc.root.kit && !doc.root.kit->IsEmpty())
            used.insert(JsToLower(*doc.root.kit));

        Walk(doc.root, [&](const UiNode& n, const UiNode*, int) {
            if (n.type != "screen" && n.type != "group" && n.type != "text" && !IsPlain(n))
            {
                report.total++;
                UiSkin s = SkinOf(n, kit);
                if (s.el)
                {
                    report.covered++;
                    used.insert(JsToLower(s.el->name));
                }
                else
                    report.missing.Add(Pair<String, String>(n.id, n.type));
                if (s.fill)
                    used.insert(JsToLower(s.fill->name));
                if (s.handle)
                    used.insert(JsToLower(s.handle->name));
            }
            // A badge's "!" or "+" is its content; elsewhere a lone symbol stands in for a kit icon
            if (n.type != "badge" && n.text && !n.text->IsEmpty() && IsGlyphText(JsTrim(*n.text)))
                report.glyphs.Add(n.id);
        });

        if (kit)
        {
            for (auto& e : kit->elements)
            {
                if (!used.count(JsToLower(e.name)))
                    report.unused.Add(e.name);
            }
        }
        return report;
    }

    UiKitCoverage KitCoverage(const UiDoc& doc, const UiKit* kit)
    {
        UiKitCoverage cov;
        Walk(doc.root, [&](const UiNode& n, const UiNode*, int) {
            if (n.type != "screen" && n.type != "group" && n.type != "text" && !IsPlain(n))
            {
                cov.total++;
                if (SkinOf(n, kit).el)
                    cov.skinned++;
                else
                    cov.missing.Add(n.id);
            }
        });
        return cov;
    }

    Vector<Vec2F> AnchorPoints(const UiNode& n, const UiRect& parent)
    {
        Anchor a = ParseAnchor(n.anchor);
        Vector<double> xs, ys;
        if (a.h == "stretch")
            xs = { parent.x, parent.x + parent.w };
        else
            xs = { a.h == "left" ? parent.x : a.h == "right" ? parent.x + parent.w : parent.x + parent.w/2 };
        if (a.v == "stretch")
            ys = { parent.y, parent.y + parent.h };
        else
            ys = { a.v == "top" ? parent.y : a.v == "bottom" ? parent.y + parent.h : parent.y + parent.h/2 };

        Vector<Vec2F> out;
        for (double x : xs)
        {
            for (double y : ys)
                out.Add(Vec2F((float)x, (float)y));
        }
        return out;
    }
}
