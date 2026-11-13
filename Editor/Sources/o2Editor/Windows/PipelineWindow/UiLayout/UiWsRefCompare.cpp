#include "o2Editor/stdafx.h"
#include "UiWsRefCompare.h"

#include <chrono>

#include "o2/Application/Input.h"
#include "o2/Render/Render.h"
#include "o2/Render/Sprite.h"
#include "o2/Utils/Editor/EditorScope.h"
#include "o2Editor/Pipeline/UiDoc/UiDocRender.h"
#include "o2Editor/Pipeline/UiLayout/UiLayoutCompare.h"
#include "o2Editor/Windows/PipelineWindow/PipelineEditor.h"
#include "o2Editor/Windows/PipelineWindow/PipelinePairViews.h"
#include "o2Editor/Windows/PipelineWindow/UiLayout/UiWsPickers.h"

namespace Editor
{
    using namespace UiWsDraw;
    using UiDoc::Json;
    namespace Ws = UiDoc::Ws;

    namespace
    {
        const int diffMaxSide = 1600;    // The longer side of the difference (a larger screen is compared scaled down)
        const float diffPause = 0.14f;   // The layout rests this long before its difference is worked out
        const float dividerReach = 7.0f; // Screen pixels either side of the divider that take it

        TextureRef MakeTexture(const UiLayout::UiImage& image, Vec2I& size)
        {
            auto bitmap = image.ToBitmap();
            TextureRef texture(*bitmap);
            texture->SetFilter(Texture::Filter::Linear);
            size = bitmap->GetSize();
            return texture;
        }

        Ref<Sprite>& SharedSprite()
        {
            static Ref<Sprite>* sprite = new Ref<Sprite>(mmake<Sprite>());
            return *sprite;
        }

        RectF Normalized(const RectF& r)
        {
            return RectF(Math::Min(r.left, r.right), Math::Max(r.top, r.bottom), Math::Max(r.left, r.right), Math::Min(r.top, r.bottom));
        }
    }

    // --- the layers --------------------------------------------------------------------------------

    float UiRefLayers::DividerX(const RectF& screen, double split)
    {
        return screen.left + (float)(screen.Width()*Math::Clamp((float)split, 0.0f, 1.0f));
    }

    double UiRefLayers::SplitAt(const RectF& screen, float x)
    {
        return screen.Width() > 0.0f ? Math::Clamp((x - screen.left)/screen.Width(), 0.0f, 1.0f) : 0.5;
    }

    bool UiRefLayers::OnDivider(const RectF& screen, double split, const Vec2F& point, float pixel /*= 1.0f*/)
    {
        float x = DividerX(screen, split);
        return Math::Abs(point.x - x) <= dividerReach*pixel && point.y <= screen.top && point.y >= screen.bottom;
    }

    String UiRefLayers::WantedKey(const UiWsContext& ctx, const String& view, bool showIds)
    {
        if (!ctx.ref.on || ctx.ref.mode != "diff" || !mRefImage)
            return String();

        if (ctx.version != mCtxVersion)
        {
            mCtxVersion = ctx.version;
            const auto& doc = ctx.Doc();
            mDocKey = UiDoc::NodeToJson(doc.root).Dump() + "|" + doc.name + "|" + UiDoc::JsNum(doc.width) + "x" + UiDoc::JsNum(doc.height) +
                "|" + (doc.dpi ? UiDoc::JsNum(*doc.dpi) : String());
        }
        return mDocKey + "|" + view + "|" + (showIds ? "ids" : "") + "|" + (String)ctx.kitVersion + "|" + (String)mRefVersion + "|" +
            UiDoc::JsNum(ctx.preview.frame.w) + "x" + UiDoc::JsNum(ctx.preview.frame.h);
    }

