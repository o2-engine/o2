#include "o2Editor/stdafx.h"
#include "PipelineNodeBodyFactories.h"

#include "o2Editor/Windows/PipelineWindow/PipelineDraw.h"
#include "o2Editor/Windows/PipelineWindow/PipelineImageTheme.h"
#include "o2Editor/Windows/PipelineWindow/PipelineTheme.h"
#include "o2/Render/VectorSprite.h"
#include "o2/Scene/UI/WidgetState.h"

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
#include "o2Editor/Windows/PipelineWindow/PipelineModelPicker.h"
#include "o2Editor/Pipeline/UiLayout/UiLayoutParse.h"
#include "o2Editor/Pipeline/UiDoc/UiDocDevice.h"
#include "o2Editor/Pipeline/Providers/LocalAgentProvider.h"
#include "o2Editor/Pipeline/PipelinePngMeta.h"
#include "o2Editor/Pipeline/PipelineSettings.h"
#include "o2Editor/Pipeline/Providers/AiRouter.h"
#include "o2Editor/Pipeline/Providers/GeminiProvider.h"
#include "o2Editor/Windows/PipelineWindow/PipelineEditor.h"
#include "o2Editor/Windows/PipelineWindow/PipelineNodeWidget.h"
#include "o2Editor/Windows/PipelineWindow/PipelinePaintEditor.h"
#include "o2Editor/Windows/PipelineWindow/PipelinePartsGrid.h"

namespace Editor
{
    using namespace PipelineControls;

    // A thin line between groups of a parameter list (.plist-div)
    static Ref<Widget> MakeListDivider()
    {
        auto divider = mmake<Widget>();
        divider->name = "divider";
        divider->AddLayer("line", MakeMenuLine(), Layout::HorStretch(VerAlign::Middle, 0, 0, 1, 0));
        return divider;
    }

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
            mPaint->onRegionRemovedById = [weakThis](const String& id) { if (auto self = weakThis.Lock()) self->RemoveRegion(id); };
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

            // Content-wide quiet buttons: "+ Add part", and "✦ Auto-split" in the accent (.grid-actions button.auto)
            auto add = MakeGhostButton("+ Add part");
            add->name = "add part";
            add->onClick = [weakThis]() { if (auto self = weakThis.Lock()) self->AddRegion(); };
            mAutoButton = MakeGhostButton("Auto-split");
            mAutoButton->name = "auto split";
            PipelineImageTheme::MakeAccentGhost(mAutoButton);
            PipelineImageTheme::KeepWholeCaption(*add);
            mAutoButton->onClick = [weakThis]() { if (auto self = weakThis.Lock()) self->AutoSplit(); };
            float addWidth = Math::Ceil(TextWidth("+ Add part", bodyFont)) + 18.0f;
            float autoWidth = 22.0f + Math::Ceil(TextWidth("Auto-split", bodyFont)) + 9.0f;
            add->layout->minWidth = add->layout->maxWidth = addWidth;
            mAutoButton->layout->minWidth = mAutoButton->layout->maxWidth = autoWidth;

            // Auto-split and the model it asks are one control: the model is only ever used by this button
            mSplitModel = MakeModelField();
            mSplitModel->name = "split model";
            SetModelFieldValue(mSplitModel, SplitModel(), PipelineModelKind::Text);
            mSplitModel->layout->minWidth = 96;
            mSplitModel->onClick = [weakThis]() { if (auto self = weakThis.Lock()) self->PickSplitModel(); };
            auto split = mmake<HorizontalLayout>();
            split->name = "split";
            split->spacing = 2;
            split->expandWidth = true;
            split->expandHeight = true;
            split->baseCorner = BaseCorner::Left;
            split->AddChild(mAutoButton);
            split->AddChild(mSplitModel);

