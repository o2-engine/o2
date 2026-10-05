#include "o2Editor/stdafx.h"
#include "PipelineNodeBodyFactories.h"

#include "o2/Render/Render.h"
#include "o2/Render/Text.h"
#include "o2/Scene/UI/UIManager.h"
#include "o2/Scene/UI/WidgetLayer.h"
#include "o2/Scene/UI/WidgetLayout.h"
#include "o2/Scene/UI/Widgets/Button.h"
#include "o2/Scene/UI/Widgets/EditBox.h"
#include "o2/Scene/UI/Widgets/HorizontalLayout.h"
#include "o2/Scene/UI/Widgets/Label.h"
#include "o2/Scene/UI/Widgets/Toggle.h"
#include "o2/Scene/UI/Widgets/VerticalLayout.h"
#include "o2/Utils/Bitmap/Bitmap.h"
#include "o2/Utils/Editor/EditorScope.h"
#include "o2Editor/Pipeline/Nodes/PipelineNodesCommon.h"
#include "o2Editor/Pipeline/PipelineRegions.h"
#include "o2Editor/Pipeline/PipelineSettings.h"
#include "o2Editor/Pipeline/Providers/AiRouter.h"
#include "o2Editor/Pipeline/Providers/GeminiProvider.h"
#include "o2Editor/Windows/PipelineWindow/PipelineEditor.h"
#include "o2Editor/Windows/PipelineWindow/PipelineNodeWidget.h"
#include "o2Editor/Windows/PipelineWindow/PipelinePaintEditor.h"

namespace Editor
{
    using namespace PipelineControls;

    // -----------------------------------------------------------------------------------------------------------------
    // Actions under the source: the part buttons, then the transparent switch at the right end of the line when the card is
    // at least 400 wide, else on a line of its own under them, at the right
    // -----------------------------------------------------------------------------------------------------------------
    class PipelineExtractActions : public Widget
    {
    public:
        static constexpr float buttonsHeight = 24.0f;  // Line of the part buttons
        static constexpr float switchLine = 19.0f;     // Line of the switch when it does not fit beside them
        static constexpr float inlineWidth = 380.0f;   // Row width of a 400 wide card
        static constexpr float switchWidth = 140.0f;   // Width of the switch

    public:
        explicit PipelineExtractActions(RefCounter* refCounter): Widget(refCounter) {}

        // Sets the line of buttons and the switch
        void Setup(const Ref<Widget>& buttons, const Ref<Widget>& toggle)
        {
            mButtons = buttons;
            mSwitch = toggle;
            AddChild(mButtons);
            AddChild(mSwitch);
        }

        // Returns the height of the row at the width
        static float HeightForWidth(float width) { return width >= inlineWidth ? buttonsHeight : buttonsHeight + switchLine; }

        void UpdateSelfTransform() override
        {
            Widget::UpdateSelfTransform();
            if (!mButtons || !mSwitch)
                return;

            if (layout->GetWidth() >= inlineWidth)
            {
                *mButtons->layout = WidgetLayout(Vec2F(0, 1), Vec2F(1, 1), Vec2F(0, -buttonsHeight), Vec2F(-switchWidth - 6.0f, 0));
                *mSwitch->layout = WidgetLayout(Vec2F(1, 1), Vec2F(1, 1), Vec2F(-switchWidth, -buttonsHeight + 3.0f), Vec2F(0, -3.0f));
            }
            else
            {
                *mButtons->layout = WidgetLayout(Vec2F(0, 1), Vec2F(1, 1), Vec2F(0, -buttonsHeight), Vec2F(0, 0));
                *mSwitch->layout = WidgetLayout(Vec2F(1, 1), Vec2F(1, 1), Vec2F(-switchWidth, -buttonsHeight - switchLine), Vec2F(0, -buttonsHeight - 1.0f));
            }
        }

    private:
        Ref<Widget> mButtons; // Add part and auto split
        Ref<Widget> mSwitch;  // Transparent background switch
    };

    // ---------------------------------------------------------------------------------
    // Progress of a part being computed: its cell dimmed and a ring with a turning arc
    // ---------------------------------------------------------------------------------
    class PipelinePartSpinner : public Widget
    {
    public:
        explicit PipelinePartSpinner(RefCounter* refCounter): Widget(refCounter) {}

        void Update(float dt) override
        {
            Widget::Update(dt);
            mTime = Math::Mod(mTime + dt, turnTime);
        }

