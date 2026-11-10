#include "o2Editor/stdafx.h"
#include "UiWsSide.h"

#include "o2/Application/Input.h"
#include "o2/Render/Render.h"
#include "o2/Scene/UI/UIManager.h"
#include "o2/Scene/UI/WidgetLayout.h"
#include "o2/Scene/UI/Widgets/Button.h"
#include "o2/Scene/UI/Widgets/EditBox.h"
#include "o2/Utils/Editor/EditorScope.h"
#include "o2/Utils/System/Time/Time.h"
#include "o2Editor/Pipeline/PipelineSettings.h"
#include "o2Editor/Pipeline/Providers/LocalAgentProvider.h"
#include "o2Editor/Windows/PipelineWindow/PipelineEditor.h"
#include "o2Editor/Windows/PipelineWindow/PipelineDraw.h"
#include "o2Editor/Windows/PipelineWindow/PipelineModelPicker.h"
#include "o2Editor/Windows/PipelineWindow/PipelineNodeBodyFactories.h"
#include "o2Editor/Windows/PipelineWindow/UiLayout/UiWsSide.h"

namespace Editor
{
    using namespace UiWsDraw;
    using UiDoc::Json;
    namespace Ws = UiDoc::Ws;

    namespace
    {
        const float tabsHeight = 40.0f;
        const char* const tabIds[] = { "agent", "inspect", "text", "history" };
        const char* const tabLabels[] = { "Agent", "Inspector", "Text", "History" };
        const char* const tabIcons[] = { "uiws_chat", "uiws_inspect", "uiws_code", "uiws_history" };

        TextStyle Wrapped(float size, const Color4& color, float lineHeight)
        {
            TextStyle s(size, 400, color);
            s.wrap = true;
            s.ver = VerAlign::Top;
            s.lineHeight = lineHeight;
            return s;
        }
    }

    float DrawDiffChips(const UiDoc::DocDiff& d, const Vec2F& leftCenter, bool draw /*= true*/)
    {
        auto& t = T();
        float x = leftCenter.x;
        auto chip = [&](const String& text, const Color4& color, const String& icon)
        {
            float w = icon.IsEmpty() ? TextWidth(text, 10.0f, 400, true) + 10.0f : 16.0f;
            if (draw)
            {
                RectF r(x, leftCenter.y + 8, x + w, leftCenter.y - 8);
                Fill(r, 4.0f, A(color, 0.12f));
                if (icon.IsEmpty())
                    Text(text, r, TextStyle(10.0f, 400, color, HorAlign::Middle, true));
                else
                    UiWsDraw::Icon(icon, r.Center(), 10.0f, color);
            }
            x += w + 4;
        };
        if (!d.added.IsEmpty()) chip("+" + (String)d.added.Count(), t.green, "");
        if (!d.changed.IsEmpty()) chip("~" + (String)d.changed.Count(), t.yellow, "");
        if (!d.moved.IsEmpty()) chip("\xE2\x86\x95" + (String)d.moved.Count(), t.portText, "");
        if (!d.removed.IsEmpty()) chip("\xE2\x88\x92" + (String)d.removed.Count(), t.red, "");
        if (d.screen) chip("", t.yellow, "uiws_type_screen");
        return x > leftCenter.x ? x - leftCenter.x - 4 : 0.0f;
    }

    // --- the side panel --------------------------------------------------------------------------

    UiWsSidePanel::UiWsSidePanel(RefCounter* refCounter):
        UiWsPanel(refCounter)
    {
        PushEditorScopeOnStack scope;
        mAgent = mmake<UiWsAgentTab>();
        mAgent->name = "agent tab";
        mInspector = mmake<UiWsInspector>();
        mInspector->name = "inspector tab";
        mText = mmake<UiWsTextTab>();
        mText->name = "text tab";
        mHistory = mmake<UiWsHistoryTab>();
        mHistory->name = "history tab";
        for (Ref<Widget> tab : { Ref<Widget>(mAgent), Ref<Widget>(mInspector), Ref<Widget>(mText), Ref<Widget>(mHistory) })
        {
            *tab->layout = WidgetLayout::BothStretch(1, 0, 0, tabsHeight);
            AddChild(tab);
        }
    }

    void UiWsSidePanel::Bind(const Ref<UiWsContext>& context)
    {
        ctx = context;
        mAgent->ctx = context;
        mInspector->ctx = context;
        mText->ctx = context;
        mHistory->ctx = context;
    }

