#include "o2Editor/stdafx.h"
#include "PipelineCheckDlg.h"

#include "o2/Application/Input.h"
#include "o2/Events/KeyboardEventsListener.h"
#include "o2/Render/Render.h"
#include "o2/Render/VectorSprite.h"
#include "o2/Scene/UI/UIManager.h"
#include "o2/Scene/UI/WidgetLayer.h"
#include "o2/Scene/UI/WidgetLayout.h"
#include "o2/Scene/UI/Widgets/Button.h"
#include "o2/Scene/UI/Widgets/HorizontalLayout.h"
#include "o2/Scene/UI/Widgets/Label.h"
#include "o2/Scene/UI/Widgets/ScrollArea.h"
#include "o2/Scene/UI/Widgets/Toggle.h"
#include "o2/Scene/UI/Widgets/VerticalLayout.h"
#include "o2/Scene/UI/Widgets/Window.h"
#include "o2/Utils/Editor/EditorScope.h"
#include "o2/Utils/FileSystem/FileSystem.h"
#include "o2Editor/Pipeline/PipelineExecutor.h"
#include "o2Editor/Pipeline/PipelineExtractCheck.h"
#include "o2Editor/Pipeline/PipelineImageOps.h"
#include "o2Editor/Pipeline/PipelineUtils.h"
#include "o2Editor/Pipeline/UiDoc/UiDocJs.h"
#include "o2Editor/Pipeline/UiDoc/UiStyleCheck.h"
#include "o2Editor/UI/Style/EditorTheme.h"
#include "o2Editor/UI/Style/EditorThemeDark.h"
#include "o2Editor/UIRoot.h"
#include "o2Editor/Windows/PipelineWindow/PipelineEditor.h"
#include "o2Editor/Windows/PipelineWindow/PipelineNodeBody.h"
#include "o2Editor/Windows/PipelineWindow/PipelineNodeBodyFactories.h"
#include "o2Editor/Windows/PipelineWindow/PipelineNodeWidget.h"
#include "o2Editor/Windows/PipelineWindow/PipelineTheme.h"

namespace Editor
{
    using namespace PipelineControls;
    using UiLayout::Json;

    void PipelineCheckRows::Add(PipelineNodeBody& body, const String& switchCaption, const String& limitCaption,
                                const Function<void()>& refresh)
    {
        auto divider = mmake<Widget>();
        divider->name = "divider";
        divider->AddLayer("line", MakeMenuLine(), Layout::HorStretch(VerAlign::Middle, 0, 0, 1, 0));
        body.AddRow(divider, 9);

        WeakRef<PipelineNodeBody> weakBody(&body);
        bool on = PipelineGraph::IsRefCheckOn(*body.mNode);
        auto toggle = MakeCheckbox(switchCaption, on);
        toggle->name = "ref check";
        toggle->onToggleByUser = [weakBody, refresh](bool value)
        {
            if (auto self = weakBody.Lock())
            {
                self->SetBool("refCheck", value, true);
                refresh();
            }
        };
        body.AddRow(toggle, PipelineNodeBody::switchRowHeight);
        if (!on)
            return;

        // At most 1 / 2 / 3 regenerations (or corrections)
        PipelineExtractCheck::Config check;
        PipelineExtractCheck::Read(*body.mNode, check);
        auto block = mmake<Widget>();
        block->name = "ref check max";
        auto segments = mmake<HorizontalLayout>();
        segments->spacing = 2;
        segments->expandWidth = true;
        segments->expandHeight = true;
        *segments->layout = WidgetLayout::BothStretch(3, 3, 3, 3);
        SkinSegmentedRow(*segments);
        for (int n : PipelineExtractCheck::Limits())
        {
            auto segment = MakeSegment("\xE2\x89\xA4 " + (String)n, n == check.limit);
            segment->name = "limit " + (String)n;
            segment->onToggleByUser = [weakBody, refresh, n](bool)
            {
                if (auto self = weakBody.Lock())
                {
                    self->SetNumber("refCheckMax", (float)n, true);
                    refresh();
                }
            };
            segments->AddChild(segment);
        }
        block->AddChild(segments);
        body.AddRow(MakeRow(limitCaption, block, 86.0f), 22);
        body.AddModelRow(PipelineTextModelPresets(), PipelineExtractCheck::defaultModel, PipelineModelKind::Text, "refCheckModel",
                         "Judge model", 86.0f);
    }