        void Draw() override
        {
            if (!mResEnabledInHierarchy || mIsClipped)
                return;

            RectF rect = layout->GetWorldRect();
            o2Render.DrawFilledPolygon({ rect.LeftBottom(), Vec2F(rect.left, rect.top), rect.RightTop(), Vec2F(rect.right, rect.bottom) },
                                       Color4(0, 0, 0, 89));

            // The ring keeps its size in canvas units, like the rest of the card
            float unitsPerPixel = Math::Max(0.0001f, o2Render.GetCamera().GetScale2D().x);
            float width = Math::Max(1.0f, stroke/unitsPerPixel);
            Vec2F center = rect.Center();
            o2Render.DrawAACircle(center, radius, Color4(255, 255, 255, 64), 32, width);

            // A quarter of the ring, turning clockwise once per turn time
            float start = Math::PI()*0.25f - mTime/turnTime*Math::PI()*2.0f;
            Vector<Vec2F> arc;
            for (int i = 0; i <= 8; i++)
            {
                float angle = start + Math::PI()*0.5f*(float)i/8.0f;
                arc.Add(center + Vec2F(Math::Cos(angle), Math::Sin(angle))*radius);
            }
            o2Render.DrawAALine(arc, accentColor, width);
        }

    private:
        static constexpr float radius = 8.0f, stroke = 2.0f, turnTime = 0.8f;

        float mTime = 0.0f; // Seconds into the current turn
    };

    // ---------------------------------------------------------------------------------
    // Parts of an extract node as a grid of cells: the result of each part with its name
    // under it, the selected one outlined, a dot before the name of a part with its own
    // background settings, a spinner over a part being computed and a red frame round a
    // part that failed; as many columns as the width fits
    // ---------------------------------------------------------------------------------
    class PipelinePartsGrid : public Widget
    {
    public:
        Function<void(const String&)>                onSelect; // A cell was clicked
        Function<void(const String&)>                onRegen;  // The regenerate button of a cell was pressed
        Function<void(const String&)>                onRemove; // The remove button of a cell was pressed
        Function<void(const String&, const String&)> onRename; // The name of the selected cell was edited

    public:
        explicit PipelinePartsGrid(RefCounter* refCounter): Widget(refCounter) {}

        // Rebuilds the cells for the parts; the selected one gets the name field and the outline
        void SetParts(const Vector<PipelineExtractRegion>& regions, const String& selectedId)
        {
            RemoveAllChildren();
            mCells.Clear();
            mSingle = regions.Count() == 1;

            WeakRef<PipelinePartsGrid> weakThis(this);
            for (int i = 0; i < regions.Count(); i++)
            {
                String id = regions[i].id;
                Cell cell;
                cell.id = id;
                cell.name = regions[i].name;
                cell.root = mmake<Widget>();
                auto frame = mmake<PipelineRoundedRect>();
                frame->color = accentColor;
                frame->radius = 5.0f;
                frame->roundBottom = true;
                cell.frame = cell.root->AddLayer("selected", frame, Layout::BothStretch(-2, -2, -2, -2));
                auto errorFrame = mmake<PipelineRoundedRect>();
                errorFrame->color = errorColor;
                errorFrame->radius = 5.0f;
                errorFrame->roundBottom = true;
                cell.errorFrame = cell.root->AddLayer("error", errorFrame, Layout::BothStretch(-2, -2, -2, -2));
                cell.errorFrame->enabled = false;

                cell.image = mmake<PipelineImageView>();
                cell.image->SetHint((String)(i + 1));
                cell.root->AddChild(cell.image);

                cell.spinner = mmake<PipelinePartSpinner>();
                cell.spinner->name = "part spinner";
                cell.root->AddChild(cell.spinner);
                cell.spinner->SetEnabledForcible(false);

                cell.pick = o2UI.CreateWidget<Button>("pipeline icon");
                cell.pick->name = "select part";
                if (auto icon = cell.pick->GetLayer("icon"))
                    icon->enabled = false;
                cell.pick->onClick = [weakThis, id]() { if (auto self = weakThis.Lock()) self->onSelect(id); };
                // The part's output port sits on the bottom-right corner of the image: that corner is the port's
                WeakRef<Button> weakPick(cell.pick);
                cell.pick->isPointInside = [weakPick](const Vec2F& p)
                {
                    auto pick = weakPick.Lock();
                    if (!pick) return false;
                    RectF rect = pick->layout->GetWorldRect();
                    return rect.IsInside(p) && (p - Vec2F(rect.right, rect.bottom)).Length() > 14.0f;
                };
                cell.root->AddChild(cell.pick);

                auto tools = mmake<HorizontalLayout>();
                tools->spacing = 2;
                tools->expandWidth = false;
                tools->expandHeight = true;
                tools->baseCorner = BaseCorner::Right;
                cell.tools = tools;
                auto regen = MakeIconButton("ui/pipeline/btn_loop.svg", textColor, Color4(255, 255, 255, 220));
                regen->name = "regen part";
                regen->layout->minWidth = 20;
                regen->layout->maxWidth = 20;
                regen->onClick = [weakThis, id]() { if (auto self = weakThis.Lock()) self->onRegen(id); };
                tools->AddChild(regen);
                if (regions.Count() > 1)
                {
                    auto remove = MakeIconButton("ui/UI4_small_trash_icon.svg", textColor, Color4(255, 255, 255, 220));
                    remove->name = "remove part";
                    remove->layout->minWidth = 20;
                    remove->layout->maxWidth = 20;
                    remove->onClick = [weakThis, id]() { if (auto self = weakThis.Lock()) self->onRemove(id); };
                    tools->AddChild(remove);
                }
                cell.root->AddChild(cell.tools);

                // One part needs no caption: the prompt field under the grid is its name
                if (!mSingle)
                {
                    String fallback = "part " + (String)(i + 1);
                    cell.edit = MakeEditBox(regions[i].name, false, fallback);
                    cell.edit->name = "part name";
                    cell.edit->onChangeCompleted = [weakThis, id](const WString& text) { if (auto self = weakThis.Lock()) self->onRename(id, (String)text); };
                    cell.root->AddChild(cell.edit);

                    cell.label = MakeLabel(regions[i].name.IsEmpty() ? fallback : regions[i].name, true);
                    cell.label->horOverflow = Label::HorOverflow::Dots;
                    cell.label->horAlign = HorAlign::Middle;
                    cell.root->AddChild(cell.label);

                    auto dot = mmake<PipelineRoundedRect>();
                    dot->color = accentColor;
                    dot->radius = dotSize*0.5f;
                    dot->roundBottom = true;
                    cell.dot = mmake<Widget>();
                    cell.dot->name = "own settings";
                    cell.dot->AddLayer("dot", dot, Layout::BothStretch());
                    cell.root->AddChild(cell.dot);
                    cell.own = regions[i].ownTransparency;
                    cell.dot->SetEnabledForcible(cell.own);
                    cell.captionWidth = Text::GetTextSize(cell.label->GetText(), cell.label->GetFont(), cell.label->GetHeight(), Vec2F(),
                                                          HorAlign::Left, VerAlign::Top, false).x;
                }

                AddChild(cell.root);
                mCells.Add(cell);
            }
            SetSelected(selectedId);
            LayoutCells();
        }

