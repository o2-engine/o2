#include "o2Editor/stdafx.h"
#include "PipelineNodeBodyFactories.h"

#include "o2/Render/Render.h"
#include "o2/Render/Sprite.h"
#include "o2/Scene/UI/UIManager.h"
#include "o2/Scene/UI/WidgetLayer.h"
#include "o2/Scene/UI/WidgetLayout.h"
#include "o2/Scene/UI/Widgets/Button.h"
#include "o2/Scene/UI/Widgets/DropDown.h"
#include "o2/Scene/UI/Widgets/EditBox.h"
#include "o2/Scene/UI/Widgets/HorizontalLayout.h"
#include "o2/Scene/UI/Widgets/Label.h"
#include "o2/Scene/UI/Widgets/ScrollArea.h"
#include "o2/Scene/UI/Widgets/Toggle.h"
#include "o2/Scene/UI/Widgets/VerticalLayout.h"
#include "o2/Utils/Bitmap/Bitmap.h"
#include "o2/Utils/Bitmap/PngFormat.h"
#include "o2/Utils/Editor/DragHandle.h"
#include "o2/Utils/Editor/FrameHandles.h"
#include "o2/Utils/FileSystem/FileSystem.h"
#include "o2/Utils/System/Clipboard.h"
#include "o2Editor/Dialogs/ColorPickerDlg.h"
#include "o2Editor/Dialogs/System/OpenSaveDialog.h"
#include "o2Editor/Pipeline/Nodes/PipelineNodesCommon.h"
#include "o2Editor/Pipeline/PipelineExecutor.h"
#include "o2Editor/Pipeline/PipelineRegions.h"
#include "o2Editor/Pipeline/PipelineSettings.h"
#include "o2Editor/Pipeline/PipelineImageOps.h"
#include "o2Editor/Pipeline/PipelineUtils.h"
#include "o2Editor/Pipeline/Providers/ElevenLabsProvider.h"
#include "o2Editor/Pipeline/Providers/GeminiProvider.h"
#include "o2Editor/Pipeline/Providers/VideoProviders.h"
#include "o2Editor/Windows/PipelineWindow/PipelineComposerStage.h"
#include "o2Editor/Windows/PipelineWindow/PipelineEditor.h"
#include "o2Editor/Windows/PipelineWindow/PipelineMediaViews.h"
#include "o2Editor/Windows/PipelineWindow/PipelineNodeWidget.h"
#include "o2Editor/Windows/PipelineWindow/PipelinePaintEditor.h"

namespace Editor
{
    using namespace PipelineControls;

    static const Vector<String> imageModelPresets = {
        "gemini-3.1-flash-image", "gemini-3-pro-image", "gemini-3-pro-image-preview", "gemini-2.5-flash-image",
        "gemini-2.5-flash-image-preview", "imagen-4.0-generate-001", "imagen-3.0-generate-001"
    };

    // ---------------------------------------------------------------------------------
    // Parts of an extract node as a grid of cells: the result of each part with its name
    // under it, the selected one outlined; as many columns as the width fits
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