    namespace
    {
        // The window is an editor window: its texts take the editor theme, whatever theme the pipeline window has
        Color4 CheckText() { return EditorTheme::IsDark() ? EditorThemeDark::text : Color4(38, 50, 56, 255); }
        Color4 CheckDim() { return EditorTheme::IsDark() ? EditorThemeDark::ink : Color4(96, 125, 139, 255); }
        Color4 CheckOk() { return EditorTheme::IsDark() ? Color4(102, 187, 106, 255) : Color4(46, 125, 50, 255); }
        Color4 CheckWarn() { return EditorTheme::IsDark() ? EditorThemeDark::yellow : Color4(214, 128, 0, 255); }
        Color4 CheckBad() { return EditorTheme::IsDark() ? EditorThemeDark::red : Color4(229, 57, 53, 255); }
        Color4 CheckZoomBack() { return EditorTheme::IsDark() ? EditorThemeDark::panel : Color4(236, 239, 241, 255); }

        String Num(const Json* v, int decimals = -1)
        {
            double value = v ? v->Num() : 0.0;
            if (decimals < 0)
                return UiDoc::JsNum(value);
            char buffer[32];
            snprintf(buffer, sizeof(buffer), "%.*f", decimals, value);
            return buffer;
        }

        String Secs(double ms)
        {
            char buffer[32];
            snprintf(buffer, sizeof(buffer), ms < 10000 ? "%.1f" : "%.0f", ms/1000.0);
            return buffer;
        }

        String Pct(double v)
        {
            char buffer[32];
            snprintf(buffer, sizeof(buffer), v < 0.1 ? "%.1f" : "%.0f", v*100.0);
            return buffer;
        }

        String Str(const Json& o, const char* key) { return o.Get(key) && o.Get(key)->IsString() ? o.Get(key)->Str() : String(); }

        String Verdict(const String& v)
        {
            return v == "ok" ? "matches" : v == "minor" ? "minor differences" : v == "significant" ? "significant differences" : "not judged";
        }

        Color4 VerdictColor(const String& v) { return v == "ok" ? CheckOk() : v == "minor" ? CheckWarn() : v == "significant" ? CheckBad() : CheckDim(); }

        // The bitmap with a 2 px frame round the box, for the reference's region
        Ref<Bitmap> WithFrame(const Ref<Bitmap>& source, const Json* box)
        {
            if (!source || !box)
                return source;

            auto out = EnsureRgba(Ref<Bitmap>(mmake<Bitmap>(*source)));
            Vec2I size = out->GetSize();
            int x0 = (int)box->Get("x")->Num(), y0 = (int)box->Get("y")->Num();
            int x1 = x0 + (int)box->Get("w")->Num() - 1, y1 = y0 + (int)box->Get("h")->Num() - 1;
            Color4 c = PipelineTheme::Get().accent;
            auto put = [&](int x, int y)
            {
                if (x < 0 || y < 0 || x >= size.x || y >= size.y)
                    return;
                UInt8* p = PipelineImageOps::Pixel(*out, x, y);
                p[0] = (UInt8)c.r; p[1] = (UInt8)c.g; p[2] = (UInt8)c.b; p[3] = 255;
            };
            for (int t = 0; t < 2; t++)
            {
                for (int x = x0; x <= x1; x++) { put(x, y0 + t); put(x, y1 - t); }
                for (int y = y0; y <= y1; y++) { put(x0 + t, y); put(x1 - t, y); }
            }
            return out;
        }
    }

