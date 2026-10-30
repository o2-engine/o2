#include "o2Editor/stdafx.h"
#include "ImageToUiDebugDlg.h"

#include "o2/Render/Render.h"
#include "o2/Render/Text.h"
#include "o2/Render/VectorSprite.h"
#include "o2/Scene/UI/UIManager.h"
#include "o2/Scene/UI/WidgetLayer.h"
#include "o2/Scene/UI/WidgetLayout.h"
#include "o2/Scene/UI/Widgets/Button.h"
#include "o2/Scene/UI/Widgets/Label.h"
#include "o2/Scene/UI/Widgets/ScrollArea.h"
#include "o2/Scene/UI/Widgets/VerticalLayout.h"
#include "o2/Scene/UI/Widgets/Window.h"
#include "o2/Utils/Bitmap/Bitmap.h"
#include "o2/Utils/Editor/EditorScope.h"
#include "o2/Utils/FileSystem/FileSystem.h"
#include "o2Editor/Pipeline/PipelineExecutor.h"
#include "o2Editor/Pipeline/PipelineImageToUi.h"
#include "o2Editor/Pipeline/PipelineUtils.h"
#include "o2Editor/Pipeline/PipelineValue.h"
#include "o2Editor/Pipeline/UiLayout/UiLayoutJson.h"
#include "o2Editor/Pipeline/UiLayout/UiLayoutPixels.h"
#include "o2Editor/UIRoot.h"
#include "o2Editor/Windows/PipelineWindow/PipelineControls.h"
#include "o2Editor/Windows/PipelineWindow/PipelineEditor.h"
#include "o2Editor/Windows/PipelineWindow/PipelinePairViews.h"

namespace Editor
{
    using namespace PipelineControls;
    using UiLayout::Json;

    namespace
    {
        const Color4 titleColor(38, 50, 56, 255);
        const Color4 warnColor(214, 128, 0, 255);
        const Color4 errorColor(229, 57, 53, 255);
        const Color4 selectedBack(96, 125, 139, 40);

        const char* kindOrder[] = { "input", "analysis", "sprite", "background", "measure", "prepare", "request", "view", "write", "check",
                                    "target", "preview", "finalize", "agent", "output" };

        // Steps whose pictures are evidence to read at full width, not thumbnails
        bool IsWide(const String& kind)
        {
            return kind == "prepare" || kind == "measure" || kind == "check" || kind == "target" || kind == "preview" ||
                kind == "finalize" || kind == "view" || kind == "request";
        }

        Color4 KindColor(const String& kind)
        {
            if (kind == "analysis") return Color4(126, 87, 194, 255);
            if (kind == "sprite") return Color4(0, 150, 136, 255);
            if (kind == "measure") return Color4(66, 133, 244, 255);
            if (kind == "background") return Color4(141, 110, 99, 255);
            if (kind == "prepare") return Color4(3, 155, 229, 255);
            if (kind == "view") return Color4(120, 144, 156, 255);
            if (kind == "write") return Color4(255, 160, 0, 255);
            if (kind == "check") return Color4(245, 124, 0, 255);
            if (kind == "target") return Color4(216, 27, 96, 255);
            if (kind == "preview") return Color4(0, 172, 193, 255);
            if (kind == "finalize") return Color4(46, 125, 50, 255);
            if (kind == "agent") return Color4(94, 53, 177, 255);
            if (kind == "output") return Color4(67, 160, 71, 255);
            if (kind == "request") return Color4(229, 57, 53, 255);
            return Color4(144, 164, 174, 255);
        }

        struct TraceImage
        {
            String title;
            String file;
            int    w = 0;
            int    h = 0;
        };

        struct TraceStep
        {
            String             id;
            String             title;
            String             kind;
            String             note;
            String             status;
            String             text;
            String             data;
            String             action; // An agent turn's action, one line of JSON
            double             ms = -1.0;
            Vector<TraceImage> images;
        };

