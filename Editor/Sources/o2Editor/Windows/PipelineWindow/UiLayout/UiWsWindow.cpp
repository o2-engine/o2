#include "o2Editor/stdafx.h"
#include "UiWsWindow.h"

#include "o2/Application/Application.h"
#include "o2/Application/Input.h"
#include "o2/Render/Render.h"
#include "o2/Scene/UI/UIManager.h"
#include "o2/Scene/UI/WidgetLayout.h"
#include "o2/Scene/UI/Widgets/EditBox.h"
#include "o2/Utils/Editor/EditorScope.h"
#include "o2/Utils/FileSystem/FileSystem.h"
#include "o2Editor/Pipeline/PipelinePairLayout.h"
#include "o2Editor/Pipeline/PipelineUtils.h"
#include "o2Editor/Windows/PipelineWindow/PipelineDraw.h"
#include "o2Editor/Windows/PipelineWindow/PipelineEditor.h"

namespace Editor
{
    using namespace UiWsDraw;
    using UiDoc::Json;
    namespace Ws = UiDoc::Ws;

    namespace
    {
        const float minW = 860.0f, minH = 520.0f;
        const float headHeight = 48.0f;
        const float stageMin = 280.0f;
        const float leftMin = 180.0f, leftMax = 520.0f, rightMin = 300.0f, rightMax = 720.0f;

        float Clamp(float v, float lo, float hi)
        {
            return Math::Max(lo, Math::Min(Math::Max(lo, hi), v));
        }
    }

    UiWsWindow::UiWsWindow(RefCounter* refCounter):
        UiWsPanel(refCounter)
    {
        PushEditorScopeOnStack scope;
        mCtx = mmake<UiWsContext>();
        ctx = mCtx;
        mStage = mmake<UiWsStage>();
        mStage->name = "stage";
        mSide = mmake<UiWsSidePanel>();
        mSide->name = "side";
        mHierarchy = mmake<UiWsHierarchy>();
        mHierarchy->name = "hierarchy";
        // The hierarchy last: the row being dragged follows the pointer over the stage
        AddChild(mStage);
        AddChild(mSide);
        AddChild(mHierarchy);
        mStage->ctx = mCtx;
        mStage->GetDevicePicker()->Bind(mCtx);
        mHierarchy->ctx = mCtx;
        mSide->Bind(mCtx);

        mCoveragePopup = mmake<UiWsPopup>();
        mCoveragePopup->name = "coverage";
        mCoverage = mmake<UiWsCoveragePanel>();
        mCoverage->name = "coverage panel";
        mCoverage->ctx = mCtx;
        mCoveragePopup->SetContent(mCoverage);
        AddChild(mCoveragePopup);
        WeakRef<UiWsWindow> weak(this);
        mCoverage->onPickBlock = [weak](const String& id)
        {
            auto self = weak.Lock();
            if (!self)
                return;
            self->mCtx->Select(id);
            self->mCtx->SetUi("uiTab", Json("inspect"));
            self->mCoveragePopup->Close();
        };
        mSide->GetAgent()->onSend = [weak](const String& prompt, const String& selected)
        {
            auto self = weak.Lock();
            if (self && self->onSend)
                self->onSend(prompt, selected);
        };
        mGrips = mmake<UiWsGrips>();
        mGrips->name = "grips";
        *mGrips->layout = WidgetLayout::BothStretch();
        AddChild(mGrips);
        mGrips->onPress = [weak](const String& zone, const Vec2F& p) { if (auto self = weak.Lock()) self->BeginDrag(zone, p); };
        mGrips->onDrag = [weak](const Vec2F& p) { if (auto self = weak.Lock()) self->DragTo(p); };
        mGrips->onRelease = [weak]() { if (auto self = weak.Lock()) self->EndDrag(); };
        mGrips->onDouble = [weak](const String& zone)
        {
            auto self = weak.Lock();
            if (self && zone.StartsWith("split:"))
                self->SetPanelWidth(zone.SubStr(6), 0.0f);
        };

        mClipChildren = true;
        LoadPrefs();
    }

    // --- grips -------------------------------------------------------------------------------------

    bool UiWsGrips::IsUnderPoint(const Vec2F& point)
    {
        for (auto& z : zones)
        {
            if (Inside(z.first, point))
                return true;
        }
        return false;
    }