        // Moves the outline and the name field to the selected part without rebuilding the cells
        void SetSelected(const String& id)
        {
            for (auto& cell : mCells)
            {
                bool on = cell.id == id;
                cell.root->name = on ? "part cell selected" : "part cell";
                if (cell.frame)
                    cell.frame->enabled = on;
                // Forcible: a widget disabled before its first update would otherwise keep drawing through its fade state
                if (cell.edit)
                {
                    cell.edit->SetEnabledForcible(on);
                    if (on)
                        cell.edit->SetText(cell.name);
                }
                if (cell.label)
                    cell.label->SetEnabledForcible(!on);

                cell.selected = on;
            }
            LayoutCells();
        }

        // Shows or hides the dot of a part with its own background settings
        void SetOwnSettings(const String& id, bool own)
        {
            for (auto& cell : mCells)
            {
                if (cell.id != id || !cell.dot)
                    continue;

                cell.own = own;
                cell.dot->SetEnabledForcible(own);
            }
            LayoutCells();
        }

        // Shows the run state of every part in place: a spinner over the running ones, a red frame round the failed ones
        void SetPartStates(const Map<String, String>& states)
        {
            for (auto& cell : mCells)
            {
                String state;
                states.TryGetValue(cell.id, state);
                cell.spinner->SetEnabledForcible(state == "running");
                cell.errorFrame->enabled = state == "error";
            }
        }

        // Returns the number of cells
        int GetCellCount() const { return mCells.Count(); }

        // Returns true and the drawn bottom-right corner of the part's image in world space, once its cell is laid out
        bool GetImageCorner(int index, Vec2F& corner) const
        {
            if (index < 0 || index >= mCells.Count() || mCells[index].image->layout->GetWidth() <= 0.0f)
                return false;

            RectF rect = mCells[index].image->layout->GetWorldRect();
            corner = Vec2F(rect.right, rect.bottom);
            return true;
        }

        // Returns the index of the part's cell, -1 when there is none
        int GetCellIndex(const String& id) const
        {
            return mCells.IndexOf([&](const Cell& cell) { return cell.id == id; });
        }

        // Shows the result of a part in its cell
        void SetPartImage(const String& id, const Ref<Bitmap>& bitmap)
        {
            for (auto& cell : mCells)
            {
                if (cell.id == id)
                    cell.image->SetBitmap(bitmap);
            }
        }

        void UpdateSelfTransform() override
        {
            Widget::UpdateSelfTransform();
            LayoutCells();
        }

    private:
        inline static const Color4 errorColor = Color4(249, 93, 72, 255);