        struct TraceFile
        {
            Vector<TraceStep> steps;
            String            designFile;
            Json              summary;
            bool              finished = false;
        };

        String Ms(double ms)
        {
            if (ms < 0)
                return String();
            if (ms < 1000)
                return (String)(int)ms + " ms";
            char buffer[32];
            snprintf(buffer, sizeof(buffer), "%.1f s", ms/1000.0);
            return buffer;
        }

        String Clip(const String& text, int limit)
        {
            if (text.Length() <= limit)
                return text;
            return text.SubStr(0, limit) + "\n... (" + (String)(text.Length() - limit) + " more characters)";
        }

        // The model's raw answer pretty-printed when it is JSON, fenced or not
        String Pretty(const String& text)
        {
            String raw = text.Trimed(" \t\r\n");
            if (raw.StartsWith("```"))
            {
                int line = raw.Find("\n");
                raw = line >= 0 ? raw.SubStr(line + 1) : String();
                if (UiLayout::HasSuffix(raw, "```"))
                    raw = raw.SubStr(0, raw.Length() - 3);
            }

            Json json;
            return Json::Parse(raw, json) ? json.Dump(2) : text;
        }

        bool ReadTrace(const String& path, TraceFile& out)
        {
            if (!o2FileSystem.IsFileExist(path))
                return false;

            Json json;
            if (!Json::Parse(PipelineUtils::ReadFileBytes(path), json) || !json.IsObject())
                return false;

            out = TraceFile();
            if (auto design = json.Get("design"))
                out.designFile = design->Get("file") ? design->Get("file")->Str() : String();
            if (auto summary = json.Get("summary"))
                out.summary = *summary;
            out.finished = json.Has("finishedAt");

            if (auto steps = json.Get("steps"))
            {
                for (auto& s : steps->Items())
                {
                    TraceStep step;
                    auto str = [&](const char* key) { auto v = s.Get(key); return v ? v->ToJsString() : String(); };
                    step.id = str("id");
                    step.title = str("title");
                    step.kind = str("kind");
                    step.note = s.Has("note") ? str("note") : String();
                    step.status = s.Has("status") ? str("status") : String();
                    step.text = s.Get("text") && s.Get("text")->IsString() ? s.Get("text")->Str() : String();
                    if (s.Get("ms") && s.Get("ms")->IsNumber())
                        step.ms = s.Get("ms")->Num();
                    if (auto data = s.Get("data"))
                    {
                        step.data = data->Dump(2);
                        if (auto action = data->Get("action"); action && !action->IsNull())
                            step.action = action->Dump();
                    }
                    if (auto images = s.Get("images"))
                    {
                        for (auto& im : images->Items())
                        {
                            TraceImage image;
                            image.title = im.Get("title") ? im.Get("title")->ToJsString() : String();
                            image.file = im.Get("file") ? im.Get("file")->ToJsString() : String();
                            image.w = im.Get("w") ? (int)im.Get("w")->Num() : 0;
                            image.h = im.Get("h") ? (int)im.Get("h")->Num() : 0;
                            step.images.Add(image);
                        }
                    }
                    out.steps.Add(step);
                }
            }
            return true;
        }

        // An agent turn: "turn-3", or "edit2-turn-3" in a round of edits
        bool IsAgentTurn(const String& id)
        {
            if (id.StartsWith("turn-"))
                return true;
            if (!id.StartsWith("edit"))
                return false;

            int i = 4;
            while (i < id.Length() && id[i] >= '0' && id[i] <= '9')
                i++;
            return i > 4 && id.SubStr(i).StartsWith("-turn-");
        }

        Ref<Text> MakeText(const String& text, const Color4& color, HorAlign align)
        {
            auto t = mmake<Text>("stdFont.ttf");
            t->text = text;
            t->horAlign = align;
            t->verAlign = VerAlign::Middle;
            t->dotsEngings = true;
            t->color = color;
            return t;
        }
    }