    void UiWsGrips::DrawPanel(const Box& box)
    {
        auto& t = T();
        for (auto& z : zones)
        {
            AddHot(z.first, z.second);
            // A divider shows its line while hovered or dragged
            if (z.second.StartsWith("split:") && (IsHover(z.second) || active == z.second))
            {
                float x = z.first.Center().x;
                Fill(RectF(x - 1, z.first.top, x + 1, z.first.bottom), 0.0f, t.accent);
            }
        }
    }

    void UiWsGrips::OnCursorPressed(const Input::Cursor& cursor)
    {
        UiWsPanel::OnCursorPressed(cursor);
        active = mPressedHot;
        if (!active.IsEmpty() && onPress)
            onPress(active, cursor.position);
    }

    void UiWsGrips::OnCursorStillDown(const Input::Cursor& cursor)
    {
        if (!active.IsEmpty() && onDrag)
            onDrag(cursor.position);
    }

    void UiWsGrips::OnCursorReleased(const Input::Cursor& cursor)
    {
        String was = active;
        active = "";
        mPressedHot = "";
        if (!was.IsEmpty() && onRelease)
            onRelease();
    }

    void UiWsGrips::OnCursorDblClicked(const Input::Cursor& cursor)
    {
        String zone = HotAt(cursor.position);
        if (!zone.IsEmpty() && onDouble)
            onDouble(zone);
    }

    void UiWsWindow::Bind(const Ref<PipelineEditor>& editor, const String& nodeId)
    {
        PushEditorScopeOnStack scope;
        bool same = mShown && mNodeId == nodeId && mCtx->editor.Lock() == editor;
        mNodeId = nodeId;
        mCtx->editor = editor;
        mCtx->nodeId = nodeId;
        if (!same)
        {
            mCtx->peek = -1;
            mStage->Fit();
        }
        mCtx->Refresh();
        mShown = true;
        mThemeVersion = PipelineTheme::GetVersion();
        SetEnabledForcible(true);
        o2UI.FocusWidget(Ref(this));
        mHasFocus = true;
    }

    void UiWsWindow::Close()
    {
        if (!mShown)
            return;

        mSide->GetAgent()->SaveDraft();
        mHierarchy->EndRename(false);
        mShown = false;
        mHasFocus = false;
        if (o2UI.GetFocusedWidget() == Ref(this))
            o2UI.FocusWidget(nullptr);
        SetEnabledForcible(false);
        o2Application.SetCursor(CursorType::Arrow);
        if (onClosed)
            onClosed();
    }

    // --- geometry --------------------------------------------------------------------------------

    Vec2F UiWsWindow::ScreenSize() const
    {
        RectF r = layout->GetWorldRect();
        Vec2F size(r.Width(), r.Height());
        if (size.x < 1 || size.y < 1)
            size = (Vec2F)o2Render.GetResolution();
        return size;
    }

    UiWsFrame UiWsWindow::ShownFrame() const
    {
        Vec2F screen = ScreenSize();
        if (mMax)
            return UiWsFrame{ 8, 8, screen.x - 16, screen.y - 16 };
        return UiWsFrame{ mX, mY, mW, mH };
    }

    UiWsFrame UiWsWindow::GetFrame() const
    {
        return ShownFrame();
    }

    void UiWsWindow::SetFrame(float x, float y, float w, float h)
    {
        Vec2F screen = ScreenSize();
        mW = Math::Max(Math::Min(minW, screen.x - 16), Math::Min(screen.x - 16, w));
        mH = Math::Max(Math::Min(minH, screen.y - 16), Math::Min(screen.y - 16, h));
        mX = Math::Max(8.0f, Math::Min(x, screen.x - mW - 8));
        mY = Math::Max(8.0f, Math::Min(y, screen.y - mH - 8));
        ClampPanels();
    }

    void UiWsWindow::ToggleMaximized()
    {
        mMax = !mMax;
        ClampPanels();
        SavePrefs();
    }

