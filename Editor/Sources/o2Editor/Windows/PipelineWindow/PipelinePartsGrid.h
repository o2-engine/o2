#pragma once

#include "o2/Render/Render.h"
#include "o2/Render/Text.h"
#include "o2/Render/VectorSprite.h"
#include "o2/Scene/UI/WidgetLayer.h"
#include "o2/Scene/UI/WidgetLayout.h"
#include "o2/Scene/UI/WidgetState.h"
#include "o2/Scene/UI/Widgets/Button.h"
#include "o2/Scene/UI/Widgets/EditBox.h"
#include "o2/Scene/UI/Widgets/HorizontalLayout.h"
#include "o2/Scene/UI/Widgets/Label.h"
#include "o2Editor/Pipeline/PipelinePairLayout.h"
#include "o2Editor/Pipeline/PipelineRegions.h"
#include "o2Editor/Windows/PipelineWindow/PipelineControls.h"
#include "o2Editor/Windows/PipelineWindow/PipelineDraw.h"
#include "o2Editor/Windows/PipelineWindow/PipelineImageTheme.h"
#include "o2Editor/Windows/PipelineWindow/PipelineTheme.h"

// The cells of the AI extract card's parts: used by PipelineExtractBody.cpp only
namespace Editor
{
    using namespace PipelineControls;

    // A 20 x 18 button of a cell's tool strip: the glyph in white, a light wash or the red when hovered
    inline Ref<Button> MakeCellTool(const String& icon, const String& name, bool danger)
    {
        auto button = mmake<Button>();
        button->name = name;
        auto hover = AddFillLayer(*button, "hover", danger ? PipelineTheme::Get().red : Color4(255, 255, 255, 46), 0.0f, Layout::BothStretch(), -1.0f);
        float strength = hover->transparency;
        hover->transparency = 0.0f;
        button->AddState("hover", AnimationClip::EaseInOut("layer/hover/transparency", 0.0f, strength, 0.12f))->offStateAnimationSpeed = 0.5f;
        auto sprite = mmake<VectorSprite>(icon);
        sprite->color = Color4::White();
        button->AddLayer("icon", sprite, Layout::Based(BaseCorner::Center, Vec2F(10, 10)));
        button->layout->minWidth = 20;
        button->layout->maxWidth = 20;
        return button;
    }

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
            o2Render.DrawAALine(arc, AccentColor(), width);
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
        Function<void(const String&)>                onCheck;  // The button of a cell's check against the reference was pressed
        Function<void(const String&, const String&)> onRename; // The name of the selected cell was edited

    public:
        explicit PipelinePartsGrid(RefCounter* refCounter): Widget(refCounter) {}

        // Zoomed far out only the pictures of the parts are drawn
        void Draw() override
        {
            if (!PipelineControls::IsFarView())
            {
                Widget::Draw();
                return;
            }

            if (!mResEnabledInHierarchy || mIsClipped)
                return;

            for (auto& cell : mCells)
                cell.image->Draw();
        }

        // Rebuilds the cells for the parts; the selected one gets the name field and the outline. check: the parts are checked
        // against the reference, each cell offers its check's details
        void SetParts(const Vector<PipelineExtractRegion>& regions, const String& selectedId, bool check = false)
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
                // Red frame round a part that failed in this run, over the picture
                auto errorFrame = mmake<PipelineRoundedFrame>();
                errorFrame->color = PipelineTheme::Get().red;
                errorFrame->radius = 7.0f;
                errorFrame->width = 1.0f;
                cell.errorFrame = cell.root->AddLayer("error", errorFrame, Layout::BothStretch(), 10.0f);
                cell.errorFrame->enabled = false;

                cell.image = mmake<PipelineCellImage>();
                cell.image->SetHint((String)(i + 1));
                cell.root->AddChild(cell.image);

                cell.spinner = mmake<PipelinePartSpinner>();
                cell.spinner->name = "part spinner";
                cell.root->AddChild(cell.spinner);
                cell.spinner->SetEnabledForcible(false);

                // What a running part does beyond its generation: compared with the reference, or drawn again
                cell.phase = MakeLabel("", true);
                cell.phase->name = "part phase";
                StyleLabel(cell.phase, 10.0f, 500, Color4::White());
                cell.phase->horAlign = HorAlign::Middle;
                cell.phase->verAlign = VerAlign::Middle;
                cell.root->AddChild(cell.phase);
                cell.phase->SetEnabledForcible(false);