    // ---------------------------------------------------------------------------------------------------
    // The popup's content: the head line, the stop line and one row per attempt; follows the debug file
    // ---------------------------------------------------------------------------------------------------
    class PipelineCheckPanel : public Widget, public KeyboardEventsListener
    {
    public:
        WeakRef<PipelineEditor> editor;
        Ref<PipelineNode>       node;
        String                  partId;   // Empty for an AI text style
        String                  partName;
        Function<void()>        onEscape;

    public:
        explicit PipelineCheckPanel(RefCounter* refCounter): Widget(refCounter)
        {
            mHead = MakeLabel("", false);
            mHead->name = "check head";
            mHead->horOverflow = Label::HorOverflow::Dots;
            mHead->SetColor(CheckText());
            *mHead->layout = WidgetLayout::HorStretch(VerAlign::Top, 0, 0, 20, 0);
            AddChild(mHead);

            mScroll = o2UI.CreateScrollArea();
            mScroll->name = "check attempts";
            *mScroll->layout = WidgetLayout::BothStretch(0, 0, 0, 26);
            AddChild(mScroll);
            mList = mmake<VerticalLayout>();
            mList->spacing = 12;
            mList->expandWidth = true;
            mList->expandHeight = false;
            mList->fitByChildren = true;
            mList->baseCorner = BaseCorner::Top;
            *mList->layout = WidgetLayout::HorStretch(VerAlign::Top, 0, 14, 0, 0);
            mScroll->AddChild(mList);

            mZoom = mmake<Widget>();
            mZoom->name = "check zoom";
            mZoom->AddLayer("back", mmake<Sprite>(CheckZoomBack()), Layout::BothStretch());
            *mZoom->layout = WidgetLayout::BothStretch();
            AddChild(mZoom);
            auto zoomScroll = o2UI.CreateScrollArea();
            *zoomScroll->layout = WidgetLayout::BothStretch(0, 0, 0, 26);
            mZoom->AddChild(zoomScroll);
            mZoomImage = mmake<PipelineImageView>();
            zoomScroll->AddChild(mZoomImage);
            mZoomTitle = MakeLabel("", true);
            mZoomTitle->SetColor(CheckText());
            *mZoomTitle->layout = WidgetLayout::HorStretch(VerAlign::Top, 90, 0, 22, 0);
            mZoom->AddChild(mZoomTitle);
            auto back = MakeButton("Back");
            back->name = "close zoom";
            *back->layout = WidgetLayout(Vec2F(0, 1), Vec2F(0, 1), Vec2F(0, -22), Vec2F(80, 0));
            back->onClick = [this]() { ShowZoom(nullptr, ""); };
            mZoom->AddChild(back);
            mZoom->SetEnabledForcible(false);
        }

        // Binds the panel to a node's check and reads it
        void Bind(const Ref<PipelineEditor>& e, const Ref<PipelineNode>& n, const String& part, const String& name)
        {
            editor = e;
            node = n;
            partId = part;
            partName = name;
            mStamp = "";
            ShowZoom(nullptr, "");
            Reload(true);
        }

        // Reads the debug file when it changed since the last read; force reads it anyway
        void Reload(bool force)
        {
            auto e = editor.Lock();
            if (!e || !node)
                return;

            mFolder = PipelineExtractCheck::CheckFolder(PipelineExecutor::GetWorkPath(e->GetPipelineId(), node->id));
            String path = mFolder + (partId.IsEmpty() ? String(UiDoc::STYLE_CHECK_FILE) : PipelineExtractCheck::PartFile(partId)) + ".json";
            String stamp = PipelineUtils::FileSignature(path) + (IsRunning() ? ":running" : "");
            if (!force && stamp == mStamp)
                return;

            mStamp = stamp;
            mData = Json();
            mLoaded = o2FileSystem.IsFileExist(path) && Json::Parse(PipelineUtils::ReadFileBytes(path), mData) && mData.IsObject();
            Rebuild();
        }