    void UiWsWindow::ClampPanels()
    {
        float winW = ShownFrame().w;
        bool small = winW < 1180.0f;
        float defLeft = small ? 228.0f : 264.0f, defRight = small ? 340.0f : 400.0f;
        mRight = Clamp(mSavedRight > 0 ? mSavedRight : defRight, rightMin, Math::Min(rightMax, winW - leftMin - stageMin));
        mLeft = Clamp(mSavedLeft > 0 ? mSavedLeft : defLeft, leftMin, Math::Min(leftMax, winW - mRight - stageMin));
    }

    void UiWsWindow::SetPanelWidth(const String& side, float width)
    {
        if (side == "left")
            mSavedLeft = width;
        else
            mSavedRight = width;
        ClampPanels();
        // What is kept is what is shown (clamped)
        if (side == "left" && width > 0)
            mSavedLeft = mLeft;
        else if (side != "left" && width > 0)
            mSavedRight = mRight;
        SavePrefs();
    }

    void UiWsWindow::LoadPrefs()
    {
        Json saved;
        DataDocument doc;
        String path = PipelinePairLayout::GetViewPrefsPath();
        if (o2FileSystem.IsFileExist(path) && doc.LoadFromFile(path) && doc.IsObject())
        {
            if (auto member = doc.FindMember("uiWorkspace"))
                saved = UiDoc::FromDataValue(*member);
        }

        Vec2F screen = ScreenSize();
        auto num = [&](const char* key) { const Json* v = saved.Get(key); return v && v->IsNumber() ? (float)v->Num() : -1.0f; };
        float w = num("w") > 0 ? num("w") : Math::Min(1560.0f, screen.x - 48);
        float h = num("h") > 0 ? num("h") : Math::Min(980.0f, screen.y - 48);
        mW = Math::Max(Math::Min(minW, screen.x - 16), Math::Min(screen.x - 16, w));
        mH = Math::Max(Math::Min(minH, screen.y - 16), Math::Min(screen.y - 16, h));
        mX = Math::Round((screen.x - mW)*0.5f);
        mY = Math::Round((screen.y - mH)*0.5f);
        const Json* max = saved.Get("max");
        mMax = max && max->IsBool() && max->Bool();
        mSavedLeft = Math::Max(0.0f, num("left"));
        mSavedRight = Math::Max(0.0f, num("right"));
        ClampPanels();
    }

    void UiWsWindow::SavePrefs() const
    {
        String path = PipelinePairLayout::GetViewPrefsPath();
        DataDocument doc;
        if (!o2FileSystem.IsFileExist(path) || !doc.LoadFromFile(path) || !doc.IsObject())
            doc.SetObject();

        Json value = Json::Object();
        value["w"] = (double)Math::Round(mW);
        value["h"] = (double)Math::Round(mH);
        value["max"] = mMax;
        if (mSavedLeft > 0)
            value["left"] = (double)Math::Round(mSavedLeft);
        if (mSavedRight > 0)
            value["right"] = (double)Math::Round(mSavedRight);
        UiDoc::SetConfigJson(doc, "uiWorkspace", value);
        o2FileSystem.FolderCreate(PipelineUtils::GetWorkPath(), true);
        doc.SaveToFile(path);
    }

    void UiWsWindow::LayoutPanels()
    {
        UiWsFrame f = ShownFrame();
        float top = f.y + headHeight, bottom = f.Bottom() - 1;
        float left = f.x + 1, right = f.Right() - 1;
        auto place = [&](const Ref<Widget>& w, float x0, float y0, float x1, float y1)
        {
            *w->layout = WidgetLayout(Vec2F(0, 1), Vec2F(0, 1), Vec2F(x0, -y1), Vec2F(x1, -y0));
        };
        place(mHierarchy, left, top, left + mLeft, bottom);
        place(mStage, left + mLeft, top, right - mRight, bottom);
        place(mSide, right - mRight, top, right, bottom);
    }

