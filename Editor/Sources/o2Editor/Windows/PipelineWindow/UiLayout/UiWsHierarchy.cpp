#include "o2Editor/stdafx.h"
#include "UiWsHierarchy.h"

#include "o2/Render/Render.h"
#include "o2/Scene/UI/UIManager.h"
#include "o2/Scene/UI/WidgetLayer.h"
#include "o2/Scene/UI/WidgetLayout.h"
#include "o2/Scene/UI/Widgets/EditBox.h"
#include "o2/Utils/Editor/EditorScope.h"
#include "o2Editor/Windows/PipelineWindow/PipelineControls.h"

namespace Editor
{
    using namespace UiWsDraw;
    using UiDoc::UiNode;
    using UiDoc::UiOp;
    namespace Ws = UiDoc::Ws;

    namespace
    {
        const float headHeight = 40.0f;
        const float rowHeight = 26.0f;
        const float indent = 14.0f;
        const float listPad = 6.0f;
    }

    UiWsHierarchy::UiWsHierarchy(RefCounter* refCounter):
        UiWsPanel(refCounter)
    {}

    void UiWsHierarchy::BuildRows()
    {
        mRows.Clear();
        if (!ctx)
            return;

        const auto& doc = ctx->Doc();
        Function<void(const UiNode&, int)> walk = [&](const UiNode& n, int depth)
        {
            mRows.Add(Row{ n.id, depth, &n == &doc.root });
            if (mFolded.Contains(n.id))
                return;
            for (auto& c : n.children)
                walk(c, depth + 1);
        };
        walk(doc.root, 0);
    }

    RectF UiWsHierarchy::ListRect() const
    {
        RectF r = layout->GetWorldRect();
        return RectF(r.left, r.top - headHeight, r.right, r.bottom);
    }

    void UiWsHierarchy::ToggleFold(const String& id)
    {
        if (mFolded.Contains(id))
            mFolded.Remove(id);
        else
            mFolded.Add(id);
    }

    void UiWsHierarchy::OnScrolled(float scroll)
    {
        float visible = ListRect().Height();
        mScroll = Math::Clamp(mScroll - scroll*0.5f, 0.0f, Math::Max(0.0f, mContentHeight - visible));
    }

    // --- drawing ---------------------------------------------------------------------------------