        static constexpr float gap = PipelinePairLayout::gridGap, dotSize = 6.0f;

        struct Cell
        {
            String                 id;
            String                 name;  // Part name, put into the field when the cell is selected
            Ref<Widget>            root;
            Ref<WidgetLayer>       frame; // Outline shown on the selected cell
            Ref<WidgetLayer>       errorFrame; // Red outline of a part that failed in this run
            Ref<PipelineImageView> image;
            Ref<PipelinePartSpinner> spinner; // Progress over a part being computed
            Ref<Button>            pick;
            Ref<Widget>            tools;
            Ref<EditBox>           edit;  // Name field, shown on the selected cell
            Ref<Label>             label; // Name caption of the other cells
            Ref<Widget>            dot;   // Mark of a part with its own background settings, before the name
            bool                   own = false;
            bool                   selected = false;
            float                  captionWidth = 0.0f; // Width of the name caption, the dot goes right before it

            bool operator==(const Cell& other) const { return id == other.id; }
        };

        struct Geometry { int cols = 1, rows = 1; float cellW = 0.0f, cellH = 0.0f, labelH = 0.0f; };

        Vector<Cell> mCells;
        bool         mSingle = true;

        // Cells of the right pane: the pair geometry of the row the grid fills
        Geometry Geom(float width, float height) const
        {
            auto pair = PipelinePairLayout::ExtractGrid(width*2.0f + PipelinePairLayout::paneGap, mCells.Count(), height);
            Geometry g;
            g.cols = pair.cols;
            g.rows = pair.rows;
            g.cellW = pair.cellW;
            g.cellH = pair.cellH;
            g.labelH = pair.labelH;
            return g;
        }

        void LayoutCells()
        {
            float width = layout->GetWidth();
            if (width <= 0.0f)
                return;

            Geometry g = Geom(width, layout->GetHeight());
            for (int i = 0; i < mCells.Count(); i++)
            {
                auto& cell = mCells[i];
                int col = i % g.cols, row = i / g.cols;
                float x = col * (g.cellW + gap), y = row * (g.cellH + g.labelH + gap);
                *cell.root->layout = WidgetLayout(Vec2F(0, 1), Vec2F(0, 1), Vec2F(x, -(y + g.cellH + g.labelH)), Vec2F(x + g.cellW, -y));
                *cell.image->layout = WidgetLayout::HorStretch(VerAlign::Top, 0, 0, g.cellH, 0);
                *cell.spinner->layout = WidgetLayout::HorStretch(VerAlign::Top, 0, 0, g.cellH, 0);
                *cell.pick->layout = WidgetLayout::HorStretch(VerAlign::Top, 0, 0, g.cellH, 0);
                if (cell.frame)
                    cell.frame->layout = Layout(Vec2F(0, 1), Vec2F(1, 1), Vec2F(-2, -g.cellH - 2), Vec2F(2, 2));
                cell.errorFrame->layout = Layout(Vec2F(0, 1), Vec2F(1, 1), Vec2F(-2, -g.cellH - 2), Vec2F(2, 2));
                *cell.tools->layout = WidgetLayout(Vec2F(1, 1), Vec2F(1, 1), Vec2F(-46, -24), Vec2F(-3, -3));
                // The name field of the selected part makes room for the dot; a centred caption has it right before its text
                float fieldLeft = cell.own ? dotSize + 4.0f : 0.0f, nameMiddle = (g.labelH - 2.0f)*0.5f;
                float captionArea = g.cellW - 16.0f;
                float dotLeft = cell.selected ? 1.0f :
                    Math::Max(1.0f, (captionArea - Math::Min(cell.captionWidth, captionArea))*0.5f - dotSize - 3.0f);
                if (cell.dot)
                {
                    *cell.dot->layout = WidgetLayout(Vec2F(0, 0), Vec2F(0, 0), Vec2F(dotLeft, nameMiddle - dotSize*0.5f),
                                                     Vec2F(dotLeft + dotSize, nameMiddle + dotSize*0.5f));
                }
                if (cell.edit)
                    *cell.edit->layout = WidgetLayout::HorStretch(VerAlign::Bottom, fieldLeft, 16, g.labelH - 2.0f, 0);
                if (cell.label)
                    *cell.label->layout = WidgetLayout::HorStretch(VerAlign::Bottom, 0, 16, g.labelH - 2.0f, 0);
            }
        }
    };

