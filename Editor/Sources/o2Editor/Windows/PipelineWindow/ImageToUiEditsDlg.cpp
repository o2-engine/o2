#include "o2Editor/stdafx.h"
#include "ImageToUiEditsDlg.h"

#include <chrono>

#include "o2/Assets/Types/FontAsset.h"
#include "o2/Events/CursorAreaEventsListener.h"
#include "o2/Render/Render.h"
#include "o2/Render/Text.h"
#include "o2/Render/VectorSprite.h"
#include "o2/Scene/UI/UIManager.h"
#include "o2/Scene/UI/WidgetLayer.h"
#include "o2/Scene/UI/WidgetLayout.h"
#include "o2/Scene/UI/Widgets/Button.h"
#include "o2/Scene/UI/Widgets/EditBox.h"
#include "o2/Scene/UI/Widgets/Label.h"
#include "o2/Scene/UI/Widgets/ScrollArea.h"
#include "o2/Scene/UI/Widgets/VerticalLayout.h"
#include "o2/Scene/UI/Widgets/Window.h"
#include "o2/Utils/Editor/EditorScope.h"
#include "o2Editor/Pipeline/PipelineImageToUi.h"
#include "o2Editor/UIRoot.h"
#include "o2Editor/Windows/PipelineWindow/PipelineControls.h"
#include "o2Editor/Windows/PipelineWindow/PipelineDraw.h"
#include "o2Editor/Windows/PipelineWindow/PipelineEditor.h"
#include "o2Editor/Windows/PipelineWindow/PipelineNodeBody.h"
#include "o2Editor/Windows/PipelineWindow/PipelineNodeWidget.h"
#include "o2Editor/Windows/PipelineWindow/PipelineTheme.h"

namespace Editor
{
    using namespace PipelineControls;
    using EditItem = PipelineImageToUiEditItem;

    namespace
    {
        // The pins of the web dialog (.uie-pin): red with a white ring and the number, an applied round's grey
        const Color4 pinColor(229, 57, 53, 255);
        const Color4 pastPinColor(120, 144, 156, 255);

        constexpr float sideWidth = 340.0f;
        constexpr float footHeight = 46.0f;

        // The note card plate (.uie-note): a faint wash of the text colour
        Color4 NoteBack()
        {
            auto& theme = PipelineTheme::Get();
            return theme.dark ? Color4(255, 255, 255, 10) : PipelineTheme::Alpha(theme.fg, 0.05f);
        }

        Ref<Label> Caption(const String& text, float size, int weight, const Color4& color, bool mono = false)
        {
            auto label = MakeLabel(text, false);
            StyleLabel(label, size, weight, color);
            if (mono)
                label->SetFontAsset(AssetRef<FontAsset>(PipelineTheme::MonoFont()));
            label->horOverflow = Label::HorOverflow::Dots;
            return label;
        }

        void SetEnabledLook(const Ref<Widget>& widget, bool enabled)
        {
            if (auto button = DynamicCast<Button>(widget))
                button->interactable = enabled;
            widget->transparency = enabled ? 1.0f : 0.45f;
        }

        // A quiet button without its border, as the web's × and round rows
        Ref<Button> MakeBareButton(const String& caption)
        {
            auto button = MakeGhostButton(caption);
            if (button->GetLayer("border"))
                button->RemoveLayer("border");
            return button;
        }
    }

    // ------------------------------------------------------------------------------------------------
    // A picture the user points at (.uie-pane): fitted into the pane, a click is a point in the picture's
    // own pixels; the pins of the notes are drawn over it
    // ------------------------------------------------------------------------------------------------
    class ImageToUiPointPane : public Widget, public CursorAreaEventsListener
    {
    public:
        String                                         on;      // design or result
        Function<void(const String& on, int x, int y)> onPoint; // Called with the pixel a click put a point on
        Vector<PipelineImageToUiPin>                   pins;    // Every pin of the dialog; the pane draws its own