    void UiWsHierarchy::DrawPanel(const Box& box)
    {
        if (!ctx)
            return;

        auto& t = T();
        float W = box.W();
        const auto& doc = ctx->Doc();
        bool editable = ctx->Editable();

        Fill(box.world, 0.0f, t.dark ? Color4(0, 0, 0, 20) : A(t.fg, 0.02f));
        Line(box.P(W - 0.5f, 0), box.P(W - 0.5f, box.H()), t.line);

        // The head: the icon, HIERARCHY and the add button
        UiWsDraw::Icon("uiws_layers", box.P(12 + 7, headHeight*0.5f), 14.0f, t.fgDim);
        Text("HIERARCHY", box.R(12 + 14 + 8, 0, W - 80, headHeight), TextStyle(11.0f, 600, t.fgDim));
        if (editable)
            Tool(box.R(W - 6 - 28, 6, 28, 28), "uiws_plus", "add", mAddMenu && mAddMenu->IsOpen());
        Line(box.P(0, headHeight - 0.5f), box.P(W - 1, headHeight - 0.5f), t.line);

        BuildRows();
        RectF list = ListRect();
        mContentHeight = listPad + mRows.Count()*rowHeight + 12.0f;
        mScroll = Math::Clamp(mScroll, 0.0f, Math::Max(0.0f, mContentHeight - list.Height()));
        o2Render.EnableScissorTest((RectI)RectF(list.left, list.top, list.right - 1, list.bottom));

        const UiNode* dragged = mDragStarted && !mDragging.IsEmpty() ? UiDoc::FindNode(doc, mDragging) : nullptr;
        float y = headHeight + listPad - mScroll;
        bool renameShown = false;
        for (auto& row : mRows)
        {
            const UiNode* n = row.root ? &doc.root : UiDoc::FindNode(doc, row.id);
            if (!n)
                continue;

            RectF r = box.R(6, y, W - 12, rowHeight);
            y += rowHeight;
            if (r.bottom > list.top || r.top < list.bottom)
                continue;

            String rowId = "row:" + n->id;
            bool selected = row.root ? ctx->selected.IsEmpty() : n->id == ctx->selected;
            bool hover = IsHover(rowId) || IsHover("caret:" + n->id) || IsHover("rename:" + n->id) || IsHover("hide:" + n->id) ||
                IsHover("delete:" + n->id);
            String zone = mDropAt && mDropAt->id == n->id ? mDropAt->zone : String();
            bool lifted = dragged && Ws::Holds(*dragged, n->id);

            if (zone == "inside")
            {
                Fill(r, 6.0f, t.accentSoft);
                Border(r, 6.0f, t.accent);
            }
            else if (selected)
                Fill(r, 6.0f, t.accentSoft);
            else if (hover && !mDragStarted)
                Fill(r, 6.0f, t.active);
            AddHot(r, rowId);

            float alpha = lifted ? 0.4f : 1.0f;
            float x = r.left;
            for (int d = 0; d < row.depth; d++)
            {
                Line(Vec2F(x + 7.5f, r.top), Vec2F(x + 7.5f, r.bottom), A(t.line, alpha));
                x += indent;
            }

            // The fold caret
            if (!n->children.empty())
            {
                RectF caret(x, r.Center().y + 8, x + 16, r.Center().y - 8);
                if (IsHover("caret:" + n->id))
                    Fill(caret, 4.0f, t.active);
                UiWsDraw::Icon(mFolded.Contains(n->id) ? "uiws_chevron_right" : "uiws_chevron_down", caret.Center(), 11.0f,
                               A(IsHover("caret:" + n->id) ? t.fg : t.fg3, alpha));
                AddHot(caret, "caret:" + n->id);
            }
            x += 16;

            float hidden = n->IsShown() ? 1.0f : 0.45f;
            UiWsDraw::Icon(TypeIcon(Ws::TypeWord(*n)), Vec2F(x + 10, r.Center().y), 14.0f, A(selected ? t.accent : t.fgDim, alpha*hidden));
            x += 20 + 3;

            // The right part: the actions on hover or selection, else the hidden mark and the kit thumbnail
            float right = r.right - 6;
            bool actions = !row.root && editable && (hover || selected) && !mDragStarted && mRenaming != n->id;
            if (actions)
            {
                auto act = [&](const String& icon, const String& id, float size, bool danger)
                {
                    RectF b(right - 20, r.Center().y + 10, right, r.Center().y - 10);
                    bool h = IsHover(id);
                    if (h)
                        Fill(b, 5.0f, t.active);
                    UiWsDraw::Icon(icon, b.Center(), size, h ? (danger ? t.red : t.fg) : t.fgDim);
                    AddHot(b, id);
                    right -= 21;
                };
                act("uiws_trash", "delete:" + n->id, 12.0f, true);
                act(n->IsShown() ? "uiws_eye" : "uiws_eye_off", "hide:" + n->id, 12.0f, false);
                act("uiws_pencil", "rename:" + n->id, 11.0f, false);
            }
            else if (!row.root && !selected && mRenaming != n->id)
            {
                bool skinned = n->type != "group" && n->type != "text";
                auto skin = skinned ? UiDoc::SkinOf(*n, ctx->hasKit ? &ctx->kit : nullptr) : UiDoc::UiSkin();
                if (skinned && skin.el)
                {
                    KitThumb(*skin.el, RectF(right - 16, r.Center().y + 8, right, r.Center().y - 8), 3.0f);
                    right -= 16 + 5;
                }
                else if (skinned && ctx->hasKit)
                {
                    Circle(Vec2F(right - 6, r.Center().y), 6.0f, A(t.yellow, 0.15f));
                    Circle(Vec2F(right - 6, r.Center().y), 3.0f, t.yellow);
                    right -= 12 + 5;
                }
                if (!n->IsShown())
                {
                    UiWsDraw::Icon("uiws_eye_off", Vec2F(right - 6.5f, r.Center().y), 13.0f, t.fg3);
                    right -= 13 + 5;
                }
            }

            if (mRenaming == n->id)
            {
                PlaceRenameField(r, x);
                renameShown = true;
            }
            else
            {
                String name = row.root ? doc.name : n->id;
                float nameW = TextWidth(name, 12.0f, row.root ? 600 : 400) + 2.0f;
                float nameRight = Math::Min(x + nameW, right - 4);
                Text(name, RectF(x, r.top, nameRight, r.bottom), TextStyle(12.0f, row.root ? 600 : 400, A(t.fg, alpha*hidden)));
                if (row.root)
                {
                    String meta = UiDoc::JsNum(doc.width) + "\xC3\x97" + UiDoc::JsNum(doc.height);
                    Text(meta, RectF(nameRight + 8, r.top, right, r.bottom), TextStyle(10.0f, 400, t.fg3, HorAlign::Left, true));
                }
            }

            // The landing line above or below the row, at the row's depth
            if (zone == "before" || zone == "after")
            {
                float lineY = zone == "before" ? r.top : r.bottom;
                float left = r.left + row.depth*indent + 18;
                Fill(RectF(left, lineY + 1, r.right - 4, lineY - 1), 1.0f, t.accent);
            }
        }

        if (mRenameField && !renameShown && mRenameField->IsEnabled())
            mRenameField->SetEnabledForcible(false);

        o2Render.DisableScissorTest();
    }

