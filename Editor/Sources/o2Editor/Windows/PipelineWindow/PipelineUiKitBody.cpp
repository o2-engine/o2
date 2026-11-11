#include "o2Editor/stdafx.h"
#include "PipelineNodeBodyFactories.h"

#include "o2/Render/Render.h"
#include "o2/Scene/UI/WidgetLayer.h"
#include "o2/Scene/UI/WidgetLayout.h"
#include "o2/Scene/UI/Widgets/Button.h"
#include "o2/Scene/UI/Widgets/DropDown.h"
#include "o2/Scene/UI/Widgets/EditBox.h"
#include "o2/Scene/UI/Widgets/HorizontalLayout.h"
#include "o2/Scene/UI/Widgets/Label.h"
#include "o2/Scene/UI/Widgets/Toggle.h"
#include "o2/Utils/Editor/DragHandle.h"
#include "o2/Utils/Editor/EditorScope.h"
#include "o2Editor/Pipeline/Nodes/UiNodes.h"
#include "o2Editor/Pipeline/PipelinePngMeta.h"
#include "o2Editor/Pipeline/PipelineUtils.h"
#include "o2Editor/Pipeline/UiDoc/UiDocDevice.h"
#include "o2Editor/Pipeline/UiDoc/UiDocJs.h"
#include "o2Editor/Pipeline/UiDoc/UiDocRender.h"
#include "o2Editor/Windows/PipelineWindow/PipelineDraw.h"
#include "o2Editor/Windows/PipelineWindow/PipelineEditor.h"
#include "o2Editor/Windows/PipelineWindow/PipelineImageTheme.h"
#include "o2Editor/Windows/PipelineWindow/PipelineNodeWidget.h"
#include "o2Editor/Windows/PipelineWindow/PipelineTheme.h"
#include "o2Editor/Windows/PipelineWindow/PipelineUiAi.h"

namespace Editor
{
    using namespace PipelineControls;

    // ---------------------------------------------------------------------------------------------------------------
    // An element's source picture with the trim box and the nine-slice guides (AssetsLine SourceGuides): the guides drag,
    // in input pixels; opposite guides never cross
    // ---------------------------------------------------------------------------------------------------------------
    class PipelineKitGuides : public PipelineImageView
    {
    public:
        Function<void(const UiDoc::UiNine&)> onLive; // A guide moved
        Function<void(const UiDoc::UiNine&)> onSet;  // A guide was let go

    public:
        explicit PipelineKitGuides(RefCounter* refCounter): PipelineImageView(refCounter)
        {
            WeakRef<PipelineKitGuides> weakThis(this);
            for (int side = 0; side < 4; side++)
            {
                auto handle = mmake<DragHandle>();
                handle->cursorType = side < 2 ? CursorType::SizeWE : CursorType::SizeNS;
                handle->isPointInside = [weakThis, side](const Vec2F& p)
                {
                    auto self = weakThis.Lock();
                    return self && self->IsOnGuide(side, p);
                };
                handle->onChangedPos = [weakThis, side](const Vec2F& p) { if (auto self = weakThis.Lock()) self->OnDragged(side, p); };
                handle->onChangeCompleted = [weakThis]()
                {
                    auto self = weakThis.Lock();
                    if (self && self->mDragging && self->onSet)
                        self->onSet(*self->mNine);
                    if (self)
                        self->mDragging = false;
                };
                mHandles.Add(handle);
            }
        }

        // Sets the picture's own size (input pixels), the trim box and the borders shown; none hides the guides
        void SetGuides(int width, int height, const UiDoc::Json& trim, const std::optional<UiDoc::UiNine>& nine, bool automatic)
        {
            mW = width;
            mH = height;
            mTrim = trim;
            if (!mDragging)
                mNine = nine;
            mAuto = automatic;
        }

        void Draw() override
        {
            PipelineImageView::Draw();
            if (!mResEnabledInHierarchy || mIsClipped || !HasImage() || mW <= 0 || mH <= 0 || PipelineControls::IsFarView())
                return;

            RectF image = GetImageRect();
            float k = image.Width() / mW;
            if (mTrim.IsObject())
            {
                float x = (float)mTrim.Get("x")->Num(), y = (float)mTrim.Get("y")->Num();
                float w = (float)mTrim.Get("w")->Num(), h = (float)mTrim.Get("h")->Num();
                if (w != mW || h != mH)
                {
                    RectF box(image.left + x*k, image.top - y*k, image.left + (x + w)*k, image.top - (y + h)*k);
                    DashedRect(box, Color4(0x6F, 0xE3, 0xCF, 255));
                }
            }

            if (!mNine)
                return;

            Color4 pink(0xFF, 0x7A, 0xB6, 255);
            for (int side = 0; side < 4; side++)
            {
                Vec2F a, b;
                GuideLine(side, a, b);
                if (mAuto && !mDragging)
                    DashedLine(a, b, pink);
                else
                    o2Render.DrawAALine({ a, b }, pink, 1.4f);

                mHandles[side]->position = (a + b)*0.5f;
                mHandles[side]->Draw();
            }
        }

