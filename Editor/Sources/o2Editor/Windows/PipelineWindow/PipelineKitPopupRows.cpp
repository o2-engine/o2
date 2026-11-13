#include "o2Editor/stdafx.h"
#include "PipelineKitCard.h"

#include "o2/Render/Render.h"
#include "o2/Scene/UI/Widgets/EditBox.h"
#include "o2/Utils/Editor/DragHandle.h"
#include "o2Editor/Pipeline/UiDoc/UiDocJs.h"
#include "o2Editor/Pipeline/UiDoc/UiDocRender.h"
#include "o2Editor/Windows/PipelineWindow/PipelineEditor.h"

namespace Editor
{
    using namespace UiWsDraw;
    using UiDoc::Json;

    // --- the element on the kit's device ----------------------------------------------------------
    // Three rows (UiDocDevice.h, "the target device"): the element's real size on the device - found from where the picture
    // came from, or typed here in inches (item.size); the density of its texture - the device's, or its own (item.dpi); and
    // the texture the kit made of the picture for that size and density, with a warning when the picture had too few pixels

    namespace
    {
        const float pad = 10.0f, rowH = 26.0f, rowGap = 6.0f, labelW = 76.0f;
        const double inchMin = 0.005, inchMax = 100.0;

        double R3(double v) { return std::round(v*1000.0)/1000.0; }

        // The size lock (.uik-lock): a padlock, closed while the sides keep their proportion; the accent then
        void DrawLock(const RectF& rect, bool locked, bool hover)
        {
            auto& t = T();
            if (hover)
                Fill(rect, 6.0f, t.active);
            Color4 color = locked ? t.accent : hover ? t.fg : t.fg3;
            Vec2F c = rect.Center();
            Fill(RectF(c.x - 4.0f, c.y + 0.5f, c.x + 4.0f, c.y - 4.5f), 1.5f, color);
            float lift = locked ? 0.0f : 2.0f, r = 2.6f, top = c.y + 2.5f + lift;
            Vector<Vec2F> shackle = { Vec2F(c.x - r, c.y + 0.5f), Vec2F(c.x - r, top) };
            for (int i = 1; i <= 8; i++)
            {
                float angle = Math::PI()*(1.0f - (float)i/8.0f);
                shackle.Add(Vec2F(c.x + Math::Cos(angle)*r, top + Math::Sin(angle)*r));
            }
            shackle.Add(Vec2F(c.x + r, locked ? c.y + 0.5f : top - 1.0f));
            for (int i = 0; i + 1 < shackle.Count(); i++)
                Line(shackle[i], shackle[i + 1], color, 1.4f);
        }

        std::optional<UiDoc::UiPair> TypedSize(const Json& item)
        {
            auto size = item.Get("size");
            if (size && size->IsObject() && size->Get("w") && size->Get("h") && size->Get("w")->Num() > 0 && size->Get("h")->Num() > 0)
                return UiDoc::UiPair{ size->Get("w")->Num(), size->Get("h")->Num() };
            return std::nullopt;
        }

        Json SizeJson(double w, double h)
        {
            Json size = Json::Object();
            size["w"] = w;
            size["h"] = h;
            return size;
        }
    }

    void PipelineKitPopup::SetSide(bool isWidth, std::optional<double> value)
    {
        auto self = body.Lock();
        if (!self)
            return;

        if (!value)
        {
            mKeptRatio.reset();
            self->SetItemFields(portId, { { "size", Json(nullptr) } });
            return;
        }

        auto item = self->ItemOf(portId);
        auto el = self->ElementOf(portId);
        auto typed = TypedSize(item);
        auto now = typed ? typed : el ? el->inches : std::nullopt;
        double n = R3(Math::Min(inchMax, *value));
        if (!mLocked || !now)
        {
            self->SetItemFields(portId, { { "size", isWidth ? SizeJson(n, now ? now->h : n) : SizeJson(now ? now->w : n, n) } });
            return;
        }

        // The sides change together: the proportion of the size when the typing began
        double ratio = mKeptRatio.value_or(now->h/now->w);
        mKeptRatio = ratio;
        double other = R3(Math::Max(inchMin, Math::Min(inchMax, isWidth ? n*ratio : n/ratio)));
        self->SetItemFields(portId, { { "size", isWidth ? SizeJson(n, other) : SizeJson(other, n) } });
    }