    void UiWsHierarchy::DrawOverlay(const Box& box)
    {
        if (!ctx || !mDragStarted || mDragging.IsEmpty())
            return;

        // The label that follows the pointer: accent while it can land, red where it cannot
        auto& t = T();
        const UiNode* n = UiDoc::FindNode(ctx->Doc(), mDragging);
        if (!n)
            return;

        float w = 7 + 13 + 6 + TextWidth(n->id, 12.0f) + 9;
        Vec2F p = mDragPoint + Vec2F(14, -10);
        RectF r(p.x, p.y, p.x + w, p.y - 24);
        Shadow(RectF(r.left, r.top - 6, r.right, r.bottom - 6), 7.0f, t.dark ? Color4(0, 0, 0, 100) : Color4(38, 50, 56, 40), 18.0f);
        Color4 edge = mDropAt ? t.accent : t.red;
        Fill(r, 7.0f, A(t.cardSolid, mDropAt ? 1.0f : 0.75f));
        Border(r, 7.0f, edge);
        UiWsDraw::Icon(TypeIcon(Ws::TypeWord(*n)), Vec2F(r.left + 7 + 6.5f, r.Center().y), 13.0f, edge);
        Text(n->id, RectF(r.left + 26, r.top, r.right - 4, r.bottom), TextStyle(12.0f, 400, t.fg));
    }

    // --- clicks ------------------------------------------------------------------------------------

    void UiWsHierarchy::OnHotClick(const String& id, const Input::Cursor& cursor)
    {
        if (!ctx)
            return;

        const auto& doc = ctx->Doc();
        auto value = [&](const char* prefix) { return id.SubStr(String(prefix).Length()); };
        if (id == "add")
            OpenAddMenu();
        else if (id.StartsWith("caret:"))
            ToggleFold(value("caret:"));
        else if (id.StartsWith("rename:"))
            StartRename(value("rename:"));
        else if (id.StartsWith("hide:"))
        {
            if (const UiNode* n = UiDoc::FindNode(doc, value("hide:")))
            {
                String label;
                UiOp op = Ws::VisibilityOp(*n, label);
                ctx->Gesture({ op }, label);
            }
        }
        else if (id.StartsWith("delete:"))
        {
            String target = value("delete:");
            bool wasSelected = ctx->selected == target;
            ctx->Gesture({ UiDoc::MakeRemoveOp({ target }) }, "\xE2\x88\x92 " + target);
            if (wasSelected)
                ctx->Select("");
        }
    }

    void UiWsHierarchy::OnHotDoubleClick(const String& id, const Input::Cursor& cursor)
    {
        if (ctx && ctx->Editable() && id.StartsWith("row:"))
            StartRename(id.SubStr(4));
    }

    void UiWsHierarchy::OnCursorPressed(const Input::Cursor& cursor)
    {
        UiWsPanel::OnCursorPressed(cursor);
        mDragging = "";
        mDragStarted = false;
        mDropAt.reset();
        if (!ctx || !mPressedHot.StartsWith("row:"))
            return;

        String id = mPressedHot.SubStr(4);
        bool root = id == ctx->Doc().root.id;
        ctx->Select(root ? String() : id);
        if (ctx->Editable() && !root && mRenaming.IsEmpty())
        {
            mDragging = id;
            mDragPoint = cursor.position;
        }
    }

    void UiWsHierarchy::OnCursorStillDown(const Input::Cursor& cursor)
    {
        if (mDragging.IsEmpty())
            return;

        if (!mDragStarted && (cursor.position - mPressPoint).Length() < 4.0f)
            return;

        mDragStarted = true;
        DragRowTo(cursor.position, 0.0f);
    }