    private:
        Vector<Ref<DragHandle>>       mHandles; // left, right, top, bottom
        int                           mW = 0, mH = 0;
        UiDoc::Json                   mTrim;
        std::optional<UiDoc::UiNine>  mNine;
        bool                          mAuto = true;
        bool                          mDragging = false;

        void GuideLine(int side, Vec2F& a, Vec2F& b) const
        {
            RectF image = GetImageRect();
            float k = image.Width() / mW;
            const auto& n = *mNine;
            if (side == 0 || side == 1)
            {
                float x = side == 0 ? image.left + (float)n.left*k : image.right - (float)n.right*k;
                a = Vec2F(x, image.bottom);
                b = Vec2F(x, image.top);
            }
            else
            {
                float y = side == 2 ? image.top - (float)n.top*k : image.bottom + (float)n.bottom*k;
                a = Vec2F(image.left, y);
                b = Vec2F(image.right, y);
            }
        }

        bool IsOnGuide(int side, const Vec2F& p) const
        {
            if (!mNine || !HasImage() || mW <= 0)
                return false;

            Vec2F a, b;
            GuideLine(side, a, b);
            float pixel = Math::Max(0.0001f, o2Render.GetCamera().GetScale2D().x);
            float grab = 5.0f*pixel;
            return side < 2 ? Math::Abs(p.x - a.x) < grab && p.y >= a.y - grab && p.y <= b.y + grab :
                Math::Abs(p.y - a.y) < grab && p.x >= a.x - grab && p.x <= b.x + grab;
        }

        void OnDragged(int side, const Vec2F& p)
        {
            if (!mNine)
                return;

            mDragging = true;
            RectF image = GetImageRect();
            float k = image.Width() / mW;
            double v = side == 0 ? (p.x - image.left)/k : side == 1 ? mW - (p.x - image.left)/k :
                side == 2 ? (image.top - p.y)/k : mH - (image.top - p.y)/k;
            auto n = *mNine;
            double span = side < 2 ? mW : mH;
            double other = side == 0 ? n.right : side == 1 ? n.left : side == 2 ? n.bottom : n.top;
            double value = Math::Max(0.0, UiDoc::JsRound(Math::Min(v, span - other - 1)));
            (side == 0 ? n.left : side == 1 ? n.right : side == 2 ? n.top : n.bottom) = value;
            mNine = n;
            if (onLive)
                onLive(n);
        }

        static void DashedLine(const Vec2F& a, const Vec2F& b, const Color4& color)
        {
            float pixel = Math::Max(0.0001f, o2Render.GetCamera().GetScale2D().x);
            float length = (b - a).Length(), dash = 5.0f*pixel, gap = 3.0f*pixel;
            Vec2F dir = length > 0.0f ? (b - a)/length : Vec2F();
            for (float t = 0.0f; t < length; t += dash + gap)
                o2Render.DrawAALine({ a + dir*t, a + dir*Math::Min(length, t + dash) }, color, 1.2f);
        }

        static void DashedRect(const RectF& r, const Color4& color)
        {
            DashedLine(Vec2F(r.left, r.top), Vec2F(r.right, r.top), color);
            DashedLine(Vec2F(r.right, r.top), Vec2F(r.right, r.bottom), color);
            DashedLine(Vec2F(r.right, r.bottom), Vec2F(r.left, r.bottom), color);
            DashedLine(Vec2F(r.left, r.bottom), Vec2F(r.left, r.top), color);
        }
    };

    // ---------------------------------------------------------------------------------------------------------------
    // The UI kit card (AssetsLine UiKitBody.tsx). Left: one row per input - a picture or a text style - with the input's
    // port on its row, and the add row. Right: the selected element - its source with the trim box and the nine-slice
    // guides, its size and density in the kit, and its meta. The AI fills the meta of every picture (or one) and removes
    // complex backgrounds when asked; the rest runs locally on every change
    // ---------------------------------------------------------------------------------------------------------------
    class UiKitBody : public PipelineNodeBody
    {
    public:
        using PipelineNodeBody::PipelineNodeBody;

        static constexpr float rowH = 44.0f, rowGap = 4.0f, addH = 30.0f, listWidth = 236.0f;