                cell.pick = mmake<Button>();
                cell.pick->name = "select part";
                cell.pick->onClick = [weakThis, id]() { if (auto self = weakThis.Lock()) self->onSelect(id); };
                // The part's output port sits on the top-right corner of the image: that corner is the port's
                WeakRef<Button> weakPick(cell.pick);
                cell.pick->isPointInside = [weakPick](const Vec2F& p)
                {
                    auto pick = weakPick.Lock();
                    if (!pick) return false;
                    RectF rect = pick->layout->GetWorldRect();
                    return rect.IsInside(p) && (p - Vec2F(rect.right, rect.top)).Length() > 14.0f;
                };
                cell.root->AddChild(cell.pick);

                auto tools = mmake<HorizontalLayout>();
                tools->spacing = 0;
                tools->expandWidth = false;
                tools->expandHeight = true;
                tools->baseCorner = BaseCorner::Left;
                cell.tools = tools;
                auto regen = MakeCellTool("ui/pipeline/al_refresh.svg", "regen part", false);
                regen->onClick = [weakThis, id]() { if (auto self = weakThis.Lock()) self->onRegen(id); };
                tools->AddChild(regen);
                cell.toolCount = 1;
                if (check)
                {
                    auto details = MakeCellTool("ui/pipeline/al_bug.svg", "check details", false);
                    details->onClick = [weakThis, id]() { if (auto self = weakThis.Lock()) { if (self->onCheck) self->onCheck(id); } };
                    tools->AddChild(details);
                    cell.toolCount++;
                }
                if (regions.Count() > 1)
                {
                    auto remove = MakeCellTool("ui/pipeline/al_trash.svg", "remove part", true);
                    remove->onClick = [weakThis, id]() { if (auto self = weakThis.Lock()) self->onRemove(id); };
                    tools->AddChild(remove);
                    cell.toolCount++;
                }
                cell.root->AddChild(cell.tools);
                cell.image->toolCount = cell.toolCount;