    // -----------------------------------------------------------------------------------------------
    // The window's content: the run summary on top, the steps on the left, the selected step on the
    // right; follows the trace file while it changes
    // -----------------------------------------------------------------------------------------------
    class ImageToUiDebugPanel : public Widget
    {
    public:
        WeakRef<PipelineEditor> editor;
        Ref<PipelineNode>       node;
        Function<void()>        onClose;
        bool                    running = false;

    public:
        explicit ImageToUiDebugPanel(RefCounter* refCounter): Widget(refCounter)
        {
            mHeader = MakeLabel("", false);
            mHeader->name = "debug summary";
            mHeader->horOverflow = Label::HorOverflow::Dots;
            mHeader->SetColor(titleColor);
            *mHeader->layout = WidgetLayout::HorStretch(VerAlign::Top, 0, 0, 20, 0);
            AddChild(mHeader);

            mKinds = MakeLabel("", true);
            mKinds->name = "debug kinds";
            mKinds->horOverflow = Label::HorOverflow::Dots;
            *mKinds->layout = WidgetLayout(Vec2F(0, 1), Vec2F(0, 1), Vec2F(0, -44), Vec2F(listWidth, -24));
            AddChild(mKinds);

            mStepsScroll = o2UI.CreateScrollArea();
            mStepsScroll->name = "debug steps";
            *mStepsScroll->layout = WidgetLayout(Vec2F(0, 0), Vec2F(0, 1), Vec2F(0, 0), Vec2F(listWidth, -48));
            AddChild(mStepsScroll);
            mSteps = mmake<VerticalLayout>();
            mSteps->spacing = 1;
            mSteps->expandWidth = true;
            mSteps->expandHeight = false;
            mSteps->fitByChildren = true;
            mSteps->baseCorner = BaseCorner::Top;
            *mSteps->layout = WidgetLayout::HorStretch(VerAlign::Top, 0, 12, 0, 0);
            mStepsScroll->AddChild(mSteps);

            mViewScroll = o2UI.CreateScrollArea();
            mViewScroll->name = "debug step view";
            *mViewScroll->layout = WidgetLayout(Vec2F(0, 0), Vec2F(1, 1), Vec2F(listWidth + 10, 0), Vec2F(0, -24));
            AddChild(mViewScroll);
            mView = mmake<VerticalLayout>();
            mView->spacing = 6;
            mView->expandWidth = true;
            mView->expandHeight = false;
            mView->fitByChildren = true;
            mView->baseCorner = BaseCorner::Top;
            *mView->layout = WidgetLayout::HorStretch(VerAlign::Top, 0, 14, 0, 0);
            mViewScroll->AddChild(mView);

            mZoom = mmake<Widget>();
            mZoom->name = "debug zoom";
            mZoom->AddLayer("back", mmake<Sprite>(Color4(236, 239, 241, 255)), Layout::BothStretch());
            *mZoom->layout = WidgetLayout::BothStretch();
            AddChild(mZoom);
            mZoomScroll = o2UI.CreateScrollArea();
            *mZoomScroll->layout = WidgetLayout::BothStretch(0, 0, 0, 26);
            mZoom->AddChild(mZoomScroll);
            mZoomImage = mmake<PipelineImageView>();
            mZoomScroll->AddChild(mZoomImage);
            mZoomTitle = MakeLabel("", true);
            mZoomTitle->horOverflow = Label::HorOverflow::Dots;
            *mZoomTitle->layout = WidgetLayout::HorStretch(VerAlign::Top, 90, 0, 22, 0);
            mZoom->AddChild(mZoomTitle);
            auto back = MakeButton("Back");
            back->name = "close zoom";
            *back->layout = WidgetLayout(Vec2F(0, 1), Vec2F(0, 1), Vec2F(0, -22), Vec2F(80, 0));
            back->onClick = [this]() { ShowZoom(-1); };
            mZoom->AddChild(back);
            mZoom->SetEnabledForcible(false);
        }

