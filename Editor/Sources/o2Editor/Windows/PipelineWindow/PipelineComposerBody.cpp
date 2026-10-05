#include "o2Editor/stdafx.h"
#include "PipelineNodeBodyFactories.h"

#include "o2/Assets/Assets.h"
#include "o2/Utils/Editor/EditorScope.h"
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
#include "o2Editor/Pipeline/PipelineExecutor.h"
#include "o2Editor/Pipeline/PipelineImageOps.h"
#include "o2Editor/Pipeline/PipelineUtils.h"
#include "o2Editor/Pipeline/Providers/ElevenLabsProvider.h"
#include "o2Editor/Pipeline/Providers/GeminiProvider.h"
#include "o2Editor/Pipeline/Providers/VideoProviders.h"
#include "o2Editor/Windows/PipelineWindow/PipelineComposerLayers.h"
#include "o2Editor/Windows/PipelineWindow/PipelineComposerStage.h"
#include "o2Editor/Windows/PipelineWindow/PipelineEditor.h"
#include "o2Editor/Windows/PipelineWindow/PipelineMediaViews.h"
#include "o2Editor/Windows/PipelineWindow/PipelineNodeWidget.h"
#include "o2Editor/Windows/PipelineWindow/PipelinePaintEditor.h"
#include "o2/Render/VectorSprite.h"

namespace Editor
{
    using namespace PipelineControls;

    // ------------------------------------------------------------------------------------------------------------
    // The composer's parameters under its work area: captioned groups, right-aligned, wrapping onto more lines when
    // the column is narrow
    // ------------------------------------------------------------------------------------------------------------
    class PipelineComposerControls : public Widget
    {
    public:
        static constexpr float lineHeight = 22.0f; // One line of groups
        static constexpr float lineGap = 4.0f;     // Between the lines
        static constexpr float groupGap = 12.0f;   // Between the groups of a line

    public:
        explicit PipelineComposerControls(RefCounter* refCounter): Widget(refCounter) {}

        // Adds a group of the width
        void AddGroup(const Ref<Widget>& group, float width)
        {
            mGroups.Add({ group, width });
            AddChild(group);
        }

        // Returns the height of the lines the shown groups take in the width
        float GetHeightForWidth(float width) const
        {
            int lines = Lines(width).Count();
            return lines > 0 ? lines*lineHeight + (lines - 1)*lineGap : 0.0f;
        }

        void UpdateSelfTransform() override
        {
            Widget::UpdateSelfTransform();
            float y = 0.0f;
            for (auto& line : Lines(layout->GetWidth()))
            {
                float x = 0.0f;
                for (int i = line.Count() - 1; i >= 0; i--)
                {
                    auto& group = mGroups[line[i]];
                    *group.widget->layout = WidgetLayout(Vec2F(1, 1), Vec2F(1, 1), Vec2F(-x - group.width, -y - lineHeight), Vec2F(-x, -y));
                    x += group.width + groupGap;
                }
                y += lineHeight + lineGap;
            }
        }

    private:
        struct Group
        {
            Ref<Widget> widget; // Caption and controls
            float       width;  // Width they take

            bool operator==(const Group& other) const { return widget == other.widget; }
        };

        Vector<Group> mGroups;

    private:
        // Returns the indices of the shown groups by line, filled in order
        Vector<Vector<int>> Lines(float width) const
        {
            Vector<Vector<int>> lines;
            float x = 0.0f;
            for (int i = 0; i < mGroups.Count(); i++)
            {
                if (!mGroups[i].widget->IsEnabled())
                    continue;

                float need = mGroups[i].width + (lines.IsEmpty() || lines.Last().IsEmpty() ? 0.0f : groupGap);
                if (lines.IsEmpty() || (x + need > width + 0.5f && !lines.Last().IsEmpty()))
                {
                    lines.Add(Vector<int>());
                    x = 0.0f;
                    need = mGroups[i].width;
                }
                lines.Last().Add(i);
                x += need;
            }
            return lines;
        }
    };

    // -------------------------------------------------------------------------------
    // Right column of the composer: the work area over its parameters, laid out for the
    // column's own width
    // -------------------------------------------------------------------------------
    class PipelineComposerColumn : public Widget
    {
    public:
        static constexpr float gap = 6.0f; // Between the work area and the parameters