                cell.image = mmake<PipelineImageView>();
                cell.image->SetHint((String)(i + 1));
                cell.root->AddChild(cell.image);

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
                auto regen = MakeIconButton("ui/pipeline/btn_loop.png", textColor, Color4(255, 255, 255, 220));
                regen->name = "regen part";
                regen->layout->minWidth = 20;
                regen->layout->maxWidth = 20;
                regen->onClick = [weakThis, id]() { if (auto self = weakThis.Lock()) self->onRegen(id); };
                tools->AddChild(regen);
                if (regions.Count() > 1)
                {
                    auto remove = MakeIconButton("ui/UI4_small_trash_icon.png", textColor, Color4(255, 255, 255, 220));
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
            }
        }

        // Returns the number of cells
        int GetCellCount() const { return mCells.Count(); }

        // Returns true and the bottom-right corner of the part's image, relative to the grid's top-left (y down), for the width
        bool GetCellCorner(const String& id, float width, Vec2F& corner) const
        {
            int index = -1;
            for (int i = 0; i < mCells.Count(); i++)
            {
                if (mCells[i].id == id)
                    index = i;
            }
            if (index < 0)
                return false;

            Geometry g = Geom(width);
            int col = index % g.cols, row = index / g.cols;
            corner = Vec2F(col * (g.cellW + gap) + g.cellW, row * (g.cellH + g.labelH + gap) + g.cellH);
            return true;
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

        // Returns the height of the rows the cells take in the width
        float GetHeightForWidth(float width) const
        {
            Geometry g = Geom(width);
            return g.rows * (g.cellH + g.labelH) + (g.rows - 1) * gap;
        }

        void UpdateSelfTransform() override
        {
            Widget::UpdateSelfTransform();
            LayoutCells();
        }

    private:
        static constexpr float gap = 6.0f, minCell = 92.0f, maxCell = 176.0f, labelHeight = 22.0f;

        struct Cell
        {
            String                 id;
            String                 name;  // Part name, put into the field when the cell is selected
            Ref<Widget>            root;
            Ref<WidgetLayer>       frame; // Outline shown on the selected cell
            Ref<PipelineImageView> image;
            Ref<Button>            pick;
            Ref<Widget>            tools;
            Ref<EditBox>           edit;  // Name field, shown on the selected cell
            Ref<Label>             label; // Name caption of the other cells

            bool operator==(const Cell& other) const { return id == other.id; }
        };

        struct Geometry { int cols = 1, rows = 1; float cellW = 0.0f, cellH = 0.0f, labelH = 0.0f; };

        Vector<Cell> mCells;
        bool         mSingle = true;

        Geometry Geom(float width) const
        {
            int n = Math::Max(1, mCells.Count());
            Geometry g;
            g.cols = Math::Clamp((int)Math::Floor((width + gap) / (minCell + gap)), 1, n);
            g.cellW = (width - gap * (g.cols - 1)) / g.cols;
            g.cellH = Math::Min(maxCell, Math::Round(g.cellW));
            g.rows = (n + g.cols - 1) / g.cols;
            g.labelH = mSingle ? 0.0f : labelHeight;
            return g;
        }

        void LayoutCells()
        {
            float width = layout->GetWidth();
            if (width <= 0.0f)
                return;

            Geometry g = Geom(width);
            for (int i = 0; i < mCells.Count(); i++)
            {
                auto& cell = mCells[i];
                int col = i % g.cols, row = i / g.cols;
                float x = col * (g.cellW + gap), y = row * (g.cellH + g.labelH + gap);
                *cell.root->layout = WidgetLayout(Vec2F(0, 1), Vec2F(0, 1), Vec2F(x, -(y + g.cellH + g.labelH)), Vec2F(x + g.cellW, -y));
                *cell.image->layout = WidgetLayout::HorStretch(VerAlign::Top, 0, 0, g.cellH, 0);
                *cell.pick->layout = WidgetLayout::HorStretch(VerAlign::Top, 0, 0, g.cellH, 0);
                if (cell.frame)
                    cell.frame->layout = Layout(Vec2F(0, 1), Vec2F(1, 1), Vec2F(-2, -g.cellH - 2), Vec2F(2, 2));
                *cell.tools->layout = WidgetLayout(Vec2F(1, 1), Vec2F(1, 1), Vec2F(-46, -24), Vec2F(-3, -3));
                if (cell.edit)
                    *cell.edit->layout = WidgetLayout::HorStretch(VerAlign::Bottom, 0, 16, g.labelH - 2.0f, 0);
                if (cell.label)
                    *cell.label->layout = WidgetLayout::HorStretch(VerAlign::Bottom, 0, 16, g.labelH - 2.0f, 0);
            }
        }
    };

    class NanoBananaBody : public PipelineNodeBody
    {
    public:
        using PipelineNodeBody::PipelineNodeBody;
        void Build() override
        {
            AddCropSection("no result - press play to compute the branch", "Prompt optional - references on the inputs (+)");
            AddPrimaryField("extraPrompt", "Extra prompt (optional style hints)");
            if (BeginParams({ "Transparent bg", "Model", "Seed" }))
            {
                AddTransparencyBlock();
                AddModelRow(imageModelPresets, GeminiProvider::defaultImageModel);
                AddSeedRow();
            }
            EndParams();
            OnOutputChanged();
        }
    };

    // Transparent by nature: only the method is a choice
    class AiRemoveBgBody : public PipelineNodeBody
    {
    public:
        using PipelineNodeBody::PipelineNodeBody;
        void Build() override
        {
            AddCropSection("no result - press play to compute the branch", "Result (transparent)");
            AddPrimaryField("hint", "What to keep (optional), e.g. the character");
            if (BeginParams({ "Mode", "Model", "Seed" }))
            {
                AddTransparencyBlock(true);
                AddModelRow(imageModelPresets, GeminiProvider::defaultImageModel);
                AddSeedRow();
            }
            EndParams();
            OnOutputChanged();
        }
    };

    class ImageEditBody : public PipelineNodeBody
    {
    public:
        using PipelineNodeBody::PipelineNodeBody;
        void Build() override
        {
            AddCropSection("no result - press play to compute the branch", "Result");
            AddPrimaryField("prompt", "Describe the change to apply to the image");
            if (BeginParams({ "Transparent bg", "Model", "Seed" }))
            {
                AddTransparencyBlock();
                AddModelRow(imageModelPresets, GeminiProvider::defaultImageModel);
                AddSeedRow();
            }
            EndParams();

            AddSectionTitle("Draw over the image");
            WeakRef<PipelineNodeBody> weakThis(this);
            mPaint = mmake<PipelinePaintEditor>();
            mPaint->onConfigChanged = [weakThis](const String& key, bool completed) { if (auto self = weakThis.Lock()) self->Notify(key, completed); };
            mPaint->Init(mNode, false);
            { auto paint = mPaint; AddFlexible(mPaint, [paint](float width) { return Math::Max(176.0f, paint->GetMinHeightForWidth(width) + 60.0f); }); MarkContent(mPaint); }

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
        }

        void OnConfigChanged() override
        {
            if (mPaint)
                mPaint->RefreshFromConfig();
        }

    private:
        Ref<PipelinePaintEditor> mPaint;
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
            { auto grid = mGrid; AddRow(mGrid, [grid](float width) { return grid->GetHeightForWidth(width); }); }
            MarkContent(mGrid);

            auto add = MakeButton("+ Add part");
            add->name = "add part";
            add->onClick = [weakThis]() { if (auto self = weakThis.Lock()) self->AddRegion(); };
            mAutoButton = MakeButton("Auto split");
            mAutoButton->name = "auto split";
            mAutoButton->onClick = [weakThis]() { if (auto self = weakThis.Lock()) self->AutoSplit(); };
            AddActions({ add, mAutoButton });

            // The name of the selected part is what the model is asked to keep
            mPromptRegion = SelectedId();
            mPrompt = MakeEditBox(NameOf(mPromptRegion), true, "What to keep, e.g. the green chip");
            mPrompt->name = "part prompt";
            mPrompt->onChangeCompleted = [weakThis](const WString& text) { if (auto self = weakThis.Lock()) self->RenameRegion(self->mPromptRegion, (String)text); };
            AddRow(mPrompt, 46);

            AddSectionTitle("Source & regions");
            mPaint = mmake<PipelinePaintEditor>();
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
            { auto paint = mPaint; AddFlexible(mPaint, [paint](float width) { return Math::Max(176.0f, paint->GetMinHeightForWidth(width) + 60.0f); }); MarkContent(mPaint); }

            if (BeginParams({ "Transparent bg", "Model", "Seed" }))
            {
                AddTransparencyBlock();
                AddModelRow(imageModelPresets, GeminiProvider::defaultImageModel);
                AddSeedRow();
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
        }

        // The output ports of several parts sit on the corners of their cells, like the links leave them in AssetsLine
        bool GetBodyPortOffset(const String& portId, Vec2F& offset) const override
        {
            if (!mGrid || mGrid->GetCellCount() < 2)
                return false;

            auto owner = mOwner.Lock();
            float width = (owner ? owner->GetCardWidth() : 260.0f) - mPadding * 2.0f;
            Vec2F corner;
            if (!mGrid->GetCellCorner(portId, width, corner))
                return false;

            offset = Vec2F(mPadding + corner.x, mSpacing + corner.y);
            return true;
        }

    private:
        Ref<PipelinePaintEditor> mPaint;        // Source image with the boxes of the parts
        Ref<PipelinePartsGrid>   mGrid;         // Cells of the parts with their results
        Ref<Button>              mAutoButton;   // Asks the model to list the parts
        Ref<EditBox>             mPrompt;       // Name of the selected part
        String                   mPromptRegion; // Part the prompt field edits
        bool mWritingRoi = false; // True while the selected box is pushed into the paint editor

        Map<String, String>      mPartData;    // Encoded result of each part the bitmap below was decoded from
        Map<String, Ref<Bitmap>> mPartBitmaps; // Decoded result of each part, kept so a selection change decodes nothing

    private:
        Vector<PipelineExtractRegion> Regions() const { return PipelineRegions::Read(*mNode); }

        String SelectedId() const
        {
            String selected = GetString("selectedRegion", "");
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

            RebuildBody();
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
            Notify("selectedRegion", true);
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
            SyncPaintRegions();
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

            String apiKey = ctx->settings.GetGeminiKey();
            if (apiKey.IsEmpty())
            {
                ctx->Log("Auto split: no Gemini key - set it in the pipeline settings");
                return;
            }

            if (mAutoButton)
                mAutoButton->interactable = false;

            String model = GetString("splitModel", GeminiProvider::defaultTextModel);
            ctx->Log("Auto split: asking " + model + " for the parts of the image");

            WeakRef<ImageExtractBody> weakThis(this);
            mAutoSplitJob = [](WeakRef<ImageExtractBody> weakBody, Ref<PipelineExecContext> context, String key, String textModel,
                               String imageBytes) -> Coroutine<void>
            {
                AiTextResult answer = co_await GeminiProvider::GenerateText(context, key, textModel, PipelineRegions::autoSplitPrompt,
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
            }(weakThis, ctx, apiKey, model, PipelineValue::Image(bitmap).GetPngBytes());

            mAutoSplitJob.Start(JobThread::Main);
        }

        Coroutine<void> mAutoSplitJob; // Running auto split request
    };

    class RemoveBgBody : public PipelineNodeBody
    {
    public:
        using PipelineNodeBody::PipelineNodeBody;
        void Build() override
        {
            AddResultImage("no image - connect the white and black renders");
            OnOutputChanged();
        }
    };

    class ImageOutlineBody : public PipelineNodeBody
    {
    public:
        using PipelineNodeBody::PipelineNodeBody;
        void Build() override
        {
            AddResultImage("no image - connect the input");
            AddColor("Color", "color", "#000000");
            AddSlider("Width", "width", 0, 64, 1, 4, "px");
            AddSegmented("position", { { "outside", "Outside" }, { "center", "Center" }, { "inside", "Inside" } }, "outside");
            AddSlider("Soft", "softness", 0, 32, 1, 0, "px");
            AddSlider("Alpha", "opacity", 0, 1, 0.05f, 1);
            OnOutputChanged();
        }
    };

    class ImageShadowBody : public PipelineNodeBody
    {
    public:
        using PipelineNodeBody::PipelineNodeBody;
        void Build() override
        {
            AddResultImage("no image - connect the input");
            AddColor("Color", "color", "#000000");
            AddSlider("Alpha", "opacity", 0, 1, 0.05f, 0.6f);
            AddSlider("Angle", "angle", 0, 359, 1, 45, "\xC2\xB0");
            AddSlider("Dist", "distance", 0, 128, 1, 8, "px");
            AddSlider("Blur", "blur", 0, 128, 1, 8, "px");
            AddSlider("Spread", "spread", 0, 64, 1, 0, "px");
            AddCheckbox("Inner shadow", "inner", false);
            OnOutputChanged();
        }
    };

    class ImageGradientBody : public PipelineNodeBody
    {
    public:
        using PipelineNodeBody::PipelineNodeBody;
        void Build() override
        {
            AddResultImage("no image - connect the input");
            AddSegmented("kind", { { "linear", "Linear" }, { "radial", "Radial" } }, "linear");
            AddSelectRow("Blend", "blend", { "normal", "multiply", "screen", "overlay", "map" }, "normal");
            AddColor("A", "color1", "#ffffff");
            AddSlider("A alpha", "alpha1", 0, 1, 0.05f, 1);
            AddColor("B", "color2", "#000000");
            AddSlider("B alpha", "alpha2", 0, 1, 0.05f, 1);
            if (GetString("blend", "normal") != "map" && GetString("kind", "linear") != "radial")
                AddSlider("Angle", "angle", 0, 359, 1, 90, "\xC2\xB0");
            AddSlider("Amount", "opacity", 0, 1, 0.05f, 1);
            AddCheckbox("Keep inside the artwork", "clipToAlpha", true);
            if (GetString("blend", "normal") == "map")
                AddMutedLine("Gradient map: A -> dark tones, B -> light tones");
            OnOutputChanged();
        }

        void OnConfigChanged() override { RebuildBody(); }
    };

    class ImageColorBody : public PipelineNodeBody
    {
    public:
        using PipelineNodeBody::PipelineNodeBody;
        void Build() override
        {
            AddResultImage("no image - connect the input");
            AddSlider("Bright", "brightness", -100, 100, 1, 0);
            AddSlider("Contr", "contrast", -100, 100, 1, 0);
            AddSlider("Sat", "saturation", -100, 100, 1, 0);
            AddSlider("Hue", "hue", -180, 180, 1, 0, "\xC2\xB0");
            AddColor("Tint", "tint", "#ff8800");
            AddSlider("Tint a", "tintStrength", 0, 1, 0.05f, 0);
            AddCheckbox("Colorize (tint hue, keep luminance)", "colorize", false);
            AddCheckbox("Grayscale", "grayscale", false);
            AddCheckbox("Invert", "invert", false);
            OnOutputChanged();
        }
    };

    class DrawImageBody : public PipelineNodeBody
    {
    public:
        using PipelineNodeBody::PipelineNodeBody;
        void Build() override
        {
            mPaint = mmake<PipelinePaintEditor>();
            WeakRef<PipelineNodeBody> weakThis(this);
            mPaint->onConfigChanged = [weakThis](const String& key, bool completed) { if (auto self = weakThis.Lock()) self->Notify(key, completed); };
            mPaint->Init(mNode, false);
            { auto paint = mPaint; AddFlexible(mPaint, [paint](float width) { return Math::Max(240.0f, paint->GetMinHeightForWidth(width) + 90.0f); }); MarkContent(mPaint); }
            OnOutputChanged();
        }

        void OnOutputChanged() override
        {
            PipelineNodeBody::OnOutputChanged();
            if (mPaint)
            {
                auto input = GetInput("background");
                mPaint->SetBackground(input.IsImage() ? input.GetBitmap() : nullptr);
            }
        }

        void OnConfigChanged() override
        {
            if (mPaint)
                mPaint->RefreshFromConfig();
        }

    private:
        Ref<PipelinePaintEditor> mPaint;
    };

    Ref<PipelineNodeBody> CreateImageNodeBody(const String& type)
    {
        if (type == "nanoBananaGen")
            return mmake<NanoBananaBody>();

        if (type == "imageEdit")
            return mmake<ImageEditBody>();

        if (type == "imageExtract")
            return mmake<ImageExtractBody>();

        if (type == "removeBackground")
            return mmake<RemoveBgBody>();

        if (type == "aiRemoveBg")
            return mmake<AiRemoveBgBody>();

        if (type == "imageOutline")
            return mmake<ImageOutlineBody>();

        if (type == "imageShadow")
            return mmake<ImageShadowBody>();

        if (type == "imageGradient")
            return mmake<ImageGradientBody>();

        if (type == "imageColor")
            return mmake<ImageColorBody>();

        if (type == "drawImage")
            return mmake<DrawImageBody>();

        return nullptr;
    }
}