        // Binds the panel to a node and reads its trace
        void Bind(const Ref<PipelineEditor>& e, const Ref<PipelineNode>& n)
        {
            bool same = node && n && node->id == n->id;
            editor = e;
            node = n;
            if (!same)
            {
                mStamp = "";
                mSelected = n ? n->GetConfigString("debugStep", "") : String();
                ShowZoom(-1);
            }
            Reload(true);
        }

        // Reads the trace when it changed since the last read; force reads it anyway
        void Reload(bool force)
        {
            auto e = editor.Lock();
            if (!e || !node)
                return;

            // The trace of the node's current state: the run, or the run and its rounds of edits as the config names them (a
            // result restored from the cache does not rewrite the live one); the pictures of every state are in trace/
            String work = PipelineExecutor::GetWorkPath(e->GetPipelineId(), node->id);
            auto graph = e->GetGraph();
            auto live = graph ? graph->FindNode(node->id) : nullptr;
            mDir = work + "trace/";
            String path = PipelineImageToUi::StateTracePath(work, PipelineImageToUi::Edits(live ? *live : *node));
            String stamp = path + ":";
            if (o2FileSystem.IsFileExist(path))
            {
                auto info = o2FileSystem.GetFileInfo(path);
                stamp += (String)(int)info.size + ":" + (String)info.editDate.mHour + ":" + (String)info.editDate.mMinute + ":" +
                    (String)info.editDate.mSecond;
            }
            if (!force && stamp == mStamp)
                return;

            mStamp = stamp;
            mBitmaps.Clear();
            TraceFile trace;
            mHasTrace = ReadTrace(path, trace);
            mTrace = trace;
            RebuildSteps();
            RebuildView();
            RefreshHeader();
        }

        // Selects a step and shows it
        void Select(const String& id)
        {
            mSelected = id;
            if (node && node->GetConfigString("debugStep", "") != id)
            {
                node->SetConfigString("debugStep", id);
                if (auto e = editor.Lock())
                {
                    if (auto widget = e->GetNodeWidget(node->id))
                        e->OnNodeConfigChanged(widget, "debugStep", true);
                }
            }
            RebuildSteps();
            RebuildView();
        }

        // Opens the image of the selected step at 1:1; -1 closes it
        void ShowZoom(int index)
        {
            const TraceStep* step = SelectedStep();
            if (index < 0 || !step || index >= step->images.Count())
            {
                mZoom->SetEnabledForcible(false);
                return;
            }

            auto& image = step->images[index];
            mZoomImage->SetBitmap(BitmapOf(image.file));
            *mZoomImage->layout = WidgetLayout(Vec2F(0, 1), Vec2F(0, 1), Vec2F(0, (float)-image.h), Vec2F((float)image.w, 0));
            mZoomTitle->text = image.title + " - " + (String)image.w + "\xC3\x97" + (String)image.h;
            mZoom->SetEnabledForcible(true);
        }

        // Returns the ids of the listed steps
        Vector<String> GetStepIds() const
        {
            Vector<String> ids;
            for (auto& step : mTrace.steps)
                ids.Add(step.id);
            return ids;
        }

        void Update(float dt) override
        {
            Widget::Update(dt);

            // A click selects after the frame: the rows are rebuilt and the clicked one must not go inside its own callback
            if (!mPendingSelect.IsEmpty())
            {
                String id = mPendingSelect;
                mPendingSelect = "";
                Select(id);
            }

            mPoll += dt;
            if (mPoll > (running ? 0.75f : 2.0f))
            {
                mPoll = 0.0f;
                Reload(false);
            }

            float width = mViewScroll->layout->GetWidth();
            if (Math::Abs(width - mViewWidth) > 1.0f)
                RebuildView();
        }

