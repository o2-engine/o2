#include "o2Editor/stdafx.h"
#include "UiWsPins.h"

#include "o2/Scene/UI/Widgets/EditBox.h"
#include "o2Editor/Windows/PipelineWindow/UiLayout/UiWsDraw.h"
#include "o2Editor/Windows/PipelineWindow/UiLayout/UiWsStage.h"

namespace Editor
{
    namespace UiWsPins
    {
        using UiDoc::Json;
        using UiDoc::UiStepPoint;

        // How many points a request may have
        static const int POINTS_MAX = 20;

        // The pin's radius on the stage, screen pixels
        static const float PIN_R = 11.0f;

        Settled Settle(const String& raw, int count)
        {
            // The same renumbering as a request sent with its points (RequestWithPins), with nothing written beside them
            Vector<UiStepPoint> blank;
            for (int i = 0; i < count; i++)
                blank.Add(UiStepPoint());
            Settled out;
            out.order = UiDoc::PinsNamed(raw, count);
            out.text = UiDoc::RequestWithPins(raw, blank).text;
            return out;
        }

        String InsertPin(const String& text, int caret, int n, int& caretAfter)
        {
            WString w = text;
            int at = Math::Clamp(caret, 0, w.Length());
            WString before = w.SubStr(0, at), after = w.SubStr(at);
            auto white = [](wchar_t c) { return c == ' ' || c == '\n' || c == '\t' || c == '\r'; };
            // A space either side, unless one is there already: a space after the caret is gone past, not doubled
            bool lead = before.Length() > 0 && !white(before[before.Length() - 1]);
            bool spaced = after.Length() > 0 && white(after[0]);
            WString token = WString(lead ? " " : "") + WString("[" + (String)n + "]" + (spaced ? "" : " "));
            caretAfter = before.Length() + token.Length() + (spaced ? 1 : 0);
            return (String)(before + token + after);
        }

        static Json PointsJson(const Vector<UiStepPoint>& points)
        {
            return UiDoc::PointsToJson(points);
        }

        String Typed(UiWsContext& ctx, const String& text)
        {
            int count = ctx.points.Count();
            Settled st = Settle(text, count);
            bool same = st.order.Count() == count;
            for (int i = 0; same && i < count; i++)
                same = st.order[i] == i + 1 && ctx.points[i].text.IsEmpty();

            Vector<Pair<String, Json>> values = { { "uiDraft", Json(st.text) } };
            if (!same)
            {
                Vector<UiStepPoint> kept;
                for (int n : st.order)
                    kept.Add(UiStepPoint{ ctx.points[n - 1].x, ctx.points[n - 1].y, "" });
                values.Add({ "uiPoints", PointsJson(kept) });
                ctx.points = kept;
            }
            if (st.text != ctx.draft || !same)
            {
                ctx.draft = st.text;
                ctx.SetUi(values);
            }
            return st.text;
        }

        void Place(UiWsContext& ctx, EditBox* field, double x, double y, int index /*= -1*/, int caret /*= -1*/)
        {
            x = UiDoc::JsRound(x);
            y = UiDoc::JsRound(y);
            if (index >= 0)
            {
                if (index >= ctx.points.Count())
                    return;
                ctx.points[index].x = x;
                ctx.points[index].y = y;
                ctx.SetUi("uiPoints", PointsJson(ctx.points));
                return;
            }
            if (ctx.points.Count() >= POINTS_MAX)
                return;

            Vector<UiStepPoint> next;
            for (auto& p : ctx.points)
                next.Add(UiStepPoint{ p.x, p.y, "" });
            next.Add(UiStepPoint{ x, y, "" });
            int n = next.Count();

            String request = UiDoc::DraftWithNotes(ctx.draft, ctx.points);
            Settled st;
            if (field)
            {
                // The token goes in at the caret of the field (the end when the field never had one)
                String text = (String)field->GetText();
                if (caret < 0)
                    caret = WString(text).Length();
                int caretAfter = 0;
                st = Settle(InsertPin(text, caret, n, caretAfter), n);
                auto changed = field->onChanged;
                auto completed = field->onChangeCompleted;
                field->onChanged = Function<void(const WString&)>();
                field->onChangeCompleted = Function<void(const WString&)>();
                field->SetText(st.text);
                field->SetCaretPosition(Math::Min(caretAfter, WString(st.text).Length()));
                field->onChanged = changed;
                field->onChangeCompleted = completed;
            }
            else
            {
                // The field is not on the screen: the point goes to the end of the request
                bool space = request.IsEmpty() || request.EndsWith(" ") || request.EndsWith("\n") || request.EndsWith("\t");
                st.text = request + (space ? "" : " ") + "[" + (String)n + "] ";
                for (int i = 1; i <= n; i++)
                    st.order.Add(i);
            }

            Vector<UiStepPoint> ordered;
            for (int k : st.order)
                ordered.Add(next[k - 1]);
            ctx.points = ordered;
            ctx.draft = st.text;
            ctx.SetUi({ { "uiPoints", PointsJson(ordered) }, { "uiDraft", Json(st.text) } });
        }