        void Build() override
        {
            auto& theme = PipelineTheme::Get();
            WeakRef<UiKitBody> weakThis(this);
            ReadKit();

            // The summary and the AI for every picture
            auto head = mmake<HorizontalLayout>();
            head->spacing = 6;
            head->expandWidth = true;
            head->expandHeight = true;
            head->baseCorner = BaseCorner::Left;
            int pictures = mNode->inputs.Sum<int>([](const PipelinePort& p) { return p.portType == PipelinePortType::Image ? 1 : 0; });
            int styles = mNode->inputs.Count() - pictures;
            auto summary = MakeLabel((String)pictures + " pictures \xC2\xB7 " + (String)styles + " text styles", true);
            StyleLabel(summary, 11.0f, 500, theme.fgDim);
            head->AddChild(summary);
            mAiAll = MakeGhostButton("Describe all with AI");
            mAiAll->name = "ai all";
            PipelineImageTheme::MakeAccentGhost(mAiAll);
            mAiAll->layout->minWidth = mAiAll->layout->maxWidth = 170;
            mAiAll->onClick = [weakThis]() { if (auto self = weakThis.Lock()) PipelineUiAi::AnalyzeKit(self->mEditor.Lock(), self->mNode->id, ""); };
            head->AddChild(mAiAll);
            AddRow(head, 28);

            // The AI's models: the vision model, and the image model that removes complex backgrounds when that is on
            AddModelRow(PipelineTextModelPresets(), UiDoc::UI_KIT_AI_DEFAULT_MODEL, PipelineModelKind::Text, "kitModel", "AI model", 86);
            auto removeBg = AddCheckbox("Remove complex backgrounds with the image model", "kitRemoveBg", false);
            removeBg->name = "kit remove bg";
            removeBg->onToggleByUser = [weakThis](bool value)
            {
                if (auto self = weakThis.Lock())
                {
                    self->SetBool("kitRemoveBg", value, true);
                    self->RebuildBodyKeepingArea();
                }
            };
            if (GetBool("kitRemoveBg", false))
                AddModelRow(PipelineImageModelPresets(), UiDoc::UI_KIT_AI_DEFAULT_IMAGE_MODEL, PipelineModelKind::Image, "kitImageModel", "Image model", 86);

            // The list beside the details
            mMain = mmake<Widget>();
            mMain->name = "kit main";
            mList = mmake<Widget>();
            mList->name = "kit list";
            mMain->AddChild(mList);
            mDetails = mmake<Widget>();
            mDetails->name = "kit details";
            mMain->AddChild(mDetails);
            AddFlexible(mMain, [weakThis](float) { auto self = weakThis.Lock(); return self ? self->MainHeight() : 504.0f; });
            MarkContent(mMain);

            BuildRows();
            BuildDetails();
        }

        void OnOutputChanged() override
        {
            PipelineNodeBody::OnOutputChanged();
            String before = mKitText;
            ReadKit();
            if (before != mKitText)
                RebuildBodyKeepingArea();
        }

        void OnConfigChanged() override
        {
            RebuildBodyKeepingArea();
        }

        void Update(float dt) override
        {
            PipelineNodeBody::Update(dt);
            bool busy = PipelineUiAi::IsBusy("kit", mNode->id);
            if (mAiAll)
            {
                mAiAll->interactable = !busy;
                mAiAll->transparency = busy ? 0.4f : 1.0f;
            }
            if (mAiOne)
            {
                mAiOne->interactable = !busy;
                mAiOne->transparency = busy ? 0.4f : 1.0f;
            }
            LayoutMain();
        }

        // The input ports sit on the rows, at the card's left edge; a link dropped on the list becomes a new input
        bool InputsInBody() const override { return true; }

        bool HasBodyPort(const String& portId) const override
        {
            return mNode->inputs.Any([&](const PipelinePort& port) { return port.id == portId; });
        }

        bool GetBodyPortOffset(const String& portId, Vec2F& offset) const override
        {
            auto owner = mOwner.Lock();
            if (!owner || !mMain || !HasBodyPort(portId))
                return false;

            float top = 0.0f, height = 0.0f;
            if (!GetRowPlacement(mMain, owner->GetCardWidth(), owner->GetBodyAreaHeight(), top, height))
                return false;

            int index = mNode->inputs.IndexOf([&](const PipelinePort& p) { return p.id == portId; });
            offset = Vec2F(0.0f, top + index*(rowH + rowGap) + rowH*0.5f);
            return true;
        }

        bool IsAddInputAt(const Vec2F& point) const override
        {
            return mList && mList->layout->GetWorldRect().IsInside(point);
        }

    private:
        Ref<Button> mAiAll;   // The AI for every picture
        Ref<Button> mAiOne;   // The AI for the selected one
        Ref<Widget> mMain;    // The list and the details side by side
        Ref<Widget> mList;    // One row per input and the add row
        Ref<Widget> mDetails; // The selected element

        UiDoc::UiKit mKit;     // The node's own output as last read
        String       mKitText;

        void ReadKit()
        {
            auto output = GetOutput();
            mKitText = output.IsValid() && output.type == PipelinePortType::UiKit ? output.data : String();
            mKit = mKitText.IsEmpty() ? UiDoc::UiKit() : UiDoc::ParseKit(mKitText);
        }