        void RefreshHeader()
        {
            Vector<String> parts;
            if (running)
                parts.Add("running");

            const Json& s = mTrace.summary;
            if (auto complete = s.Get("complete"); complete && complete->IsBool())
            {
                if (complete->Bool())
                    parts.Add("completed \xC2\xB7 " + (s.Get("check_count") ? s.Get("check_count")->ToJsString() : String("0")) + " checks");
                else
                    parts.Add("not completed \xC2\xB7 " + (String)(s.Get("blockers") ? s.Get("blockers")->Count() : 0) + " blockers");
            }
            if (auto turns = s.Get("turns"))
                parts.Add(turns->ToJsString() + " turns");
            if (auto ms = s.Get("ms"))
                parts.Add(Ms(ms->Num()));
            if (auto textCalls = s.Get("textCalls"))
            {
                parts.Add("model calls: " + textCalls->ToJsString() + " text, " + (s.Get("imageCalls") ? s.Get("imageCalls")->ToJsString() : String("0")) +
                          " image, " + (s.Get("replayed") ? s.Get("replayed")->ToJsString() : String("0")) + " replayed");
            }
            if (!mHasTrace)
                parts.Add("No trace yet - run the node");

            String line;
            for (int i = 0; i < parts.Count(); i++)
                line += (i ? "  \xC2\xB7  " : "") + parts[i];
            mHeader->text = line;

            Map<String, int> counts;
            for (auto& step : mTrace.steps)
                counts[step.kind]++;
            String kinds;
            for (auto kind : kindOrder)
            {
                int n = 0;
                if (counts.TryGetValue(kind, n) && n > 0)
                    kinds += (kinds.IsEmpty() ? "" : "  ") + String(kind) + " " + (String)n;
            }
            mKinds->text = kinds;
        }

    private:
        static constexpr float listWidth = 300.0f;

        Ref<Label>          mHeader;
        Ref<Label>          mKinds;
        Ref<ScrollArea>     mStepsScroll;
        Ref<VerticalLayout> mSteps;
        Ref<ScrollArea>     mViewScroll;
        Ref<VerticalLayout> mView;
        Ref<Widget>         mZoom;
        Ref<ScrollArea>     mZoomScroll;
        Ref<PipelineImageView> mZoomImage;
        Ref<Label>          mZoomTitle;

        String    mDir;
        String    mStamp;
        String    mSelected;
        String    mPendingSelect;
        TraceFile mTrace;
        bool      mHasTrace = false;
        float     mPoll = 0.0f;
        float     mViewWidth = -1.0f;

        Map<String, Ref<Bitmap>> mBitmaps; // Decoded pictures of the trace by file

    private:
        const TraceStep* SelectedStep() const
        {
            for (auto& step : mTrace.steps)
            {
                if (step.id == mSelected)
                    return &step;
            }
            return mTrace.steps.IsEmpty() ? nullptr : &mTrace.steps.Last();
        }

        Ref<Bitmap> BitmapOf(const String& file)
        {
            Ref<Bitmap> bitmap;
            if (mBitmaps.TryGetValue(file, bitmap))
                return bitmap;

            String path = mDir + file;
            bitmap = o2FileSystem.IsFileExist(path) ? DecodeImageBytes(PipelineUtils::ReadFileBytes(path)) : nullptr;
            mBitmaps[file] = bitmap;
            return bitmap;
        }