        void Update(float dt) override
        {
            Widget::Update(dt);
            mPoll += dt;
            if (mPoll >= 0.5f && IsEnabledInHierarchy())
            {
                mPoll = 0.0f;
                Reload(false);
            }
        }

        // Escape with the cursor over the window closes the zoomed picture, then the window
        void OnKeyPressed(const Input::Key& key) override
        {
            if (key.keyCode != VK_ESCAPE || !IsEnabledInHierarchy())
                return;

            auto parent = GetParentWidget().Lock();
            RectF area = parent ? parent->layout->GetWorldRect() : layout->GetWorldRect();
            if (!area.IsInside(o2Input.GetCursorPos()))
                return;

            if (mZoom->IsEnabled())
                ShowZoom(nullptr, "");
            else if (onEscape)
                onEscape();
        }

        String GetStopLine() const { return mStopLine; }

        int GetAttemptCount() const { return mAttempts; }

    private:
        Ref<Label>             mHead;
        Ref<ScrollArea>        mScroll;
        Ref<VerticalLayout>    mList;
        Ref<Widget>            mZoom;
        Ref<PipelineImageView> mZoomImage;
        Ref<Label>             mZoomTitle;

        String mFolder;
        String mStamp;
        Json   mData;
        bool   mLoaded = false;
        float  mPoll = 0.0f;
        String mStopLine;
        int    mAttempts = 0;

        bool IsStyle() const { return partId.IsEmpty(); }

        // The part or the node is being run now: the file is still being written
        bool IsRunning() const
        {
            auto e = editor.Lock();
            auto widget = e && node ? e->GetNodeWidget(node->id) : nullptr;
            if (!widget)
                return false;

            auto& rt = widget->GetRuntime();
            String state;
            if (!partId.IsEmpty() && rt.portStates.TryGetValue(partId, state))
                return state == "running";
            return rt.state == "running";
        }

        void ShowZoom(const Ref<Bitmap>& bitmap, const String& title)
        {
            if (!bitmap)
            {
                mZoom->SetEnabledForcible(false);
                return;
            }

            Vec2I size = bitmap->GetSize();
            mZoomImage->SetBitmap(bitmap);
            *mZoomImage->layout = WidgetLayout(Vec2F(0, 1), Vec2F(0, 1), Vec2F(0, -(float)size.y), Vec2F((float)size.x, 0));
            mZoomTitle->text = title;
            mZoom->SetEnabledForcible(true);
        }

        Ref<Bitmap> Picture(const String& file) const
        {
            return file.IsEmpty() ? nullptr : DecodeImageBytes(PipelineUtils::ReadFileBytes(mFolder + file));
        }

        void AddLine(const String& text, const Color4& color, float width, bool wrap = true)
        {
            auto label = MakeLabel(text, wrap);
            label->SetColor(color);
            label->horOverflow = wrap ? Label::HorOverflow::Wrap : Label::HorOverflow::Dots;
            label->verAlign = VerAlign::Top;
            int perLine = Math::Max(20, (int)(width/6.4f));
            int lines = 0;
            for (auto& paragraph : text.Split("\n"))
                lines += Math::Max(1, (paragraph.Length() + perLine - 1)/perLine);
            float height = (wrap ? lines : 1)*16.0f + 2.0f;
            label->layout->minHeight = label->layout->maxHeight = height;
            mList->AddChild(label);
        }