        float MainHeight() const
        {
            return Math::Max(mNode->inputs.Count()*(rowH + rowGap) + addH, 504.0f);
        }

        void LayoutMain()
        {
            if (!mMain)
                return;

            *mList->layout = WidgetLayout(Vec2F(0, 0), Vec2F(0, 1), Vec2F(0, 0), Vec2F(listWidth, 0));
            *mDetails->layout = WidgetLayout(Vec2F(0, 0), Vec2F(1, 1), Vec2F(listWidth + 10, 0), Vec2F(0, 0));
        }

        String SelectedId() const
        {
            String selected = mNode->GetConfigString("kitSelected", "");
            if (mNode->FindInput(selected))
                return selected;
            return mNode->inputs.IsEmpty() ? String() : mNode->inputs[0].id;
        }

        UiDoc::Json Items() const
        {
            auto raw = mNode->GetConfigValue("items");
            return raw && raw->IsObject() ? UiDoc::FromDataValue(*raw) : UiDoc::Json::Object();
        }

        UiDoc::Json ItemOf(const String& portId) const
        {
            auto items = Items();
            auto item = items.Get(portId);
            return item && item->IsObject() ? *item : UiDoc::Json::Object();
        }

        // Writes one field of an element's settings (null removes it) as one undo step
        void SetItem(const String& portId, const String& key, const UiDoc::Json& value, bool completed = true)
        {
            auto items = Items();
            auto item = ItemOf(portId);
            if (value.IsNull() && key != "nineSlice" && key != "dpi")
                item.Remove(key);
            else
                item[key] = value;
            items[portId] = item;
            UiDoc::SetConfigJson(mNode->config, "items", items);
            Notify("items", completed);
        }

        const UiDoc::UiKitElement* ElementOf(const String& portId) const
        {
            for (auto& el : mKit.elements)
            {
                if (el.port && *el.port == portId)
                    return &el;
            }
            return nullptr;
        }

        String NameOf(const PipelinePort& port, int index) const
        {
            auto item = ItemOf(port.id);
            String own = item.Get("name") ? item.Get("name")->ToJsString() : String();
            UiDoc::Json raw(!own.IsEmpty() ? own : port.name);
            return UiDoc::CleanKitName(&raw, port.portType == PipelinePortType::TextStyle ? "style_" + (String)(index + 1) :
                                       "element_" + (String)(index + 1));
        }

        void BuildRows()
        {
            auto& theme = PipelineTheme::Get();
            WeakRef<UiKitBody> weakThis(this);
            String selected = SelectedId();
            for (int i = 0; i < mNode->inputs.Count(); i++)
            {
                auto& port = mNode->inputs[i];
                String id = port.id;
                bool isStyle = port.portType == PipelinePortType::TextStyle;
                auto value = GetEditorInput(id);
                auto item = ItemOf(id);
                auto el = ElementOf(id);

                auto row = MakeButton("");
                row->name = "kit row";
                if (auto caption = row->FindLayer("caption"))
                    caption->enabled = false;
                *row->layout = WidgetLayout(Vec2F(0, 1), Vec2F(1, 1), Vec2F(0, -(i*(rowH + rowGap) + rowH)), Vec2F(0, -i*(rowH + rowGap)));
                if (id == selected)
                    AddBorderLayer(*row, "selected", theme.accent, controlRadius, 1.5f, Layout::BothStretch(), 1.0f);
                row->onClick = [weakThis, id]() { if (auto self = weakThis.Lock()) { self->SetString("kitSelected", id, true); self->RebuildBodyKeepingArea(); } };

                auto thumb = mmake<PipelineImageView>();
                thumb->SetFramed(false);
                *thumb->layout = WidgetLayout(Vec2F(0, 0), Vec2F(0, 1), Vec2F(4, 4), Vec2F(40, -4));
                if (isStyle)
                    thumb->SetHint("Aa");
                else if (el)
                    thumb->SetBitmap(DecodeImageBytes(PipelineUtils::Base64Decode(el->png)));
                else
                    thumb->SetBitmap(value.IsImage() ? Picture(value) : nullptr);
                row->AddChild(thumb);

                String type = item.Get("type") ? item.Get("type")->ToJsString() : UiDoc::GuessElementType(NameOf(port, i));
                String asset = item.Get("assetType") ? item.Get("assetType")->ToJsString() : UiDoc::DefaultAssetType(type);
                String sub = !value.IsValid() ? String("not linked") : isStyle ? String("text style") :
                    type + " \xC2\xB7 " + asset + (el && el->nineSlice ? " \xC2\xB7 9" : "");
                auto title = MakeLabel(NameOf(port, i), false);
                StyleLabel(title, 11.5f, 600, theme.fg);
                *title->layout = WidgetLayout(Vec2F(0, 0.5f), Vec2F(1, 1), Vec2F(48, 0), Vec2F(-22, -4));
                row->AddChild(title);
                auto line = MakeLabel(sub + (item.Get("ai") && item.Get("ai")->Bool() ? " \xC2\xB7 AI" : ""), true);
                StyleLabel(line, 10.5f, 400, theme.fg3);
                *line->layout = WidgetLayout(Vec2F(0, 0), Vec2F(1, 0.5f), Vec2F(48, 4), Vec2F(-22, 0));
                row->AddChild(line);

                auto remove = MakeIconButton("ui/pipeline/al_trash.svg", theme.fgDim, Color4(0, 0, 0, 0));
                remove->name = "remove input";
                *remove->layout = WidgetLayout(Vec2F(1, 0.5f), Vec2F(1, 0.5f), Vec2F(-20, -8), Vec2F(-4, 8));
                remove->onClick = [weakThis, id]() { if (auto self = weakThis.Lock()) self->RemoveInput(id); };
                row->AddChild(remove);
                mList->AddChild(row);
            }

            // The add row: the type menu, as every node adds inputs; a link dropped anywhere on the list lands too
            auto add = MakeGhostButton("+ picture or text style");
            add->name = "kit add";
            int n = mNode->inputs.Count();
            *add->layout = WidgetLayout(Vec2F(0, 1), Vec2F(1, 1), Vec2F(0, -(n*(rowH + rowGap) + addH)), Vec2F(0, -n*(rowH + rowGap)));
            add->onClick = [weakThis]()
            {
                auto self = weakThis.Lock();
                auto editor = self ? self->mEditor.Lock() : nullptr;
                if (editor)
                    editor->AddCustomInput(self->mOwner.Lock());
            };
            mList->AddChild(add);
        }