        void RebuildSteps()
        {
            PushEditorScopeOnStack scope;
            mSteps->RemoveAllChildren();
            const TraceStep* selected = SelectedStep();
            for (int i = 0; i < mTrace.steps.Count(); i++)
            {
                auto& step = mTrace.steps[i];
                auto row = o2UI.CreateWidget<Button>("pipeline icon");
                row->name = "step " + step.id;
                if (auto icon = row->GetLayer("icon"))
                    icon->enabled = false;
                row->layout->minHeight = 22;
                row->layout->maxHeight = 22;

                auto back = mmake<PipelineRoundedRect>();
                back->color = selectedBack;
                back->radius = 4.0f;
                back->roundBottom = true;
                auto backLayer = row->AddLayer("selected", back, Layout::BothStretch());
                backLayer->enabled = &step == selected;

                auto marker = mmake<PipelineRoundedRect>();
                marker->color = KindColor(step.kind);
                marker->radius = 2.0f;
                marker->roundBottom = true;
                row->AddLayer("kind", marker, Layout(Vec2F(0, 0), Vec2F(0, 1), Vec2F(2, 4), Vec2F(5, -4)));

                row->AddLayer("number", MakeText((String)(i + 1), DimTextColor(), HorAlign::Right), Layout::BothStretch(6, 0, 0, 0));
                Color4 color = step.status == "warn" ? warnColor : step.status == "error" ? errorColor : titleColor;
                auto titleText = MakeText(step.title, color, HorAlign::Left);
                titleText->dotsEngings = true;
                row->AddLayer("title", titleText, Layout(Vec2F(0, 0), Vec2F(1, 1), Vec2F(42, 0), Vec2F(-58, 0)));
                row->AddLayer("ms", MakeText(Ms(step.ms), DimTextColor(), HorAlign::Right), Layout(Vec2F(1, 0), Vec2F(1, 1), Vec2F(-56, 0), Vec2F(-4, 0)));
                if (auto number = row->GetLayer("number"))
                    number->layout = Layout(Vec2F(0, 0), Vec2F(0, 1), Vec2F(6, 0), Vec2F(36, 0));

                String id = step.id;
                row->onClick = [this, id]() { mPendingSelect = id; };
                mSteps->AddChild(row);
            }
        }

        void AddViewLabel(const String& text, const Color4& color, bool wrap)
        {
            auto label = MakeLabel(text, !wrap ? false : true);
            label->SetColor(color);
            label->horOverflow = wrap ? Label::HorOverflow::Wrap : Label::HorOverflow::Dots;
            int perLine = Math::Max(20, (int)(mViewWidth/6.6f));
            int lines = wrap ? Math::Max(1, (text.Length() + perLine - 1)/perLine) : 1;
            label->layout->minHeight = lines*16.0f + 2.0f;
            label->layout->maxHeight = lines*16.0f + 2.0f;
            mView->AddChild(label);
        }

        void AddTextBlock(const String& caption, const String& text, float height)
        {
            AddViewLabel(caption, titleColor, false);
            auto view = mmake<PipelineTextView>();
            view->SetText(Clip(text, 30000));
            view->layout->minHeight = height;
            view->layout->maxHeight = height;
            mView->AddChild(view);
        }