                // One part needs no caption: the prompt field under the grid is its name
                if (!mSingle)
                {
                    String fallback = "part " + (String)(i + 1);
                    cell.edit = MakeEditBox(regions[i].name, false, fallback);
                    cell.edit->name = "part name";
                    PipelineImageTheme::SkinCellNameField(cell.edit);
                    cell.edit->onChangeCompleted = [weakThis, id](const WString& text) { if (auto self = weakThis.Lock()) self->onRename(id, (String)text); };
                    cell.root->AddChild(cell.edit);

                    cell.label = MakeLabel(regions[i].name.IsEmpty() ? fallback : regions[i].name, true);
                    StyleLabel(cell.label, 10.5f, 400, PipelineTheme::Get().fgDim);
                    cell.label->horOverflow = Label::HorOverflow::Dots;
                    cell.label->horAlign = HorAlign::Middle;
                    cell.label->verAlign = VerAlign::Middle;
                    cell.root->AddChild(cell.label);

                    auto dot = mmake<PipelineRoundedRect>();
                    dot->color = AccentColor();
                    dot->radius = dotSize*0.5f;
                    dot->roundBottom = true;
                    cell.dot = mmake<Widget>();
                    cell.dot->name = "own settings";
                    cell.dot->AddLayer("dot", dot, Layout::BothStretch());
                    cell.root->AddChild(cell.dot);
                    cell.own = regions[i].IsOwn();
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

        // Gives a part's cell its new name in place; false when the grid has no such captioned cell
        bool RenamePart(const String& id, const String& name)
        {
            for (auto& cell : mCells)
            {
                if (cell.id != id || !cell.label)
                    continue;

                cell.name = name;
                String caption = name.IsEmpty() ? "part " + (String)(mCells.IndexOf(cell) + 1) : name;
                cell.label->text = caption;
                cell.captionWidth = Text::GetTextSize(cell.label->GetText(), cell.label->GetFont(), cell.label->GetHeight(), Vec2F(),
                                                      HorAlign::Left, VerAlign::Top, false).x;
                if (cell.edit && !cell.edit->IsFocused() && (String)cell.edit->GetText() != name)
                    cell.edit->SetText(name);
                LayoutCells();
                return true;
            }
            return false;
        }

        // Moves the outline and the name field to the selected part without rebuilding the cells
        void SetSelected(const String& id)
        {
            for (auto& cell : mCells)
            {
                bool on = cell.id == id;
                cell.root->name = on ? "part cell selected" : "part cell";
                cell.image->selected = on;
                cell.image->tools = on;
                cell.tools->SetEnabledForcible(on);
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

        // Shows the run state of every part in place: a spinner over the running ones, with what a part does beyond its
        // generation (phases: check, redo), a red frame round the failed ones
        void SetPartStates(const Map<String, String>& states, const Map<String, String>& phases = {})
        {
            for (auto& cell : mCells)
            {
                String state, phase;
                states.TryGetValue(cell.id, state);
                phases.TryGetValue(cell.id, phase);
                cell.spinner->SetEnabledForcible(state == "running");
                cell.errorFrame->enabled = state == "error";
                bool shown = state == "running" && !phase.IsEmpty();
                cell.phase->text = phase == "redo" ? "redrawing" : "checking";
                cell.phase->SetEnabledForcible(shown);
            }
        }

        // Returns the phase caption shown over a part's cell, empty when none is (tests)
        String GetPartPhase(const String& id) const
        {
            for (auto& cell : mCells)
            {
                if (cell.id == id && cell.phase->IsEnabled())
                    return (String)cell.phase->GetText();
            }
            return "";
        }

        // Returns the number of cells
        int GetCellCount() const { return mCells.Count(); }

        // Returns true and the drawn top-right corner of the part's image in world space, once its cell is laid out
        bool GetImageCorner(int index, Vec2F& corner) const
        {
            if (index < 0 || index >= mCells.Count() || mCells[index].image->layout->GetWidth() <= 0.0f)
                return false;

            RectF rect = mCells[index].image->layout->GetWorldRect();
            corner = Vec2F(rect.right, rect.top);
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
            Ref<PipelineCellImage> image; // Picture, outlined while selected
            Ref<WidgetLayer>       errorFrame; // Red outline of a part that failed in this run
            Ref<PipelinePartSpinner> spinner; // Progress over a part being computed
            Ref<Label>             phase; // What a running part does beyond its generation, under the spinner
            int                    toolCount = 1; // Buttons of the tool strip
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

        struct Geometry { int cols = 1, rows = 1; float cellW = 0.0f, cellH = 0.0f, labelH = 0.0f, offX = 0.0f, offY = 0.0f; };

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
            g.offX = pair.offX;
            g.offY = pair.offY;
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
                float x = g.offX + col * (g.cellW + gap), y = g.offY + row * (g.cellH + g.labelH + gap);
                *cell.root->layout = WidgetLayout(Vec2F(0, 1), Vec2F(0, 1), Vec2F(x, -(y + g.cellH + g.labelH)), Vec2F(x + g.cellW, -y));
                *cell.image->layout = WidgetLayout::HorStretch(VerAlign::Top, 0, 0, g.cellH, 0);
                *cell.spinner->layout = WidgetLayout::HorStretch(VerAlign::Top, 0, 0, g.cellH, 0);
                *cell.phase->layout = WidgetLayout::HorStretch(VerAlign::Top, 0, 0, 14, g.cellH*0.5f + 12.0f);
                *cell.pick->layout = WidgetLayout::HorStretch(VerAlign::Top, 0, 0, g.cellH, 0);
                cell.errorFrame->layout = Layout(Vec2F(0, 1), Vec2F(1, 1), Vec2F(0, -g.cellH), Vec2F(0, 0));
                // Top left: the part's output port sits on the top-right corner
                *cell.tools->layout = WidgetLayout(Vec2F(0, 1), Vec2F(0, 1), Vec2F(1, -19), Vec2F(1.0f + 20.0f*cell.toolCount, -1));
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
                    *cell.edit->layout = WidgetLayout::HorStretch(VerAlign::Bottom, fieldLeft, 0, g.labelH - 2.0f, 0);
                if (cell.label)
                    *cell.label->layout = WidgetLayout::HorStretch(VerAlign::Bottom, 0, 0, g.labelH - 2.0f, 0);
            }
        }
    };
}