        PipelineValue GetEditorInput(const String& portId) const
        {
            auto editor = mEditor.Lock();
            auto graph = editor ? editor->GetGraph() : nullptr;
            auto node = graph ? graph->FindNode(mNode->id) : nullptr;
            return node ? editor->GetInputValueById(node, portId) : PipelineValue();
        }

        void RemoveInput(const String& portId)
        {
            auto customs = mNode->GetCustomInputs();
            customs.RemoveAll([&](const PipelinePort& p) { return p.id == portId; });
            mNode->SetCustomInputs(customs);
            auto items = Items();
            items.Remove(portId);
            UiDoc::SetConfigJson(mNode->config, "items", items);
            if (mNode->GetConfigString("kitSelected", "") == portId)
                mNode->RemoveConfig("kitSelected");

            PipelineNodeRegistry::SyncNodeWithSchema(mNode);
            if (auto editor = mEditor.Lock())
            {
                if (auto graph = editor->GetGraph())
                    graph->RemoveDanglingEdges();
            }
            Notify("customInputs", true);
            RebuildBodyKeepingArea();
        }

        // --- the details pane ---

        Ref<HorizontalLayout> DetailRow(float& y, const String& label, float height = 26.0f)
        {
            auto line = mmake<HorizontalLayout>();
            line->spacing = 6;
            line->expandWidth = true;
            line->expandHeight = true;
            line->baseCorner = BaseCorner::Left;
            auto row = MakeRow(label, line, 70);
            *row->layout = WidgetLayout(Vec2F(0, 1), Vec2F(1, 1), Vec2F(0, -(y + height)), Vec2F(0, -y));
            mDetails->AddChild(row);
            y += height + 5.0f;
            return line;
        }

        void Segments(const Ref<HorizontalLayout>& line, const Vector<Pair<String, String>>& options, const String& current,
                      const Function<void(const String&)>& onPick)
        {
            auto block = mmake<Widget>();
            auto row = mmake<HorizontalLayout>();
            row->spacing = 2;
            row->expandWidth = true;
            row->expandHeight = true;
            *row->layout = WidgetLayout::BothStretch(3, 3, 3, 3);
            SkinSegmentedRow(*row);
            for (auto& option : options)
            {
                String value = option.first;
                auto segment = MakeSegment(option.second, value == current);
                segment->name = "segment " + value;
                segment->onToggleByUser = [onPick, value](bool) { onPick(value); };
                row->AddChild(segment);
            }
            block->AddChild(row);
            line->AddChild(block);
        }

        void BuildDetails()
        {
            String id = SelectedId();
            auto port = mNode->FindInput(id);
            if (!port)
            {
                auto empty = MakeLabel("Add pictures and text styles with the + row (or drop links on the list)", true);
                *empty->layout = WidgetLayout::BothStretch();
                mDetails->AddChild(empty);
                return;
            }

            if (port->portType == PipelinePortType::TextStyle)
                BuildStyleDetails(*port);
            else
                BuildElementDetails(*port);
        }