    void UiWsHierarchy::OnCursorReleased(const Input::Cursor& cursor)
    {
        if (!mDragStarted)
        {
            mDragging = "";
            UiWsPanel::OnCursorReleased(cursor);
            return;
        }

        String dragId = mDragging;
        auto at = mDropAt;
        mDragging = "";
        mDragStarted = false;
        mDropAt.reset();
        mPressedHot = "";
        if (at && ctx)
        {
            Vector<UiOp> ops;
            String label;
            if (Ws::DropOps(ctx->Doc(), ctx->rects, dragId, *at, ops, label))
                ctx->Gesture(ops, label);
        }
    }

    void UiWsHierarchy::OnCursorPressBreak(const Input::Cursor& cursor)
    {
        mDragging = "";
        mDragStarted = false;
        mDropAt.reset();
    }

    bool UiWsHierarchy::DropTarget(const String& dragId, const Vec2F& point, UiDoc::Ws::UiDropAt& at)
    {
        if (!ctx || !Inside(ListRect(), point))
            return false;

        for (auto& hot : mDrawnHot)
        {
            if (!hot.id.StartsWith("row:") || point.y > hot.rect.top || point.y < hot.rect.bottom)
                continue;

            String target = hot.id.SubStr(4);
            float f = (hot.rect.top - point.y)/Math::Max(1.0f, hot.rect.top - hot.rect.bottom);
            return Ws::ZoneAt(ctx->Doc(), dragId, target, f, mFolded.Contains(target), at);
        }

        // Below the last row: the end of the screen
        return Ws::ZoneAt(ctx->Doc(), dragId, "", 0.0f, false, at);
    }

    void UiWsHierarchy::DragRowTo(const Vec2F& point, float dt)
    {
        mDragPoint = point;
        UiDoc::Ws::UiDropAt at;
        if (DropTarget(mDragging, point, at))
            mDropAt = at;
        else
            mDropAt.reset();

        // Held over a folded block, it opens
        if (mDropAt && mDropAt->zone == "inside" && mFolded.Contains(mDropAt->id))
        {
            if (mUnfoldId != mDropAt->id)
            {
                mUnfoldId = mDropAt->id;
                mUnfoldTime = 0.0f;
            }
        }
        else
            mUnfoldId = "";
    }

    void UiWsHierarchy::Update(float dt)
    {
        UiWsPanel::Update(dt);
        if (!mDragStarted)
            return;

        // Near the list's top or bottom it scrolls
        RectF list = ListRect();
        if (mDragPoint.y > list.top - 24.0f)
            mScroll = Math::Max(0.0f, mScroll - 12.0f);
        else if (mDragPoint.y < list.bottom + 24.0f)
            mScroll = Math::Min(Math::Max(0.0f, mContentHeight - list.Height()), mScroll + 12.0f);

        if (!mUnfoldId.IsEmpty())
        {
            mUnfoldTime += dt;
            if (mUnfoldTime >= 0.65f)
            {
                mFolded.Remove(mUnfoldId);
                mUnfoldId = "";
            }
        }
        DragRowTo(mDragPoint, dt);
    }

    // --- rename -----------------------------------------------------------------------------------

    void UiWsHierarchy::StartRename(const String& id)
    {
        if (!ctx || !ctx->Editable())
            return;

        PushEditorScopeOnStack scope;
        const auto& doc = ctx->Doc();
        bool root = id.IsEmpty() || id == doc.root.id;
        String value = root ? doc.name : id;
        mRenaming = root ? doc.root.id : id;
        mRenameBad = false;
        if (!mRenameField)
        {
            mRenameField = UiWsControls::TextField("", "", false, {});
            mRenameField->name = "rename";
            AddChild(mRenameField);
            WeakRef<UiWsHierarchy> weak(this);
            mRenameField->onChanged = [weak](const WString& text)
            {
                auto self = weak.Lock();
                if (!self || !self->ctx || self->mRenaming.IsEmpty())
                    return;
                const auto& d = self->ctx->Doc();
                bool isRoot = self->mRenaming == d.root.id;
                String clean = UiDoc::CleanId((String)text);
                String current = isRoot ? d.name : self->mRenaming;
                self->mRenameBad = !clean.IsEmpty() && clean != current && !isRoot && UiDoc::FindNode(d, clean);
                if (auto border = self->mRenameField->GetLayer("border"))
                    border->transparency = self->mRenameBad ? 0.0f : 1.0f;
            };
            mRenameField->onChangeCompleted = [weak](const WString&)
            {
                if (auto self = weak.Lock())
                    self->EndRename(true);
            };
        }

        UiWsControls::SetTextQuiet(mRenameField, value);
        mRenameField->SetEnabledForcible(true);
        mRenameField->Focus();
        mRenameField->SelectAll();
    }

