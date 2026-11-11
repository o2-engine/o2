#include "o2Editor/stdafx.h"
#include "UiWsSide.h"

#include "o2/Application/Input.h"
#include "o2/Assets/Types/FontAsset.h"
#include "o2/Render/Render.h"
#include "o2/Render/Text.h"
#include "o2/Scene/UI/WidgetLayer.h"
#include "o2/Scene/UI/WidgetLayout.h"
#include "o2/Scene/UI/Widgets/EditBox.h"
#include "o2/Utils/Editor/EditorScope.h"

// The Text and History tabs of the workspace's right panel (UiWsSide.h)
namespace Editor
{
    using namespace UiWsDraw;
    using UiDoc::Json;
    namespace Ws = UiDoc::Ws;

    namespace
    {
        const float lineStep = 18.4f; // 11.5 px mono at 1.6

        TextStyle Wrapped(float size, const Color4& color, float lineHeight)
        {
            TextStyle s(size, 400, color);
            s.wrap = true;
            s.ver = VerAlign::Top;
            s.lineHeight = lineHeight;
            return s;
        }
    }

    // --- the text tab --------------------------------------------------------------------------------

    UiWsTextTab::UiWsTextTab(RefCounter* refCounter):
        UiWsPanel(refCounter)
    {
        PushEditorScopeOnStack scope;
        auto& t = T();
        mField = UiWsControls::TextField("", "", true, {});
        mField->name = "uiml";
        mField->SetWordWrap(false);
        for (auto layer : { "back", "border", "focus" })
        {
            if (mField->GetLayer(layer))
                mField->RemoveLayer(String(layer));
        }
        mField->SetViewLayout(Layout::BothStretch(10, 9, 10, 9));
        mField->SetClippingLayout(Layout::BothStretch(0, 0, 0, 0));
        if (auto text = mField->GetTextDrawable())
        {
            text->SetFontAsset(AssetRef<FontAsset>(PipelineTheme::MonoFont()));
            text->height = PipelineTheme::Size(11.5f);
            text->color = t.fg;
            if (auto font = text->GetFont())
            {
                float natural = font->GetLineHeightPx(text->height);
                text->linesDistanceCoef = natural > 0.0f ? lineStep/natural : 1.0f;
            }
        }
        WeakRef<UiWsTextTab> weak(this);
        mField->onChanged = [weak](const WString&)
        {
            if (auto self = weak.Lock())
                self->mDraft = true;
        };
        AddChild(mField);
    }

    bool UiWsTextTab::Apply()
    {
        if (!ctx || !mDraft || !ctx->Editable())
            return false;

        auto res = UiDoc::ParseUiml((String)mField->GetText(), &ctx->Doc());
        mErrors = res.errors;
        if (!mErrors.IsEmpty())
            return false;

        mDraft = false;
        mShownFor = "";
        ctx->Op({ UiDoc::MakeDocOp(res.doc) }, "text edited");
        return true;
    }

    void UiWsTextTab::Reset()
    {
        mDraft = false;
        mErrors.Clear();
        mShownFor = "";
    }

    void UiWsTextTab::Update(float dt)
    {
        UiWsPanel::Update(dt);
        if (!ctx)
            return;

        static String lastText;
        bool ctrl = o2Input.IsKeyDown(VK_CTRL_CMD) || o2Input.IsKeyDown(VK_CONTROL);
        if (mField->IsFocused() && ctrl && (o2Input.IsKeyPressed(VK_RETURN) || o2Input.IsKeyPressed(VK_RETURN_NUMPAD)))
        {
            // The Enter put a line break into the text first: the text before it is applied
            UiWsControls::SetTextQuiet(mField, lastText);
            Apply();
        }

        if (!mDraft)
        {
            String uiml = UiDoc::DocToUiml(ctx->Doc());
            if (uiml != mShownFor)
            {
                mShownFor = uiml;
                UiWsControls::SetTextQuiet(mField, uiml);
            }
        }
        mField->interactable = ctx->Editable();
        lastText = (String)mField->GetText();
    }