        void BuildStyleDetails(const PipelinePort& port)
        {
            auto& theme = PipelineTheme::Get();
            WeakRef<UiKitBody> weakThis(this);
            String id = port.id;
            int index = mNode->inputs.IndexOf([&](const PipelinePort& p) { return p.id == id; });
            float y = 0.0f;
            auto nameLine = DetailRow(y, "Name");
            auto name = MakeEditBox(ItemOf(id).Get("name") ? ItemOf(id).Get("name")->ToJsString() : String(), false, NameOf(port, index));
            name->name = "kit item name";
            name->onChangeCompleted = [weakThis, id](const WString& text) { if (auto self = weakThis.Lock()) self->SetItem(id, "name", UiDoc::Json(Clean((String)text))); };
            nameLine->AddChild(name);

            auto value = GetEditorInput(id);
            auto demo = mmake<PipelineImageView>();
            demo->name = "style demo";
            *demo->layout = WidgetLayout(Vec2F(0, 1), Vec2F(1, 1), Vec2F(0, -(y + 120)), Vec2F(0, -y));
            if (value.type == PipelinePortType::TextStyle)
            {
                UiLayout::Json raw;
                UiLayout::Json::Parse(value.data, raw);
                auto style = UiDoc::NormalizeTextStyle(raw);
                auto picture = UiDoc::RenderTextStyleSample(style, style.role == "number" ? "1 250" : "Forest Quest", true, 1.0);
                demo->SetBitmap(picture.IsEmpty() ? nullptr : picture.ToBitmap());
            }
            else
                demo->SetHint("not linked");
            mDetails->AddChild(demo);
            y += 126;

            auto note = MakeLabel("The style keeps its own settings in its node; the kit only renames it", true);
            StyleLabel(note, 10.5f, 400, theme.fg3);
            *note->layout = WidgetLayout(Vec2F(0, 1), Vec2F(1, 1), Vec2F(0, -(y + 30)), Vec2F(0, -y));
            note->horOverflow = Label::HorOverflow::Wrap;
            mDetails->AddChild(note);
        }

        static String Clean(const String& text)
        {
            String clean;
            for (auto c : text)
            {
                if (isalnum((unsigned char)c) || c == '_' || c == '-')
                    clean += c;
            }
            return clean;
        }