    void UiWsSidePanel::OpenTab(const String& tab)
    {
        if (ctx && ctx->tab != tab)
        {
            if (ctx->tab == "agent")
                mAgent->SaveDraft();
            ctx->SetUi("uiTab", Json(tab));
        }
    }

    void UiWsSidePanel::Update(float dt)
    {
        String tab = ctx ? ctx->tab : String("agent");
        auto show = [&](const Ref<Widget>& w, bool on)
        {
            if (w->IsEnabled() != on)
                w->SetEnabledForcible(on);
        };
        show(mAgent, tab == "agent");
        show(mInspector, tab == "inspect");
        show(mText, tab == "text");
        show(mHistory, tab == "history");
        UiWsPanel::Update(dt);
    }

    void UiWsSidePanel::DrawPanel(const Box& box)
    {
        if (!ctx)
            return;

        auto& t = T();
        float W = box.W();
        Line(box.P(0.5f, 0), box.P(0.5f, box.H()), t.line);
        Line(box.P(0, tabsHeight - 0.5f), box.P(W, tabsHeight - 0.5f), t.line);

        // Natural widths; a narrow panel names only the open tab
        bool narrow = W <= 364.0f;
        int steps = ctx->timeline.GetSteps().Count();
        float natural[4], total = 0.0f;
        for (int i = 0; i < 4; i++)
        {
            bool on = ctx->tab == tabIds[i];
            float count = i == 3 && steps > 0 ? TextWidth((String)steps, 10.0f, 400, true) + 10 + 6 : 0.0f;
            natural[i] = narrow && !on ? 9 + 14 + count + 9 : 6 + 14 + 6 + TextWidth(tabLabels[i], 12.0f) + count + 6;
            total += natural[i];
        }
        float inner = W - 12 - 6, extra = Math::Max(0.0f, inner - total);
        float x = 6;
        for (int i = 0; i < 4; i++)
        {
            bool on = ctx->tab == tabIds[i];
            bool grows = !narrow || on;
            float w = natural[i] + (grows ? extra/(narrow ? 1.0f : 4.0f) : 0.0f);
            RectF r = box.R(x, 6, w, 28);
            String id = String("tab:") + tabIds[i];
            if (on)
                Fill(r, 7.0f, t.active);
            else if (IsHover(id))
                Fill(r, 7.0f, t.field);
            float content = natural[i] - 12, cx = r.left + (r.Width() - content)*0.5f;
            if (narrow && !on)
                cx = r.left + (r.Width() - (natural[i] - 18))*0.5f;
            UiWsDraw::Icon(tabIcons[i], Vec2F(cx + 7, r.Center().y), 14.0f, on ? t.accent : IsHover(id) ? t.fg : t.fgDim);
            cx += 14;
            if (!narrow || on)
            {
                cx += 6;
                float lw = TextWidth(tabLabels[i], 12.0f);
                Text(tabLabels[i], RectF(cx, r.top, cx + lw + 2, r.bottom), TextStyle(12.0f, 400, on || IsHover(id) ? t.fg : t.fgDim));
                cx += lw;
            }
            if (i == 3 && steps > 0)
            {
                String n = (String)steps;
                float cw = TextWidth(n, 10.0f, 400, true) + 10;
                RectF chip(cx + 6, r.Center().y + 8, cx + 6 + cw, r.Center().y - 8);
                Fill(chip, 8.0f, t.field);
                Text(n, chip, TextStyle(10.0f, 400, t.fg3, HorAlign::Middle, true));
            }
            AddHot(r, id);
            x += w + 2;
        }
    }

    void UiWsSidePanel::OnHotClick(const String& id, const Input::Cursor& cursor)
    {
        if (id.StartsWith("tab:"))
            OpenTab(id.SubStr(4));
    }

    // --- the agent tab --------------------------------------------------------------------------------

    UiWsAgentTab::UiWsAgentTab(RefCounter* refCounter):
        UiWsPanel(refCounter)
    {
        PushEditorScopeOnStack scope;
        WeakRef<UiWsAgentTab> weak(this);
        mPrompt = UiWsControls::TextField("", "What to build or change, e.g. \"main menu: title, Play / Shop / Settings, coins top-right\" (Ctrl+Enter)",
                                         true, {}, [weak](const String&) { if (auto self = weak.Lock()) self->SaveDraft(); });
        mPrompt->name = "prompt";
        if (auto text = mPrompt->GetTextDrawable())
            text->height = PipelineTheme::Size(12.5f);
        AddChild(mPrompt);

        mModel = PipelineControls::MakeModelField();
        mModel->name = "agent model";
        mModel->onClick = [weak]() { if (auto self = weak.Lock()) self->OpenModelPicker(); };
        AddChild(mModel);
    }