    void UiWsWindow::Update(float dt)
    {
        if (!mShown)
            return;

        // A new theme: the panels' fields are made again in its colours
        if (PipelineTheme::GetVersion() != mThemeVersion)
            mThemeVersion = PipelineTheme::GetVersion();

        mPoll += dt;
        if (mPoll > 0.25f)
        {
            mPoll = 0.0f;
            mCtx->RefreshIfChanged();
        }

        ClampPanels();
        LayoutPanels();

        // The window takes the keys after a press inside it (a field keeps them while it is typed into)
        if (o2Input.IsCursorPressed())
        {
            UiWsFrame f = ShownFrame();
            Box screen(layout->GetWorldRect());
            bool inside = Inside(screen.R(f.x, f.y, f.w, f.h), o2Input.GetCursorPos()) || UiWsPopup::AnyOpen();
            mHasFocus = inside;
            if (inside && !DynamicCast<EditBox>(o2UI.GetFocusedWidget()))
                o2UI.FocusWidget(Ref(this));
        }

        UiWsPanel::Update(dt);
        Sweep();
    }

    // --- drawing ---------------------------------------------------------------------------------

    void UiWsWindow::DrawPanel(const Box& box)
    {
        if (!mShown)
            return;

        auto& t = T();
        UiWsFrame f = ShownFrame();
        RectF frame = box.R(f.x, f.y, f.w, f.h);
        mChildClip = frame;

        // The backdrop: a press on it closes the window
        Fill(box.world, 0.0f, t.dark ? Color4(6, 7, 10, 128) : Color4(38, 50, 56, 56));
        AddHot(box.world, "backdrop");

        float radius = mMax ? 10.0f : 12.0f;
        Shadow(RectF(frame.left, frame.top - 24, frame.right, frame.bottom - 24), radius, t.dark ? Color4(0, 0, 0, 140) : Color4(38, 50, 56, 56), 64.0f);
        Fill(frame, radius, t.cardSolid);
        Border(frame, radius, t.border);
        AddHot(frame, "frame");

        // The header: moved by it, a double-click maximizes
        RectF head(frame.left, frame.top, frame.right, frame.top - headHeight);
        PipelineDraw::Fill(RectF(head.left + 1, head.top - 1, head.right - 1, head.bottom), PipelineDraw::Corners(radius - 1, 0.0f),
                           PipelineDraw::Gradient(t.active, A(t.active, 0.0f)));
        Line(Vec2F(head.left + 1, head.bottom + 0.5f), Vec2F(head.right - 1, head.bottom + 0.5f), t.line);
        AddHot(head, "head");

        float x = head.left + 12, cy = head.Center().y;
        Fill(RectF(x, cy + 14, x + 28, cy - 14), 8.0f, t.accentSoft);
        UiWsDraw::Icon("uiws_uiwire", Vec2F(x + 14, cy), 15.0f, t.accent);
        x += 28 + 12;
        const auto& doc = mCtx->Doc();
        String sub = doc.name + " \xC2\xB7 " + UiDoc::JsNum(doc.width) + "\xC3\x97" + UiDoc::JsNum(doc.height);
        float titlesW = Math::Max(TextWidth("AI UI layout", 13.0f, 600), TextWidth(sub, 10.5f, 400, true)) + 4;
        Text("AI UI layout", RectF(x, cy + 15, x + titlesW, cy - 1), TextStyle(13.0f, 600, t.fg));
        Text(sub, RectF(x, cy + 1, x + titlesW, cy - 14), TextStyle(10.5f, 400, t.fg3, HorAlign::Left, true));
        x += titlesW + 12;

        // The kit coverage chip
        if (mCtx->hasKit)
        {
            auto& report = mCtx->report;
            float pct = report.total ? (float)report.covered/report.total : 1.0f;
            Color4 tone = pct >= 0.8f ? t.green : pct >= 0.4f ? t.yellow : t.red;
            String text = "kit covers " + (String)report.covered + "/" + (String)report.total;
            float w = 10 + 34 + 7 + TextWidth(text, 10.5f, 400, true) + 7 + 11 + 10;
            RectF chip(x, cy + 14, x + w, cy - 14);
            bool open = mCoveragePopup->IsOpen();
            Fill(chip, 14.0f, t.dark ? Color4(24, 26, 34, 158) : A(t.cardSolid, 0.9f));
            Border(chip, 14.0f, open || IsHover("cov") ? t.accent : t.line);
            RectF bar(chip.left + 10, cy + 2, chip.left + 44, cy - 2);
            Fill(bar, 2.0f, t.field);
            Fill(RectF(bar.left, bar.top, bar.left + bar.Width()*pct, bar.bottom), 2.0f, tone);
            Text(text, RectF(bar.right + 7, chip.top, chip.right - 24, chip.bottom), TextStyle(10.5f, 400, t.fg, HorAlign::Left, true));
            UiWsDraw::Icon(open ? "uiws_chevron_down" : "uiws_chevron_down", Vec2F(chip.right - 10 - 5.5f, cy), 11.0f, t.fg3);
            AddHot(chip, "cov");
        }

        // The tools on the right
        bool editable = mCtx->Editable() && !mCtx->Busy();
        float rx = head.right - 12;
        auto tool = [&](const String& icon, const String& id, bool enabled, float size = 15.0f)
        {
            Tool(RectF(rx - 28, cy + 14, rx, cy - 14), icon, id, false, enabled, size);
            rx -= 28 + 4;
        };
        auto sep = [&]()
        {
            Separator(Vec2F(rx - 6.5f, cy), 18.0f);
            rx -= 13 + 4;
        };
        tool("uiws_close", "close", true, 14.0f);
        tool(mMax ? "uiws_restore" : "uiws_maximize", "max", true);
        sep();
        float dlW = 10 + 13 + 6 + TextWidth("Download", 12.0f) + 10;
        bool canDownload = onDownload && !mCtx->current.root.children.empty();
        Button(RectF(rx - dlW, cy + 14, rx, cy - 14), "Download", "download", false, canDownload, "uiws_download");
        rx -= dlW + 4;
        Segments(RectF(rx - 170, cy + 14, rx, cy - 14), { { "unity", "Unity3D" }, { "o2", "o2" }, { "json", "JSON" } }, mCtx->exportFor, "export:");
        rx -= 170 + 4;
        sep();
        tool("uiws_redo", "redo", editable && !mCtx->redo.IsEmpty());
        tool("uiws_undo", "undo", editable && !mCtx->timeline.GetSteps().IsEmpty());

        // The resize edges and corners, the dividers between the panels: zones of the grips over the panels
        mGrips->zones.Clear();
        float top = frame.top - headHeight, bottom = frame.bottom + 1;
        float lx = frame.left + 1 + mLeft, rx2 = frame.right - 1 - mRight;
        mGrips->zones.Add(Pair<RectF, String>(RectF(lx - 4.5f, top, lx + 4.5f, bottom), "split:left"));
        mGrips->zones.Add(Pair<RectF, String>(RectF(rx2 - 4.5f, top, rx2 + 4.5f, bottom), "split:right"));
        if (!mMax)
        {
            float l = frame.left, r = frame.right, tp = frame.top, b = frame.bottom;
            auto zone = [&](const RectF& z, const char* id) { mGrips->zones.Add(Pair<RectF, String>(z, id)); };
            zone(RectF(l + 10, tp + 3, r - 10, tp - 4), "rz:n");
            zone(RectF(l + 10, b + 4, r - 10, b - 3), "rz:s");
            zone(RectF(r - 4, tp - 10, r + 3, b + 10), "rz:e");
            zone(RectF(l - 3, tp - 10, l + 4, b + 10), "rz:w");
            zone(RectF(r - 11, tp + 3, r + 3, tp - 11), "rz:ne");
            zone(RectF(l - 3, tp + 3, l + 11, tp - 11), "rz:nw");
            zone(RectF(r - 11, b + 11, r + 3, b - 3), "rz:se");
            zone(RectF(l - 3, b + 11, l + 11, b - 3), "rz:sw");
        }
    }