    void UiWsTextTab::DrawPanel(const Box& box)
    {
        if (!ctx)
            return;

        auto& t = T();
        float W = box.W(), H = box.H();
        float errorsH = mErrors.Count()*(15 + 3);
        float footTop = H - 12 - 28;
        float editorBottom = footTop - 6 - errorsH;
        RectF editor = box.R(12, 12, W - 24, editorBottom - 12);
        Fill(editor, 8.0f, t.dark ? Color4(0, 0, 0, 56) : t.cardSolid);
        Border(editor, 8.0f, mField->IsFocused() ? t.accent : t.line);

        // The gutter: one number a line, scrolled with the text
        RectF gutter(editor.left + 1, editor.top - 1, editor.left + 38, editor.bottom + 1);
        Fill(gutter, 0.0f, t.dark ? Color4(0, 0, 0, 31) : A(t.fg, 0.03f));
        Line(Vec2F(gutter.right + 0.5f, gutter.top), Vec2F(gutter.right + 0.5f, gutter.bottom), t.line);
        String text = (String)mField->GetText();
        int lines = text.CountOf("\n") + 1;
        float scroll = mField->GetScroll().y;
        o2Render.EnableScissorTest((RectI)gutter);
        for (int i = 0; i < lines; i++)
        {
            float y = gutter.top - 9 - i*lineStep + scroll;
            if (y < gutter.bottom)
                break;
            if (y - lineStep > gutter.top)
                continue;
            Text((String)(i + 1), RectF(gutter.left, y, gutter.right - 8, y - lineStep), TextStyle(11.5f, 400, t.fg3, HorAlign::Right, true));
        }
        o2Render.DisableScissorTest();

        RectF me = layout->GetWorldRect();
        *mField->layout = WidgetLayout(Vec2F(0, 0), Vec2F(1, 1), Vec2F(12 + 39, H - editorBottom + 1), Vec2F(-13, -13));
        (void)me;

        float y = editorBottom + 6;
        for (auto& er : mErrors)
        {
            UiWsDraw::Icon("uiws_warn", box.P(12 + 6, y + 7.5f), 12.0f, t.red);
            Text(er, box.R(12 + 17, y, W - 24 - 17, 15), TextStyle(10.5f, 400, t.red, HorAlign::Left, true));
            y += 18;
        }

        // The foot: what the text is, Reset and Apply
        float applyW = TextWidth("Apply", 12.0f, 600) + 20, resetW = TextWidth("Reset", 12.0f) + 20;
        String note = mDraft ? String("Edited \xE2\x80\x94 apply to check and record it as a step") :
            String("UIML: one block per line, children indented. Ctrl+Enter applies.");
        Text(note, box.R(12, footTop, W - 24 - applyW - resetW - 12, 28), TextStyle(11.0f, 400, t.fg3));
        Button(box.R(W - 12 - applyW - 6 - resetW, footTop, resetW, 28), "Reset", "reset", false, mDraft);
        Button(box.R(W - 12 - applyW, footTop, applyW, 28), "Apply", "apply", true, mDraft && ctx->Editable());
    }

    void UiWsTextTab::OnHotClick(const String& id, const Input::Cursor& cursor)
    {
        if (id == "apply")
            Apply();
        else if (id == "reset")
            Reset();
    }

    // --- the history tab -----------------------------------------------------------------------------

    UiWsHistoryTab::UiWsHistoryTab(RefCounter* refCounter):
        UiWsPanel(refCounter)
    {}

    void UiWsHistoryTab::ToggleActions(int step)
    {
        if (mOpen.Contains(step))
            mOpen.Remove(step);
        else
            mOpen.Add(step);
    }

    void UiWsHistoryTab::OnScrolled(float scroll)
    {
        mScroll = Math::Clamp(mScroll - scroll*0.5f, 0.0f, Math::Max(0.0f, mContentHeight - layout->GetHeight()));
    }