    float PipelineKitPopup::LayDevice(const Box& box, bool draw, float y)
    {
        auto& t = T();
        auto self = body.Lock();
        auto item = self->ItemOf(portId);
        auto el = self->ElementOf(portId);
        auto device = self->Device();
        float W = box.W(), inner = W - pad*2.0f;
        auto label = [&](const String& text)
        {
            if (draw)
                Text(text, box.R(pad, y, labelW - 6.0f, rowH), TextStyle(11.0f, 400, t.fgDim));
        };

        // Size: W, the lock, H, inches, auto; under it the device pixels and where the size came from
        label("Size");
        auto typed = TypedSize(item);
        auto now = typed ? typed : el ? el->inches : std::nullopt;
        String from = typed ? String("item") : el && el->sizeFrom && *el->sizeFrom != "item" ? *el->sizeFrom : String();
        float x = pad + labelW, fieldW = 74.0f;
        Field("sizeW", box.R(x, y, fieldW, rowH));
        RectF lockRect = box.R(x + fieldW + 3.0f, y, 20.0f, rowH);
        if (draw)
            DrawLock(lockRect, mLocked, IsHover("lock"));
        AddHot(lockRect, "lock");
        Field("sizeH", box.R(x + fieldW + 26.0f, y, fieldW, rowH));
        float unitX = x + fieldW*2.0f + 26.0f + 4.0f;
        if (draw)
            Text("in", box.R(unitX, y, 16.0f, rowH), TextStyle(11.0f, 400, t.fg3, HorAlign::Left, true));
        if (typed)
        {
            RectF autoRect = box.R(W - pad - 46.0f, y, 46.0f, rowH);
            if (draw)
                Button(autoRect, "auto", "sizeAuto");
            AddHot(autoRect, "sizeAuto");
        }
        y += rowH + 3.0f;
        String origin;
        if (from == "source")
            origin = el && el->share ? "from the source \xC2\xB7 " + (String)(int)Math::Round((float)(*el->share*100.0)) + "% of the screen's width" :
                String("from the source");
        else if (from == "item")
            origin = "set by hand";
        else if (from == "tag")
            origin = "from the picture's density tag";
        else if (from == "pixels")
            origin = "pixel for pixel";
        String px = now ? UiDoc::JsNum(UiDoc::JsRound(now->w*device.dpi)) + " \xC3\x97 " + UiDoc::JsNum(UiDoc::JsRound(now->h*device.dpi)) + " px" : String();
        String note = px.IsEmpty() ? origin : origin.IsEmpty() ? px : px + " \xC2\xB7 " + origin;
        if (!note.IsEmpty())
        {
            if (draw)
                Text(note, box.R(x, y, inner - labelW, 16.0f), TextStyle(11.0f, 400, t.fg3));
            y += 16.0f;
        }
        y += rowGap;

        // DPI: the kit's device or its own
        label("DPI");
        auto own = UiDoc::CleanDpi(item.Get("dpi"));
        RectF seg = box.R(x, y, 170.0f, rowH);
        if (draw)
            Segments(seg, { { "kit", "Kit \xC2\xB7 " + PipelineKitCards::DpiText(device.dpi) }, { "own", "Own" } }, own ? "own" : "kit", "dpi:");
        Field("dpi", box.R(x + 176.0f, y, 72.0f, rowH));
        y += rowH + rowGap;

        // Texture: what the kit made
        label("Texture");
        double dpi = el ? UiDoc::GraphicDpi(el->dpi) : 0.0;
        String texture = el ? UiDoc::JsNum(el->width) + " \xC3\x97 " + UiDoc::JsNum(el->height) + " px" +
            (el->texture ? " (from " + UiDoc::JsNum(el->texture->w) + "\xC3\x97" + UiDoc::JsNum(el->texture->h) + ")" : String()) +
            " \xC2\xB7 " + PipelineKitCards::DpiText(dpi) + " dpi \xC2\xB7 " + UiDoc::DensityName(dpi) : String("appears after the kit renders");
        if (draw)
            Text(texture, box.R(x, y, inner - labelW, rowH), TextStyle(11.0f, 400, t.fgDim));
        y += rowH;
        if (el && el->soft)
        {
            String warn = "The picture has fewer pixels than this size needs at " + PipelineKitCards::DpiText(own.value_or(device.dpi)) +
                " dpi: it is kept as it is, at " + PipelineKitCards::DpiText(dpi) + " dpi, and will look soft";
            TextStyle style(11.0f, 400, t.yellow);
            style.wrap = true;
            style.ver = VerAlign::Top;
            style.lineHeight = 15.4f;
            float h = TextHeight(warn, inner - labelW - 18.0f, style);
            if (draw)
            {
                Icon("uiws_warn", box.P(x + 6.0f, y + 8.0f), 12.0f, t.yellow);
                Text(warn, box.R(x + 18.0f, y, inner - labelW - 18.0f, h), style);
            }
            y += h + 2.0f;
        }
        return y + rowGap;
    }