        String StopText(bool live) const
        {
            if (!mLoaded || mData.Get("missing") || live)
                return "";

            String stop = Str(mData, "stop"), note = Str(mData, "note");
            String limit = Num(mData.Get("limit")), kept = Num(mData.Get("kept"));
            if (IsStyle())
            {
                if (stop == "ok") return "The sample looks like the lettering in the picture.";
                if (stop == "minor") return "Small differences only: the style is accepted.";
                if (stop == "limit") return "Still different after " + limit + " correction(s): attempt " + kept + ", the closest one, is kept.";
            }
            else
            {
                if (stop == "ok") return "Accepted: the sprite matches the reference.";
                if (stop == "minor") return "Accepted: only minor differences.";
                if (stop == "limit") return "Still different after " + limit + " regeneration(s) \xE2\x80\x94 the limit. Attempt " + kept +
                    " is the closest and was kept.";
                if (stop == "skipped")
                {
                    String why = Str(mData, "skipped");
                    why = why == "opaque" ? String("an opaque part has no cut-out to lay over the reference") :
                        why == "plate" ? String("a background plate has everything on it removed on purpose") :
                        why == "empty" ? String("the sprite is empty") : note;
                    return "Not compared: " + why + ".";
                }
            }
            return "The check could not finish: " + note + ". Nothing was stored for it \xE2\x80\x94 the next run tries again.";
        }

        void Rebuild()
        {
            PushEditorScopeOnStack scope;
            mList->RemoveAllChildren();
            mStopLine = "";
            mAttempts = 0;
            float width = Math::Max(300.0f, mScroll->layout->GetWidth() - 16.0f);
            bool live = IsRunning() || (mLoaded && mData.Get("running") && mData.Get("running")->Bool());

            // The head: what is checked, its calls and time
            String head = IsStyle() ? String("Check against the picture") : String("Check against the reference");
            if (IsStyle() && mLoaded && !Str(mData, "text").IsEmpty())
                head += "  \xC2\xB7  \xC2\xAB" + Str(mData, "text") + "\xC2\xBB";
            else if (!IsStyle())
                head += "  \xC2\xB7  " + partName;
            if (live)
                head += "  \xC2\xB7  checking\xE2\x80\xA6";
            if (mLoaded && mData.Get("calls"))
            {
                auto calls = mData.Get("calls");
                head += IsStyle() ? "  \xC2\xB7  check calls: " + Num(calls->Get("judge")) :
                    "  \xC2\xB7  judge calls: " + Num(calls->Get("judge")) + " \xC2\xB7 regenerations: " + Num(calls->Get("draw"));
                head += " \xC2\xB7 " + Secs(mData.Get("ms") ? mData.Get("ms")->Num() : 0) + " s";
            }
            mHead->text = head;

            if (!mLoaded)
            {
                AddLine(live ? String("checking\xE2\x80\xA6") : IsStyle() ?
                        String("This style has not been checked yet. Run the node (or \xE2\x86\xBB) with the check on.") :
                        String("This part has not been checked yet. Run it (or \xE2\x86\xBB it) with the check switched on."), CheckDim(), width);
                return;
            }
            if (mData.Get("missing"))
            {
                AddLine("This result came from the cache, and the details of its check are no longer kept. \xE2\x86\xBB on the part checks it again.",
                        CheckDim(), width);
                return;
            }

            mStopLine = StopText(live);
            auto attempts = mData.Get("attempts");
            int count = attempts ? attempts->Count() : 0;
            int kept = (int)(mData.Get("kept") ? mData.Get("kept")->Num() : 1);
            String stop = Str(mData, "stop");
            if (!mStopLine.IsEmpty())
            {
                String line = mStopLine;
                if (count > 1 && kept != count && stop != "limit")
                    line += " Attempt " + (String)kept + " was kept: the closest to the " + (IsStyle() ? "picture" : "reference") +
                        " by the judge's likeness.";
                AddLine(line, stop == "ok" || stop == "minor" ? CheckOk() : stop == "failed" ? CheckBad() : CheckWarn(), width);
            }

            for (int i = 0; i < count; i++)
                AddAttempt(*attempts->At(i), kept, live, stop, width);
            mAttempts = count;
        }