    void UiWsHistoryTab::DrawPanel(const Box& box)
    {
        if (!ctx)
            return;

        auto& t = T();
        float W = box.W(), H = box.H();
        const auto& steps = ctx->timeline.GetSteps();
        bool editable = ctx->Editable() && !ctx->Busy();
        o2Render.EnableScissorTest((RectI)box.world);
        float y = 12 - mScroll;

        // Undo / Redo by action
        float undoW = 12 + 12 + 5 + TextWidth("Undo", 12.0f) + 10;
        String redoText = "Redo" + (ctx->redo.IsEmpty() ? String() : " \xC2\xB7 " + (String)ctx->redo.Count());
        float redoW = 12 + 12 + 5 + TextWidth(redoText, 12.0f) + 10;
        Button(box.R(12, y, undoW, 28), "Undo", "undo", false, editable && !steps.IsEmpty(), "uiws_undo");
        Button(box.R(12 + undoW + 6, y, redoW, 28), redoText, "redo", false, editable && !ctx->redo.IsEmpty(), "uiws_redo");
        y += 28 + 12;

        float listTop = y;
        float cardX = 12 + 19 + 10, cardW = W - 12 - cardX;
        TextStyle title = Wrapped(12.0f, t.fg, 17.4f);
        TextStyle muted = Wrapped(11.5f, t.fg3, 16.7f);

        // One item: the dot, the card; returns its height
        auto item = [&](int k, bool draw) -> float
        {
            bool first = k == 0;
            const UiDoc::UiStep* s = first ? nullptr : &steps[k - 1];
            bool last = !first && k == steps.Count();
            float h = 7;
            String titleText = first ? String("empty screen") : Ws::StepTitle(*s);
            float th = Math::Min(17.4f*2, TextHeight(titleText, cardW - 20, first ? muted : title));
            h += th;
            String reply = s && s->Reply() ? *s->Reply() : String();
            float rh = reply.IsEmpty() ? 0.0f : Math::Min(16.7f*2, TextHeight(reply, cardW - 20, muted));
            if (rh > 0)
                h += 2 + rh;
            auto parts = s && !s->IsAgent() ? UiDoc::StepParts(*s) : Vector<UiDoc::UiStepPart>();
            bool actions = parts.Count() > 1;
            bool open = actions && mOpen.Contains(k);
            if (actions)
                h += 4 + 15 + (open ? 4 + Math::Min(180.0f, parts.Count()*16.7f) : 0.0f);
            if (s)
                h += 5 + 16;
            h += 7;

            if (!draw)
                return h;

            RectF card = box.R(cardX, y, cardW, h);
            String id = "step:" + (String)k;
            bool peek = ctx->peek == k;
            bool hover = IsHover(id) || IsHover("peek:" + (String)k) || IsHover("revert:" + (String)k) || IsHover("acts:" + (String)k);
            if (peek)
            {
                Fill(card, 9.0f, t.accentSoft);
                Border(card, 9.0f, t.accent);
            }
            else if (hover)
                Fill(card, 9.0f, t.field);
            AddHot(card, id);

            // The dot: the agent's sparkle, a hand edit's pencil; filled at now
            Vec2F dot = box.P(12 + 9.5f, y + 7 + 9.5f);
            bool agent = s && s->IsAgent();
            Color4 ring = last ? t.accent : agent ? A(t.accent, 0.45f) : t.border;
            Circle(dot, 9.5f, last ? t.accent : ring);
            Circle(dot, 8.5f, last ? t.accent : t.cardSolid);
            if (s)
                UiWsDraw::Icon(agent ? "uiws_sparkle" : "uiws_pencil", dot, agent ? 9.0f : 8.0f, last ? t.accentInk : agent ? t.accent : t.fgDim);

            float cy = y + 7;
            o2Render.EnableScissorTest((RectI)RectF(card.left, card.top - 6, card.right, card.top - 7 - th));
            Text(titleText, box.R(cardX + 10, cy, cardW - 20, 17.4f*2 + 4), first ? muted : title);
            o2Render.DisableScissorTest();
            cy += th;
            if (rh > 0)
            {
                cy += 2;
                o2Render.EnableScissorTest((RectI)box.R(cardX, cy, cardW, rh));
                Text(reply, box.R(cardX + 10, cy, cardW - 20, 16.7f*2 + 4), muted);
                o2Render.DisableScissorTest();
                cy += rh;
            }
            if (actions)
            {
                cy += 4;
                String summary = (String)parts.Count() + " actions";
                Color4 summaryColor = IsHover("acts:" + (String)k) ? t.fgDim : t.fg3;
                UiWsDraw::Icon(open ? "uiws_chevron_down" : "uiws_chevron_right", box.P(cardX + 10 + 4.5f, cy + 7.5f), 9.0f, summaryColor);
                Text(summary, box.R(cardX + 22, cy, cardW - 32, 15), TextStyle(10.5f, 400, summaryColor, HorAlign::Left, true));
                AddHot(box.R(cardX + 10, cy, TextWidth(summary, 10.5f, 400, true) + 16, 15), "acts:" + (String)k);
                cy += 15;
                if (open)
                {
                    cy += 4;
                    float listH = Math::Min(180.0f, parts.Count()*16.7f);
                    for (int i = 0; i < parts.Count() && i*16.7f < listH; i++)
                    {
                        float time = TextWidth(Ws::LocalTime(parts[i].at), 10.0f, 400, true);
                        Text(parts[i].label, box.R(cardX + 10, cy + i*16.7f, cardW - 28 - time, 16.7f), TextStyle(11.5f, 400, t.fgDim));
                        Text(Ws::LocalTime(parts[i].at), box.R(cardX + cardW - 10 - time, cy + i*16.7f, time, 16.7f),
                             TextStyle(10.0f, 400, t.fg3, HorAlign::Right, true));
                    }
                    cy += listH;
                }
            }
            if (s)
            {
                cy += 5;
                float cw = DrawDiffChips(ctx->DiffOf(k - 1), box.P(cardX + 10, cy + 8));
                String meta = Ws::LocalTime(s->At()) + (s->Model() && !s->Model()->IsEmpty() ? " \xC2\xB7 " + *s->Model() : String());
                float mx = cardX + 10 + cw + (cw > 0 ? 8 : 0);
                Text(meta, box.R(mx, cy, cardW - 10 - mx + cardX, 16), TextStyle(10.0f, 400, t.fg3, HorAlign::Left, true));
            }

            // Peek and revert on hover, for every step but now
            if (s && !last && hover)
            {
                for (int b = 0; b < 2; b++)
                {
                    String bid = (b == 0 ? "peek:" : "revert:") + (String)k;
                    RectF r(card.right - 4 - 24*(2 - b) - 2*(1 - b), card.top - 4, card.right - 4 - 24*(1 - b) - 2*(1 - b), card.top - 28);
                    Fill(r, 5.0f, t.cardSolid);
                    if (IsHover(bid))
                        Fill(r, 5.0f, t.active);
                    UiWsDraw::Icon(b == 0 ? "uiws_peek" : "uiws_revert", r.Center(), 13.0f, IsHover(bid) ? t.fg : t.fgDim);
                    AddHot(r, bid);
                }
            }
            return h;
        };

        // The line behind the dots, then the items
        float total = 0;
        for (int k = 0; k <= steps.Count(); k++)
            total += item(k, false) + 10;
        if (total > 30)
            Line(box.P(12 + 9.5f, listTop + 10), box.P(12 + 9.5f, listTop + total - 10 - 10), t.line);
        for (int k = 0; k <= steps.Count(); k++)
        {
            float h = item(k, true);
            y += h + 10;
        }
        mContentHeight = y + mScroll + 16;
        o2Render.DisableScissorTest();
    }

    void UiWsHistoryTab::OnHotClick(const String& id, const Input::Cursor& cursor)
    {
        if (!ctx)
            return;

        int steps = ctx->timeline.GetSteps().Count();
        auto k = [&](int prefix) { return atoi(id.SubStr(prefix).Data()); };
        if (id == "undo")
            ctx->Undo();
        else if (id == "redo")
            ctx->Redo();
        else if (id.StartsWith("step:"))
        {
            int step = k(5);
            bool last = step == steps && step > 0;
            ctx->SetPeek(ctx->peek == step || last ? -1 : step);
        }
        else if (id.StartsWith("peek:"))
            ctx->SetPeek(k(5));
        else if (id.StartsWith("revert:"))
            ctx->Revert(k(7));
        else if (id.StartsWith("acts:"))
            ToggleActions(k(5));
    }
}