        void BuildElementDetails(const PipelinePort& port)
        {
            auto& theme = PipelineTheme::Get();
            WeakRef<UiKitBody> weakThis(this);
            String id = port.id;
            int index = mNode->inputs.IndexOf([&](const PipelinePort& p) { return p.id == id; });
            auto item = ItemOf(id);
            auto el = ElementOf(id);
            String name = NameOf(port, index);
            String type = item.Get("type") ? item.Get("type")->ToJsString() : UiDoc::GuessElementType(name);
            String asset = item.Get("assetType") ? item.Get("assetType")->ToJsString() : UiDoc::DefaultAssetType(type);
            String bg = item.Get("bg") ? item.Get("bg")->ToJsString() : String("keep");
            auto setNine = UiDoc::NormalizeNine(item.Get("nineSlice"));
            std::optional<UiDoc::UiNine> detected;
            if (el && el->source.IsObject() && el->source.Get("detected"))
                detected = UiDoc::NormalizeNine(el->source.Get("detected"));
            auto nine = setNine ? setNine : detected;
            float y = 0.0f;

            // Name and type
            auto head = DetailRow(y, "Name");
            auto nameField = MakeEditBox(item.Get("name") ? item.Get("name")->ToJsString() : String(), false, name);
            nameField->name = "kit item name";
            nameField->onChangeCompleted = [weakThis, id](const WString& text) { if (auto self = weakThis.Lock()) self->SetItem(id, "name", UiDoc::Json(Clean((String)text))); };
            head->AddChild(nameField);
            auto typePick = MakeDropDown(UiDoc::UiElementTypes(), type);
            typePick->name = "kit item type";
            bool assetSet = item.Get("assetType") != nullptr;
            typePick->onSelectedText = [weakThis, id, assetSet](const WString& text)
            {
                auto self = weakThis.Lock();
                if (!self)
                    return;
                String t = (String)text;
                self->SetItem(id, "type", UiDoc::Json(t), assetSet);
                if (!assetSet)
                    self->SetItem(id, "assetType", UiDoc::Json(UiDoc::DefaultAssetType(t)));
                self->RebuildBodyKeepingArea();
            };
            head->AddChild(typePick);

            // The source with the guides
            auto value = GetEditorInput(id);
            String sourcePng = value.IsImage() ? value.GetPngBytes() : String();
            String uploadId = item.Get("prepared") && item.Get("prepared")->Get("uploadId") ? item.Get("prepared")->Get("uploadId")->ToJsString() : String();
            if (bg == "ai" && !uploadId.IsEmpty())
            {
                String prepared = PipelineUtils::ReadFileBytes(PipelineUtils::GetUploadPath(uploadId));
                if (!prepared.IsEmpty())
                    sourcePng = prepared;
            }
            auto guides = mmake<PipelineKitGuides>();
            guides->name = "kit source";
            *guides->layout = WidgetLayout(Vec2F(0, 1), Vec2F(1, 1), Vec2F(0, -(y + 170)), Vec2F(0, -y));
            int sw = 0, sh = 0;
            if (!sourcePng.IsEmpty() && PipelinePngMeta::Size(sourcePng, sw, sh))
                guides->SetBitmap(DecodeImageBytes(sourcePng));
            else
                guides->SetHint("not linked - the picture shows once its input is produced");
            if (el && el->source.IsObject())
            {
                sw = (int)el->source.Get("w")->Num(sw);
                sh = (int)el->source.Get("h")->Num(sh);
            }
            guides->SetGuides(sw, sh, el && el->source.IsObject() && el->source.Get("trim") ? *el->source.Get("trim") : UiDoc::Json(),
                              asset == "sliced" ? nine : std::nullopt, !setNine.has_value());
            guides->onSet = [weakThis, id](const UiDoc::UiNine& n)
            {
                if (auto self = weakThis.Lock())
                {
                    self->SetItem(id, "nineFrom", UiDoc::Json(nullptr), false);
                    self->SetItem(id, "nineSlice", UiDoc::NineToJson(n));
                    self->RebuildBodyKeepingArea();
                }
            };
            mDetails->AddChild(guides);
            y += 172;

            String caption = "source";
            if (asset == "sliced")
                caption += setNine ? (item.Get("nineFrom") && item.Get("nineFrom")->ToJsString() == "ai" ? " \xC2\xB7 9-slice by AI" : " \xC2\xB7 9-slice set") : " \xC2\xB7 9-slice found";
            if (el)
            {
                caption += "   in the kit " + UiDoc::JsNum(el->width) + "\xC3\x97" + UiDoc::JsNum(el->height) + " \xC2\xB7 " +
                    (String)(int)UiDoc::JsRound(el->dpi.value_or(UiDoc::UI_BASE_DPI)) + " dpi";
                if (el->source.IsObject() && el->source.Get("collapsed") && el->source.Get("collapsed")->Bool())
                    caption += " \xC2\xB7 middle shrunk";
            }
            auto captionLabel = MakeLabel(caption, true);
            StyleLabel(captionLabel, 10.0f, 400, theme.fg3);
            *captionLabel->layout = WidgetLayout(Vec2F(0, 1), Vec2F(1, 1), Vec2F(0, -(y + 14)), Vec2F(0, -y));
            mDetails->AddChild(captionLabel);
            y += 18;

            Segments(DetailRow(y, "Asset"), { { "simple", "Simple" }, { "sliced", "9-slice" }, { "tiled", "Tiled" } }, asset,
                     [weakThis, id](const String& v) { if (auto self = weakThis.Lock()) { self->SetItem(id, "assetType", UiDoc::Json(v)); self->RebuildBodyKeepingArea(); } });

            auto variantLine = DetailRow(y, "Variant");
            auto variant = MakeEditBox(item.Get("variant") ? item.Get("variant")->ToJsString() : String(), false, "primary, disabled, selected...");
            variant->name = "kit item variant";
            variant->onChangeCompleted = [weakThis, id](const WString& text) { if (auto self = weakThis.Lock()) self->SetItem(id, "variant", UiDoc::Json((String)text)); };
            variantLine->AddChild(variant);

            Segments(DetailRow(y, "Background"), { { "keep", "Keep" }, { "key", "Cut flat" }, { "ai", "AI" } }, bg,
                     [weakThis, id](const String& v) { if (auto self = weakThis.Lock()) { self->SetItem(id, "bg", UiDoc::Json(v)); self->RebuildBodyKeepingArea(); } });

            auto trimLine = DetailRow(y, "Margins");
            bool trim = !(item.Get("trim") && item.Get("trim")->IsBool() && !item.Get("trim")->Bool());
            auto trimToggle = MakeCheckbox("Trim transparent margins", trim);
            trimToggle->onToggleByUser = [weakThis, id](bool v) { if (auto self = weakThis.Lock()) self->SetItem(id, "trim", UiDoc::Json(v)); };
            trimLine->AddChild(trimToggle);

            // The density: set here, else the input's, else the base
            auto dpiLine = DetailRow(y, "DPI");
            auto dpiSet = UiDoc::CleanDpi(item.Get("dpi"));
            double dpiNow = dpiSet ? *dpiSet : el ? el->dpi.value_or(UiDoc::UI_BASE_DPI) : UiDoc::UI_BASE_DPI;
            auto dpiField = MakeEditBox(dpiSet ? UiDoc::JsNum(*dpiSet) : String(), false, (String)(int)UiDoc::JsRound(dpiNow));
            dpiField->name = "kit item dpi";
            dpiField->layout->minWidth = dpiField->layout->maxWidth = 60;
            dpiField->onChangeCompleted = [weakThis, id](const WString& text)
            {
                auto self = weakThis.Lock();
                if (!self)
                    return;
                String s = ((String)text).Trimed();
                UiDoc::Json json(s);
                auto dpi = s.IsEmpty() ? std::nullopt : UiDoc::CleanDpi(&json);
                if (s.IsEmpty())
                    self->SetItem(id, "dpi", UiDoc::Json(nullptr));
                else if (dpi)
                    self->SetItem(id, "dpi", UiDoc::Json(*dpi));
            };
            dpiLine->AddChild(dpiField);
            String from = dpiSet ? "set here" : el && el->dpiFrom.value_or("") == "input" ? "from the input" : "the base";
            auto dpiNote = MakeLabel(from + " \xC2\xB7 " + UiDoc::DensityName(dpiNow), true);
            StyleLabel(dpiNote, 10.5f, 400, theme.fg3);
            dpiLine->AddChild(dpiNote);

            if (asset == "sliced")
            {
                auto nineLine = DetailRow(y, "9-slice");
                const char* letters[] = { "L", "T", "R", "B" };
                for (int side = 0; side < 4; side++)
                {
                    double v = !nine ? 0.0 : side == 0 ? nine->left : side == 1 ? nine->top : side == 2 ? nine->right : nine->bottom;
                    auto field = MakeEditBox(nine ? UiDoc::JsNum(v) : String(), false, letters[side]);
                    field->name = String("kit nine ") + letters[side];
                    field->SetFilterInteger();
                    field->layout->minWidth = 34;
                    auto base = nine.value_or(UiDoc::UiNine());
                    field->onChangeCompleted = [weakThis, id, base, side](const WString& text)
                    {
                        auto self = weakThis.Lock();
                        if (!self)
                            return;
                        auto n = base;
                        double value = Math::Max(0.0, UiDoc::JsRound(atof(((String)text).Data())));
                        (side == 0 ? n.left : side == 1 ? n.top : side == 2 ? n.right : n.bottom) = value;
                        self->SetItem(id, "nineFrom", UiDoc::Json(nullptr), false);
                        self->SetItem(id, "nineSlice", UiDoc::NineToJson(n));
                        self->RebuildBodyKeepingArea();
                    };
                    nineLine->AddChild(field);
                }
                auto autoButton = MakeGhostButton("Auto");
                autoButton->layout->minWidth = autoButton->layout->maxWidth = 46;
                autoButton->interactable = setNine.has_value();
                autoButton->onClick = [weakThis, id]()
                {
                    if (auto self = weakThis.Lock())
                    {
                        self->SetItem(id, "nineFrom", UiDoc::Json(nullptr), false);
                        self->SetItem(id, "nineSlice", UiDoc::Json(nullptr));
                        self->RebuildBodyKeepingArea();
                    }
                };
                nineLine->AddChild(autoButton);

                auto middleLine = DetailRow(y, "Middle");
                bool optimize = !(item.Get("optimize") && item.Get("optimize")->IsBool() && !item.Get("optimize")->Bool());
                auto middle = MakeCheckbox("Shrink a uniform middle", optimize);
                middle->onToggleByUser = [weakThis, id](bool v) { if (auto self = weakThis.Lock()) self->SetItem(id, "optimize", UiDoc::Json(v)); };
                middleLine->AddChild(middle);
            }

            auto descLine = DetailRow(y, "Description", 44);
            auto description = MakeEditBox(item.Get("description") ? item.Get("description")->ToJsString() : String(), true,
                                           "What it is and where it is used");
            description->name = "kit item description";
            description->onChangeCompleted = [weakThis, id](const WString& text) { if (auto self = weakThis.Lock()) self->SetItem(id, "description", UiDoc::Json((String)text)); };
            descLine->AddChild(description);

            mAiOne = MakeGhostButton("Describe this one with AI");
            mAiOne->name = "ai one";
            PipelineImageTheme::MakeAccentGhost(mAiOne);
            *mAiOne->layout = WidgetLayout(Vec2F(0, 1), Vec2F(0, 1), Vec2F(0, -(y + 26)), Vec2F(200, -y));
            mAiOne->onClick = [weakThis, id]() { if (auto self = weakThis.Lock()) PipelineUiAi::AnalyzeKit(self->mEditor.Lock(), self->mNode->id, id); };
            mDetails->AddChild(mAiOne);
        }
    };

    Ref<PipelineNodeBody> CreateUiKitNodeBody(const String& type)
    {
        return type == "uiKit" ? mmake<UiKitBody>() : nullptr;
    }
}