    void UiRefLayers::Update(const UiWsContext& ctx, const String& view, bool showIds, float dt, bool resting)
    {
        // The reference picture, read again only when its bytes changed
        if (ctx.referencePng != mRefPng)
        {
            mRefPng = ctx.referencePng;
            mRefVersion++;
            mRefTexture = TextureRef();
            mRefImage = nullptr;
            UiLayout::UiImage image;
            if (!mRefPng.IsEmpty() && UiLayout::UiImage::Decode(mRefPng, image) && !image.IsEmpty())
            {
                mRefImage = std::make_shared<UiLayout::UiImage>(image);
                mRefTexture = MakeTexture(image, mRefSize);
            }
        }

        TakeDiff();
        String want = WantedKey(ctx, view, showIds);
        if (want != mWantKey)
        {
            mWantKey = want;
            mQuiet = 0.0f;
        }
        else
            mQuiet += dt;

        if (!mWantKey.IsEmpty() && mWantKey != mDiffKey && !mJob && resting && mQuiet >= diffPause)
            StartDiff(ctx, view, showIds);
    }

    void UiRefLayers::StartDiff(const UiWsContext& ctx, const String& view, bool showIds)
    {
        const auto& frame = ctx.preview.frame;
        double k = std::min(1.0, diffMaxSide/std::max(1.0, std::max(frame.w, frame.h)));
        UiDoc::UiRenderOptions o;
        o.mode = view == "kit" && ctx.hasKit ? "kit" : "lines";
        o.width = frame.w;
        o.height = frame.h;
        o.showIds = showIds;
        o.scale = k;

        auto job = std::make_shared<DiffJob>();
        job->key = mWantKey;
        auto doc = std::make_shared<UiDoc::UiDoc>(ctx.Doc());
        auto kit = ctx.hasKit ? std::make_shared<UiDoc::UiKit>(ctx.kit) : nullptr;
        auto rects = std::make_shared<UiDoc::UiRects>(ctx.rects);
        auto reference = mRefImage;
        job->picture = std::async(std::launch::async, [doc, kit, rects, reference, o]() mutable
        {
            o.kit = kit.get();
            o.rects = rects.get();
            UiLayout::UiImage shot = UiDoc::RenderLayout(*doc, o);
            if (shot.IsEmpty() || !reference || reference->IsEmpty())
                return UiLayout::UiImage();

            // The reference stretched over the same screen, both on the same backdrop
            UiLayout::UiImage ref = reference->Resize(shot.w, shot.h);
            return UiLayout::DiffPicture(ref, shot, std::max(shot.w, shot.h));
        });
        mJob = job;
    }

    void UiRefLayers::TakeDiff()
    {
        if (!mJob || mJob->picture.wait_for(std::chrono::seconds(0)) != std::future_status::ready)
            return;

        UiLayout::UiImage picture = mJob->picture.get();
        String key = mJob->key;
        mJob = nullptr;
        if (picture.IsEmpty())
            return;

        mDiffTexture = MakeTexture(picture, mDiffSize);
        mDiffKey = key;
    }

    void UiRefLayers::FinishDiff(const UiWsContext& ctx, const String& view, bool showIds)
    {
        if (mJob)
            mJob->picture.wait();
        TakeDiff();
        mWantKey = WantedKey(ctx, view, showIds);
        if (!mWantKey.IsEmpty() && mWantKey != mDiffKey)
        {
            StartDiff(ctx, view, showIds);
            mJob->picture.wait();
            TakeDiff();
        }
    }

    void UiRefLayers::Draw(const RectF& rawScreen, const Ws::UiRefSetting& setting, double split, bool dividerActive)
    {
        if (!setting.on || !mRefTexture)
            return;

        RectF screen = Normalized(rawScreen);
        auto& sprite = SharedSprite();
        if (setting.mode == "alpha")
        {
            if (setting.alpha > 0.0)
                Picture(mRefTexture, mRefSize, screen, (float)setting.alpha);
        }
        else if (setting.mode == "split")
        {
            // The reference left of the divider, the layout right of it; the divider of the image nodes' compare view
            float x = DividerX(screen, split);
            sprite->SetTexture(mRefTexture);
            sprite->transparency = 1.0f;
            PipelinePairDraw::DrawClipped(*sprite, mRefSize, screen, screen.left, x);
            PipelinePairDraw::DrawDivider(screen, x, dividerActive);
        }
        else if (setting.mode == "diff")
        {
            // The difference once it is worked out (a frame or two after the layout rests)
            if (mDiffTexture && setting.alpha > 0.0)
                Picture(mDiffTexture, mDiffSize, screen, (float)setting.alpha);
        }
    }

    // --- the card's overlay -----------------------------------------------------------------------