        void RebuildView()
        {
            PushEditorScopeOnStack scope;
            mViewWidth = mViewScroll->layout->GetWidth();
            float width = Math::Max(200.0f, mViewWidth - 16.0f);
            mView->RemoveAllChildren();

            const TraceStep* step = SelectedStep();
            if (!step)
            {
                AddViewLabel(mHasTrace ? String("The trace has no steps yet") : String("No trace yet - run the node"), DimTextColor(), false);
                return;
            }

            bool agentTurn = IsAgentTurn(step->id);
            AddViewLabel(step->kind + "  \xC2\xB7  " + step->title, step->status == "warn" ? warnColor : step->status == "error" ? errorColor : titleColor, true);
            if (!step->note.IsEmpty() && !agentTurn)
                AddViewLabel(step->note, step->status == "warn" ? warnColor : DimTextColor(), true);
            if (agentTurn && !step->note.IsEmpty())
            {
                AddViewLabel("The model's note", titleColor, false);
                AddViewLabel(step->note, DimTextColor(), true);
            }
            if (agentTurn && !step->action.IsEmpty())
            {
                auto action = mmake<PipelineTextView>();
                action->name = "debug action";
                action->SetText(Clip(step->action, 4000));
                float height = Math::Min(120.0f, 22.0f + 16.0f*(float)(step->action.Length()/Math::Max(20, (int)(mViewWidth/6.6f))));
                action->layout->minHeight = height;
                action->layout->maxHeight = height;
                mView->AddChild(action);
            }

            // After a check or a target: the design and the reconstruction under one divider
            String renderFile;
            if (step->kind == "check" || step->kind == "target")
            {
                for (auto& image : step->images)
                {
                    if (image.title == "reconstruction.png")
                        renderFile = image.file;
                }
            }
            if (!renderFile.IsEmpty() && !mTrace.designFile.IsEmpty())
            {
                auto compare = mmake<PipelineCompareView>();
                compare->name = "debug compare";
                compare->SetNode((node ? node->id : String()) + "#debug");
                compare->SetImages(BitmapOf(mTrace.designFile), BitmapOf(renderFile));
                float height = 420.0f;
                for (auto& image : step->images)
                {
                    if (image.file == renderFile && image.w > 0)
                        height = Math::Min(640.0f, Math::Max(200.0f, width*image.h/image.w));
                }
                compare->layout->minHeight = height;
                compare->layout->maxHeight = height;
                mView->AddChild(compare);
                AddViewLabel("design | reconstruction - drag the divider", DimTextColor(), false);
            }

            Vector<int> shown;
            for (int i = 0; i < step->images.Count(); i++)
            {
                if (step->images[i].file != renderFile)
                    shown.Add(i);
            }

            if (!shown.IsEmpty())
            {
                bool small = step->kind == "sprite", wide = IsWide(step->kind);
                float boxW = wide ? width : small ? 150.0f : 300.0f, boxH = wide ? 2000.0f : small ? 130.0f : 260.0f;
                auto figures = mmake<PipelineWrapRow>();
                figures->name = "debug images";
                figures->spacing = 8;
                figures->lineSpacing = 8;
                float lineHeight = 0.0f;
                for (int i : shown)
                {
                    auto& image = step->images[i];
                    float scale = image.w > 0 && image.h > 0 ? Math::Min(wide ? 4.0f : 2.0f, Math::Min(boxW/image.w, boxH/image.h)) : 1.0f;
                    float w = Math::Max(40.0f, image.w*scale), h = Math::Max(20.0f, image.h*scale);
                    lineHeight = Math::Max(lineHeight, h + 20.0f);

                    auto figure = mmake<Widget>();
                    figure->name = "debug image";
                    float figureW = wide ? width : Math::Max(w, 90.0f);
                    figure->layout->minWidth = figureW;
                    figure->layout->maxWidth = figureW;
                    auto view = mmake<PipelineImageView>();
                    view->SetBitmap(BitmapOf(image.file));
                    *view->layout = WidgetLayout(Vec2F(0, 1), Vec2F(0, 1), Vec2F(0, -h), Vec2F(w, 0));
                    figure->AddChild(view);
                    auto caption = MakeLabel(image.title + "  " + (String)image.w + "\xC3\x97" + (String)image.h, true);
                    caption->horOverflow = Label::HorOverflow::Dots;
                    *caption->layout = WidgetLayout::HorStretch(VerAlign::Top, 0, 0, 18, h + 2);
                    figure->AddChild(caption);
                    auto open = o2UI.CreateWidget<Button>("pipeline icon");
                    open->name = "zoom image";
                    if (auto icon = open->GetLayer("icon"))
                        icon->enabled = false;
                    *open->layout = WidgetLayout(Vec2F(0, 1), Vec2F(0, 1), Vec2F(0, -h), Vec2F(w, 0));
                    open->onClick = [this, i]() { ShowZoom(i); };
                    figure->AddChild(open);
                    figures->AddChild(figure);
                }
                // Full-width evidence: one picture a line, as tall as it is
                figures->lineHeight = wide ? lineHeight : boxH + 20.0f;
                float height = figures->GetHeightForWidth(width);
                figures->layout->minHeight = height;
                figures->layout->maxHeight = height;
                mView->AddChild(figures);
            }

            if (!step->text.IsEmpty())
            {
                bool request = step->kind == "request";
                bool result = agentTurn || step->kind == "prepare" || request;
                String caption = step->kind == "output" ? String("ui_structure.json") : request ? String("The request")
                    : result ? String("Result") : String("Model answer");
                AddTextBlock(caption, result ? step->text : Pretty(step->text), step->kind == "analysis" || result ? 360.0f : 220.0f);
            }
            if (!step->data.IsEmpty() && !agentTurn)
                AddTextBlock("Data", step->data, step->kind == "output" ? 260.0f : 160.0f);
        }
    };