        Ref<Widget>                   stage;    // Work area
        Ref<PipelineComposerControls> controls; // Parameters

    public:
        explicit PipelineComposerColumn(RefCounter* refCounter): Widget(refCounter) {}

        void UpdateSelfTransform() override
        {
            Widget::UpdateSelfTransform();
            float height = controls ? controls->GetHeightForWidth(layout->GetWidth()) : 0.0f;
            if (stage)
                *stage->layout = WidgetLayout::BothStretch(0, height + gap, 0, 0);
            if (controls)
                *controls->layout = WidgetLayout(Vec2F(0, 0), Vec2F(1, 0), Vec2F(0, 0), Vec2F(0, height));
        }
    };

    class ComposerBody : public PipelineNodeBody
    {
    public:
        using PipelineNodeBody::PipelineNodeBody;

        static constexpr float panelMin = 200.0f;
        static constexpr float panelMax = 560.0f;
        static constexpr float panelDefault = 290.0f;
        static constexpr float workAreaMin = 300.0f;

        void Build() override
        {
            WeakRef<ComposerBody> weakThis(this);

            mStage = mmake<PipelineComposerStage>();
            mStage->imageOfPort = [weakThis](const String& portId) -> Ref<Bitmap>
            {
                auto self = weakThis.Lock();
                auto editor = self ? self->mEditor.Lock() : nullptr;
                if (!editor)
                    return nullptr;
                auto value = editor->GetInputValueById(self->mNode, portId);
                return value.IsImage() ? value.GetBitmap() : nullptr;
            };
            mStage->onConfigChanged = [weakThis](const String& key, bool completed)
            {
                if (auto self = weakThis.Lock())
                {
                    self->Notify(key, completed);
                    // Only the highlight moves: a rebuild would take the focus from a name field that selected its layer
                    if (key == "selectedLayer")
                    {
                        if (self->mLayers)
                            self->mLayers->UpdateSelection();
                        self->UpdateToolbarState();
                    }
                }
            };
            mStage->Init(mNode);

            BuildMain();
            BuildControls();
            BuildAssetRows();
            RefreshLayers(false);
            UpdateToolbarState();
            OnOutputChanged();
        }

        void OnOutputChanged() override
        {
            PipelineNodeBody::OnOutputChanged();
            if (mStage)
            {
                mStage->Refresh();
                RefreshLayers(false);
            }
        }

        // The input ports sit on the layer rows, at the card's left edge
        bool InputsInBody() const override { return true; }

        bool HasBodyPort(const String& portId) const override
        {
            return mNode->inputs.Any([&](const PipelinePort& port) { return port.id == portId; });
        }

        bool GetBodyPortOffset(const String& portId, Vec2F& offset) const override
        {
            auto owner = mOwner.Lock();
            if (!owner || !mLayers || !HasBodyPort(portId))
                return false;

            float top = 0.0f, height = 0.0f;
            if (!GetRowPlacement(mMain, owner->GetCardWidth(), owner->GetBodyAreaHeight(), top, height))
                return false;

            // An input without a layer row sits by "+ input"
            float y = 0.0f;
            if (!mLayers->GetPortCenter(portId, y))
                y = mLayers->GetAddRowCenter();

            offset = Vec2F(0.0f, top + y);
            return true;
        }

        bool IsAddInputAt(const Vec2F& point) const override
        {
            return mLayers && mLayers->GetAddRow() && mLayers->GetAddRow()->layout->IsPointInside(point);
        }

        void DrawFarContent() override
        {
            if (mStage)
                mStage->Draw();
        }

        void OnConfigChanged() override
        {
            if (mStage)
            {
                mStage->Refresh();
                RefreshLayers(true);
                UpdateToolbarState();
            }
        }

        void Update(float dt) override
        {
            PipelineNodeBody::Update(dt);
            if (mStage && mZoomButton)
            {
                String caption = (String)(int)Math::Round(mStage->GetViewScale() * 100.0f) + "%";
                if (mZoomCaption != caption)
                {
                    mZoomCaption = caption;
                    mZoomButton->caption = caption;
                }
            }
        }