    public:
        explicit ImageToUiPointPane(RefCounter* refCounter): Widget(refCounter)
        {
            mView = mmake<PipelineImageView>();
            mView->name = "picture";
            mView->SetHint("\xE2\x80\x94");
            AddChild(mView);
            mPinText = MakeThemeText("", 11.0f, 600, Color4::White(), HorAlign::Middle);
            mPinText->dotsEngings = false;
        }

        // Shows the picture, none shows a dash
        void SetBitmap(const Ref<Bitmap>& bitmap)
        {
            mView->SetBitmap(bitmap);
            FitPicture();
        }

        // Returns true when a picture is shown
        bool HasPicture() const { return mView->HasImage(); }

        // Returns where the picture is drawn
        RectF GetPictureRect() const { return mView->GetImageRect(); }

        // Returns the size of the picture in its own pixels
        Vec2I GetPictureSize() const { return mView->GetImageSize(); }

        void UpdateSelfTransform() override
        {
            Widget::UpdateSelfTransform();
            FitPicture();
        }

        void Draw() override
        {
            if (!mResEnabledInHierarchy || mIsClipped)
                return;

            Widget::Draw();
            DrawPins();
            CursorAreaEventsListener::OnDrawn();
        }

        bool IsUnderPoint(const Vec2F& point) override
        {
            return mView->HasImage() && mView->GetImageRect().IsInside(point);
        }

    protected:
        Ref<PipelineImageView> mView;
        Ref<Text>              mPinText;

    protected:
        void OnCursorReleased(const Input::Cursor& cursor) override
        {
            int x = 0, y = 0;
            if (onPoint && PipelineImageToUi::PictureClick(mView->GetImageRect(), mView->GetImageSize(), cursor.position, x, y))
                onPoint(on, x, y);
        }

        // The picture's plate hugs the picture (.uie-img): its aspect fitted into the pane, centred
        void FitPicture()
        {
            Vec2F size(layout->GetWidth(), layout->GetHeight());
            Vec2I image = mView->GetImageSize();
            if (size.x <= 0.0f || size.y <= 0.0f)
                return;

            float w = size.x, h = size.y;
            if (mView->HasImage() && image.x > 0 && image.y > 0)
            {
                float scale = Math::Min(size.x/image.x, size.y/image.y);
                w = image.x*scale;
                h = image.y*scale;
            }
            Vec2F offset((size.x - w)*0.5f, 0.0f);
            *mView->layout = WidgetLayout(Vec2F(0, 1), Vec2F(0, 1), Vec2F(offset.x, -h), Vec2F(offset.x + w, 0));
        }

        void DrawPins()
        {
            if (!mView->HasImage())
                return;

            RectF rect = mView->GetImageRect();
            Vec2I size = mView->GetImageSize();
            if (size.x <= 0 || size.y <= 0)
                return;

            for (auto& pin : pins)
            {
                if (pin.on != on)
                    continue;

                Vec2F center(rect.left + (float)pin.x/size.x*rect.Width(), rect.top - (float)pin.y/size.y*rect.Height());
                PipelineDraw::Circle(center + Vec2F(0, -2), 13.0f, Color4(0, 0, 0, 90));
                PipelineDraw::Circle(center, 13.0f, Color4::White());
                PipelineDraw::Circle(center, 11.0f, pin.past ? pastPinColor : pinColor);
                mPinText->text = (String)pin.n;
                mPinText->SetRect(RectF(center.x - 11, center.y + 11, center.x + 11, center.y - 11));
                mPinText->Draw();
            }
        }
    };

    // -----------------------------------------------------------------------------------------------
    // The window's content: the hint on top, source | result on the left, the applied rounds, the
    // notes and Apply on the right
    // -----------------------------------------------------------------------------------------------
    class ImageToUiEditsPanel : public Widget
    {
    public:
        WeakRef<PipelineEditor> editor;
        String                  nodeId;
        Function<void()>        onClose;
        bool                    running = false;