    class ImageExtractBody : public PipelineNodeBody
    {
    public:
        using PipelineNodeBody::PipelineNodeBody;
        void Build() override
        {
            WeakRef<ImageExtractBody> weakThis(this);

            mGrid = mmake<PipelinePartsGrid>();
            mGrid->name = "parts";
            mGrid->onSelect = [weakThis](const String& id) { if (auto self = weakThis.Lock()) self->SelectRegion(id); };
            mGrid->onRename = [weakThis](const String& id, const String& name) { if (auto self = weakThis.Lock()) self->RenameRegion(id, name); };
            mGrid->onRemove = [weakThis](const String& id) { if (auto self = weakThis.Lock()) self->RemoveRegion(id); };
            mGrid->onRegen = [weakThis](const String& id)
            {
                auto self = weakThis.Lock();
                if (!self)
                    return;

                if (auto editor = self->mEditor.Lock())
                    editor->RunNodePort(self->mNode->id, id);
            };
            // No drawing here: the source with the part boxes and the parts grid share one row
            mPaint = mmake<PipelinePaintEditor>();
            mPaint->name = "draw stage";
            mPaint->onConfigChanged = [weakThis](const String& key, bool completed)
            {
                auto self = weakThis.Lock();
                if (!self)
                    return;

                // The region tool edits the box of the selected part
                if (key == "roi")
                    self->ReadSelectedBox(completed);
                else
                    self->Notify(key, completed);
            };
            mPaint->onRegionPicked = [weakThis](const String& id) { if (auto self = weakThis.Lock()) self->SelectRegion(id); };
            mPaint->onRegionRemoved = [weakThis]() { if (auto self = weakThis.Lock()) self->RemoveRegion(self->SelectedId()); };
            mPaint->Init(mNode, true);
            mPaint->SetDrawingEnabled(false);
            // The stage edits the "roi" box, which a pipeline saved elsewhere may not keep in step with the selected part
            if (RoiIsStale())
                PushSelectedBox();
            mPaint->SetBeside(mGrid);
            AddFlexible(mPaint, [weakThis](float width)
            {
                auto self = weakThis.Lock();
                return self ? self->mPaint->GetBarsHeight(width) + PipelinePairLayout::ExtractGrid(width, self->Regions().Count()).rowNatural : 0.0f;
            });
            MarkContent(mPaint);

            auto add = MakeButton("+ Add part");
            add->name = "add part";
            add->onClick = [weakThis]() { if (auto self = weakThis.Lock()) self->AddRegion(); };
            mAutoButton = MakeButton("Auto split");
            mAutoButton->name = "auto split";
            mAutoButton->onClick = [weakThis]() { if (auto self = weakThis.Lock()) self->AutoSplit(); };
            auto buttons = mmake<HorizontalLayout>();
            buttons->name = "part actions";
            buttons->spacing = 6;
            buttons->expandWidth = true;
            buttons->expandHeight = true;
            buttons->baseCorner = BaseCorner::Left;
            buttons->AddChild(add);
            buttons->AddChild(mAutoButton);
            mTransparentSwitch = MakeTransparentSwitch();
            auto actions = mmake<PipelineExtractActions>();
            actions->name = "actions";
            actions->Setup(buttons, mTransparentSwitch);
            AddRow(actions, [](float width) { return PipelineExtractActions::HeightForWidth(width); });

            // The name of the selected part is what the model is asked to keep
            mPromptRegion = SelectedId();
            mPrompt = MakeEditBox(NameOf(mPromptRegion), true, "What to keep, e.g. the green chip");
            mPrompt->name = "part prompt";
            mPrompt->onChangeCompleted = [weakThis](const WString& text) { if (auto self = weakThis.Lock()) self->RenameRegion(self->mPromptRegion, (String)text); };
            AddRow(mPrompt, 46);

            if (BeginParams(GetBool("transparentBg", false) ? Vector<String>{ "Model", "Seed", "Transparency" } : Vector<String>{ "Model", "Seed" }))
            {
                mParamsList = mParams;
                AddParamRows();
            }
            EndParams();

            RebuildParts();
            OnOutputChanged();
        }

        void OnOutputChanged() override
        {
            PipelineNodeBody::OnOutputChanged();
            if (mPaint)
            {
                auto input = GetInput("image");
                mPaint->SetBackground(input.IsImage() ? input.GetBitmap() : nullptr);
            }

            RefreshPartResults();
        }

        void OnConfigChanged() override
        {
            if (mPaint)
                mPaint->RefreshFromConfig();

            RebuildParts();
            RefreshParamRows(false);
        }

        void OnRunStateChanged() override
        {
            ShowPartStates();
        }

        // With several parts the transparency rows edit the selected part's own settings when it has them
        DataValue* GetPartTransparency() const override
        {
            return PipelineRegions::FindTransparency(*mNode, SelectedId());
        }

        // The output ports of several parts sit on the image corners of their cells, like the links leave them in AssetsLine
        bool HasBodyPort(const String& portId) const override
        {
            return mGrid && mGrid->GetCellCount() > 1 && mGrid->GetCellIndex(portId) >= 0;
        }