    private:
        Ref<PipelineComposerStage> mStage;
        Ref<Widget> mMain;
        Ref<PipelineComposerLayersPanel> mLayers;
        Ref<DragHandle> mSplitter;
        Ref<Button> mZoomButton;
        String mZoomCaption;
        Ref<Toggle> mFlipH;
        Ref<Toggle> mFlipV;
        Ref<Widget> mFlipGroup;                    // Flips, shown while a layer is selected
        Ref<PipelineComposerControls> mControls; // Parameters under the work area
        Ref<PipelineComposerColumn>   mColumn;   // The work area over the parameters
        Ref<EditBox> mFolderEdit; // Folder inside Assets the layers are written to
        Ref<EditBox> mNameEdit;   // File name prefix of the layer assets
        Ref<Label> mSaveInfo;     // Target pattern or the outcome of the last save

        float GetPanelWidth() const
        {
            return Math::Clamp(GetNumber("layersPanelW", panelDefault), panelMin, panelMax);
        }

        Ref<EditBox> MakeNumberEdit(float value, float width, const Function<void(float)>& onChange)
        {
            auto edit = MakeEditBox(FormatNumber(value, 1), false);
            edit->SetFilterInteger();
            edit->layout->minWidth = width;
            edit->layout->maxWidth = width;
            edit->onChangeCompleted = [onChange](const WString& text) { onChange((float)atof(((String)text).Data())); };
            return edit;
        }

        // Returns a group of controls after a small dim caption; width gets the width they take
        Ref<Widget> MakeGroup(const String& caption, const Vector<Ref<Widget>>& controls, float& width)
        {
            auto group = mmake<HorizontalLayout>();
            group->name = caption;
            group->spacing = 3;
            group->expandWidth = false;
            group->expandHeight = true;
            group->baseCorner = BaseCorner::Left;

            auto label = MakeLabel(caption, true);
            label->height = 10;
            label->horOverflow = Label::HorOverflow::None;
            // Glyphs the font has not rasterized yet measure too narrow
            auto font = mmake<Text>("stdFont.ttf")->GetFont();
            font->CheckCharacters(caption, 10);
            float captionWidth = Math::Ceil(Text::GetTextSize(caption, font, 10, Vec2F(), HorAlign::Left, VerAlign::Top, false).x) + 4.0f;
            label->layout->minWidth = captionWidth;
            label->layout->maxWidth = captionWidth;
            group->AddChild(label);
            width = captionWidth;
            for (auto& control : controls)
            {
                group->AddChild(control);
                width += group->spacing + control->layout->minWidth;
            }
            return group;
        }