        void AddAttempt(const Json& a, int kept, bool live, const String& stop, float width)
        {
            int n = (int)(a.Get("n") ? a.Get("n")->Num() : 1);
            String verdict = Str(a, "verdict");
            bool isKept = !live && kept == n && stop != "skipped";

            String head = "Attempt " + (String)n + "  \xC2\xB7  " + (n == 1 ? (IsStyle() ? String("the style as read") : String("the part's own generation")) :
                (IsStyle() ? "correction " : "regeneration ") + (String)(n - 1)) + "  \xC2\xB7  " + Verdict(verdict);
            if (a.Get("likeness"))
                head += "  \xC2\xB7  likeness " + Num(a.Get("likeness"), 2);
            if (!IsStyle() && a.Get("pixelDiff"))
                head += "  \xC2\xB7  pixels " + Pct(a.Get("pixelDiff")->Num()) + "% \xC2\xB7 " + Pct(a.Get("pixelChanged") ? a.Get("pixelChanged")->Num() : 0) +
                    "% changed";
            if (isKept)
                head += "  \xC2\xB7  kept";
            auto title = MakeLabel(head, false);
            title->name = "attempt head";
            title->SetColor(VerdictColor(verdict));
            title->horOverflow = Label::HorOverflow::Dots;
            title->layout->minHeight = title->layout->maxHeight = 18;
            mList->AddChild(title);

            // The pictures side by side, the words beside them
            struct Figure { String file, caption; Ref<Bitmap> bitmap; };
            Vector<Figure> figures;
            if (IsStyle())
            {
                figures.Add({ Str(mData, "reference"), "Picture", Picture(Str(mData, "reference")) });
                figures.Add({ Str(a, "render"), "Sample in this style", Picture(Str(a, "render")) });
            }
            else
            {
                figures.Add({ Str(mData, "reference"), "Reference", WithFrame(Picture(Str(mData, "reference")), mData.Get("regionInReference")) });
                figures.Add({ Str(a, "overlay"), "Sprite over the reference", Picture(Str(a, "overlay")) });
                auto size = a.Get("spriteSize");
                String dims = size ? "  " + Num(size->Get("w")) + "\xC3\x97" + Num(size->Get("h")) : String();
                figures.Add({ Str(a, "sprite"), "Sprite" + dims, Picture(Str(a, "sprite")) });
            }

            const float box = 170.0f, gap = 8.0f;
            float textLeft = (box + gap)*figures.Count() + 4.0f, textWidth = Math::Max(160.0f, width - textLeft);
            String text = AttemptText(a);
            int perLine = Math::Max(20, (int)(textWidth/6.4f));
            int lines = 0;
            for (auto& paragraph : text.Split("\n"))
                lines += Math::Max(1, (paragraph.Length() + perLine - 1)/perLine);
            float height = Math::Max(box + 20.0f, lines*16.0f + 4.0f);

            auto row = mmake<Widget>();
            row->name = isKept ? "attempt kept" : "attempt";
            if (isKept)
                AddBorderLayer(*row, "kept", PipelineTheme::Alpha(CheckOk(), 0.8f), 8.0f, 1.0f, Layout::BothStretch(-4, -4, -4, -4));
            row->layout->minHeight = row->layout->maxHeight = height;
            float x = 0.0f;
            for (auto& figure : figures)
            {
                Vec2I size = figure.bitmap ? figure.bitmap->GetSize() : Vec2I(1, 1);
                float k = Math::Min(box/Math::Max(1, size.x), box/Math::Max(1, size.y));
                float w = Math::Max(24.0f, size.x*k), h = Math::Max(16.0f, size.y*k);
                auto view = mmake<PipelineImageView>();
                view->name = "attempt picture";
                view->SetBitmap(figure.bitmap);
                view->SetFramed(true);
                *view->layout = WidgetLayout(Vec2F(0, 1), Vec2F(0, 1), Vec2F(x + (box - w)*0.5f, -h), Vec2F(x + (box + w)*0.5f, 0));
                row->AddChild(view);

                auto open = mmake<Button>();
                open->name = "zoom picture";
                *open->layout = *view->layout;
                Ref<Bitmap> bitmap = figure.bitmap;
                String caption = figure.caption + "  \xC2\xB7  attempt " + (String)n;
                open->onClick = [this, bitmap, caption]() { ShowZoom(bitmap, caption); };
                row->AddChild(open);

                auto label = MakeLabel(figure.caption, true);
                label->SetColor(CheckDim());
                label->horOverflow = Label::HorOverflow::Dots;
                label->horAlign = HorAlign::Middle;
                *label->layout = WidgetLayout(Vec2F(0, 1), Vec2F(0, 1), Vec2F(x, -(box + 18.0f)), Vec2F(x + box, -box - 2.0f));
                row->AddChild(label);
                x += box + gap;
            }

            auto words = MakeLabel(text, true);
            words->name = "attempt text";
            words->SetColor(a.Get("error") ? CheckBad() : CheckText());
            words->horOverflow = Label::HorOverflow::Wrap;
            words->verAlign = VerAlign::Top;
            *words->layout = WidgetLayout(Vec2F(0, 0), Vec2F(1, 1), Vec2F(textLeft, 0), Vec2F(0, 0));
            row->AddChild(words);
            mList->AddChild(row);
        }

