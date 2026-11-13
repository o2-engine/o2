#include "o2Editor/stdafx.h"
#include "UiWsScreen.h"

#include <cmath>

#include "o2Editor/Pipeline/UiDoc/UiDocFp.h"

namespace Editor::UiDoc::Ws
{
    static const char* const times = "\xC3\x97"; // ×

    // --- comparing with the reference ---------------------------------------------------------

    const Vector<String>& RefModes()
    {
        static const Vector<String> modes = { "alpha", "split", "diff" };
        return modes;
    }

    static double Clamp01(double v)
    {
        return std::max(0.0, std::min(1.0, v));
    }

    UiRefSetting RefSettingOf(const Json& config, bool hasReference)
    {
        UiRefSetting s;
        const Json* overlay = JsGet(config, "uiRefOverlay");
        s.alpha = overlay && overlay->IsNumber() ? Clamp01(overlay->Num()) : 0.0;
        const Json* on = JsGet(config, "uiRefOn");
        s.on = hasReference && (on && on->IsBool() ? on->Bool() : s.alpha > 0);
        const Json* mode = JsGet(config, "uiRefMode");
        if (mode && mode->IsString() && RefModes().Contains(mode->Str()))
            s.mode = mode->Str();
        const Json* split = JsGet(config, "uiRefSplit");
        s.split = split && split->IsNumber() ? Clamp01(split->Num()) : 0.5;
        return s;
    }

    Vector<Pair<String, Json>> RefTogglePatch(const UiRefSetting& setting)
    {
        if (setting.on)
            return { { "uiRefOn", Json(false) } };

        Vector<Pair<String, Json>> patch = { { "uiRefOn", Json(true) } };
        if (!(setting.alpha > 0))
            patch.Add({ "uiRefOverlay", Json(0.5) });
        return patch;
    }

    Vector<Pair<String, Json>> RefModePatch(const UiRefSetting& setting, const String& mode, bool withOff)
    {
        if (mode == "off")
            return { { "uiRefOn", Json(false) } };

        Vector<Pair<String, Json>> patch = { { "uiRefMode", Json(mode) } };
        if (withOff)
        {
            patch.Add({ "uiRefOn", Json(true) });
            if (!(setting.alpha > 0))
                patch.Add({ "uiRefOverlay", Json(0.5) });
        }
        return patch;
    }

    Vector<Pair<String, Json>> RefAmountPatch(const UiRefSetting& setting, double value)
    {
        value = Clamp01(value);
        if (setting.mode == "split")
            return { { "uiRefSplit", Json(JsRound(value*100)/100) } };

        double v = JsRound(JsRound(value/0.05)*0.05*100)/100;
        return { { "uiRefOverlay", Json(v) }, { "uiRefOn", Json(true) } };
    }

    double RefAmount(const UiRefSetting& setting)
    {
        return setting.mode == "split" ? setting.split : setting.alpha;
    }

    String RefModeLabel(const String& mode)
    {
        return mode == "alpha" ? "Alpha" : mode == "split" ? "Split" : mode == "diff" ? "Diff" : "Off";
    }

    // --- the layout's device and units ------------------------------------------------------------

    UiTarget LayoutTarget(const UiKit* kit)
    {
        return kit && kit->device ? *kit->device : DefaultTarget();
    }

    String TargetKind(const UiTarget& target)
    {
        if (const UiDevice* d = DeviceById(target.id))
            return d->kind;
        return target.w > target.h ? "desktop" : "phone";
    }

    bool InOlderUnits(const UiDoc& doc, const UiTarget& target)
    {
        return !doc.root.children.empty() && !InTargetUnits(doc.dpi, target.Screen());
    }

    std::optional<UiOp> FirstScreenOp(const UiDoc& doc, const UiTarget& target, const UiScreen* reference)
    {
        if (!doc.root.children.empty() || InTargetUnits(doc.dpi, target.Screen()))
            return std::nullopt;

        return MakeScreenOp("", reference ? reference->w : target.w, reference ? reference->h : target.h, target.dpi);
    }

    std::optional<UiOp> BringOp(const UiDoc& doc, const UiTarget& target, double* scale /*= nullptr*/)
    {
        if (!InOlderUnits(doc, target))
            return std::nullopt;

        UiTargetUnits to = ToTargetUnits(doc.width, doc.height, doc.dpi, target.Screen());
        if (scale)
            *scale = to.scale;
        return MakeRescaleOp(to.width, to.height, to.dpi, to.scale, to.fromDpi);
    }

    UiScreen ReferenceScreenOf(const UiDoc& doc, const UiTarget& target, double pictureW, double pictureH,
                               std::optional<double> pictureDpi)
    {
        bool onDevice = doc.root.children.empty() || InTargetUnits(doc.dpi, target.Screen());
        if (!onDevice)
            return UiScreen{ pictureW, pictureH, pictureDpi ? *pictureDpi : UI_BASE_DPI };

        UiOnTarget on = SourceOnTarget(target.Screen(), pictureW, pictureH);
        return UiScreen{ on.w, on.h, target.dpi };
    }