        void BuildControls()
        {
            WeakRef<ComposerBody> weakThis(this);
            mControls = mmake<PipelineComposerControls>();
            mControls->name = "composer controls";
            mColumn->controls = mControls;
            mColumn->AddChild(mControls);

            auto fixed = [](const Ref<Widget>& widget, float width) { widget->layout->minWidth = width; widget->layout->maxWidth = width; return widget; };
            auto small = [&](const String& text, float width)
            {
                auto label = MakeLabel(text, false);
                label->horOverflow = Label::HorOverflow::None;
                return fixed(label, width);
            };

            float width = 0.0f;
            auto canvas = MakeGroup("Canvas", {
                small("W", 12),
                fixed(MakeNumberEdit(GetNumber("canvasW", 1024), 54, [weakThis](float v)
                {
                    if (auto self = weakThis.Lock()) self->SetNumber("canvasW", Math::Clamp(Math::Round(v), 16.0f, 8192.0f), true);
                }), 54),
                small("x", 8),
                small("H", 12),
                fixed(MakeNumberEdit(GetNumber("canvasH", 1024), 54, [weakThis](float v)
                {
                    if (auto self = weakThis.Lock()) self->SetNumber("canvasH", Math::Clamp(Math::Round(v), 16.0f, 8192.0f), true);
                }), 54) }, width);
            mControls->AddGroup(canvas, width);

            auto zoomOut = MakeButton("-");
            zoomOut->onClick = [weakThis]() { if (auto self = weakThis.Lock()) self->SetZoom(self->GetNumber("viewZoom", 1)/1.25f); };
            mZoomButton = MakeButton("100%");
            mZoomButton->onClick = [weakThis]() { if (auto self = weakThis.Lock()) self->SetZoom(1.0f); };
            auto zoomIn = MakeButton("+");
            zoomIn->onClick = [weakThis]() { if (auto self = weakThis.Lock()) self->SetZoom(self->GetNumber("viewZoom", 1)*1.25f); };
            auto zoom = MakeGroup("Zoom", { fixed(zoomOut, 22), fixed(mZoomButton, 48), fixed(zoomIn, 22) }, width);
            mControls->AddGroup(zoom, width);

            auto makeFlip = [&](bool vertical)
            {
                auto toggle = MakeSegment("", false);
                auto icon = mmake<VectorSprite>(vertical ? "ui/pipeline/btn_flip_v.svg" : "ui/pipeline/btn_flip_h.svg");
                icon->color = PipelineControls::textColor;
                toggle->AddLayer("icon", icon, Layout::Based(BaseCorner::Center, Vec2F(14, 14)));
                toggle->onToggleByUser = [weakThis, vertical](bool) { if (auto self = weakThis.Lock()) self->FlipSelected(vertical); };
                return fixed(toggle, 26);
            };
            mFlipH = DynamicCast<Toggle>(makeFlip(false));
            mFlipV = DynamicCast<Toggle>(makeFlip(true));
            mFlipGroup = MakeGroup("Flip", { mFlipH, mFlipV }, width);
            mControls->AddGroup(mFlipGroup, width);

            auto preview = MakeCheckbox("Preview", GetBool("cmpBgEnabled", false));
            preview->name = "preview background";
            preview->onToggleByUser = [weakThis](bool v) { if (auto self = weakThis.Lock()) { self->SetBool("cmpBgEnabled", v, true); self->RebuildBody(); } };
            Vector<Ref<Widget>> background = { fixed(preview, 84) };
            if (GetBool("cmpBgEnabled", false))
                background.Add(MakeColorSwatch("cmpBg", "#3a6ea5"));
            auto output = MakeCheckbox("Output", GetBool("outBgEnabled", false));
            output->name = "output background";
            output->onToggleByUser = [weakThis](bool v) { if (auto self = weakThis.Lock()) { self->SetBool("outBgEnabled", v, true); self->RebuildBody(); } };
            background.Add(fixed(output, 80));
            if (GetBool("outBgEnabled", false))
                background.Add(MakeColorSwatch("outBg", "#ffffff"));
            mControls->AddGroup(MakeGroup("Background", background, width), width);

            LayoutMain();
        }

        // Returns the height of the parameters under the work area for the card width
        float ControlsHeight(float cardWidth) const
        {
            float column = cardWidth - mPadding*2.0f - GetPanelWidth() - 8.0f;
            return mControls ? mControls->GetHeightForWidth(column) : PipelineComposerControls::lineHeight;
        }

        Ref<Widget> MakeColorSwatch(const String& key, const String& def)
        {
            Color4 color;
            if (!PipelineUtils::ParseHexColor(GetString(key, def), color))
                PipelineUtils::ParseHexColor(def, color);
            auto swatch = MakeButton("");
            swatch->layout->minWidth = 26; swatch->layout->maxWidth = 26;
            if (auto regular = swatch->GetLayerDrawableBasedOn<IRectDrawable>("regular")) regular->color = Color4(color.r, color.g, color.b, 255);
            WeakRef<ComposerBody> weakThis(this);
            String k = key;
            swatch->onClick = [weakThis, k]()
            {
                auto self = weakThis.Lock();
                if (!self) return;
                Color4 current;
                PipelineUtils::ParseHexColor(self->GetString(k, "#ffffff"), current);
                ColorPickerDlg::Show(current, [weakThis, k](const Color4& value, bool)
                {
                    if (auto self = weakThis.Lock()) self->SetString(k, PipelineUtils::ColorToHex(Color4(value.r, value.g, value.b, 255)), false);
                }, [weakThis, k]()
                {
                    if (auto self = weakThis.Lock()) { self->SetString(k, self->GetString(k, "#ffffff"), true); self->RebuildBody(); }
                });
            };
            return swatch;
        }