            auto buttons = mmake<HorizontalLayout>();
            buttons->name = "part actions";
            buttons->spacing = 6;
            buttons->expandWidth = true;
            buttons->expandHeight = true;
            buttons->baseCorner = BaseCorner::Left;
            buttons->AddChild(add);
            buttons->AddChild(split);
            AddRow(buttons, 27.52f);

            // The name of the selected part is what the model is asked to keep
            mPromptRegion = SelectedId();
            mPrompt = MakeEditBox(NameOf(mPromptRegion), true, "What to keep, e.g. the green chip");
            mPrompt->name = "part prompt";
            mPrompt->onChangeCompleted = [weakThis](const WString& text) { if (auto self = weakThis.Lock()) self->RenameRegion(self->mPromptRegion, (String)text); };
            AddRow(mPrompt, FieldHeight(NameOf(mPromptRegion), "What to keep, e.g. the green chip"));

            if (BeginParams(ParamsSummary()))
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
                mPaint->SetBackground(Picture(GetInput("image")));
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

        // The model and background rows read the selected part's own settings when it has them
        const DataValue* FindPartSetting(const String& key) const override
        {
            String id = SelectedId();
            if (!IsOwnPart(id))
                return nullptr;

            if (key == "model")
            {
                auto part = PipelineRegions::FindPart(*mNode, id);
                return part ? part->FindMember("model") : nullptr;
            }

            if (!PipelineTransparency::SettingKeys().Contains(key))
                return nullptr;

            auto own = PipelineRegions::FindTransparency(*mNode, id);
            return own ? own->FindMember(key.Data()) : nullptr;
        }