        void OnFieldChanged(UiWsContext& ctx, EditBox* field)
        {
            // Only what is typed: a field shown again with the stored draft (another view of the node wrote it) has nothing
            // to settle, and its context may not have read the node's new points yet
            if (!field || !field->IsFocused())
                return;
            String text = (String)field->GetText();
            String settled = Typed(ctx, text);
            if (settled == text)
                return;

            // Typed by hand ("[2]") or numbered anew: the field shows the settled text, the caret kept
            int caret = field->GetCaretPosition();
            auto changed = field->onChanged;
            auto completed = field->onChangeCompleted;
            field->onChanged = Function<void(const WString&)>();
            field->onChangeCompleted = Function<void(const WString&)>();
            field->SetText(settled);
            field->SetCaretPosition(Math::Min(caret, WString(settled).Length()));
            field->onChanged = changed;
            field->onChangeCompleted = completed;
        }

        // --- the stage ---------------------------------------------------------------------------------------------------

        // The pin being dragged (-1: none) and whether it moved
        static int  dragged = -1;
        static bool draggedMoved = false;

        // The index of the pin under a world point, -1 for none
        static int PinAt(const UiWsStage& stage, const UiWsContext& ctx, const Vec2F& world)
        {
            for (int i = ctx.points.Count() - 1; i >= 0; i--)
            {
                Vec2F p = stage.DesignToWorld(ctx.points[i].x, ctx.points[i].y);
                if ((p - world).Length() <= PIN_R + 2)
                    return i;
            }
            return -1;
        }

        void Draw(const UiWsStage& stage, const UiWsContext& ctx)
        {
            for (int i = 0; i < ctx.points.Count(); i++)
            {
                Vec2F p = stage.DesignToWorld(ctx.points[i].x, ctx.points[i].y);
                UiWsDraw::Circle(p, PIN_R + 2, Color4(255, 255, 255, 255));
                UiWsDraw::Circle(p, PIN_R, Color4(255, 45, 111, 255));
                UiWsDraw::Text((String)(i + 1), RectF(p.x - PIN_R, p.y + PIN_R, p.x + PIN_R, p.y - PIN_R),
                               UiWsDraw::TextStyle(11.5f, 700, Color4::White(), HorAlign::Middle));
            }
        }

        bool Press(const UiWsStage& stage, UiWsContext& ctx, const Vec2F& world)
        {
            dragged = PinAt(stage, ctx, world);
            draggedMoved = false;
            if (dragged >= 0)
                return true;
            if (!ctx.pinning || ctx.Busy())
                return false;

            Vec2F design = stage.WorldToDesign(world);
            const auto& doc = ctx.Doc();
            if (design.x < 0 || design.y < 0 || design.x > doc.width || design.y > doc.height)
                return true;
            if (ctx.onPin)
                ctx.onPin(design.x, design.y, -1);
            else
                Place(ctx, nullptr, design.x, design.y);
            return true;
        }

        bool Drag(const UiWsStage& stage, UiWsContext& ctx, const Vec2F& world)
        {
            if (dragged < 0 || dragged >= ctx.points.Count())
                return false;
            Vec2F design = stage.WorldToDesign(world);
            const auto& doc = ctx.Doc();
            ctx.points[dragged].x = UiDoc::JsRound(Math::Clamp((double)design.x, 0.0, doc.width));
            ctx.points[dragged].y = UiDoc::JsRound(Math::Clamp((double)design.y, 0.0, doc.height));
            draggedMoved = true;
            return true;
        }

        bool Release(const UiWsStage& stage, UiWsContext& ctx, const Vec2F& world)
        {
            if (dragged < 0)
                return false;
            int index = dragged;
            dragged = -1;
            if (draggedMoved && index < ctx.points.Count())
                Place(ctx, nullptr, ctx.points[index].x, ctx.points[index].y, index);
            return true;
        }
    }
}