    public:
        explicit ImageToUiEditsPanel(RefCounter* refCounter): Widget(refCounter)
        {
            auto& theme = PipelineTheme::Get();
            mHint = Caption("", smallFont, 400, theme.fgDim);
            mHint->name = "edits hint";
            *mHint->layout = WidgetLayout::HorStretch(VerAlign::Top, 0, 0, 20, 0);
            AddChild(mHint);

            for (int i = 0; i < 2; i++)
            {
                String on = i == 0 ? "design" : "result";
                auto title = Caption(i == 0 ? "SOURCE" : "RESULT", 10.0f, 600, theme.fgDim, true);
                title->horAlign = HorAlign::Middle;
                title->name = on + " title";
                AddChild(title);
                mTitles.Add(title);

                auto pane = mmake<ImageToUiPointPane>();
                pane->name = on + " picture";
                pane->on = on;
                pane->onPoint = [this](const String& side, int x, int y) { mPending = [this, side, x, y]() { Point(side, x, y); }; };
                AddChild(pane);
                mPanes.Add(pane);
            }

            mSide = mmake<Widget>();
            mSide->name = "edits side";
            AddBorderLayer(*mSide, "line", theme.border, 0.0f, 1.0f, Layout(Vec2F(0, 0), Vec2F(0, 1), Vec2F(0, 0), Vec2F(1, 0)));
            AddChild(mSide);

            mScroll = o2UI.CreateScrollArea();
            mScroll->name = "edits notes scroll";
            *mScroll->layout = WidgetLayout::BothStretch(1, footHeight, 0, 0);
            mSide->AddChild(mScroll);
            mList = mmake<VerticalLayout>();
            mList->spacing = 8;
            mList->expandWidth = true;
            mList->expandHeight = false;
            mList->fitByChildren = true;
            mList->baseCorner = BaseCorner::Top;
            *mList->layout = WidgetLayout::HorStretch(VerAlign::Top, 12, 14, 0, -10);
            mScroll->AddChild(mList);

            mApply = MakePrimaryButton("Apply (0)");
            mApply->name = "apply edits";
            *mApply->layout = WidgetLayout(Vec2F(1, 0), Vec2F(1, 0), Vec2F(-132, 9), Vec2F(-12, 37));
            mApply->onClick = [this]() { mPending = [this]() { Apply(); }; };
            mSide->AddChild(mApply);
        }

        // Binds the panel to a node: its draft, its rounds and its pictures
        void Bind(const Ref<PipelineEditor>& e, const Ref<PipelineNode>& n)
        {
            editor = e;
            nodeId = n ? n->id : String();
            mDraft = n ? PipelineImageToUi::ReadDraft(*n) : Vector<EditItem>();
            mView = -1;
            mFocus = -1;
            Refresh();
        }

        // Reads the rounds and the pictures again and rebuilds the side; executor events call it between frames
        void Refresh()
        {
            PushEditorScopeOnStack scope;
            auto node = Node();
            mRounds = node ? PipelineImageToUi::Edits(*node) : Vector<PipelineImageToUiEditRound>();
            if (mView >= mRounds.Count())
                mView = -1;
            RefreshPictures();
            Rebuild();
        }

        void Point(const String& on, int x, int y)
        {
            if (running)
                return;

            mView = -1;
            EditItem item;
            item.at = PipelineImageToUiEditPoint{ on == "design" ? "design" : "result", x, y };
            mDraft.Add(item);
            mFocus = mDraft.Count() - 1;
            SaveDraft();
            Rebuild();
        }

        void AddGeneralNote()
        {
            if (running)
                return;

            mDraft.Add(EditItem());
            mFocus = mDraft.Count() - 1;
            SaveDraft();
            Rebuild();
        }

        void SetNoteText(int index, const String& text, bool rebuild)
        {
            if (index < 0 || index >= mDraft.Count() || mDraft[index].text == text)
                return;

            mDraft[index].text = text;
            SaveDraft();
            if (rebuild)
                Rebuild();
            else
                UpdateApply();
        }

        void RemoveNote(int index)
        {
            if (index < 0 || index >= mDraft.Count())
                return;

            mDraft.RemoveAt(index);
            mFocus = -1;
            SaveDraft();
            Rebuild();
        }