    void UiWsWindow::DrawOverlay(const Box& box)
    {
        if (!mShown)
            return;

        auto& t = T();
        UiWsFrame f = ShownFrame();
        RectF frame = box.R(f.x, f.y, f.w, f.h);
        if (!mMax)
        {
            // The grip in the bottom-right corner
            Vec2F c(frame.right - 9.5f, frame.bottom + 9.5f);
            Line(c + Vec2F(3.5f, 3.5f), c + Vec2F(3.5f, -3.5f), A(t.fg3, 0.6f), 1.5f);
            Line(c + Vec2F(3.5f, -3.5f), c + Vec2F(-3.5f, -3.5f), A(t.fg3, 0.6f), 1.5f);
        }
    }

    // --- input -----------------------------------------------------------------------------------

    void UiWsWindow::OnCursorPressed(const Input::Cursor& cursor)
    {
        UiWsPanel::OnCursorPressed(cursor);
        mDrag = "";
        if (mPressedHot == "head")
            BeginDrag("head", cursor.position);
        else if (mPressedHot == "backdrop")
        {
            mPressedHot = "";
            Close();
        }
    }

    void UiWsWindow::OnCursorStillDown(const Input::Cursor& cursor)
    {
        DragTo(cursor.position);
    }

    void UiWsWindow::BeginDrag(const String& zone, const Vec2F& point)
    {
        mDrag = "";
        if ((zone == "head" || zone.StartsWith("rz:")) && !mMax)
            mDrag = zone;
        else if (zone.StartsWith("split:"))
        {
            mDrag = zone;
            mPanelStart = zone == "split:left" ? mLeft : mRight;
        }
        mDragStart = point;
        mFrameStart = UiWsFrame{ mX, mY, mW, mH };
    }