        void SetZoom(float zoom)
        {
            SetNumber("viewZoom", Math::Clamp(zoom, 0.1f, 8.0f), true);
        }

        void FlipSelected(bool vertical)
        {
            String selected = mStage->GetSelectedLayer();
            for (auto& layer : mStage->GetLayers())
            {
                if (layer.id != selected)
                    continue;
                auto p = mStage->GetPlacement(layer);
                if (vertical) p.flipV = !p.flipV; else p.flipH = !p.flipH;
                mStage->WritePlacement(layer.id, p, true);
            }
            UpdateToolbarState();
        }

        void UpdateToolbarState()
        {
            String selected = mStage ? mStage->GetSelectedLayer() : String();
            bool has = false;
            ComposerLayerPlacement p;
            for (auto& layer : mStage->GetLayers())
            {
                if (layer.id == selected) { has = true; p = mStage->GetPlacement(layer); }
            }
            if (mFlipGroup && mFlipGroup->IsEnabled() != has)
            {
                mFlipGroup->enabled = has;
                LayoutMain();
            }
            mFlipH->SetValue(has && p.flipH);
            mFlipV->SetValue(has && p.flipV);
        }

        void BuildMain()
        {
            WeakRef<ComposerBody> weakThis(this);
            mMain = mmake<Widget>();
            mMain->name = "composer main";
            // The list never scrolls: the row is as tall as the list, at least the work area's minimum
            // The right column reserves two lines of parameters under the work area's minimum
            AddFlexible(mMain, [weakThis](float rowWidth)
            {
                auto self = weakThis.Lock();
                if (!self || !self->mLayers)
                    return workAreaMin;

                float controls = Math::Max(self->ControlsHeight(rowWidth + self->mPadding*2.0f),
                                           PipelineComposerControls::lineHeight*2.0f + PipelineComposerControls::lineGap);
                return Math::Max(self->mLayers->GetContentHeight(), workAreaMin + PipelineComposerColumn::gap + controls);
            });

            mColumn = mmake<PipelineComposerColumn>();
            mColumn->name = "composer column";
            mColumn->stage = mStage;
            mColumn->AddChild(mStage);
            mMain->AddChild(mColumn);

            mLayers = mmake<PipelineComposerLayersPanel>();
            mLayers->name = "layers panel";
            mLayers->Setup(mNode, mStage);
            mLayers->onConfigChanged = [weakThis](const String& key, bool completed) { if (auto self = weakThis.Lock()) self->Notify(key, completed); };
            mLayers->onLayoutChanged = [weakThis]() { if (auto self = weakThis.Lock()) self->RelayoutCard(); };
            mLayers->onDuplicate = [weakThis](const String& id) { if (auto self = weakThis.Lock()) self->DuplicateLayer(id); };
            mLayers->onMove = [weakThis](int index, int dir) { if (auto self = weakThis.Lock()) self->ReorderLayer(index, dir); };
            mLayers->onRemove = [weakThis](const String& id, const String& portId, bool dup)
            {
                if (auto self = weakThis.Lock()) self->RemoveLayer(id, portId, dup);
            };
            mLayers->onRename = [weakThis](const String& id, const String& portId, bool dup, const String& name)
            {
                if (auto self = weakThis.Lock()) self->RenameLayer(id, portId, dup, name);
            };
            mLayers->onReorder = [weakThis](const Vector<String>& ids)
            {
                if (auto self = weakThis.Lock())
                {
                    self->WriteOrder(ids);
                    self->AfterStructureChange("layerOrder");
                }
            };
            mLayers->onAddInput = [weakThis]()
            {
                auto self = weakThis.Lock();
                auto editor = self ? self->mEditor.Lock() : nullptr;
                auto owner = self ? self->mOwner.Lock() : nullptr;
                if (editor && owner)
                    editor->AddCustomInput(owner);
            };
            mMain->AddChild(mLayers);

            auto bar = mmake<VectorSprite>("ui/UI4_Ver_separator.svg");
            auto barHover = mmake<VectorSprite>("ui/UI4_Ver_separator.svg");
            barHover->color = PipelineControls::accentColor;
            mSplitter = mmake<DragHandle>(bar, barHover, barHover);
            mSplitter->SetDrawablesSize(Vec2F(8, 40));
            mSplitter->cursorType = CursorType::SizeWE;
            mSplitter->onChangedPos = [weakThis](const Vec2F& pos)
            {
                if (auto self = weakThis.Lock())
                {
                    // Dragging the splitter to the right widens the list
                    RectF rect = self->mMain->layout->GetWorldRect();
                    float width = Math::Clamp(pos.x - rect.left - 4.0f, panelMin, panelMax);
                    self->mNode->SetConfigNumber("layersPanelW", Math::Round(width));
                    self->Notify("layersPanelW", false);
                    self->LayoutMain();
                }
            };
            mSplitter->onChangeCompleted = [weakThis]() { if (auto self = weakThis.Lock()) self->Notify("layersPanelW", true); };
            mMain->onDraw = [weakThis]()
            {
                if (auto self = weakThis.Lock())
                {
                    RectF rect = self->mMain->layout->GetWorldRect();
                    float x = rect.left + self->GetPanelWidth() + 4.0f;
                    if (!self->mSplitter->IsPressed())
                        self->mSplitter->position = Vec2F(x, rect.Center().y);
                    self->mSplitter->SetDrawablesSize(Vec2F(8, Math::Max(20.0f, rect.Height())));
                    self->mSplitter->Draw();
                }
            };
            LayoutMain();
        }