        void ViewRound(int index)
        {
            mView = index >= 0 && index < mRounds.Count() && mView != index ? index : -1;
            Rebuild();
        }

        bool CanApply() const
        {
            return PipelineImageToUi::CanApplyEdits(running, mHasResult, mDraft);
        }

        String ApplyCaption() const
        {
            return "Apply (" + (String)PipelineImageToUi::EditItems(PipelineImageToUi::EditItemsJson(mDraft)).Count() + ")";
        }

        // Sends the notes as one round and runs the node (nodeRenderers.tsx applyEdits)
        bool Apply()
        {
            if (!CanApply())
                return false;

            auto items = PipelineImageToUi::EditItems(PipelineImageToUi::EditItemsJson(mDraft));
            Int64 now = (Int64)std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
            String id = PipelineImageToUi::NewEditId(now);
            mDraft.Clear();
            return ChangeAndRun([items, id](PipelineNode& node) { return PipelineImageToUi::ApplyEditRound(node, items, id); });
        }

        // Drops the last round and runs the node: the result before it is cached, or rebuilt from the files kept for it
        bool Undo()
        {
            if (running || mRounds.IsEmpty())
                return false;

            mView = -1;
            return ChangeAndRun([](PipelineNode& node) { return PipelineImageToUi::UndoEditRound(node); });
        }

        const Vector<EditItem>& GetDraft() const { return mDraft; }

        Vector<PipelineImageToUiPin> GetPins() const { return PipelineImageToUi::EditPins(mDraft, mRounds, mView); }

        void Update(float dt) override
        {
            Widget::Update(dt);

            // A click acts after the frame: the side is rebuilt and the clicked widget must not go inside its own callback
            if (mPending)
            {
                auto pending = mPending;
                mPending = Function<void()>();
                pending();
            }

            mPoll += dt;
            if (mPicturesPending && mPoll > 0.3f)
            {
                mPoll = 0.0f;
                RefreshPictures();
                UpdateApply();
            }

            Vec2F size(layout->GetWidth(), layout->GetHeight());
            if (size != mLaidOut)
                LayoutParts();
        }

        // The hint, the pictures and Apply after a change of the running state
        void RefreshState()
        {
            PushEditorScopeOnStack scope;
            RefreshPictures();
            Rebuild();
        }

    private:
        Ref<Label>                     mHint;
        Vector<Ref<Label>>             mTitles;
        Vector<Ref<ImageToUiPointPane>> mPanes;
        Ref<Widget>                    mSide;
        Ref<ScrollArea>                mScroll;
        Ref<VerticalLayout>            mList;
        Ref<Button>                    mApply;

        Vector<EditItem>                   mDraft;
        Vector<PipelineImageToUiEditRound> mRounds;
        int                                mView = -1;  // The applied round looked at, -1 for none
        int                                mFocus = -1; // The note whose field takes the focus after the rebuild
        bool                               mHasResult = false;
        bool                               mPicturesPending = false;
        float                              mPoll = 0.0f;
        Vec2F                              mLaidOut;
        Function<void()>                   mPending;

    private:
        Ref<PipelineNode> Node() const
        {
            auto e = editor.Lock();
            auto graph = e ? e->GetGraph() : nullptr;
            return graph ? graph->FindNode(nodeId) : nullptr;
        }

        void SaveDraft()
        {
            auto e = editor.Lock();
            auto node = Node();
            if (!e || !node)
                return;

            PipelineImageToUi::WriteDraft(*node, mDraft);
            if (auto widget = e->GetNodeWidget(nodeId))
                e->OnNodeConfigChanged(widget, "editsDraft", true);
        }