    void UiWsWindow::DragTo(const Vec2F& point)
    {
        if (mDrag.IsEmpty())
            return;

        Vec2F screen = ScreenSize();
        float dx = point.x - mDragStart.x, dy = -(point.y - mDragStart.y);
        float x = mFrameStart.x, y = mFrameStart.y, w = mFrameStart.w, h = mFrameStart.h;
        if (mDrag == "head")
        {
            mX = Math::Max(120 - w, Math::Min(screen.x - 120, x + dx));
            mY = Math::Max(8.0f, Math::Min(screen.y - 48, y + dy));
            return;
        }

        if (mDrag.StartsWith("split:"))
        {
            float v = Math::Round(mDrag == "split:left" ? mPanelStart + dx : mPanelStart - dx);
            if (mDrag == "split:left")
                mSavedLeft = v;
            else
                mSavedRight = v;
            ClampPanels();
            return;
        }

        String edges = mDrag.SubStr(3);
        if (edges.Contains('e'))
            w = Math::Max(minW, Math::Min(screen.x - x - 8, mFrameStart.w + dx));
        if (edges.Contains('s'))
            h = Math::Max(minH, Math::Min(screen.y - y - 8, mFrameStart.h + dy));
        if (edges.Contains('w'))
        {
            float nw = Math::Max(minW, Math::Min(mFrameStart.x + mFrameStart.w - 8, mFrameStart.w - dx));
            x = mFrameStart.x + mFrameStart.w - nw;
            w = nw;
        }
        if (edges.Contains('n'))
        {
            float nh = Math::Max(minH, Math::Min(mFrameStart.y + mFrameStart.h - 8, mFrameStart.h - dy));
            y = mFrameStart.y + mFrameStart.h - nh;
            h = nh;
        }
        mX = x; mY = y; mW = w; mH = h;
        ClampPanels();
    }

    void UiWsWindow::OnCursorReleased(const Input::Cursor& cursor)
    {
        EndDrag();
        UiWsPanel::OnCursorReleased(cursor);
    }

    void UiWsWindow::EndDrag()
    {
        String drag = mDrag;
        mDrag = "";
        if (drag.StartsWith("rz:"))
            SavePrefs();
        else if (drag.StartsWith("split:"))
        {
            // What is kept is what was shown (clamped)
            if (drag == "split:left")
                mSavedLeft = mLeft;
            else
                mSavedRight = mRight;
            SavePrefs();
        }
    }

    void UiWsWindow::OnHotClick(const String& id, const Input::Cursor& cursor)
    {
        if (id == "close")
            Close();
        else if (id == "max")
            ToggleMaximized();
        else if (id == "undo")
            mCtx->Undo();
        else if (id == "redo")
            mCtx->Redo();
        else if (id == "cov")
            ToggleCoverage();
        else if (id == "download" && onDownload)
            onDownload();
        else if (id.StartsWith("export:"))
            mCtx->SetUi("exportFor", Json(id.SubStr(7)));
    }

    void UiWsWindow::OnHotDoubleClick(const String& id, const Input::Cursor& cursor)
    {
        if (id == "head")
            ToggleMaximized();
        else if (id == "split:left" || id == "split:right")
            SetPanelWidth(id.SubStr(6), 0.0f);
    }