        void LayoutMain()
        {
            // The list keeps its own height at the top; the work area takes the rest of the right column over the parameters
            float panelW = GetPanelWidth();
            float panelH = mLayers ? mLayers->GetContentHeight() : workAreaMin;
            *mLayers->layout = WidgetLayout(Vec2F(0, 1), Vec2F(0, 1), Vec2F(0, -panelH), Vec2F(panelW, 0));
            *mColumn->layout = WidgetLayout(Vec2F(0, 0), Vec2F(1, 1), Vec2F(panelW + 8, 0), Vec2F(0, 0));
            mColumn->SetLayoutDirty();
        }

        // Rebuilds the layer rows; relayout moves the card and its ports with a list that changed its height
        void RefreshLayers(bool relayout)
        {
            if (!mLayers)
                return;

            mLayers->Rebuild();
            LayoutMain();
            if (relayout)
                RelayoutCard();
        }

        void RelayoutCard()
        {
            LayoutMain();
            if (auto owner = mOwner.Lock())
                owner->UpdateFromNode();
        }

        Vector<String> OrderIds() const
        {
            Vector<String> ids;
            for (auto& layer : mStage->GetLayers())
                ids.Add(layer.id);
            return ids;
        }

        void WriteOrder(const Vector<String>& ids)
        {
            mNode->RemoveConfig("layerOrder");
            auto& arr = mNode->config["layerOrder"];
            arr.SetArray();
            for (auto& id : ids)
                arr.AddElement() = id;
        }

        void AfterStructureChange(const String& key)
        {
            Notify(key, true);
            mStage->Refresh();
            RefreshLayers(true);
            UpdateToolbarState();
        }

        void ReorderLayer(int index, int dir)
        {
            auto ids = OrderIds();
            int j = index + dir;
            if (index < 0 || index >= ids.Count() || j < 0 || j >= ids.Count())
                return;
            String tmp = ids[index]; ids[index] = ids[j]; ids[j] = tmp;
            WriteOrder(ids);
            AfterStructureChange("layerOrder");
        }

        void DuplicateLayer(const String& id)
        {
            auto layers = mStage->GetLayers();
            auto source = layers.Find([&](const ComposerLayerRef& l) { return l.id == id; });
            if (!source)
                return;

            String dupId = PipelineNode::GenerateId();
            auto& dups = mNode->config["dupLayers"];
            if (!dups.IsArray())
                dups.SetArray();
            auto& item = dups.AddElement();
            item.SetObject();
            item["id"] = dupId;
            item["srcPortId"] = source->portId;
            item["name"] = (source->name.IsEmpty() ? String("layer") : source->name) + " copy";

            auto ids = OrderIds();
            int at = ids.IndexOf(id);
            ids.Insert(dupId, at + 1);
            WriteOrder(ids);

            auto p = mStage->GetPlacement(*source);
            p.x += 24; p.y += 24;
            mStage->WritePlacement(dupId, p, true);
            mNode->SetConfigString("selectedLayer", dupId);
            AfterStructureChange("dupLayers");
        }