        // A change of the rounds as one undo step, then the card follows it and the node runs
        bool ChangeAndRun(const Function<bool(PipelineNode&)>& change)
        {
            auto e = editor.Lock();
            auto node = Node();
            auto widget = e ? e->GetNodeWidget(nodeId) : nullptr;
            if (!e || !node || !widget)
                return false;

            String id = nodeId;
            e->OnNodeConfigChanged(widget, "edits", false);
            bool changed = change(*node);
            e->OnNodeConfigChanged(widget, "edits", true);
            if (auto body = widget->GetBody())
                body->OnConfigChanged();
            if (changed)
                e->RunNode(id, false);
            if (nodeId == id)
                Refresh();
            return changed;
        }

        void RefreshPictures()
        {
            mPicturesPending = false;
            auto e = editor.Lock();
            auto node = Node();
            Ref<Bitmap> design, result;
            if (e && node)
            {
                PipelineValue input = e->GetInputValue(node, "image");
                if (input.IsImage())
                {
                    design = e->GetPicture(input, nodeId);
                    mPicturesPending = mPicturesPending || !design;
                }

                const PipelineNodeRuntime* rt = e->GetRuntime(nodeId);
                if (rt && !running && rt->output.IsImage())
                {
                    result = e->GetPicture(rt->output, nodeId);
                    mPicturesPending = mPicturesPending || !result;
                }
            }
            mHasResult = result != nullptr;
            mPanes[0]->SetBitmap(design);
            mPanes[1]->SetBitmap(result);
        }

        void LayoutParts()
        {
            mLaidOut = Vec2F(layout->GetWidth(), layout->GetHeight());
            float picsWidth = Math::Max(100.0f, mLaidOut.x - sideWidth - 14.0f);
            float paneWidth = (picsWidth - 16.0f)*0.5f;
            for (int i = 0; i < 2; i++)
            {
                float left = i*(paneWidth + 16.0f);
                *mTitles[i]->layout = WidgetLayout(Vec2F(0, 1), Vec2F(0, 1), Vec2F(left, -44), Vec2F(left + paneWidth, -28));
                *mPanes[i]->layout = WidgetLayout(Vec2F(0, 0), Vec2F(0, 1), Vec2F(left, 0), Vec2F(left + paneWidth, -50));
            }
            *mSide->layout = WidgetLayout(Vec2F(1, 0), Vec2F(1, 1), Vec2F(-sideWidth, 0), Vec2F(0, -26));
        }

        void UpdateApply()
        {
            mApply->caption = ApplyCaption();
            SetEnabledLook(mApply, CanApply());
            mHint->text = running ? String("The node is running\xE2\x80\xA6")
                : mHasResult ? String("Click the source or the result to put a point, then say what to change there. A note without a "
                                      "point is about the whole screen.")
                : String("Run the node first: edits change its result.");
        }

        void AddFixed(const Ref<Widget>& widget, float height)
        {
            widget->layout->minHeight = height;
            widget->layout->maxHeight = height;
            mList->AddChild(widget);
        }

        void AddSectionTitle(const String& text, const Ref<Widget>& right = nullptr)
        {
            auto row = mmake<Widget>();
            auto title = Caption(text, 10.0f, 600, PipelineTheme::Get().fgDim, true);
            *title->layout = WidgetLayout::BothStretch(0, 0, right ? 130 : 0, 0);
            row->AddChild(title);
            if (right)
            {
                *right->layout = WidgetLayout(Vec2F(1, 0), Vec2F(1, 1), Vec2F(-126, 0), Vec2F(0, 0));
                row->AddChild(right);
            }
            AddFixed(row, 24.0f);
        }