        bool GetBodyPortOffset(const String& portId, Vec2F& offset) const override
        {
            auto owner = mOwner.Lock();
            if (!owner || !mPaint || !HasBodyPort(portId))
                return false;

            float width = owner->GetCardWidth(), rowWidth = width - mPadding*2.0f;
            float top = 0.0f, height = 0.0f;
            if (!GetRowPlacement(mPaint, width, owner->GetBodyAreaHeight(), top, height))
                return false;

            float bars = mPaint->GetBarsHeight(rowWidth);
            auto grid = PipelinePairLayout::ExtractGrid(rowWidth, mGrid->GetCellCount(), height - bars);
            int index = mGrid->GetCellIndex(portId);
            Vec2F corner = PipelinePairLayout::ExtractCellCorner(grid, index);
            offset = Vec2F(mPadding + corner.x, top + bars + corner.y);

            // The cells snap to whole pixels on their way down the card: the drawn corner wins unless it is from an older layout
            Vec2F drawn;
            if (mGrid->GetImageCorner(index, drawn))
            {
                RectF body = layout->GetWorldRect();
                Vec2F fromBody(drawn.x - body.left, body.top - drawn.y);
                if (Math::Abs(fromBody.x - offset.x) < 2.0f && Math::Abs(fromBody.y - offset.y) < 2.0f)
                    offset = fromBody;
            }
            return true;
        }

    private:
        Ref<PipelinePaintEditor> mPaint;        // Source image with the boxes of the parts
        Ref<PipelinePartsGrid>   mGrid;         // Cells of the parts with their results
        Ref<Button>              mAutoButton;   // Asks the model to list the parts
        Ref<Toggle>              mTransparentSwitch; // Transparent background of the node or of the selected part
        Ref<EditBox>             mPrompt;       // Name of the selected part
        String                   mPromptRegion; // Part the prompt field edits
        bool mWritingRoi = false; // True while the selected box is pushed into the paint editor

        Map<String, String>      mPartData;    // Encoded result of each part the bitmap below was decoded from
        Map<String, Ref<Bitmap>> mPartBitmaps; // Decoded result of each part, kept so a selection change decodes nothing

        Ref<ParamsList> mParamsList; // The open parameter list, refilled in place when the edited part changes

    private:
        Vector<PipelineExtractRegion> Regions() const { return PipelineRegions::Read(*mNode); }

        String SelectedId() const
        {
            String selected = mNode->GetConfigString("selectedRegion", "");
            auto regions = Regions();
            for (auto& region : regions)
            {
                if (region.id == selected)
                    return selected;
            }

            return regions.IsEmpty() ? String() : regions[0].id;
        }

        // Writes the regions, keeps the output ports in step and rebuilds the card
        void WriteRegions(const Vector<PipelineExtractRegion>& regions, const String& selectId)
        {
            PipelineRegions::Write(*mNode, regions);
            PipelineRegions::SyncPorts(*mNode);
            mNode->SetConfigString("selectedRegion", selectId);

            // The ports changed with the parts: the card and the links that hung on a removed one follow
            if (auto editor = mEditor.Lock())
            {
                if (auto graph = editor->GetGraph())
                    graph->RemoveDanglingEdges();

                editor->OnNodeConfigChanged(mOwner.Lock(), "regions", true);
            }

            RebuildBodyKeepingArea();
        }

        void PushSelectedBox()
        {
            auto regions = Regions();
            String selected = SelectedId();
            for (auto& region : regions)
            {
                if (region.id != selected)
                    continue;

                mWritingRoi = true;
                if (!mNode->config.IsObject())
                    mNode->config.SetObject();

                mNode->RemoveConfig("roi");
                auto& roi = mNode->config["roi"];
                roi.SetObject();
                roi["x"] = region.x;
                roi["y"] = region.y;
                roi["w"] = region.w;
                roi["h"] = region.h;
                mWritingRoi = false;
                if (mPaint)
                    mPaint->RefreshFromConfig();

                return;
            }
        }

        // Returns true when the "roi" box is not the selected part's box
        bool RoiIsStale() const
        {
            String selected = SelectedId();
            auto value = mNode->GetConfigValue("roi");
            PipelineImageOps::CropRect roi;
            PipelineImageOps::ParseCrop(value, roi);
            for (auto& region : Regions())
            {
                if (region.id != selected)
                    continue;

                if (!value || !value->IsObject())
                    return true;

                const float e = 0.0005f;
                return Math::Abs(roi.x - region.x) > e || Math::Abs(roi.y - region.y) > e || Math::Abs(roi.w - region.w) > e ||
                    Math::Abs(roi.h - region.h) > e;
            }
            return false;
        }