    void UiWsAgentTab::SaveDraft()
    {
        if (!ctx)
            return;

        String text = (String)mPrompt->GetText();
        if (text != ctx->draft)
            ctx->SetUi("uiDraft", Json(text));
    }

    bool UiWsAgentTab::Send()
    {
        if (!ctx)
            return false;

        String prompt = (String)mPrompt->GetText();
        if (prompt.Trimed(" \t\r\n").IsEmpty() || ctx->Busy() || !onSend)
            return false;

        SaveDraft();
        if (ctx->peek >= 0)
            ctx->SetPeek(-1);
        onSend(prompt, ctx->selected);
        return true;
    }

    void UiWsAgentTab::ToggleEdits(int step)
    {
        if (mOpen.Contains(step))
            mOpen.Remove(step);
        else
            mOpen.Add(step);
    }

    void UiWsAgentTab::OnScrolled(float scroll)
    {
        mScroll = Math::Max(0.0f, mScroll - scroll*0.5f);
        mStickToEnd = false;
    }

    void UiWsAgentTab::Update(float dt)
    {
        UiWsPanel::Update(dt);
        if (!ctx)
            return;

        // The field shows the stored draft unless it is being typed into
        if (!mPrompt->IsFocused() && (String)mPrompt->GetText() != ctx->draft)
            UiWsControls::SetTextQuiet(mPrompt, ctx->draft);

        bool ctrl = o2Input.IsKeyDown(VK_CTRL_CMD) || o2Input.IsKeyDown(VK_CONTROL);
        if (mPrompt->IsFocused() && ctrl && (o2Input.IsKeyPressed(VK_RETURN) || o2Input.IsKeyPressed(VK_RETURN_NUMPAD)))
        {
            // The Enter typed a line break into the field first
            UiWsControls::SetTextQuiet(mPrompt, mLastPrompt);
            Send();
        }

        mLastPrompt = (String)mPrompt->GetText();
        PipelineControls::SetModelFieldValue(mModel, Ws::AgentModelOf(ctx->config), PipelineModelKind::Text);
        int steps = ctx->timeline.GetSteps().Count();
        if (steps != mSeenSteps || ctx->Busy() != mSeenBusy)
        {
            mSeenSteps = steps;
            mSeenBusy = ctx->Busy();
            mStickToEnd = true;
        }
    }

    void UiWsAgentTab::OpenModelPicker()
    {
        auto editor = ctx ? ctx->editor.Lock() : nullptr;
        if (!editor)
            return;

        if (auto picker = editor->GetModelPicker())
        {
            if (picker->IsOpenFor(mModel))
            {
                picker->Close();
                return;
            }
            if (picker->ConsumeClosedByField(mModel))
                return;
        }

        PipelineModelPickerRequest request;
        request.ids = PipelineTextModelPresets();
        request.ids.Add(LocalAgentProvider::OfferedModels(PipelineSettings::Load()));
        request.kind = PipelineModelKind::Text;
        request.current = Ws::AgentModelOf(ctx->config);
        request.anchor = mModel->layout->GetWorldRect();
        request.field = mModel;
        WeakRef<UiWsAgentTab> weak(this);
        request.onPick = [weak](const String& id)
        {
            if (auto self = weak.Lock())
                self->ctx->SetUi("agentModel", Json(id));
        };
        editor->ShowModelPicker(request);
    }