        void RemoveLayerConfig(const String& id)
        {
            if (auto layers = mNode->config.FindMember("layers"))
            {
                if (layers->IsObject())
                    layers->RemoveMember(id.Data());
            }
        }

        void RemoveLayer(const String& id, const String& portId, bool dup)
        {
            Vector<String> dead = { id };
            if (dup)
            {
                auto dups = mNode->GetConfigValue("dupLayers");
                DataDocument copy;
                copy.SetArray();
                if (dups && dups->IsArray())
                {
                    for (auto& d : *dups)
                    {
                        auto did = d.IsObject() ? d.FindMember("id") : nullptr;
                        if (did && PipelineUtils::ValueToString(*did) == id)
                            continue;
                        copy.AddElement() = d;
                    }
                }
                mNode->RemoveConfig("dupLayers");
                mNode->config["dupLayers"] = copy;
            }
            else
            {
                DataDocument copy;
                copy.SetArray();
                auto dups = mNode->GetConfigValue("dupLayers");
                if (dups && dups->IsArray())
                {
                    for (auto& d : *dups)
                    {
                        auto src = d.IsObject() ? d.FindMember("srcPortId") : nullptr;
                        auto did = d.IsObject() ? d.FindMember("id") : nullptr;
                        if (src && PipelineUtils::ValueToString(*src) == portId)
                        {
                            if (did) dead.Add(PipelineUtils::ValueToString(*did));
                            continue;
                        }
                        copy.AddElement() = d;
                    }
                }
                mNode->RemoveConfig("dupLayers");
                mNode->config["dupLayers"] = copy;
            }

            auto ids = OrderIds();
            ids.RemoveAll([&](const String& x) { return dead.Contains(x); });
            WriteOrder(ids);
            for (auto& d : dead)
                RemoveLayerConfig(d);
            if (dead.Contains(mNode->GetConfigString("selectedLayer", "")))
                mNode->SetConfigString("selectedLayer", "");
            if (dead.Contains(mNode->GetConfigString("openLayerSettings", "")))
                mNode->SetConfigString("openLayerSettings", "");

            if (dup)
            {
                AfterStructureChange("dupLayers");
                return;
            }

            auto editor = mEditor.Lock();
            auto owner = mOwner.Lock();
            if (editor && owner)
                editor->RemoveCustomInput(owner, portId);
        }

        void RenameLayer(const String& id, const String& portId, bool dup, const String& name)
        {
            if (dup)
            {
                if (auto dups = mNode->config.FindMember("dupLayers"))
                {
                    if (dups->IsArray())
                    {
                        for (auto& d : *dups)
                        {
                            auto did = d.IsObject() ? d.FindMember("id") : nullptr;
                            if (did && PipelineUtils::ValueToString(*did) == id)
                                d["name"] = name;
                        }
                    }
                }
                AfterStructureChange("dupLayers");
                return;
            }

            auto editor = mEditor.Lock();
            auto owner = mOwner.Lock();
            if (editor && owner)
                editor->RenameCustomInput(owner, portId, name);
        }