        // The region tool wrote a new box: it belongs to the selected part
        void ReadSelectedBox(bool completed)
        {
            if (mWritingRoi)
                return;

            PipelineImageOps::CropRect roi;
            if (!ReadNodeCrop(*mNode, "roi", roi))
            {
                roi.x = 0; roi.y = 0; roi.w = 1; roi.h = 1;
            }

            auto regions = Regions();
            String selected = SelectedId();
            for (auto& region : regions)
            {
                if (region.id != selected)
                    continue;

                region.x = roi.x; region.y = roi.y; region.w = roi.w; region.h = roi.h;
                PipelineRegions::NormalizeBox(region.x, region.y, region.w, region.h);
                break;
            }

            PipelineRegions::Write(*mNode, regions);
            Notify("regions", completed);
        }

        void AddRegion()
        {
            auto regions = Regions();
            PipelineExtractRegion region;
            region.id = PipelineNode::GenerateId();
            region.name = "";
            regions.Add(region);
            WriteRegions(regions, region.id);
        }

        void RemoveRegion(const String& id)
        {
            auto regions = Regions();
            if (regions.Count() <= 1)
                return;

            regions.RemoveAll([&](const PipelineExtractRegion& r) { return r.id == id; });
            WriteRegions(regions, regions.IsEmpty() ? String() : regions[0].id);
        }

        String NameOf(const String& id) const
        {
            for (auto& region : Regions())
            {
                if (region.id == id)
                    return region.name;
            }
            return String();
        }

        // Selecting a part touches only what shows the selection: the cells stay, nothing is decoded again
        void SelectRegion(const String& id)
        {
            mNode->SetConfigString("selectedRegion", id);
            PushSelectedBox();
            if (mGrid)
                mGrid->SetSelected(id);
            mPromptRegion = id;
            if (mPrompt)
                mPrompt->SetText(NameOf(id));
            SyncPaintRegions();
            RefreshParamRows();
            Notify("selectedRegion", true);
        }

        void AddParamRows()
        {
            AddModelRow(PipelineImageModelPresets(), GeminiProvider::defaultImageModel, PipelineModelKind::Image);
            AddSeedRow();
            AddScopeRow();
            AddTransparencyBlock();
        }

        // Rebuilds the rows of the open parameter list for the selected part, leaving the rest of the card alone; for the user's
        // own change a hand-sized card follows the rows, so the pictures keep their size
        void RefreshParamRows(bool keepArea = true)
        {
            // The switch follows the selected part, which may have settings of its own
            if (mTransparentSwitch)
                mTransparentSwitch->SetValue(GetBool("transparentBg", false));

            auto owner = mOwner.Lock();
            if (!mParamsList || !owner)
                return;

            PushEditorScopeOnStack scope;
            auto rebuild = [this]()
            {
                mParamsList->list->RemoveAllChildren();
                mParamsList->heights.Clear();
                mParams = mParamsList;
                AddParamRows();
                mParams = nullptr;
            };
            if (keepArea)
                owner->KeepAreaThrough(rebuild);
            else
            {
                rebuild();
                owner->UpdateFromNode();
            }
        }

        // "Settings for": the transparency rows below edit every part (the node) or the selected part alone
        void AddScopeRow()
        {
            if (Regions().Count() < 2)
                return;

            bool own = GetPartTransparency() != nullptr;
            auto segments = mmake<HorizontalLayout>();
            segments->name = "settings scope";
            segments->spacing = 4;
            segments->expandWidth = true;
            segments->expandHeight = true;
            segments->baseCorner = BaseCorner::Left;

            WeakRef<ImageExtractBody> weakThis(this);
            for (bool part : { false, true })
            {
                auto segment = MakeSegment(part ? "This part" : "All parts", part == own);
                segment->name = part ? "this part" : "all parts";
                segment->onToggleByUser = [weakThis, part](bool) { if (auto self = weakThis.Lock()) self->SetPartScope(part); };
                segments->AddChild(segment);
            }
            AddRow(MakeRow("Settings for", segments, 86.0f), 22);
        }

        // Gives the selected part its own background settings, copied from what it uses now, or returns it to the node's
        void SetPartScope(bool own)
        {
            String id = SelectedId();
            if (PipelineRegions::SetOwnTransparency(*mNode, id, own))
                Notify("regions", true);

            if (mGrid)
                mGrid->SetOwnSettings(id, own);

            RefreshParamRows();
        }

        // Tells the paint editor which boxes to outline besides the selected one
        void SyncPaintRegions()
        {
            if (!mPaint)
                return;

            auto regions = Regions();
            String selected = SelectedId();
            Vector<PipelinePaintEditor::RegionBox> others;
            int selectedIndex = 0;
            for (int i = 0; i < regions.Count(); i++)
            {
                if (regions[i].id == selected)
                {
                    selectedIndex = i + 1;
                    continue;
                }

                PipelinePaintEditor::RegionBox box;
                box.id = regions[i].id;
                box.index = i + 1;
                box.x = regions[i].x; box.y = regions[i].y; box.w = regions[i].w; box.h = regions[i].h;
                others.Add(box);
            }
            mPaint->SetRegions(others, regions.Count() > 1 ? selectedIndex : 0, regions.Count() > 1);
        }