    float UiWsAgentTab::LayMessages(const Box& box, float top, float bottom, bool draw)
    {
        auto& t = T();
        float W = box.W();
        float y = top + 12 - (draw ? mScroll : 0.0f);
        const auto& steps = ctx->timeline.GetSteps();
        TextStyle body = Wrapped(12.5f, t.fg, 18.75f);
        auto visible = [&](float y0, float h) { return draw && y0 + h > top && y0 < bottom; };

        if (steps.IsEmpty() && !ctx->Busy())
        {
            String hello = "Describe the screen: what it is for, its blocks, where they sit. The agent builds it as an adaptive layout from "
                "the kit you connected; ask for changes in the same way.";
            TextStyle style = Wrapped(12.0f, t.fg3, 18.6f);
            style.hor = HorAlign::Middle;
            float w = Math::Min(300.0f, W - 40);
            float h = TextHeight(hello, w, style);
            float cy = (top + bottom)*0.5f - (h + 28)*0.5f;
            if (draw)
            {
                UiWsDraw::Icon("uiws_sparkle", box.P(W*0.5f, cy + 9), 18.0f, t.accent);
                Text(hello, box.R((W - w)*0.5f, cy + 28, w, h + 2), style);
            }
            return 0.0f;
        }

        for (int i = 0; i < steps.Count(); i++)
        {
            auto& s = steps[i];
            const auto& diff = ctx->DiffOf(i);
            if (!s.IsAgent())
            {
                // A hand edit: a quiet line in the middle
                String title = Ws::StepTitle(s);
                float chipsW = DrawDiffChips(diff, Vec2F(), false);
                float titleW = Math::Min(TextWidth(title, 11.0f) + 2, W - 24 - 17 - chipsW - 6);
                float w = 11 + 6 + titleW + (chipsW > 0 ? 6 + chipsW : 0);
                if (visible(y, 18))
                {
                    float x = box.world.left + (W - w)*0.5f, cy = box.world.top - y - 9;
                    UiWsDraw::Icon("uiws_pencil", Vec2F(x + 5.5f, cy), 11.0f, t.fg3);
                    Text(title, RectF(x + 17, cy + 9, x + 17 + titleW, cy - 9), TextStyle(11.0f, 400, t.fg3));
                    DrawDiffChips(diff, Vec2F(x + 17 + titleW + 6, cy));
                }
                y += 18 + 10;
                continue;
            }

            // The request: a bubble on the right
            String prompt = s.Prompt() ? *s.Prompt() : String();
            float maxW = (W - 24)*0.86f - 22;
            float promptW = Math::Min(maxW, TextWidth(prompt, 12.5f) + 2);
            float promptH = TextHeight(prompt, promptW, body);
            if (visible(y, promptH + 14))
            {
                RectF bubble = box.R(W - 12 - promptW - 22, y, promptW + 22, promptH + 14);
                PipelineDraw::Corners corners(12.0f);
                corners.rb = 4.0f;
                PipelineDraw::Fill(bubble, corners, t.accentSoft);
                Text(prompt, RectF(bubble.left + 11, bubble.top - 7, bubble.right - 11, bubble.bottom + 5), body);
            }
            y += promptH + 14 + 10;

            // The answer: the avatar, the reply, the check pass, the edits
            float bx = 12 + 22 + 9, bw = W - 12 - bx;
            float y0 = y;
            if (visible(y, 22))
            {
                Fill(box.R(12, y + 1, 22, 22), 7.0f, t.accentSoft);
                UiWsDraw::Icon("uiws_sparkle", box.P(12 + 11, y + 12), 12.0f, t.accent);
            }
            String reply = s.Reply() ? *s.Reply() : String();
            float replyH = reply.IsEmpty() ? 0.0f : TextHeight(reply, bw, body);
            if (!reply.IsEmpty() && visible(y + 2, replyH))
                Text(reply, box.R(bx, y + 2, bw, replyH + 2), body);
            y += 2 + replyH;

            String reviewReply;
            double reviewEdits = 0;
            if (s.Review(reviewReply, reviewEdits))
            {
                String text = reviewReply.IsEmpty() ? String("Checked against the kit \xE2\x80\x94 nothing to fix") : reviewReply;
                String fixes = reviewEdits > 0 ? "fixes: " + UiDoc::JsNum(reviewEdits) : String();
                float fixesW = fixes.IsEmpty() ? 0.0f : TextWidth(fixes, 10.5f, 400, true) + 8;
                TextStyle small = Wrapped(11.5f, t.fgDim, 16.7f);
                float th = Math::Max(16.7f, TextHeight(text, bw - 16 - 19 - fixesW, small));
                y += 6;
                if (visible(y, th + 12))
                {
                    RectF r = box.R(bx, y, bw, th + 12);
                    Fill(r, 7.0f, t.field);
                    UiWsDraw::Icon("uiws_review", Vec2F(r.left + 8 + 6.5f, r.top - 6 - 8), 13.0f, t.green);
                    Text(text, RectF(r.left + 27, r.top - 6, r.right - 8 - fixesW, r.bottom + 4), small);
                    if (!fixes.IsEmpty())
                        Text(fixes, RectF(r.right - 8 - fixesW, r.top - 6, r.right - 8, r.top - 22), TextStyle(10.5f, 400, t.fg3, HorAlign::Right, true));
                }
                y += th + 12;
            }

            // The summary of the edits: the chips, their count, the failed ones; folded
            y += 6;
            auto& errs = ctx->timeline.ErrorsOf(i + 1);
            if (visible(y, 16))
            {
                float x = box.world.left + bx, cy = box.world.top - y - 8;
                float cw = DrawDiffChips(diff, Vec2F(x, cy));
                x += cw + (cw > 0 ? 8 : 0);
                String edits = "edits: " + (String)s.Ops().Count();
                float ew = TextWidth(edits, 10.5f, 400, true);
                Text(edits, RectF(x, cy + 8, x + ew + 2, cy - 8), TextStyle(10.5f, 400, IsHover("edits:" + (String)i) ? t.fgDim : t.fg3, HorAlign::Left, true));
                x += ew + 8;
                if (!errs.IsEmpty())
                {
                    UiWsDraw::Icon("uiws_warn", Vec2F(x + 5.5f, cy), 11.0f, t.yellow);
                    Text((String)errs.Count(), RectF(x + 14, cy + 8, x + 40, cy - 8), TextStyle(10.5f, 400, t.yellow, HorAlign::Left, true));
                }
                AddHot(box.R(bx, y, bw, 16), "edits:" + (String)i);
            }
            y += 16;
            if (mOpen.Contains(i))
            {
                const String& ops = ctx->OpsTextOf(i);
                TextStyle mono(10.5f, 400, t.fgDim, HorAlign::Left, true);
                mono.ver = VerAlign::Top;
                int lines = ops.CountOf("\n") + 1;
                float ph = Math::Min(240.0f, lines*15.75f + 16);
                y += 6;
                if (visible(y, ph))
                {
                    RectF r = box.R(bx, y, bw, ph);
                    Fill(r, 7.0f, t.dark ? Color4(0, 0, 0, 56) : A(t.fg, 0.04f));
                    o2Render.EnableScissorTest((RectI)r);
                    Text(ops, RectF(r.left + 10, r.top - 8, r.right + 600, r.bottom), mono);
                    o2Render.DisableScissorTest();
                }
                y += ph;
                for (auto& er : errs)
                {
                    y += 3;
                    if (visible(y, 15))
                        Text(er, box.R(bx, y, bw, 15), TextStyle(10.5f, 400, t.red, HorAlign::Left, true));
                    y += 15;
                }
            }
            y = Math::Max(y, y0 + 22) + 10;
        }

        if (ctx->Busy())
        {
            if (visible(y, 22))
            {
                float pulse = 0.55f + 0.45f*std::sin(o2Time.GetApplicationTime()*5.2f);
                Fill(box.R(12, y + 1, 22, 22), 7.0f, A(t.accentSoft, pulse));
                UiWsDraw::Icon("uiws_sparkle", box.P(12 + 11, y + 12), 12.0f, A(t.accent, pulse));
                Text("The agent is working\xE2\x80\xA6", box.R(12 + 31, y + 2, W - 55, 18), TextStyle(12.5f, 400, t.fgDim));
            }
            y += 22 + 10;
        }

        String error = UiAgentState::GetError(ctx->nodeId);
        if (!error.IsEmpty() && !ctx->Busy())
        {
            TextStyle style = Wrapped(12.5f, t.red, 18.75f);
            float h = TextHeight(error, W - 24 - 20 - 19, style);
            if (visible(y, h + 14))
            {
                RectF r = box.R(12, y, W - 24, h + 14);
                Fill(r, 8.0f, A(t.red, 0.08f));
                UiWsDraw::Icon("uiws_warn", Vec2F(r.left + 10 + 6.5f, r.top - 7 - 9), 13.0f, t.red);
                Text(error, RectF(r.left + 29, r.top - 7, r.right - 10, r.bottom + 5), style);
            }
            y += h + 14 + 10;
        }
        return y + (draw ? mScroll : 0.0f) - top;
    }