        void AddRounds()
        {
            auto& theme = PipelineTheme::Get();
            AddSectionTitle("APPLIED");
            for (int i = 0; i < mRounds.Count(); i++)
            {
                auto& round = mRounds[i];
                bool open = mView == i;
                auto head = MakeBareButton("");
                head->name = "round " + (String)(i + 1);
                if (open)
                {
                    AddFillLayer(*head, "on", theme.accentSoft, controlRadius, Layout::BothStretch(), -2.5f);
                    AddBorderLayer(*head, "onBorder", PipelineTheme::Alpha(theme.accent, 0.4f), controlRadius, 1.0f, Layout::BothStretch(), -2.4f);
                }
                head->AddLayer("title", MakeThemeText("Round " + (String)(i + 1), bodyFont, 600, theme.fg, HorAlign::Left),
                               Layout::BothStretch(6, 0, 90, 0));
                auto count = MakeThemeText((String)round.items.Count() + " note(s)", monoFont, 400, theme.fgDim, HorAlign::Right);
                head->AddLayer("count", count, Layout::BothStretch(90, 0, 6, 0));
                head->onClick = [this, i]() { mPending = [this, i]() { ViewRound(i); }; };
                AddFixed(head, 26.0f);

                if (!open)
                    continue;

                for (int j = 0; j < round.items.Count(); j++)
                {
                    auto& item = round.items[j];
                    String line = (String)(j + 1) + ". " + PipelineImageToUi::EditPlace(item) + "  " + item.text;
                    auto label = Caption(line, bodyFont, 400, theme.fg);
                    label->horOverflow = Label::HorOverflow::Wrap;
                    int lines = PipelineNodeBody::EstimateLines(line, 44, 8);
                    AddFixed(label, 16.0f*lines + 2.0f);
                }
            }

            auto undo = MakeGhostButton("Undo the last round");
            undo->name = "undo edits";
            undo->onClick = [this]() { mPending = [this]() { Undo(); }; };
            SetEnabledLook(undo, !running);
            auto row = mmake<Widget>();
            float width = Math::Ceil(TextWidth("Undo the last round", bodyFont)) + 24.0f;
            *undo->layout = WidgetLayout(Vec2F(0, 0), Vec2F(0, 1), Vec2F(0, 0), Vec2F(width, 0));
            row->AddChild(undo);
            AddFixed(row, 26.0f);
        }

        void AddNote(int index)
        {
            auto& theme = PipelineTheme::Get();
            auto& item = mDraft[index];
            auto note = mmake<Widget>();
            note->name = "note " + (String)(index + 1);
            AddFillLayer(*note, "back", NoteBack(), 8.0f, Layout::BothStretch(), -3.0f);

            auto badge = mmake<Widget>();
            badge->name = "badge";
            AddFillLayer(*badge, "back", item.at ? pinColor : (theme.dark ? Color4(255, 255, 255, 30) : PipelineTheme::Alpha(theme.fg, 0.1f)),
                         10.0f, Layout::BothStretch());
            badge->AddLayer("number", MakeThemeText(item.at ? (String)(index + 1) : String("\xE2\x80\xA2"), 10.5f, 600,
                                                    item.at ? Color4::White() : theme.fgDim, HorAlign::Middle), Layout::BothStretch());
            *badge->layout = WidgetLayout(Vec2F(0, 1), Vec2F(0, 1), Vec2F(8, -28), Vec2F(28, -8));
            note->AddChild(badge);

            auto place = Caption(PipelineImageToUi::EditPlace(item), monoFont, 400, theme.fgDim, true);
            place->name = "place";
            *place->layout = WidgetLayout::HorStretch(VerAlign::Top, 36, 32, 20, 8);
            note->AddChild(place);

            auto remove = MakeBareButton("\xC3\x97");
            remove->name = "remove note";
            *remove->layout = WidgetLayout(Vec2F(1, 1), Vec2F(1, 1), Vec2F(-30, -30), Vec2F(-6, -6));
            remove->onClick = [this, index]() { mPending = [this, index]() { RemoveNote(index); }; };
            note->AddChild(remove);

            auto edit = MakeEditBox(item.text, true, item.at ? "What to change here" : "What to change on the screen");
            edit->name = "note text";
            *edit->layout = WidgetLayout::BothStretch(8, 8, 8, 34);
            edit->onChanged = [this, index](const WString& text) { SetNoteText(index, (String)text, false); };
            SetEnabledLook(edit, !running);
            edit->interactable = !running;
            note->AddChild(edit);
            AddFixed(note, 100.0f);
            if (index == mFocus && !running)
                edit->Focus();
        }

