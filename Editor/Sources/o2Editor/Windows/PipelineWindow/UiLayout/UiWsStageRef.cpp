#include "o2Editor/stdafx.h"
#include "UiWsStage.h"

#include "o2/Application/Application.h"
#include "o2/Utils/System/Time/Time.h"
#include "o2Editor/Windows/PipelineWindow/UiLayout/UiWsPickers.h"

// The stage's comparison with the reference picture (UiWsStage.h, port of Stage.tsx's RefLayers and the island's
// RefControls): the layers over the screen, the split's divider dragged on the screen, the switch and - while it is
// on - the modes and the amount on the island
namespace Editor
{
    using namespace UiWsDraw;
    namespace Ws = UiDoc::Ws;

    namespace
    {
        const float segmentsWidth = 156.0f;
        const float sliderWidth = 112.0f;
        const float dividerReach = 7.0f;
    }

    double UiWsStage::GetRefSplit() const
    {
        return mRefLive >= 0.0 ? mRefLive : ctx ? ctx->ref.split : 0.5;
    }

    void UiWsStage::DrawReference(const RectF& page)
    {
        if (!ctx)
            return;

        bool resting = mDrag == DragKind::None && mIdle > 0.15f;
        mRef.Update(*ctx, ctx->view, ctx->showIds, o2Time.GetDeltaTime(), resting);
        if (!ctx->ref.on)
            return;

        // The divider takes the pointer over the selection's handles and the blocks
        if (ctx->ref.mode == "split")
        {
            float x = UiRefLayers::DividerX(page, GetRefSplit());
            AddHot(RectF(x - dividerReach, page.top, x + dividerReach, page.bottom), "cmp:divider");
        }
        mRef.Draw(page, ctx->ref, GetRefSplit(), mRefLive >= 0.0 || IsHover("cmp:divider"));
    }

    float UiWsStage::RefToolsWidth() const
    {
        if (!ctx || !ctx->reference)
            return 0.0f;

        float w = 28 + 2;
        if (ctx->ref.on)
            w += 4 + segmentsWidth + 8 + sliderWidth + 4;
        return w;
    }

    void UiWsStage::DrawRefTools(float& x, float top, const RectF& island)
    {
        if (!ctx || !ctx->reference)
            return;

        auto& s = ctx->ref;
        Tool(RectF(x, top, x + 28, top - 28), "uiws_overlay", "cmp:toggle", s.on, true, 15.0f);
        x += 28 + 2;
        if (!s.on)
            return;

        x += 4;
        Vector<Pair<String, String>> modes;
        for (auto& m : Ws::RefModes())
            modes.Add({ m, Ws::RefModeLabel(m) });
        Segments(RectF(x, top, x + segmentsWidth, top - 28), modes, s.mode, "cmp:mode:");
        x += segmentsWidth + 8;
        RectF slider(x, island.Center().y + 11, x + sliderWidth, island.Center().y - 11);
        double v = s.mode == "split" ? GetRefSplit() : Ws::RefAmount(s);
        UiWsParts::SliderBar(slider, (float)v, (String)Math::RoundToInt((float)v*100.0f) + "%", !mRefSlide.IsEmpty() || IsHover("cmp:slider"));
        AddHot(RectF(slider.left, slider.top, slider.right - 48, slider.bottom), "cmp:slider");
        x += sliderWidth + 4;
    }

    bool UiWsStage::OnRefClick(const String& id)
    {
        if (!ctx || !id.StartsWith("cmp:"))
            return false;

        if (id == "cmp:toggle")
            ctx->SetUi(Ws::RefTogglePatch(ctx->ref));
        else if (id.StartsWith("cmp:mode:"))
            ctx->SetUi(Ws::RefModePatch(ctx->ref, id.SubStr(9), false));
        return true;
    }

    bool UiWsStage::OnRefPress(const Input::Cursor& cursor)
    {
        if (!ctx)
            return false;

        if (mPressedHot == "cmp:divider")
        {
            Vec2F frame = FrameSize();
            mRefLive = UiRefLayers::SplitAt(ToWorld(UiDoc::UiRect(0, 0, frame.x, frame.y)), cursor.position.x);
            o2Application.SetCursor(CursorType::SizeWE);
            return true;
        }
        if (mPressedHot == "cmp:slider")
        {
            mRefSlide = mPressedHot;
            OnRefDrag(cursor);
            return true;
        }
        return false;
    }

    bool UiWsStage::OnRefDrag(const Input::Cursor& cursor)
    {
        if (!ctx)
            return false;

        if (mRefLive >= 0.0)
        {
            Vec2F frame = FrameSize();
            mRefLive = UiRefLayers::SplitAt(ToWorld(UiDoc::UiRect(0, 0, frame.x, frame.y)), cursor.position.x);
            return true;
        }
        if (!mRefSlide.IsEmpty())
        {
            RectF track = HotRect(mRefSlide);
            double v = Math::Clamp01((cursor.position.x - track.left)/Math::Max(1.0f, track.Width()));
            ctx->SetUi(Ws::RefAmountPatch(ctx->ref, v));
            return true;
        }
        return false;
    }

    bool UiWsStage::OnRefRelease(const Input::Cursor& cursor)
    {
        if (mRefLive >= 0.0)
        {
            // Let go: written once (a UI-only key), rounded as the web rounds it
            double v = UiDoc::JsRound(mRefLive*1000)/1000;
            mRefLive = -1.0;
            mPressedHot = "";
            if (ctx)
                ctx->SetUi("uiRefSplit", UiDoc::Json(v));
            return true;
        }
        if (!mRefSlide.IsEmpty())
        {
            mRefSlide = "";
            mPressedHot = "";
            return true;
        }
        return false;
    }
}