    void UiWsHierarchy::EndRename(bool keep)
    {
        if (mRenaming.IsEmpty() || !ctx)
            return;

        String id = mRenaming;
        mRenaming = "";
        String typed = mRenameField ? (String)mRenameField->GetText() : String();
        if (mRenameField)
        {
            if (mRenameField->IsFocused())
                o2UI.FocusWidget(nullptr);
            mRenameField->SetEnabledForcible(false);
        }
        if (!keep || mRenameBad)
            return;

        UiOp op;
        String label;
        const auto& doc = ctx->Doc();
        bool root = id == doc.root.id;
        if (!Ws::RenameOp(doc, id, typed, op, label))
            return;

        bool follow = !root && ctx->selected == id;
        ctx->Gesture({ op }, label);
        if (follow)
            ctx->Select(UiDoc::CleanId(typed));
    }

    void UiWsHierarchy::PlaceRenameField(const RectF& row, float nameLeft)
    {
        if (!mRenameField)
            return;

        RectF me = layout->GetWorldRect();
        float left = nameLeft - me.left, right = row.right - 4 - me.left;
        float top = me.top - row.top + 2.5f;
        *mRenameField->layout = WidgetLayout(Vec2F(0, 1), Vec2F(0, 1), Vec2F(left, -(top + 21)), Vec2F(right, -top));
        auto& t = T();
        if (!mRenameField->GetLayer("bad"))
        {
            auto bad = PipelineControls::AddBorderLayer(*mRenameField, "bad", t.red, 5.0f, 1.0f, Layout::BothStretch(), 0.5f);
            bad->transparency = 0.0f;
        }
        mRenameField->GetLayer("bad")->transparency = mRenameBad ? 1.0f : 0.0f;
    }

    // --- add ---------------------------------------------------------------------------------------

    void UiWsHierarchy::AddBlock(const String& word)
    {
        if (!ctx || !ctx->Editable())
            return;

        String label, id;
        UiOp op = Ws::AddBlockOp(ctx->Doc(), ctx->selected, word, label, id);
        ctx->Gesture({ op }, label);
        ctx->Select(id);
    }

    void UiWsHierarchy::OpenAddMenu()
    {
        PushEditorScopeOnStack scope;
        if (!mAddMenu)
        {
            mAddMenu = mmake<UiWsPopup>();
            mAddMenu->name = "add menu";
            mAddPanel = mmake<UiWsAddMenuPanel>();
            mAddPanel->name = "add menu panel";
            mAddMenu->SetContent(mAddPanel);
            AddChild(mAddMenu);
            WeakRef<UiWsHierarchy> weak(this);
            mAddPanel->onPick = [weak](const String& word)
            {
                auto self = weak.Lock();
                if (!self)
                    return;
                self->mAddMenu->Close();
                self->AddBlock(word);
            };
        }

        if (mAddMenu->IsOpen() || UiWsPopup::ClosedThisFrame())
        {
            mAddMenu->Close();
            return;
        }

        RectF anchor = HotRect("add");
        mAddPanel->ctx = ctx;
        mAddMenu->OpenAt(anchor, 300.0f, UiWsAddMenuPanel::ContentHeight());
    }

    float UiWsAddMenuPanel::ContentHeight()
    {
        int rows = (Ws::AddableWords().Count() + 1)/2;
        return 12.0f + rows*28.0f + (rows - 1)*2.0f;
    }

    void UiWsAddMenuPanel::DrawPanel(const Box& box)
    {
        auto& t = T();
        float colW = (box.W() - 12 - 2)*0.5f;
        auto& words = Ws::AddableWords();
        for (int i = 0; i < words.Count(); i++)
        {
            float x = 6 + (i % 2)*(colW + 2), y = 6 + (i/2)*30;
            RectF r = box.R(x, y, colW, 28);
            String id = "add:" + words[i];
            bool hover = IsHover(id);
            if (hover)
                Fill(r, 6.0f, t.active);
            UiWsDraw::Icon(TypeIcon(words[i]), Vec2F(r.left + 8 + 7.5f, r.Center().y), 15.0f, hover ? t.accent : t.fgDim);
            Text(Ws::TypeLabel(words[i]), RectF(r.left + 31, r.top, r.right - 4, r.bottom), TextStyle(12.0f, 400, t.fg));
            AddHot(r, id);
        }
    }

    void UiWsAddMenuPanel::OnHotClick(const String& id, const Input::Cursor& cursor)
    {
        if (id.StartsWith("add:") && onPick)
            onPick(id.SubStr(4));
    }
}