    void UiWsWindow::ToggleCoverage()
    {
        if (mCoveragePopup->IsOpen() || UiWsPopup::ClosedThisFrame())
        {
            mCoveragePopup->Close();
            return;
        }

        RectF anchor = HotRect("cov");
        mCoveragePopup->OpenAt(anchor, 420.0f, Math::Min(440.0f, mCoverage->ContentHeight(420.0f)));
    }

    // --- keys --------------------------------------------------------------------------------------

    bool UiWsWindow::TakesKeys() const
    {
        if (!mShown || !mHasFocus)
            return false;

        auto focused = o2UI.GetFocusedWidget();
        return !focused || focused == Ref(const_cast<UiWsWindow*>(this));
    }

    void UiWsWindow::OnKeyPressed(const Input::Key& key)
    {
        if (!mShown)
            return;

        if (key.keyCode == VK_ESCAPE)
        {
            if (UiWsPopup::AnyOpen() || UiWsPopup::ClosedThisFrame() || !mHasFocus)
                return;
            HandleKey(VK_ESCAPE, false);
            return;
        }

        if (TakesKeys())
            HandleKey(key.keyCode, false);
    }

    void UiWsWindow::OnKeyStayDown(const Input::Key& key)
    {
        if (TakesKeys() && o2Input.IsKeyRepeating(key.keyCode) &&
            (key.keyCode == VK_LEFT || key.keyCode == VK_RIGHT || key.keyCode == VK_UP || key.keyCode == VK_DOWN))
            HandleKey(key.keyCode, true);
    }

    void UiWsWindow::OnKeyReleased(const Input::Key& key)
    {
        if (key.keyCode == VK_SPACE)
            mStage->SetSpaceHeld(false);
    }

    void UiWsWindow::PressKey(int keyCode)
    {
        HandleKey(keyCode, false);
    }

    bool UiWsWindow::HandleKey(int keyCode, bool repeat)
    {
        bool ctrl = o2Input.IsKeyDown(VK_CTRL_CMD) || o2Input.IsKeyDown(VK_CONTROL);
        bool shift = o2Input.IsKeyDown(VK_SHIFT);
        bool alt = o2Input.IsKeyDown(VK_MENU);
        bool editable = mCtx->Editable();
        const String& selected = mCtx->selected;

        if (keyCode == VK_ESCAPE)
        {
            if (!mHierarchy->GetRenaming().IsEmpty())
                mHierarchy->EndRename(false);
            else if (DynamicCast<EditBox>(o2UI.GetFocusedWidget()))
                o2UI.FocusWidget(Ref(this));
            else if (mCtx->peek >= 0)
                mCtx->SetPeek(-1);
            else
                Close();
            return true;
        }

        if (keyCode == VK_SPACE)
        {
            mStage->SetSpaceHeld(true);
            return true;
        }

        if (ctrl && keyCode == VK_Z)
        {
            if (shift)
                mCtx->Redo();
            else
                mCtx->Undo();
            return true;
        }
        if (ctrl && keyCode == VK_Y)
        {
            mCtx->Redo();
            return true;
        }
        if ((keyCode == VK_DELETE || keyCode == VK_BACK) && !selected.IsEmpty() && editable)
        {
            String id = selected;
            mCtx->Op({ UiDoc::MakeRemoveOp({ id }) }, "\xE2\x88\x92 " + id);
            mCtx->Select("");
            return true;
        }
        if (keyCode == VK_F2 && editable)
        {
            mHierarchy->StartRename(selected.IsEmpty() ? mCtx->Doc().root.id : selected);
            return true;
        }
        if (!ctrl && !alt && keyCode == VK_F)
        {
            mStage->Frame(selected);
            return true;
        }

        int dx = keyCode == VK_LEFT ? -1 : keyCode == VK_RIGHT ? 1 : 0;
        int dy = keyCode == VK_UP ? -1 : keyCode == VK_DOWN ? 1 : 0;
        if ((dx || dy) && !selected.IsEmpty() && editable)
        {
            int step = shift ? 10 : 1;
            UiDoc::UiOp op;
            if (UiDoc::NudgeOp(mCtx->Doc(), mCtx->rects, selected, dx*step, dy*step, op))
                mCtx->Op({ op }, selected + ": nudge");
            return true;
        }
        return false;
    }
}