        void RenameRegion(const String& id, const String& name)
        {
            auto regions = Regions();
            for (auto& region : regions)
            {
                if (region.id == id)
                    region.name = name;
            }

            WriteRegions(regions, id);
        }

        void RebuildParts()
        {
            if (!mGrid)
                return;

            mGrid->SetParts(Regions(), SelectedId());
            RefreshPartResults();
            ShowPartStates();
            SyncPaintRegions();
        }

        // A single part is computed as the node: it spins while the node runs
        void ShowPartStates()
        {
            auto owner = mOwner.Lock();
            if (!owner || !mGrid)
                return;

            auto& runtime = owner->GetRuntime();
            if (mGrid->GetCellCount() != 1)
            {
                mGrid->SetPartStates(runtime.portStates);
                return;
            }

            Map<String, String> states;
            if (runtime.state == "running")
                states[Regions()[0].id] = "running";

            mGrid->SetPartStates(states);
        }

        void RefreshPartResults()
        {
            auto owner = mOwner.Lock();
            if (!owner || !mGrid)
                return;

            auto& runtime = owner->GetRuntime();
            for (auto& region : Regions())
            {
                PipelineValue value;
                runtime.portOutputs.TryGetValue(region.id, value);
                String data = value.IsImage() ? value.data : String();
                String known;
                if (!mPartData.TryGetValue(region.id, known) || known != data)
                {
                    mPartData[region.id] = data;
                    mPartBitmaps[region.id] = value.IsImage() ? value.GetBitmap() : nullptr;
                }
                Ref<Bitmap> bitmap;
                mPartBitmaps.TryGetValue(region.id, bitmap);
                mGrid->SetPartImage(region.id, bitmap);
            }
        }

        // Asks the vision model for the parts of the source image and turns its answer into regions
        void AutoSplit()
        {
            auto editor = mEditor.Lock();
            auto input = GetInput("image");
            if (!editor || !input.IsImage())
                return;

            auto bitmap = input.GetBitmap();
            if (!bitmap)
                return;

            auto ctx = mmake<PipelineExecContext>();
            ctx->settings = PipelineSettings::Load();
            ctx->pipelineId = editor->GetPipelineId();
            WeakRef<PipelineEditor> weakEditor(editor);
            ctx->log = [weakEditor](const String& message) { if (auto e = weakEditor.Lock()) { if (e->onLog) e->onLog(message); } };

            String model = GetString("splitModel", GeminiProvider::defaultTextModel);
            AiRouteKeys keys = AiRouter::KeysOf(ctx->settings);
            AiRoute route = AiRouter::ChooseRoute(model, keys);
            if (keys.Of(route.provider).IsEmpty())
            {
                ctx->Log("Auto split: no " + AiRouter::ProviderName(route.provider) + " key - set it in the pipeline settings");
                return;
            }

            if (mAutoButton)
                mAutoButton->interactable = false;

            ctx->Log("Auto split: asking " + model + " for the parts of the image");

            WeakRef<ImageExtractBody> weakThis(this);
            mAutoSplitJob = [](WeakRef<ImageExtractBody> weakBody, Ref<PipelineExecContext> context, String textModel,
                               String imageBytes) -> Coroutine<void>
            {
                AiTextResult answer = co_await AiRouter::GenerateText(context, textModel, PipelineRegions::autoSplitPrompt,
                                                                      { { "image/png", imageBytes } });
                auto body = weakBody.Lock();
                if (!body)
                    co_return;

                if (body->mAutoButton)
                    body->mAutoButton->interactable = true;

                if (!answer.ok)
                {
                    context->Log("Auto split failed: " + answer.error);
                    co_return;
                }

                auto regions = PipelineRegions::ParseAutoSplit(answer.text);
                if (regions.IsEmpty())
                {
                    context->Log("Auto split: the model listed no parts");
                    co_return;
                }

                context->Log("Auto split: " + (String)regions.Count() + " parts");
                body->WriteRegions(regions, regions[0].id);
            }(weakThis, ctx, model, PipelineValue::Image(bitmap).GetPngBytes());

            mAutoSplitJob.Start(JobThread::Main);
        }

        Coroutine<void> mAutoSplitJob; // Running auto split request
    };

    Ref<PipelineNodeBody> CreateExtractNodeBody(const String& type)
    {
        return type == "imageExtract" ? mmake<ImageExtractBody>() : nullptr;
    }
}