        // The words of an attempt: the judge's differences, what it asked or changed, then how it was made
        String AttemptText(const Json& a) const
        {
            Vector<String> lines;
            if (a.Get("error"))
                lines.Add(Str(a, "error"));
            else
            {
                lines.Add("Differences");
                auto differences = UiDoc::JsonToStrings(a.Get("differences"));
                if (differences.IsEmpty())
                    lines.Add("No differences found.");
                for (auto& d : differences)
                    lines.Add("\xE2\x80\xA2 " + d);
            }

            if (IsStyle())
            {
                auto changes = UiDoc::JsonToStrings(a.Get("changes"));
                if (!changes.IsEmpty())
                {
                    lines.Add("Changed for the next attempt");
                    lines.Add(changes);
                }
                if (auto style = a.Get("style"))
                {
                    String look = Str(*style, "font") + " " + Num(style->Get("weight")) + " \xC2\xB7 " + Num(style->Get("size")) + " px";
                    lines.Add(look);
                }
                lines.Add("judged by " + Str(a, "judge") + " \xC2\xB7 check " + Secs(a.Get("judgeMs") ? a.Get("judgeMs")->Num() : 0) + " s" +
                          (a.Get("replayed") ? " \xC2\xB7 replayed from an earlier identical call" : ""));
                return UiDoc::Join(lines, "\n");
            }

            if (!Str(a, "instructions").IsEmpty())
            {
                lines.Add("What the judge asked of the next attempt");
                lines.Add(Str(a, "instructions"));
            }
            if (!Str(a, "clarifications").IsEmpty())
            {
                lines.Add("Added to this attempt's prompt");
                lines.Add(Str(a, "clarifications"));
            }
            if (auto p = a.Get("placement"))
            {
                String place = "laid at " + Num(p->Get("x"), 0) + ", " + Num(p->Get("y"), 0) + " \xC2\xB7 " + Num(p->Get("w"), 0) + "\xC3\x97" +
                    Num(p->Get("h"), 0) + " px of the source";
                if (auto fit = p->Get("fit"))
                    place += " \xC2\xB7 registered: \xC3\x97" + Num(fit->Get("scale"), 3) + ", shift " + Num(fit->Get("dx"), 1) + ", " +
                        Num(fit->Get("dy"), 1) + " px";
                if (Str(*p, "from") == "centred")
                    place += " \xC2\xB7 its place in the model's frame was not kept \xE2\x80\x94 centred, then registered";
                lines.Add(place);
            }
            lines.Add("drawn by " + Str(a, "model") + " \xC2\xB7 judged by " + Str(a, "judge"));
            lines.Add("drawing " + Secs(a.Get("drawMs") ? a.Get("drawMs")->Num() : 0) + " s \xC2\xB7 judging " +
                      Secs(a.Get("judgeMs") ? a.Get("judgeMs")->Num() : 0) + " s" +
                      (a.Get("replayed") ? " \xC2\xB7 replayed from an earlier identical call" : ""));
            return UiDoc::Join(lines, "\n");
        }
    };