    // --- a text style's settings ---------------------------------------------------------------------

    float PipelineKitPopup::LayStyle(const Box& box, bool draw, float y)
    {
        auto& t = T();
        auto self = body.Lock();
        float W = box.W(), inner = W - pad*2.0f;
        Field("name", box.R(pad, y, inner - 26.0f - 6.0f, 28.0f));
        if (draw)
            Tool(box.R(W - pad - 26.0f, y + 1.0f, 26.0f, 26.0f), "uiws_close", "close", false, true, 12.0f);
        y += 28.0f + 8.0f;

        // The sample on the dark paper
        auto value = self->InputValue(portId);
        std::optional<UiDoc::TextStyle> style;
        if (value.type == PipelinePortType::TextStyle)
        {
            Json raw;
            Json::Parse(value.data, raw);
            style = UiDoc::NormalizeTextStyle(raw);
        }
        RectF demo = box.R(pad, y, inner, 110.0f);
        if (draw)
        {
            Fill(demo, 7.0f, t.bg2);
            if (style)
            {
                String key = UiDoc::TextStyleToJson(*style).Dump();
                if (key != mStyleKey)
                {
                    mStyleKey = key;
                    auto picture = UiDoc::RenderTextStyleSample(*style, style->role == "number" ? "1 250" : "Forest Quest", true, 1.0);
                    mStyleSample = picture.IsEmpty() ? nullptr : picture.ToBitmap();
                    mStyleTexture = mStyleSample ? TextureRef(*mStyleSample) : TextureRef();
                }
                if (mStyleSample)
                {
                    Vec2I size = mStyleSample->GetSize();
                    float k = Math::Min(demo.Width()/Math::Max(1, size.x), demo.Height()/Math::Max(1, size.y));
                    Vec2F c = demo.Center(), half(size.x*k*0.5f, size.y*k*0.5f);
                    Picture(mStyleTexture, size, RectF(c.x - half.x, c.y + half.y, c.x + half.x, c.y - half.y));
                }
            }
            else
                Text("not connected", demo, TextStyle(11.5f, 400, t.fg3, HorAlign::Middle));
        }
        y += 110.0f + 8.0f;

        if (style)
        {
            if (draw)
            {
                String meta = style->role + " \xC2\xB7 " + style->font + " " + UiDoc::JsNum(style->weight) + " \xC2\xB7 " + UiDoc::JsNum(style->size) + "px";
                Text(meta, box.R(pad, y, inner, 14.0f), TextStyle(10.5f, 400, t.fg3, HorAlign::Left, true));
            }
            y += 14.0f + 8.0f;
        }

        String note = "Edit the style in its Text style node; the kit takes it as it is.";
        TextStyle noteStyle(11.0f, 400, t.fgDim);
        noteStyle.wrap = true;
        noteStyle.lineHeight = 15.0f;
        float h = TextHeight(note, inner, noteStyle);
        if (draw)
            Text(note, box.R(pad, y, inner, h), noteStyle);
        y += h + 8.0f;

        if (draw)
        {
            Line(box.P(0, y), box.P(W, y), t.line);
            Button(box.R(W - pad - 100.0f, y + 8.0f, 100.0f, 26.0f), "Remove", "remove", false, true, "uiws_trash");
        }
        AddHot(box.R(W - pad - 100.0f, y + 8.0f, 100.0f, 26.0f), "remove");
        return y + 8.0f + 26.0f;
    }

    // --- the source with its guides -----------------------------------------------------------------