        void BuildAssetRows()
        {
            WeakRef<ComposerBody> weakThis(this);

            auto folderRow = MakeRow("Folder", nullptr, 56);
            mFolderEdit = MakeEditBox(GetString("layersFolder", "Generated"), false, "folder inside Assets");
            mFolderEdit->onChangeCompleted = [weakThis](const WString& text)
            {
                if (auto self = weakThis.Lock())
                {
                    self->SetString("layersFolder", CleanPath((String)text), true);
                    self->UpdateSaveInfo();
                }
            };
            folderRow->AddChild(mFolderEdit);
            auto browse = MakeButton("...");
            browse->name = "browse";
            browse->layout->minWidth = 28;
            browse->layout->maxWidth = 28;
            browse->onClick = [weakThis]()
            {
                auto self = weakThis.Lock();
                if (!self)
                    return;

                self->ShowAssetFolderMenu([weakThis](const String& folder)
                {
                    if (auto self = weakThis.Lock())
                    {
                        self->mFolderEdit->SetText(folder);
                        self->SetString("layersFolder", folder, true);
                        self->UpdateSaveInfo();
                    }
                });
            };
            folderRow->AddChild(browse);
            AddRow(folderRow, 22);

            auto nameRow = MakeRow("Name", nullptr, 56);
            mNameEdit = MakeEditBox(GetString("layersName", "layer"), false, "file name prefix");
            mNameEdit->onChangeCompleted = [weakThis](const WString& text)
            {
                if (auto self = weakThis.Lock())
                {
                    self->SetString("layersName", CleanPath((String)text), true);
                    self->UpdateSaveInfo();
                }
            };
            nameRow->AddChild(mNameEdit);
            AddRow(nameRow, 22);

            mSaveInfo = MakeLabel("", true);
            mSaveInfo->horOverflow = Label::HorOverflow::Dots;
            AddRow(mSaveInfo, 18);

            auto save = MakeButton("Save layers to Assets");
            save->name = "save layers";
            save->onClick = [weakThis]() { if (auto self = weakThis.Lock()) self->SaveLayersToAssets(); };
            AddRow(save, 24);
            UpdateSaveInfo();
        }

        static String CleanPath(const String& text)
        {
            String value = text.Trimed(" \n\r\t");
            value.ReplaceAll("\\", "/");
            while (value.StartsWith("/"))
                value = value.SubStr(1);
            while (value.EndsWith("/"))
                value = value.SubStr(0, value.Length() - 1);
            return value;
        }

        static String SafeFileName(const String& name)
        {
            String safe;
            for (int i = 0; i < name.Length(); i++)
            {
                char c = name[i];
                safe += (isalnum((unsigned char)c) || c == '-' || c == '_') ? c : '_';
            }
            return safe.IsEmpty() ? String("layer") : safe;
        }

        String LayersFolder() const
        {
            return CleanPath(GetString("layersFolder", "Generated"));
        }

        String LayersName() const
        {
            String name = CleanPath(GetString("layersName", "layer"));
            return name.IsEmpty() ? String("layer") : name;
        }

        void UpdateSaveInfo()
        {
            if (!mSaveInfo)
                return;

            String folder = LayersFolder();
            mSaveInfo->text = "Assets/" + (folder.IsEmpty() ? String() : folder + "/") + LayersName() + "_<layer>.png";
        }

        bool SaveToAssets() override
        {
            return SaveLayersToAssets() > 0;
        }

        // Writes every layer as its own PNG asset, the way a finish node writes its result; returns how many were written
        int SaveLayersToAssets()
        {
            String folder = LayersFolder();
            String dir = o2Assets.GetAssetsPath() + (folder.IsEmpty() ? String() : folder + "/");
            o2FileSystem.FolderCreate(dir, true);

            int saved = 0;
            Vector<String> taken;
            for (auto& layer : mStage->GetLayers())
            {
                auto bitmap = mStage->RenderLayer(layer);
                if (!bitmap)
                    continue;

                String file = LayersName() + "_" + SafeFileName(layer.name);
                String unique = file;
                for (int n = 2; taken.Contains(unique); n++)
                    unique = file + "_" + (String)n;
                taken.Add(unique);

                if (PipelineUtils::WriteFileBytes(dir + unique + ".png", PipelineValue::Image(bitmap).GetPngBytes()))
                    saved++;
            }

            if (saved > 0)
                o2Assets.RebuildAssets();

            if (mSaveInfo)
                mSaveInfo->text = saved > 0 ? (String)saved + " layers saved to Assets/" + folder : String("Nothing to save - connect image layers");

            return saved;
        }
    };

    Ref<PipelineNodeBody> CreateComposerNodeBody(const String& type)
    {
        if (type == "composer")
            return mmake<ComposerBody>();

        return nullptr;
    }
}