    String OlderText(const UiDoc& doc, const UiTarget& target)
    {
        return "This layout is in older units (" + SizeText(doc.width, doc.height) + " at " + JsNum(DocDpi(doc)) +
            " dpi). Bring it to the kit's device: " + target.name;
    }

    String OlderShort(const UiDoc& doc)
    {
        return "Older units \xC2\xB7 " + SizeText(doc.width, doc.height) + " at " + JsNum(DocDpi(doc)) + " dpi";
    }

    String BringTitle(const UiTarget& target)
    {
        return "One step: every size goes to the pixels of " + target.name + " (" + SizeText(target.w, target.h) + ", " +
            JsNum(target.dpi) + " dpi). The layout looks the same; undo brings the old units back";
    }

    String BringLabel(const UiTarget& target)
    {
        return "Units \xE2\x86\x92 " + target.name;
    }

    // --- the screen picker's words ---------------------------------------------------------------

    String SizeText(double w, double h)
    {
        return JsNum(w) + times + JsNum(h);
    }

    UiSize FrameExtra(const UiDoc& doc, const UiFrame& frame)
    {
        return UiSize{ std::max(0.0, frame.w - doc.width), std::max(0.0, frame.h - doc.height) };
    }

    String AddedText(const UiDoc& doc, const UiPreview& preview)
    {
        UiSize extra = FrameExtra(doc, preview.frame);
        Vector<String> parts;
        if (extra.w)
            parts.Add("+" + JsNum(extra.w) + " px width");
        if (extra.h)
            parts.Add("+" + JsNum(extra.h) + " px height");
        return Join(parts, ", ");
    }

    String CanvasText(const UiDoc& doc, double screenW, double screenH)
    {
        UiFrame frame = PreviewFrame(doc.width, doc.height, doc.dpi, UiScreen{ screenW, screenH, DocDpi(doc) }, "fit");
        return SizeText(frame.w, frame.h);
    }

    String PreviewIconKind(const UiPreview& preview, const UiTarget& target)
    {
        return preview.kind == "kit" ? TargetKind(target) : preview.kind;
    }

    // --- text style previews -------------------------------------------------------------------

    String DefaultStyleSample(const TextStyle& style)
    {
        return style.role == "number" ? "1 250" : style.role == "button" ? "Play" : style.role == "title" ? "Forest Quest" :
            style.role == "caption" ? "Tap to continue" : "Hello, world";
    }

    String StyleSampleText(const UiNode& n, const TextStyle& style)
    {
        // Every run of white space as one space, trimmed; the length in characters (UTF-8 lead bytes)
        std::string raw = n.text ? std::string(n.text->Data()) : std::string();
        std::string own;
        bool space = false;
        int length = 0;
        for (char c : raw)
        {
            if (c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f' || c == '\v')
            {
                space = !own.empty();
                continue;
            }
            if (space)
            {
                own += ' ';
                length++;
            }
            space = false;
            own += c;
            if (((unsigned char)c & 0xC0) != 0x80)
                length++;
        }
        return length > 0 && length <= 22 ? String(own.c_str()) : DefaultStyleSample(style);
    }

    UiDoc TightStyleDoc(const TextStyle& style, const String& text)
    {
        TextReach reach = EffectsReach(style);
        double pad = JsRound(style.size*0.2);
        double w = std::ceil(EstimateTextWidth(text, style)*1.04 + reach.left + reach.right + 2*pad);
        double h = std::ceil(style.size*style.lineHeight + reach.top + reach.bottom + pad);
        UiDoc d = EmptyDoc("Sample", std::max(64.0, w), std::max(64.0, h));
        d.root = UiNode();
        d.root.id = "root";
        d.root.type = "screen";
        d.root.Set("color", Json(String("#20232B")));

        UiNode label;
        label.id = "sample";
        label.type = "text";
        label.Set("anchor", Json(String("stretch")));
        Json margin = Json::Array();
        for (double v : { reach.top + pad/2, reach.right + pad, reach.bottom + pad/2, reach.left + pad })
            margin.Add(Json(v));
        label.Set("margin", margin);
        label.Set("style", Json(style.name));
        label.Set("text", Json(text));
        label.Set("textAlign", Json(String("center")));
        d.root.SetChildren({ label });
        return d;
    }

    TextStyle AutoStyleOf(const UiNode& n, const UiKit* kit)
    {
        UiNode plain = n;
        plain.Unset("style");
        return StyleOf(plain, kit, n.type == "text" ? nullptr : &n);
    }
}