    namespace ImageToUiDebug
    {
        // Never destroyed: the window must not outlive the UI at exit
        struct State
        {
            Ref<Window>              window;
            Ref<ImageToUiDebugPanel> panel;
            bool                     shown = false;
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
            st.window = o2UI.CreateWindow("AI image to UI - steps of the last run");
            st.window->name = "image to ui debug";
            st.window->SetIcon(mmake<VectorSprite>("ui/pipeline/window_icon.svg"));
            st.window->SetIconLayout(Layout::Based(BaseCorner::LeftTop, Vec2F(20, 20), Vec2F(-1, 1)));
            Vec2F screen = (Vec2F)o2Render.GetResolution();
            *st.window->layout = WidgetLayout::Based(BaseCorner::Center, Vec2F(Math::Min(1080.0f, screen.x - 40.0f), Math::Min(720.0f, screen.y - 40.0f)));
            st.window->SetClippingLayout(Layout::BothStretch(-1, 0, 0, 17));
            st.window->SetViewLayout(Layout::BothStretch(0, 5, 0, 18));

            st.panel = mmake<ImageToUiDebugPanel>();
            st.panel->name = "debug panel";
            *st.panel->layout = WidgetLayout::BothStretch(10, 8, 10, 8);
            st.window->AddChild(st.panel);
            st.window->onClosed = []()
            {
                auto& st = St();
                st.shown = false;
                if (st.panel && st.panel->onClose)
                {
                    auto close = st.panel->onClose;
                    st.panel->onClose = Function<void()>();
                    close();
                }
            };
            EditorUIRoot.AddWidget(st.window);
            st.window->Hide(true);
        }

        void Show(const Ref<PipelineEditor>& editor, const Ref<PipelineNode>& node, const Function<void()>& onClose)
        {
            auto& st = St();
            if (!st.window)
                Create();

            PushEditorScopeOnStack scope;
            st.panel->onClose = onClose;
            st.panel->Bind(editor, node);
            if (!st.window->GetParent())
                EditorUIRoot.AddWidget(st.window);
            if (!st.shown)
            {
                st.window->Show();
                st.shown = true;
            }
        }

        void Hide(const String& nodeId)
        {
            if (!IsShownFor(nodeId))
                return;

            auto& st = St();
            st.panel->onClose = Function<void()>();
            st.shown = false;
            st.window->Hide();
        }

        bool IsShownFor(const String& nodeId)
        {
            auto& st = St();
            return st.window && st.panel && st.panel->node && st.panel->node->id == nodeId && st.shown;
        }

        void Refresh(const String& nodeId)
        {
            if (IsShownFor(nodeId))
                St().panel->Reload(false);
        }

        void SetRunning(const String& nodeId, bool running)
        {
            auto& st = St();
            if (!st.panel || !st.panel->node || st.panel->node->id != nodeId || st.panel->running == running)
                return;

            st.panel->running = running;
            st.panel->RefreshHeader();
            if (!running)
                st.panel->Reload(false);
        }

        void SelectStep(const String& stepId)
        {
            if (St().panel)
                St().panel->Select(stepId);
        }

        void Zoom(int imageIndex)
        {
            if (St().panel)
                St().panel->ShowZoom(imageIndex);
        }

        Vector<String> GetStepIds()
        {
            return St().panel ? St().panel->GetStepIds() : Vector<String>();
        }

        Ref<Window> GetWindow()
        {
            return St().window;
        }
    }
}