    UiRefOverlayView::UiRefOverlayView(RefCounter* refCounter):
        UiWsPanel(refCounter)
    {}

    bool UiRefOverlayView::Shown() const
    {
        return ctx && ctx->ref.on && !ctx->Doc().root.children.empty() && screenRect;
    }

    double UiRefOverlayView::GetSplit() const
    {
        return mLive >= 0.0 ? mLive : ctx ? ctx->ref.split : 0.5;
    }

    void UiRefOverlayView::Update(float dt)
    {
        UiWsPanel::Update(dt);
        if (ctx)
            mLayers.Update(*ctx, ctx->view, false, dt, mLive < 0.0);
    }

    bool UiRefOverlayView::IsUnderPoint(const Vec2F& point)
    {
        if (!Shown() || ctx->ref.mode != "split")
            return false;

        return UiRefLayers::OnDivider(Normalized(screenRect()), GetSplit(), point, PipelinePairDraw::PixelSize());
    }

    void UiRefOverlayView::DrawPanel(const Box& box)
    {
        if (!Shown())
            return;

        RectF screen = Normalized(screenRect());
        if (screen.Width() < 1.0f)
            return;

        if (ctx->ref.mode == "split")
        {
            float reach = dividerReach*PipelinePairDraw::PixelSize(), x = UiRefLayers::DividerX(screen, GetSplit());
            AddHot(RectF(x - reach, screen.top, x + reach, screen.bottom), "divider");
        }
        mLayers.Draw(screen, ctx->ref, GetSplit(), mLive >= 0.0 || IsHover("divider"));
    }

    void UiRefOverlayView::OnCursorPressed(const Input::Cursor& cursor)
    {
        if (Shown())
            mLive = UiRefLayers::SplitAt(Normalized(screenRect()), cursor.position.x);
    }

    void UiRefOverlayView::OnCursorStillDown(const Input::Cursor& cursor)
    {
        if (mLive >= 0.0 && Shown())
            mLive = UiRefLayers::SplitAt(Normalized(screenRect()), cursor.position.x);
    }

    void UiRefOverlayView::OnCursorReleased(const Input::Cursor& cursor)
    {
        if (mLive < 0.0)
            return;

        // Let go at another place: written (a UI-only key), rounded as the web rounds it
        double v = UiDoc::JsRound(mLive*1000)/1000;
        mLive = -1.0;
        if (ctx)
            ctx->SetUi("uiRefSplit", Json(v));
    }

    void UiRefOverlayView::OnCursorPressBreak(const Input::Cursor& cursor)
    {
        mLive = -1.0;
    }

    // --- the card's button and menu ---------------------------------------------------------------

    UiRefButton::UiRefButton(RefCounter* refCounter):
        UiWsPanel(refCounter)
    {}

    bool UiRefButton::IsUsable() const
    {
        return ctx && ctx->reference && !ctx->current.root.children.empty();
    }

    void UiRefButton::DrawPanel(const Box& box)
    {
        if (!ctx)
            return;

        auto& t = T();
        RectF r = box.world;
        bool usable = IsUsable();
        bool open = IsMenuOpen();
        bool on = ctx->ref.on;
        // .util-tool: transparent, a wash on hover, the accent while on, the hover wash while its menu is open
        if (open)
            Fill(r, 5.0f, t.active);
        else if (on)
            Fill(r, 5.0f, t.accentSoft);
        else if (usable && IsHover("toggle"))
            Fill(r, 5.0f, t.active);
        Color4 color = open ? t.fg : on ? t.accent : usable && IsHover("toggle") ? t.fg : t.fgDim;
        UiWsDraw::Icon("uiws_overlay", r.Center(), 15.0f, A(color, usable ? 1.0f : 0.35f));
        if (usable)
            AddHot(r, "toggle");
    }

    void UiRefButton::OnHotClick(const String& id, const Input::Cursor& cursor)
    {
        if (id == "toggle")
            ToggleMenu();
    }

    bool UiRefButton::IsMenuOpen() const
    {
        return mMenu && mMenu->IsOpen();
    }