    void UiWsAgentTab::DrawPanel(const Box& box)
    {
        if (!ctx)
            return;

        auto& t = T();
        float W = box.W(), H = box.H();
        float y = 0;

        // The kit's state above the chat
        if (!ctx->hasKit || ctx->undescribed > 0)
        {
            String note = !ctx->hasKit ? String("No UI kit connected \xE2\x80\x94 the agent will lay out line art only.")
                : (String)ctx->undescribed + " kit graphics have no name or description \xE2\x80\x94 the agent can only guess what they are. "
                  "Run \xE2\x80\x9C" "AI: describe all\xE2\x80\x9D in the UI kit first.";
            TextStyle style = Wrapped(11.5f, t.fgDim, 16.7f);
            float h = TextHeight(note, W - 24 - 20 - 21, style);
            RectF r = box.R(12, 12, W - 24, h + 16);
            Fill(r, 8.0f, Color4(241, 190, 91, 20));
            Border(r, 8.0f, Color4(241, 190, 91, 56));
            UiWsDraw::Icon("uiws_warn", Vec2F(r.left + 10 + 6.5f, r.top - 8 - 8.5f), 13.0f, t.yellow);
            Text(note, RectF(r.left + 31, r.top - 8, r.right - 10, r.bottom + 6), style);
            y = 12 + h + 16;
        }

        // The compose area at the bottom
        bool context = !ctx->selected.IsEmpty() && UiDoc::FindNode(ctx->Doc(), ctx->selected);
        float composeH = 12 + (context ? 22 + 6 : 0) + 70 + 6 + 28 + 12;
        float composeTop = H - composeH;
        Line(box.P(0, composeTop + 0.5f), box.P(W, composeTop + 0.5f), t.line);

        // The messages, scrolled
        float top = y, bottom = composeTop;
        mContentHeight = LayMessages(box, top, bottom, false);
        float maxScroll = Math::Max(0.0f, mContentHeight - (bottom - top));
        if (mStickToEnd)
            mScroll = maxScroll;
        mScroll = Math::Clamp(mScroll, 0.0f, maxScroll);
        o2Render.EnableScissorTest((RectI)box.R(0, top, W, bottom - top));
        LayMessages(box, top, bottom, true);
        o2Render.DisableScissorTest();

        float cy = composeTop + 12;
        if (context)
        {
            const UiDoc::UiNode* n = UiDoc::FindNode(ctx->Doc(), ctx->selected);
            float idW = TextWidth(n->id, 11.0f, 600);
            float w = 8 + 12 + 6 + TextWidth("About", 11.0f) + 4 + idW + 6 + 16 + 4;
            RectF r = box.R(12, cy, w, 22);
            Fill(r, 11.0f, t.field);
            Border(r, 11.0f, t.line);
            UiWsDraw::Icon(TypeIcon(Ws::TypeWord(*n)), Vec2F(r.left + 8 + 6, r.Center().y), 12.0f, t.accent);
            float x = r.left + 26;
            float aw = TextWidth("About", 11.0f);
            Text("About", RectF(x, r.top, x + aw + 2, r.bottom), TextStyle(11.0f, 400, t.fgDim));
            Text(n->id, RectF(x + aw + 4, r.top, x + aw + 4 + idW + 2, r.bottom), TextStyle(11.0f, 600, t.fg));
            RectF close(r.right - 4 - 16, r.Center().y + 8, r.right - 4, r.Center().y - 8);
            if (IsHover("unselect"))
                Fill(close, 8.0f, t.active);
            UiWsDraw::Icon("uiws_close", close.Center(), 10.0f, IsHover("unselect") ? t.fg : t.fg3);
            AddHot(close, "unselect");
            cy += 22 + 6;
        }

        RectF me = layout->GetWorldRect();
        *mPrompt->layout = WidgetLayout(Vec2F(0, 1), Vec2F(1, 1), Vec2F(12, -(cy + 70)), Vec2F(-12, -cy));
        cy += 70 + 6;
        float sendW = 12 + 14 + 6 + TextWidth("Send", 12.0f, 600) + 12;
        *mModel->layout = WidgetLayout(Vec2F(0, 1), Vec2F(1, 1), Vec2F(12, -(cy + 28)), Vec2F(-(12 + sendW + 6 + 28 + 6), -cy));
        Tool(box.R(W - 12 - sendW - 6 - 28, cy, 28, 28), "uiws_review", "review", ctx->review, true, 15.0f);
        bool canSend = !String(mPrompt->GetText()).Trimed(" \t\r\n").IsEmpty() && !ctx->Busy() && onSend;
        Button(box.R(W - 12 - sendW, cy, sendW, 28), "Send", "send", true, canSend, "uiws_send");
        (void)me;
    }

    void UiWsAgentTab::OnHotClick(const String& id, const Input::Cursor& cursor)
    {
        if (!ctx)
            return;

        if (id == "send")
            Send();
        else if (id == "review")
            ctx->SetUi("agentReview", Json(!ctx->review));
        else if (id == "unselect")
            ctx->Select("");
        else if (id.StartsWith("edits:"))
            ToggleEdits(atoi(id.SubStr(6).Data()));
    }
}
