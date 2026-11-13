#include "o2Editor/stdafx.h"
#include "UiWsInspector.h"

#include "o2/Scene/UI/Widgets/EditBox.h"

// The root inspector's screen rows (UiWsInspector.h, port of RootScreen in AssetsLine UiWorkspace.tsx): the device the
// layout is laid out for (the kit's), the design size with where it comes from, the DPI - the device's, so read-only once
// the layout is in its pixels - and the two ways to set the design: the reference's area on the device, or the device's
// whole screen. A layout still in older units says so here and offers the step that brings it over; until then its size
// and DPI are edited as before
namespace Editor
{
    using namespace UiWsDraw;
    using UiDoc::Json;
    namespace Ws = UiDoc::Ws;

    namespace
    {
        const float pad = 12.0f;
        const float rowH = 28.0f;
        const float rowGap = 6.0f;
        const float labelW = 100.0f;

        TextStyle Wrapped(float size, const Color4& color, float lineHeight)
        {
            TextStyle style(size, 400, color);
            style.wrap = true;
            style.ver = VerAlign::Top;
            style.lineHeight = lineHeight;
            return style;
        }
    }

    void UiWsInspector::DrawRootScreen(const Box& box, float& y)
    {
        auto& t = T();
        const auto& doc = ctx->Doc();
        const auto& target = ctx->target;
        const auto& reference = ctx->reference;
        bool editable = ctx->Editable();
        float W = box.W();
        float ctrlX = pad + labelW + 8, ctrlW = W - pad - ctrlX;
        WeakRef<UiWsInspector> weak(this);

        double dpi = UiDoc::DocDpi(doc);
        bool onDevice = UiDoc::InTargetUnits(doc.dpi, target.Screen());
        bool older = Ws::InOlderUnits(doc, target);
        bool isRef = reference && reference->w == doc.width && reference->h == doc.height && reference->dpi == dpi;
        bool isDevice = onDevice && target.w == doc.width && target.h == doc.height;
        String from = older ? String() : isDevice ? String("the device's whole screen") :
            isRef && onDevice ? "the reference's area on the device (picture " + Ws::SizeText(ctx->referencePicture.w, ctx->referencePicture.h) + ")" :
            onDevice ? String("set by hand") : String();

        bool first = true;
        auto row = [&](const String& label, float height = rowH) -> RectF
        {
            if (!first)
                y += rowGap;
            first = false;
            Text(label, box.R(pad, y, labelW, rowH), TextStyle(12.0f, 400, t.fgDim));
            RectF r = box.R(ctrlX, y, ctrlW, height);
            y += height;
            return r;
        };
        // A layout made before devices: the notice and the step that brings it over
        if (older)
        {
            String text = Ws::OlderText(doc, target);
            float textW = W - pad*2 - 20 - 13 - 8;
            float textH = TextHeight(text, textW, Wrapped(11.5f, t.fgDim, 16.7f));
            float bringW = TextWidth("Bring to the kit's device", 12.0f) + 20;
            RectF note = box.R(pad, y, W - pad*2, 8 + textH + 8 + 28 + 8);
            Fill(note, 8.0f, Color4(241, 190, 91, 20));
            Border(note, 8.0f, Color4(241, 190, 91, 56));
            UiWsDraw::Icon("uiws_warn", box.P(pad + 10 + 6.5f, y + 8 + 8), 13.0f, t.yellow);
            Text(text, box.R(pad + 10 + 13 + 8, y + 8, textW, textH + 2), Wrapped(11.5f, t.fgDim, 16.7f));
            Button(box.R(pad + 10 + 21, y + 8 + textH + 8, Math::Min(bringW, W - pad*2 - 20 - 21), 28), "Bring to the kit's device", "bring", false,
                   editable && !ctx->Busy());
            y += note.Height() + 10;
        }

        // The device: the kit's, set in the UI kit node
        {
            RectF r = row("Device");
            float cy = r.Center().y;
            UiWsDraw::Icon(DeviceIcon(Ws::TargetKind(target)), Vec2F(r.left + 6.5f, cy), 13.0f, t.fgDim);
            float nameW = TextWidth(target.name, 12.0f, 500) + 2;
            Text(target.name, RectF(r.left + 13 + 6, r.top, r.left + 13 + 6 + nameW, r.bottom), TextStyle(12.0f, 500, t.fg));
            Text(Ws::SizeText(target.w, target.h) + " \xC2\xB7 " + UiDoc::JsNum(target.dpi) + " dpi",
                 RectF(r.left + 13 + 6 + nameW + 6, r.top, r.right, r.bottom), TextStyle(11.0f, 400, t.fg3));
        }

        // The design size and where it comes from
        {
            RectF r = row("Size");
            float cw = (r.Width() - 6.0f)/2.0f;
            auto side = [weak](bool width)
            {
                return [weak, width](std::optional<double> v)
                {
                    auto s = weak.Lock();
                    if (!s || !v || *v < 64)
                        return;
                    const auto& d = s->ctx->Doc();
                    s->Screen(UiDoc::MakeScreenOp("", width ? *v : d.width, width ? d.height : *v, std::nullopt), "screen size");
                };
            };
            auto fieldW = PlaceField("num:screen:w", RectF(r.left, r.top, r.left + cw, r.bottom), [&]()
            {
                return UiWsControls::NumberField("W", doc.width, "", side(true), 64.0);
            });
            UiWsControls::SetNumber(fieldW, doc.width);
            auto fieldH = PlaceField("num:screen:h", RectF(r.left + cw + 6, r.top, r.right, r.bottom), [&]()
            {
                return UiWsControls::NumberField("H", doc.height, "", side(false), 64.0);
            });
            UiWsControls::SetNumber(fieldH, doc.height);
        }
        if (!from.IsEmpty())
        {
            float h = TextHeight(from, ctrlW, Wrapped(11.0f, t.fg3, 14.9f));
            y += rowGap - 4;
            Text(from, box.R(ctrlX, y, ctrlW, h + 2), Wrapped(11.0f, t.fg3, 14.9f));
            y += h;
        }

        // The DPI: the device's once the layout is in its pixels
        {
            RectF r = row("DPI");
            if (onDevice)
            {
                float dpiW = TextWidth(UiDoc::JsNum(dpi), 12.0f, 500) + 2;
                Text(UiDoc::JsNum(dpi), RectF(r.left, r.top, r.left + dpiW, r.bottom), TextStyle(12.0f, 500, t.fg));
                Text(UiDoc::DensityName(dpi) + " \xC2\xB7 the kit's device", RectF(r.left + dpiW + 6, r.top, r.right, r.bottom),
                     TextStyle(11.0f, 400, t.fg3));
            }
            else
            {
                float cw = (r.Width() - 6.0f)/2.0f;
                auto field = PlaceField("num:screen:dpi", RectF(r.left, r.top, r.left + cw, r.bottom), [&]()
                {
                    return UiWsControls::NumberField("", dpi, UiDoc::JsNum(UiDoc::UI_BASE_DPI), [weak](std::optional<double> v)
                    {
                        auto s = weak.Lock();
                        auto d = v ? UiDoc::CleanDpi(*v) : std::nullopt;
                        if (s && d)
                            s->Screen(UiDoc::MakeScreenOp("", std::nullopt, std::nullopt, *d), "screen dpi");
                    }, 36.0);
                });
                UiWsControls::SetNumber(field, dpi);
                Text(UiDoc::DensityName(dpi), RectF(r.left + cw + 6 + 2, r.top, r.right, r.bottom), TextStyle(11.0f, 400, t.fg3, HorAlign::Left, true));
            }
        }

        // The two ways to set the design: the reference's area on the device, the device's whole screen
        if (reference && !isRef)
        {
            RectF r = row("");
            Button(r, "Same as the reference \xC2\xB7 " + Ws::SizeText(reference->w, reference->h), "asref", false, editable, "uiws_dev_reference");
        }
        if (!older && !isDevice)
        {
            RectF r = row("");
            Button(r, "As the kit's device \xC2\xB7 " + Ws::SizeText(target.w, target.h), "askit", false, editable, "uiws_dev_phone");
        }
    }

    bool UiWsInspector::OnRootClick(const String& id)
    {
        if (!ctx || !ctx->Editable())
            return false;

        if (id == "bring")
        {
            ctx->Bring();
            return true;
        }
        if (id == "asref" && ctx->reference)
        {
            auto& ref = *ctx->reference;
            Screen(UiDoc::MakeScreenOp("", ref.w, ref.h, ref.dpi), "screen = reference");
            return true;
        }
        if (id == "askit")
        {
            auto& target = ctx->target;
            Screen(UiDoc::MakeScreenOp("", target.w, target.h, target.dpi), "screen = device");
            return true;
        }
        return false;
    }
}