    namespace PipelineCheckDlg
    {
        // Never destroyed: the window must not outlive the UI at exit
        struct State
        {
            Ref<Window>             window;
            Ref<PipelineCheckPanel> panel;
            bool                    shown = false;
        };

        static State& St()
        {
            static State* state = new State();
            return *state;
        }

        static void Create()
        {
            PushEditorScopeOnStack scope;
            auto& st = St();
            st.window = o2UI.CreateWindow("Check against the reference");
            st.window->name = "check details";
            st.window->SetIcon(mmake<VectorSprite>("ui/pipeline/window_icon.svg"));
            st.window->SetIconLayout(Layout::Based(BaseCorner::LeftTop, Vec2F(20, 20), Vec2F(-1, 1)));
            Vec2F screen = (Vec2F)o2Render.GetResolution();
            *st.window->layout = WidgetLayout::Based(BaseCorner::Center, Vec2F(Math::Min(980.0f, screen.x - 40.0f), Math::Min(680.0f, screen.y - 40.0f)));
            st.window->SetClippingLayout(Layout::BothStretch(-1, 0, 0, 17));
            st.window->SetViewLayout(Layout::BothStretch(0, 5, 0, 18));

            st.panel = mmake<PipelineCheckPanel>();
            st.panel->name = "check panel";
            *st.panel->layout = WidgetLayout::BothStretch(10, 8, 10, 8);
            st.window->AddChild(st.panel);
            st.window->onClosed = []() { St().shown = false; };
            st.panel->onEscape = []()
            {
                auto& st = St();
                st.window->Hide();
                st.shown = false;
            };
            EditorUIRoot.AddWidget(st.window);
            st.window->Hide(true);
        }

        void Show(const Ref<PipelineEditor>& editor, const Ref<PipelineNode>& node, const String& partId, const String& partName)
        {
            auto& st = St();
            if (!st.window)
                Create();

            PushEditorScopeOnStack scope;
            st.window->SetCaption((WString)String(partId.IsEmpty() ? "Check against the picture" : "Check against the reference"));
            st.panel->Bind(editor, node, partId, partName);
            if (!st.window->GetParent())
                EditorUIRoot.AddWidget(st.window);
            if (!st.shown)
            {
                st.window->Show();
                st.shown = true;
            }
        }

        void Hide()
        {
            auto& st = St();
            if (!st.window || !st.shown)
                return;

            st.shown = false;
            st.window->Hide();
        }

        bool IsShownFor(const String& nodeId, const String& partId)
        {
            auto& st = St();
            return st.window && st.panel && st.shown && st.panel->node && st.panel->node->id == nodeId && st.panel->partId == partId;
        }

        void Refresh()
        {
            if (St().panel)
                St().panel->Reload(true);
        }

        String GetStopLine()
        {
            return St().panel ? St().panel->GetStopLine() : String();
        }

        int GetAttemptCount()
        {
            return St().panel ? St().panel->GetAttemptCount() : 0;
        }

        Ref<Window> GetWindow()
        {
            return St().window;
        }

        void Reset()
        {
            auto& st = St();
            if (st.window)
            {
                st.window->Hide(true);
                if (auto parent = st.window->GetParentWidget().Lock())
                    parent->RemoveChild(st.window);
                else
                    EditorUIRoot.RemoveWidget(st.window);
            }
            st.window = nullptr;
            st.panel = nullptr;
            st.shown = false;
        }
    }
}