    void UiRefButton::ToggleMenu()
    {
        PushEditorScopeOnStack scope;
        if (!mMenu)
        {
            mMenu = mmake<UiWsPopup>();
            mMenu->name = "compare menu";
            mMenuPanel = mmake<UiRefMenuPanel>();
            mMenuPanel->name = "compare menu panel";
            mMenu->SetContent(mMenuPanel);
            AddChild(mMenu);
        }

        if (mMenu->IsOpen() || UiWsPopup::ClosedThisFrame())
        {
            mMenu->Close();
            return;
        }
        if (!IsUsable())
            return;

        mMenuPanel->ctx = ctx;
        mMenuPanel->popup = mMenu;
        // The button sits on the editor's canvas: the menu opens in screen space, right-aligned under it
        RectF anchor = layout->GetWorldRect();
        if (auto editor = ctx->editor.Lock())
        {
            Vec2F a = editor->LocalToScreenPoint(Vec2F(anchor.left, anchor.bottom));
            Vec2F b = editor->LocalToScreenPoint(Vec2F(anchor.right, anchor.top));
            anchor = RectF(Math::Min(a.x, b.x), Math::Max(a.y, b.y), Math::Max(a.x, b.x), Math::Min(a.y, b.y));
        }
        Vec2F size = mMenuPanel->GetContentSize();
        mMenu->OpenAt(anchor, size.x, size.y, true);
    }

    UiRefMenuPanel::UiRefMenuPanel(RefCounter* refCounter):
        UiWsPanel(refCounter)
    {}

    Vec2F UiRefMenuPanel::GetContentSize() const
    {
        bool on = ctx && ctx->ref.on;
        return Vec2F(8 + 4*62 + 8, 8 + 28 + (on ? 8 + 24 : 0) + 8);
    }

    void UiRefMenuPanel::Pick(const String& mode)
    {
        if (!ctx)
            return;

        ctx->SetUi(Ws::RefModePatch(ctx->ref, mode, true));
        // The amount row comes and goes with the comparison
        if (auto menu = popup.Lock())
        {
            Vec2F size = GetContentSize();
            menu->Resize(size.x, size.y);
        }
    }

    void UiRefMenuPanel::SetAmount(double value)
    {
        if (ctx)
            ctx->SetUi(Ws::RefAmountPatch(ctx->ref, value));
    }

    void UiRefMenuPanel::DrawPanel(const Box& box)
    {
        if (!ctx)
            return;

        float W = box.W();
        const auto& s = ctx->ref;
        Vector<Pair<String, String>> modes = { { "off", "Off" } };
        for (auto& m : Ws::RefModes())
            modes.Add({ m, Ws::RefModeLabel(m) });
        Segments(box.R(8, 8, W - 16, 28), modes, s.on ? s.mode : String("off"), "cmp:");
        if (!s.on)
            return;

        RectF r = box.R(8, 8 + 28 + 8, W - 16, 24);
        double v = Ws::RefAmount(s);
        UiWsParts::SliderBar(r, (float)v, (String)Math::RoundToInt((float)v*100.0f) + "%", !mSliding.IsEmpty() || IsHover("cmp:slider"));
        AddHot(RectF(r.left, r.top, r.right - 48, r.bottom), "cmp:slider");
    }

    void UiRefMenuPanel::OnHotClick(const String& id, const Input::Cursor& cursor)
    {
        if (id.StartsWith("cmp:") && id != "cmp:slider")
            Pick(id.SubStr(4));
    }

    void UiRefMenuPanel::OnCursorPressed(const Input::Cursor& cursor)
    {
        UiWsPanel::OnCursorPressed(cursor);
        if (mPressedHot == "cmp:slider")
        {
            mSliding = mPressedHot;
            OnCursorStillDown(cursor);
        }
    }

    void UiRefMenuPanel::OnCursorStillDown(const Input::Cursor& cursor)
    {
        if (mSliding.IsEmpty())
            return;

        RectF track = HotRect(mSliding);
        SetAmount(Math::Clamp01((cursor.position.x - track.left)/Math::Max(1.0f, track.Width())));
    }

    void UiRefMenuPanel::OnCursorReleased(const Input::Cursor& cursor)
    {
        if (!mSliding.IsEmpty())
        {
            mSliding = "";
            mPressedHot = "";
            return;
        }
        UiWsPanel::OnCursorReleased(cursor);
    }
}