        // ...and write them there; a part with only its own model gets its own background copy on the first background edit
        DataValue* PartSettingSlot(const String& key) override
        {
            String id = SelectedId();
            if (!IsOwnPart(id))
                return nullptr;

            if (key == "model")
            {
                auto part = PipelineRegions::FindPart(*mNode, id);
                return part ? &(*part)["model"] : nullptr;
            }

            if (!PipelineTransparency::SettingKeys().Contains(key))
                return nullptr;

            if (!PipelineRegions::FindTransparency(*mNode, id))
                PipelineRegions::SetOwnTransparency(*mNode, id, true);

            auto own = PipelineRegions::FindTransparency(*mNode, id);
            return own ? &(*own)[key.Data()] : nullptr;
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
        Ref<Button>              mSplitModel;   // The model the auto-split asks
        Ref<EditBox>             mPrompt;       // Name of the selected part
        String                   mPromptRegion; // Part the prompt field edits
        bool mWritingRoi = false; // True while the selected box is pushed into the paint editor


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

        bool IsOwnPart(const String& id) const
        {
            for (auto& region : Regions())
            {
                if (region.id == id)
                    return region.IsOwn();
            }
            return false;
        }

        String SplitModel() const
        {
            String model = GetString("splitModel", "").Trimed(" \n\r\t");
            return model.IsEmpty() ? String(PipelineRegions::autoSplitDefaultModel) : model;
        }

        // What the folded list says: the node's model, its background, how many parts have their own settings
        Vector<String> ParamsSummary() const
        {
            String model = mNode->GetConfigString("model", GeminiProvider::defaultImageModel);
            String bg = !mNode->GetConfigBool("transparentBg", false) ? "Opaque" :
                PipelineTransparency::SupportsNativeTransparency(model) ? "Bg by model" :
                mNode->GetConfigString("transparentMode", "twoPass") == "chroma" ? "Chroma key" : "White / black";
            Vector<String> names = { PipelineUtils::PrettyModelName(model), bg };
            auto regions = Regions();
            int own = regions.Count() > 1 ? regions.Sum<int>([](const PipelineExtractRegion& r) { return r.IsOwn() ? 1 : 0; }) : 0;
            if (own > 0)
                names.Add((String)own + " own");

            return names;
        }

        // The line under the scope switch: who the rows below change
        String ScopeNote() const
        {
            auto regions = Regions();
            String id = SelectedId();
            for (int i = 0; i < regions.Count(); i++)
            {
                if (regions[i].id != id || !regions[i].IsOwn())
                    continue;

                String head = regions[i].name;
                int dash = head.Find(" - ");
                if (dash >= 0)
                    head = head.SubStr(0, dash);
                head = head.Trimed(" \n\r\t");
                if (UiDoc::JsSlice(head, 0, 40).Length() < head.Length())
                    head = UiDoc::JsSlice(head, 0, 31) + "\xE2\x80\xA6";
                return "Changes only \xE2\x80\x9C" + (head.IsEmpty() ? "part " + (String)(i + 1) : head) + "\xE2\x80\x9D";
            }

            int follow = regions.Sum<int>([](const PipelineExtractRegion& r) { return r.IsOwn() ? 0 : 1; });
            return follow == regions.Count() ? String("Changes every part") :
                "Changes " + (String)follow + " of " + (String)regions.Count() + " parts \xE2\x80\x94 the others have their own";
        }

        // Several parts: the scope switch and its note; then one set of rows for whatever it says - model, background,
        // method; then the node's own rows: seed and the source's DPI
        void AddParamRows()
        {
            AddScopeRow();
            AddModelRow(PipelineImageModelPresets(), GeminiProvider::defaultImageModel, PipelineModelKind::Image);
            WeakRef<ImageExtractBody> weakThis(this);
            auto transparent = AddCheckbox("Transparent background", "transparentBg", false);
            transparent->name = "transparent background";
            transparent->onToggleByUser = [weakThis](bool value)
            {
                if (auto self = weakThis.Lock())
                {
                    self->SetBool("transparentBg", value, true);
                    self->RefreshParamRows();
                }
            };
            AddTransparencyBlock();
            AddRow(MakeListDivider(), 9);
            AddSeedRow();
            AddSourceDpiRow();
        }

        // The density of the source the parts are measured against: typed, else the picture's own tag, else the base
        void AddSourceDpiRow()
        {
            String typed = mNode->HasConfig("sourceDpi") ? PipelineUtils::ValueToString(*mNode->GetConfigValue("sourceDpi"), "") : String();
            auto tagged = PipelinePngMeta::ReadDpi(GetInput("image").GetPngBytes());
            String placeholder = (String)(int)Math::Round((float)tagged.value_or(UiDoc::UI_BASE_DPI));
            auto field = MakeEditBox(typed, false, placeholder);
            field->name = "source dpi";
            field->layout->minWidth = field->layout->maxWidth = 64;
            WeakRef<ImageExtractBody> weakThis(this);
            field->onChangeCompleted = [weakThis](const WString& text)
            {
                auto self = weakThis.Lock();
                if (!self)
                    return;

                String s = ((String)text).Trimed();
                UiLayout::Json value(s);
                auto dpi = s.IsEmpty() ? std::nullopt : UiDoc::CleanDpi(&value);
                if (s.IsEmpty())
                {
                    self->mNode->RemoveConfig("sourceDpi");
                    self->Notify("sourceDpi", true);
                }
                else if (dpi)
                    self->SetNumber("sourceDpi", (float)*dpi, true);

                self->RefreshParamRows();
            };

            auto note = MakeLabel(!typed.IsEmpty() ? "set here" : tagged ? "the picture's own" : "the base, 480", true);
            StyleLabel(note, 10.5f, 400, PipelineTheme::Get().fg3);
            auto line = mmake<HorizontalLayout>();
            line->spacing = 6;
            line->expandWidth = true;
            line->expandHeight = true;
            line->baseCorner = BaseCorner::Left;
            line->AddChild(field);
            line->AddChild(note);
            AddRow(MakeRow("Source DPI", line, 86.0f), 22);
        }

        void PickSplitModel()
        {
            auto editor = mEditor.Lock();
            if (!editor || !mSplitModel)
                return;

            if (auto picker = editor->GetModelPicker())
            {
                if (picker->IsOpenFor(mSplitModel))
                {
                    picker->Close();
                    return;
                }

                if (picker->ConsumeClosedByField(mSplitModel))
                    return;
            }

            RectF world = mSplitModel->layout->GetWorldRect();
            Vec2F a = editor->LocalToScreenPoint(Vec2F(world.left, world.bottom));
            Vec2F b = editor->LocalToScreenPoint(Vec2F(world.right, world.top));
            PipelineModelPickerRequest request;
            request.ids = PipelineTextModelPresets();
            request.ids.Add(LocalAgentProvider::OfferedModels(PipelineSettings::Load()));
            request.kind = PipelineModelKind::Text;
            request.current = SplitModel();
            request.anchor = RectF(Math::Min(a.x, b.x), Math::Max(a.y, b.y), Math::Max(a.x, b.x), Math::Min(a.y, b.y));
            request.field = mSplitModel;
            WeakRef<ImageExtractBody> weakThis(this);
            request.onPick = [weakThis](const String& id)
            {
                auto self = weakThis.Lock();
                if (!self)
                    return;

                self->SetString("splitModel", id, true);
                SetModelFieldValue(self->mSplitModel, id, PipelineModelKind::Text);
            };
            editor->ShowModelPicker(request);
        }

        // Rebuilds the rows of the open parameter list for the selected part, leaving the rest of the card alone; for the user's
        // own change a hand-sized card follows the rows, so the pictures keep their size
        void RefreshParamRows(bool keepArea = true)
        {
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

        // "Model & bg": the model and background rows below edit every part (the node) or the selected part alone
        void AddScopeRow()
        {
            if (Regions().Count() < 2)
                return;

            bool own = IsOwnPart(SelectedId());
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
            AddRow(MakeRow("Model & bg", segments, 86.0f), 22);
            AddMutedLine(ScopeNote());
        }

        // Gives the selected part its own model and background settings, copied from what it uses now, or returns it to the
        // node's
        void SetPartScope(bool own)
        {
            String id = SelectedId();
            if (PipelineRegions::SetOwnPart(*mNode, id, own))
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

            // One of several parts renamed keeps the card: the ports keep their ids, the cell and the field take the name
            if (regions.Count() > 1 && mGrid && mGrid->GetCellCount() == regions.Count())
            {
                PipelineRegions::Write(*mNode, regions);
                PipelineRegions::SyncPorts(*mNode);
                mNode->SetConfigString("selectedRegion", id);
                if (mGrid->RenamePart(id, name))
                {
                    if (mPrompt && mPromptRegion == id && !mPrompt->IsFocused() && (String)mPrompt->GetText() != name)
                        mPrompt->SetText(name);
                    mGrid->SetSelected(id);
                    SyncPaintRegions();
                    if (auto editor = mEditor.Lock())
                        editor->OnNodeConfigChanged(mOwner.Lock(), "regions", true);
                    return;
                }
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
                mGrid->SetPartImage(region.id, Picture(value));
            }
        }

        // Sends the selected frame of the source to the vision model (splitModel), which cuts it into the pieces of a UI kit;
        // the pieces take the frame's place in the list (the first one keeps its port, so a link hanging off it survives)
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

            String model = SplitModel();
            AiRouteKeys keys = AiRouter::KeysOf(ctx->settings);
            AiRoute route = AiRouter::ChooseRoute(model, keys);
            if (route.provider != AiProvider::Local && keys.Of(route.provider).IsEmpty())
            {
                ctx->Log("Auto split: no " + AiRouter::ProviderName(route.provider) + " key - set it in the pipeline settings");
                return;
            }

            // The selected part is the frame: the model sees only that part of the source
            PipelineExtractRegion frame = PipelineRegions::RegionOfPort(*mNode, SelectedId());
            bool framed = frame.x > 0.001f || frame.y > 0.001f || frame.w < 0.999f || frame.h < 0.999f;
            Ref<Bitmap> cropped = framed ? PipelineImageOps::Crop(*bitmap, { frame.x, frame.y, frame.w, frame.h }) : bitmap;

            // Gemini places boxes on its own grid; any other model answers pixels of the picture it gets, scaled to what it reads
            // without scaling it itself
            Vec2I size = cropped->GetSize();
            auto analysis = UiLayout::AnalysisFrameOf(model, size.x, size.y);
            if (analysis.boxes == "pixels" && analysis.scale < 1.0)
                cropped = PipelineImageOps::Resize(*cropped, Vec2I(analysis.w, analysis.h));

            PipelineRegions::SplitFrame split;
            split.boxes = analysis.boxes;
            split.w = analysis.w;
            split.h = analysis.h;

            if (mAutoButton)
                mAutoButton->interactable = false;

            ctx->Log("Auto split: asking " + model + " for the pieces of the image");

            WeakRef<ImageExtractBody> weakThis(this);
            mAutoSplitJob = [](WeakRef<ImageExtractBody> weakBody, Ref<PipelineExecContext> context, String textModel,
                               String imageBytes, PipelineRegions::SplitFrame frameAsked, PipelineExtractRegion box) -> Coroutine<void>
            {
                AiTextResult answer = co_await AiRouter::GenerateText(context, textModel, PipelineRegions::AutoSplitPrompt(frameAsked),
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

                auto found = PipelineRegions::ParseAutoSplit(answer.text, &frameAsked);
                if (found.IsEmpty())
                {
                    context->Log("Auto split: the model listed no parts");
                    co_return;
                }

                context->Log("Auto split: " + (String)found.Count() + " parts");
                body->PlaceSplitParts(found, box);
            }(weakThis, ctx, model, EncodeBitmapPng(*cropped), split, frame);

            mAutoSplitJob.Start(JobThread::Main);
        }

        // Puts the pieces the auto-split found in place of the frame they were found in; the background plate is opaque (its own
        // setting), every other piece a transparent sprite
        void PlaceSplitParts(Vector<PipelineExtractRegion> found, const PipelineExtractRegion& frame)
        {
            for (int i = 0; i < found.Count(); i++)
            {
                auto& r = found[i];
                r.x = frame.x + r.x*frame.w;
                r.y = frame.y + r.y*frame.h;
                r.w *= frame.w;
                r.h *= frame.h;
                if (i == 0)
                    r.id = frame.id;
            }

            auto regions = Regions();
            int at = Math::Max(0, regions.IndexOf([&](const PipelineExtractRegion& r) { return r.id == frame.id; }));
            Vector<PipelineExtractRegion> next;
            for (int i = 0; i < at && i < regions.Count(); i++)
                next.Add(regions[i]);
            next.Add(found);
            for (int i = at + 1; i < regions.Count(); i++)
                next.Add(regions[i]);

            // The cut-outs are sprites, so transparency is on
            mNode->SetConfigBool("transparentBg", true);
            if (mNode->GetConfigString("transparentMode", "") != "twoPass")
                mNode->SetConfigString("transparentMode", "chroma");

            PipelineRegions::Write(*mNode, next);
            for (auto& r : found)
            {
                if (!r.background)
                    continue;

                if (auto part = PipelineRegions::FindPart(*mNode, r.id))
                {
                    DataDocument own;
                    own.SetObject();
                    own["transparentBg"] = false;
                    part->RemoveMember("transparency");
                    (*part)["transparency"] = static_cast<const DataValue&>(own);
                }
            }

            WriteRegions(PipelineRegions::Read(*mNode), found[0].id);
        }

        Coroutine<void> mAutoSplitJob; // Running auto split request
    };

    Ref<PipelineNodeBody> CreateExtractNodeBody(const String& type)
    {
        return type == "imageExtract" ? mmake<ImageExtractBody>() : nullptr;
    }
}