    PipelineKitGuides::PipelineKitGuides(RefCounter* refCounter):
        PipelineImageView(refCounter)
    {
        WeakRef<PipelineKitGuides> weakThis(this);
        for (int side = 0; side < 4; side++)
        {
            auto handle = mmake<DragHandle>();
            handle->cursorType = side < 2 ? CursorType::SizeWE : CursorType::SizeNS;
            handle->isPointInside = [weakThis, side](const Vec2F& p)
            {
                auto self = weakThis.Lock();
                return self && self->IsOnGuide(side, p);
            };
            handle->onChangedPos = [weakThis, side](const Vec2F& p) { if (auto self = weakThis.Lock()) self->OnDragged(side, p); };
            handle->onChangeCompleted = [weakThis]()
            {
                auto self = weakThis.Lock();
                if (self && self->mDragging && self->onSet && self->mNine)
                    self->onSet(*self->mNine);
                if (self)
                    self->mDragging = false;
            };
            mHandles.Add(handle);
        }
    }

    void PipelineKitGuides::SetGuides(int width, int height, const Json& trim, const std::optional<UiDoc::UiNine>& nine, bool automatic)
    {
        mW = width;
        mH = height;
        mTrim = trim;
        if (!mDragging)
            mNine = nine;
        mAuto = automatic;
    }

    void PipelineKitGuides::Draw()
    {
        PipelineImageView::Draw();
        if (!mResEnabledInHierarchy || mIsClipped || !HasImage() || mW <= 0 || mH <= 0 || PipelineControls::IsFarView())
            return;

        RectF image = GetImageRect();
        float k = image.Width()/mW;
        if (mTrim.IsObject())
        {
            float x = (float)mTrim.Get("x")->Num(), y = (float)mTrim.Get("y")->Num();
            float w = (float)mTrim.Get("w")->Num(), h = (float)mTrim.Get("h")->Num();
            if (w != mW || h != mH)
                DashedFrame(RectF(image.left + x*k, image.top - y*k, image.left + (x + w)*k, image.top - (y + h)*k),
                            Color4(0x6F, 0xE3, 0xCF, 255), 4.0f, 3.0f, 1.2f);
        }

        if (!mNine)
            return;

        Color4 pink(0xFF, 0x7A, 0xB6, 255);
        for (int side = 0; side < 4; side++)
        {
            Vec2F a, b;
            GuideLine(side, a, b);
            if (mAuto && !mDragging)
                DashedLine(a, b, pink, 5.0f, 3.0f, 1.2f);
            else
                o2Render.DrawAALine({ a, b }, pink, 1.4f);

            mHandles[side]->position = (a + b)*0.5f;
            mHandles[side]->Draw();
        }
    }

    void PipelineKitGuides::GuideLine(int side, Vec2F& a, Vec2F& b) const
    {
        RectF image = GetImageRect();
        float k = image.Width()/mW;
        const auto& n = *mNine;
        if (side == 0 || side == 1)
        {
            float x = side == 0 ? image.left + (float)n.left*k : image.right - (float)n.right*k;
            a = Vec2F(x, image.bottom);
            b = Vec2F(x, image.top);
        }
        else
        {
            float y = side == 2 ? image.top - (float)n.top*k : image.bottom + (float)n.bottom*k;
            a = Vec2F(image.left, y);
            b = Vec2F(image.right, y);
        }
    }

    bool PipelineKitGuides::IsOnGuide(int side, const Vec2F& p) const
    {
        if (!mNine || !HasImage() || mW <= 0)
            return false;

        Vec2F a, b;
        GuideLine(side, a, b);
        float pixel = Math::Max(0.0001f, o2Render.GetCamera().GetScale2D().x);
        float grab = 5.0f*pixel;
        return side < 2 ? Math::Abs(p.x - a.x) < grab && p.y >= a.y - grab && p.y <= b.y + grab :
            Math::Abs(p.y - a.y) < grab && p.x >= a.x - grab && p.x <= b.x + grab;
    }

    void PipelineKitGuides::OnDragged(int side, const Vec2F& p)
    {
        if (!mNine)
            return;

        mDragging = true;
        RectF image = GetImageRect();
        float k = image.Width()/mW;
        double v = side == 0 ? (p.x - image.left)/k : side == 1 ? mW - (p.x - image.left)/k :
            side == 2 ? (image.top - p.y)/k : mH - (image.top - p.y)/k;
        auto n = *mNine;
        double span = side < 2 ? mW : mH;
        double other = side == 0 ? n.right : side == 1 ? n.left : side == 2 ? n.bottom : n.top;
        double value = Math::Max(0.0, UiDoc::JsRound(Math::Min(v, span - other - 1)));
        (side == 0 ? n.left : side == 1 ? n.right : side == 2 ? n.top : n.bottom) = value;
        mNine = n;
        if (onLive)
            onLive(n);
    }
}