        void Rebuild()
        {
            PushEditorScopeOnStack scope;
            auto pins = GetPins();
            for (auto& pane : mPanes)
                pane->pins = pins;

            mList->RemoveAllChildren();
            if (!mRounds.IsEmpty())
                AddRounds();

            auto add = MakeGhostButton("+ Without a point");
            add->name = "add general note";
            add->onClick = [this]() { mPending = [this]() { AddGeneralNote(); }; };
            SetEnabledLook(add, !running);
            AddSectionTitle("NOTES", add);

            if (mDraft.IsEmpty())
            {
                auto empty = Caption("No notes yet \xE2\x80\x94 click a picture to put a point.", smallFont, 400, PipelineTheme::Get().fg3);
                empty->name = "no notes";
                AddFixed(empty, 20.0f);
            }
            for (int i = 0; i < mDraft.Count(); i++)
                AddNote(i);

            mFocus = -1;
            UpdateApply();
        }
    };

    namespace ImageToUiEdits
    {
        // Never destroyed: the window must not outlive the UI at exit
        struct State
        {
            Ref<Window>              window;
            Ref<ImageToUiEditsPanel> panel;
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
            st.window = o2UI.CreateWindow("AI image to UI \xE2\x80\x94 edits");
            st.window->name = "image to ui edits";
            st.window->SetIcon(mmake<VectorSprite>("ui/pipeline/window_icon.svg"));
            st.window->SetIconLayout(Layout::Based(BaseCorner::LeftTop, Vec2F(20, 20), Vec2F(-1, 1)));
            Vec2F screen = (Vec2F)o2Render.GetResolution();
            *st.window->layout = WidgetLayout::Based(BaseCorner::Center, Vec2F(Math::Min(1320.0f, screen.x - 40.0f), Math::Min(820.0f, screen.y - 40.0f)));
            st.window->SetClippingLayout(Layout::BothStretch(-1, 0, 0, 17));
            st.window->SetViewLayout(Layout::BothStretch(0, 5, 0, 18));

            st.panel = mmake<ImageToUiEditsPanel>();
            st.panel->name = "edits panel";
            *st.panel->layout = WidgetLayout::BothStretch(12, 10, 12, 8);
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
            bool same = st.shown && node && st.panel->nodeId == node->id;
            st.panel->onClose = onClose;
            if (!same)
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
            return st.window && st.panel && st.panel->nodeId == nodeId && st.shown;
        }

        void Refresh(const String& nodeId)
        {
            if (IsShownFor(nodeId))
                St().panel->Refresh();
        }

        void SetRunning(const String& nodeId, bool running)
        {
            auto& st = St();
            if (!st.panel || st.panel->nodeId != nodeId || st.panel->running == running)
                return;

            st.panel->running = running;
            st.panel->RefreshState();
        }

        void Point(const String& on, int x, int y)
        {
            if (St().panel)
                St().panel->Point(on, x, y);
        }

        void AddGeneralNote()
        {
            if (St().panel)
                St().panel->AddGeneralNote();
        }

        void SetNoteText(int index, const String& text)
        {
            if (St().panel)
                St().panel->SetNoteText(index, text, true);
        }

        void RemoveNote(int index)
        {
            if (St().panel)
                St().panel->RemoveNote(index);
        }

        void ViewRound(int index)
        {
            if (St().panel)
                St().panel->ViewRound(index);
        }

        bool Apply()
        {
            return St().panel && St().panel->Apply();
        }

        bool Undo()
        {
            return St().panel && St().panel->Undo();
        }

        bool CanApply()
        {
            return St().panel && St().panel->CanApply();
        }

        String GetApplyCaption()
        {
            return St().panel ? St().panel->ApplyCaption() : String();
        }

        Vector<PipelineImageToUiEditItem> GetDraft()
        {
            return St().panel ? St().panel->GetDraft() : Vector<PipelineImageToUiEditItem>();
        }

        Vector<PipelineImageToUiPin> GetPins()
        {
            return St().panel ? St().panel->GetPins() : Vector<PipelineImageToUiPin>();
        }

        Ref<Window> GetWindow()
        {
            return St().window;
        }
    }
}
