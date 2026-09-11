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
#include "o2Editor/Windows/PipelineWindow/PipelineComposerStage.h"
#include "o2Editor/Windows/PipelineWindow/PipelineEditor.h"
#include "o2Editor/Windows/PipelineWindow/PipelineMediaViews.h"
#include "o2Editor/Windows/PipelineWindow/PipelineNodeWidget.h"
#include "o2Editor/Windows/PipelineWindow/PipelinePaintEditor.h"

namespace Editor
{
    using namespace PipelineControls;

    class ComposerBody : public PipelineNodeBody
    {
    public:
        using PipelineNodeBody::PipelineNodeBody;

        static constexpr float panelMin = 150.0f;
        static constexpr float panelMax = 560.0f;
        static constexpr float panelDefault = 230.0f;

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
                    if (key == "selectedLayer")
                    {
                        self->RebuildLayersPanel();
                        self->UpdateToolbarState();
                    }
                }
            };
            mStage->Init(mNode);

            BuildToolbar();
            BuildMain();
            BuildAssetRows();
            RebuildLayersPanel();
            UpdateToolbarState();
            OnOutputChanged();
        }

        void OnOutputChanged() override
        {
            PipelineNodeBody::OnOutputChanged();
            if (mStage)
            {
                mStage->Refresh();
                RebuildLayersPanel();
            }
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
                RebuildLayersPanel();
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
        Ref<ScrollArea> mPanel;
        Ref<VerticalLayout> mList;
        Ref<DragHandle> mSplitter;
        Ref<Button> mZoomButton;
        String mZoomCaption;
        Ref<Toggle> mFlipH;
        Ref<Toggle> mFlipV;
        Ref<EditBox> mFolderEdit; // Folder inside Assets the layers are written to
        Ref<EditBox> mNameEdit;   // File name prefix of the layer assets
        Ref<Label> mSaveInfo;     // Target pattern or the outcome of the last save

        float GetPanelWidth() const
        {
            return Math::Clamp(GetNumber("layersPanelW", panelDefault), panelMin, panelMax);
        }

        Ref<PipelineWrapRow> MakeToolRow()
        {
            auto row = mmake<PipelineWrapRow>();
            row->spacing = 4;
            return row;
        }

        // Adds a tool row whose height follows the lines it wraps into
        void AddToolRow(const Ref<PipelineWrapRow>& row)
        {
            AddRow(row, [row](float width) { return row->GetHeightForWidth(width); });
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

        void BuildToolbar()
        {
            WeakRef<ComposerBody> weakThis(this);

            auto row1 = MakeToolRow();
            auto wLabel = MakeLabel("W", false); wLabel->layout->minWidth = 16; wLabel->layout->maxWidth = 16; wLabel->horOverflow = Label::HorOverflow::None;
            row1->AddChild(wLabel);
            row1->AddChild(MakeNumberEdit(GetNumber("canvasW", 1024), 54, [weakThis](float v)
            {
                if (auto self = weakThis.Lock()) self->SetNumber("canvasW", Math::Clamp(Math::Round(v), 16.0f, 8192.0f), true);
            }));
            auto hLabel = MakeLabel("H", false); hLabel->layout->minWidth = 16; hLabel->layout->maxWidth = 16; hLabel->horOverflow = Label::HorOverflow::None;
            row1->AddChild(hLabel);
            row1->AddChild(MakeNumberEdit(GetNumber("canvasH", 1024), 54, [weakThis](float v)
            {
                if (auto self = weakThis.Lock()) self->SetNumber("canvasH", Math::Clamp(Math::Round(v), 16.0f, 8192.0f), true);
            }));

            auto zoomOut = MakeButton("-");
            zoomOut->layout->minWidth = 22; zoomOut->layout->maxWidth = 22;
            zoomOut->onClick = [weakThis]() { if (auto self = weakThis.Lock()) self->SetZoom(self->GetNumber("viewZoom", 1) / 1.25f); };
            row1->AddChild(zoomOut);
            mZoomButton = MakeButton("100%");
            mZoomButton->layout->minWidth = 48; mZoomButton->layout->maxWidth = 48;
            mZoomButton->onClick = [weakThis]() { if (auto self = weakThis.Lock()) self->SetZoom(1.0f); };
            row1->AddChild(mZoomButton);
            auto zoomIn = MakeButton("+");
            zoomIn->layout->minWidth = 22; zoomIn->layout->maxWidth = 22;
            zoomIn->onClick = [weakThis]() { if (auto self = weakThis.Lock()) self->SetZoom(self->GetNumber("viewZoom", 1) * 1.25f); };
            row1->AddChild(zoomIn);

            auto makeFlip = [&](bool vertical)
            {
                auto toggle = MakeSegment("", false);
                toggle->layout->minWidth = 26; toggle->layout->maxWidth = 26;
                auto icon = mmake<Sprite>(vertical ? "ui/pipeline/btn_flip_v.png" : "ui/pipeline/btn_flip_h.png");
                icon->color = PipelineControls::textColor;
                toggle->AddLayer("icon", icon, Layout::Based(BaseCorner::Center, Vec2F(14, 14)));
                toggle->onToggleByUser = [weakThis, vertical](bool) { if (auto self = weakThis.Lock()) self->FlipSelected(vertical); };
                row1->AddChild(toggle);
                return toggle;
            };
            mFlipH = makeFlip(false);
            mFlipV = makeFlip(true);
            AddToolRow(row1);

            auto row2 = MakeToolRow();
            auto bg = MakeCheckbox("BG", GetBool("cmpBgEnabled", false));
            bg->layout->minWidth = 54; bg->layout->maxWidth = 54;
            bg->onToggleByUser = [weakThis](bool v) { if (auto self = weakThis.Lock()) { self->SetBool("cmpBgEnabled", v, true); self->RebuildBody(); } };
            row2->AddChild(bg);
            if (GetBool("cmpBgEnabled", false))
                row2->AddChild(MakeColorSwatch("cmpBg", "#3a6ea5"));

            auto outBg = MakeCheckbox("Out BG", GetBool("outBgEnabled", false));
            outBg->layout->minWidth = 80; outBg->layout->maxWidth = 80;
            outBg->onToggleByUser = [weakThis](bool v) { if (auto self = weakThis.Lock()) { self->SetBool("outBgEnabled", v, true); self->RebuildBody(); } };
            row2->AddChild(outBg);
            if (GetBool("outBgEnabled", false))
                row2->AddChild(MakeColorSwatch("outBg", "#ffffff"));

            AddToolRow(row2);
        }

        Ref<Text> MakeCaptionText(const String& caption)
        {
            auto text = mmake<Text>("stdFont.ttf");
            text->text = caption;
            text->horAlign = HorAlign::Left;
            text->verAlign = VerAlign::Middle;
            text->color = PipelineControls::textColor;
            return text;
        }

        Ref<Widget> MakeColorSwatch(const String& key, const String& def)
        {
            Color4 color;
            if (!PipelineUtils::ParseHexColor(GetString(key, def), color))
                PipelineUtils::ParseHexColor(def, color);
            auto swatch = MakeButton("");
            swatch->layout->minWidth = 26; swatch->layout->maxWidth = 26;
            if (auto regular = swatch->GetLayerDrawable<Sprite>("regular")) regular->color = Color4(color.r, color.g, color.b, 255);
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
            mFlipH->enabled = has;
            mFlipV->enabled = has;
            mFlipH->SetValue(has && p.flipH);
            mFlipV->SetValue(has && p.flipV);
        }

        void BuildMain()
        {
            mMain = mmake<Widget>();
            mMain->name = "composer main";
            AddFlexible(mMain, 300);

            mMain->AddChild(mStage);

            mPanel = o2UI.CreateScrollArea();
            mPanel->name = "layers panel";
            mPanel->SetEnableScrollsHiding(true);
            mMain->AddChild(mPanel);

            mList = mmake<VerticalLayout>();
            mList->spacing = 2;
            mList->expandWidth = true;
            mList->expandHeight = false;
            mList->fitByChildren = true;
            mList->baseCorner = BaseCorner::Top;
            *mList->layout = WidgetLayout::HorStretch(VerAlign::Top, 0, 0, 20, 0);
            mPanel->AddChild(mList);

            auto bar = mmake<Sprite>("ui/UI4_Ver_separator.png");
            auto barHover = mmake<Sprite>("ui/UI4_Ver_separator.png");
            barHover->color = PipelineControls::accentColor;
            mSplitter = mmake<DragHandle>(bar, barHover, barHover);
            mSplitter->SetDrawablesSize(Vec2F(8, 40));
            mSplitter->cursorType = CursorType::SizeWE;
            WeakRef<ComposerBody> weakThis(this);
            mSplitter->onChangedPos = [weakThis](const Vec2F& pos)
            {
                if (auto self = weakThis.Lock())
                {
                    RectF rect = self->mMain->layout->GetWorldRect();
                    float width = Math::Clamp(rect.right - pos.x - 4.0f, panelMin, panelMax);
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
                    float x = rect.right - self->GetPanelWidth() - 4.0f;
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
            float panelW = GetPanelWidth();
            *mStage->layout = WidgetLayout(Vec2F(0, 0), Vec2F(1, 1), Vec2F(0, 0), Vec2F(-(panelW + 8), 0));
            *mPanel->layout = WidgetLayout(Vec2F(1, 0), Vec2F(1, 1), Vec2F(-panelW, 0), Vec2F(0, 0));
        }

        Ref<Button> MakeRowAction(const String& icon, float angle, bool enabled, const Function<void()>& action)
        {
            auto button = MakeIconButton(icon, PipelineControls::textColor, Color4(0, 0, 0, 0));
            button->layout->minWidth = 18; button->layout->maxWidth = 18;
            if (auto ic = button->GetLayerDrawable<Sprite>("icon"))
            {
                ic->angleDegree = angle;
                if (auto layer = button->GetLayer("icon")) layer->layout = Layout::Based(BaseCorner::Center, Vec2F(13, 13));
            }
            button->interactable = enabled;
            button->transparency = enabled ? 1.0f : 0.35f;
            button->onClick = action;
            return button;
        }

        void RebuildLayersPanel()
        {
            PushEditorScopeOnStack scope;
            if (!mList || !mStage)
                return;

            mList->RemoveAllChildren();

            auto layers = mStage->GetLayers();
            String selected = mStage->GetSelectedLayer();
            String open = GetString("openLayerSettings", "");
            WeakRef<ComposerBody> weakThis(this);

            auto head = MakeLabel("Layers - " + (String)layers.Count(), true);
            head->layout->minHeight = 18;
            mList->AddChild(head);

            if (layers.IsEmpty())
            {
                auto empty = MakeLabel("Add image inputs with + and connect sprites", true);
                empty->layout->minHeight = 34;
                mList->AddChild(empty);
            }

            for (int i = layers.Count() - 1; i >= 0; i--)
            {
                auto layer = layers[i];
                auto p = mStage->GetPlacement(layer);
                auto image = mStage->GetLayerImage(layer.portId);
                String id = layer.id;
                String portId = layer.portId;
                bool isSelected = id == selected;

                auto container = mmake<Widget>();
                container->layout->minHeight = 24;
                if (isSelected)
                    container->AddLayer("select", mmake<Sprite>(PipelineControls::accentColor), Layout::BothStretch(0, 0, 0, 0))->transparency = 0.16f;

                auto selectButton = o2UI.CreateWidget<Button>("pipeline icon");
                if (auto icon = selectButton->GetLayer("icon")) selectButton->RemoveLayer(icon);
                *selectButton->layout = WidgetLayout::BothStretch(0, 0, 0, 0);
                selectButton->onClick = [weakThis, id]() { if (auto self = weakThis.Lock()) self->mStage->SelectLayer(id); };
                container->AddChild(selectButton);

                auto row = mmake<HorizontalLayout>();
                row->spacing = 2;
                row->expandWidth = true;
                row->expandHeight = true;
                row->baseCorner = BaseCorner::Left;
                *row->layout = WidgetLayout::BothStretch(2, 1, 2, 1);
                container->AddChild(row);

                auto expand = o2UI.CreateWidget<Button>("expand");
                expand->layout->minWidth = 16; expand->layout->maxWidth = 16;
                expand->SetStateForcible("expanded", open == id);
                expand->onClick = [weakThis, id]()
                {
                    if (auto self = weakThis.Lock())
                    {
                        bool wasOpen = self->GetString("openLayerSettings", "") == id;
                        self->mNode->SetConfigString("openLayerSettings", wasOpen ? "" : id);
                        self->Notify("openLayerSettings", true);
                        self->mStage->SelectLayer(id);
                        self->RebuildLayersPanel();
                    }
                };
                row->AddChild(expand);

                row->AddChild(MakeRowAction(p.hidden ? "ui/UI4_eye_closed_icon.png" : "ui/UI4_eye_opened_icon.png", 0, true, [weakThis, id, layer]()
                {
                    if (auto self = weakThis.Lock())
                    {
                        auto np = self->mStage->GetPlacement(layer);
                        np.hidden = !np.hidden;
                        self->mStage->WritePlacement(id, np, true);
                        self->RebuildLayersPanel();
                    }
                }));

                auto thumb = mmake<PipelineImageView>();
                thumb->layout->minSize = Vec2F(22, 22);
                thumb->layout->maxWidth = 22;
                thumb->SetHint("");
                thumb->SetBitmap(image);
                row->AddChild(thumb);

                auto name = MakeEditBox(layer.name, false);
                name->layout->minWidth = 40;
                bool dup = layer.dup;
                name->onChangeCompleted = [weakThis, id, portId, dup](const WString& text)
                {
                    if (auto self = weakThis.Lock()) self->RenameLayer(id, portId, dup, (String)text);
                };
                row->AddChild(name);

                if (layer.dup)
                {
                    auto tag = MakeLabel("copy", true);
                    tag->layout->minWidth = 30; tag->layout->maxWidth = 30;
                    row->AddChild(tag);
                }

                row->AddChild(MakeRowAction("ui/pipeline/btn_copy.png", 0, true, [weakThis, id]() { if (auto self = weakThis.Lock()) self->DuplicateLayer(id); }));
                row->AddChild(MakeRowAction("ui/UI4_Down_icn.png", 180, i < layers.Count() - 1, [weakThis, i]() { if (auto self = weakThis.Lock()) self->ReorderLayer(i, 1); }));
                row->AddChild(MakeRowAction("ui/UI4_Down_icn.png", 0, i > 0, [weakThis, i]() { if (auto self = weakThis.Lock()) self->ReorderLayer(i, -1); }));
                row->AddChild(MakeRowAction("ui/UI4_revert.png", 0, true, [weakThis, id, layer]()
                {
                    if (auto self = weakThis.Lock())
                    {
                        auto np = self->mStage->DefaultPlacement(layer.portId);
                        np.hidden = self->mStage->GetPlacement(layer).hidden;
                        self->mStage->WritePlacement(id, np, true);
                    }
                }));
                row->AddChild(MakeRowAction("ui/UI4_small_trash_icon.png", 0, true, [weakThis, id, portId, dup]() { if (auto self = weakThis.Lock()) self->RemoveLayer(id, portId, dup); }));

                mList->AddChild(container);

                if (open == id)
                    BuildLayerSettings(layer, p, image);
            }
        }

        void BuildLayerSettings(const ComposerLayerRef& layer, const ComposerLayerPlacement& p, const Ref<Bitmap>& image)
        {
            WeakRef<ComposerBody> weakThis(this);
            String id = layer.id;
            Vec2I natural = image ? image->GetSize() : Vec2I();

            auto patch = [weakThis, layer](const Function<void(ComposerLayerPlacement&)>& change, bool completed)
            {
                if (auto self = weakThis.Lock())
                {
                    auto np = self->mStage->GetPlacement(layer);
                    change(np);
                    self->mStage->WritePlacement(layer.id, np, completed);
                }
            };

            auto sizeRow = mmake<HorizontalLayout>();
            sizeRow->spacing = 3; sizeRow->expandWidth = false; sizeRow->expandHeight = true; sizeRow->baseCorner = BaseCorner::Left;
            sizeRow->layout->minHeight = 22;
            auto sizeLabel = MakeLabel("Size", true); sizeLabel->layout->minWidth = 34; sizeLabel->layout->maxWidth = 34;
            sizeRow->AddChild(sizeLabel);
            bool lock = p.lockAspect;
            sizeRow->AddChild(MakeNumberEdit(Math::Round(p.w), 48, [patch, lock](float v)
            {
                patch([v, lock](ComposerLayerPlacement& np)
                {
                    float w = Math::Max(1.0f, Math::Round(v));
                    float h = lock && np.w > 0 ? Math::Round(w * np.h / np.w) : np.h;
                    np.w = w; np.h = Math::Max(1.0f, h);
                }, true);
            }));
            auto lockToggle = MakeSegment("", p.lockAspect);
            lockToggle->layout->minWidth = 22; lockToggle->layout->maxWidth = 22;
            auto lockIcon = mmake<Sprite>("ui/pipeline/btn_link.png");
            lockIcon->color = PipelineControls::textColor;
            lockToggle->AddLayer("icon", lockIcon, Layout::Based(BaseCorner::Center, Vec2F(14, 14)));
            lockToggle->onToggleByUser = [patch, weakThis](bool v) { patch([v](ComposerLayerPlacement& np) { np.lockAspect = v; }, true); if (auto self = weakThis.Lock()) self->RebuildLayersPanel(); };
            sizeRow->AddChild(lockToggle);
            sizeRow->AddChild(MakeNumberEdit(Math::Round(p.h), 48, [patch, lock](float v)
            {
                patch([v, lock](ComposerLayerPlacement& np)
                {
                    float h = Math::Max(1.0f, Math::Round(v));
                    float w = lock && np.h > 0 ? Math::Round(h * np.w / np.h) : np.w;
                    np.h = h; np.w = Math::Max(1.0f, w);
                }, true);
            }));
            if (natural.x > 0)
            {
                auto one = MakeButton("1:1");
                one->layout->minWidth = 34; one->layout->maxWidth = 34;
                one->onClick = [patch, natural, weakThis]() { patch([natural](ComposerLayerPlacement& np) { np.w = (float)natural.x; np.h = (float)natural.y; }, true); if (auto self = weakThis.Lock()) self->RebuildLayersPanel(); };
                sizeRow->AddChild(one);
            }
            mList->AddChild(sizeRow);

            auto opacity = mmake<PipelineSlider>();
            opacity->layout->minHeight = 20;
            opacity->Setup("Alpha", 0, 100, 1, Math::Round(p.opacity * 100.0f), "%");
            opacity->onChanged = [patch](float v, bool completed) { patch([v](ComposerLayerPlacement& np) { np.opacity = v / 100.0f; }, completed); };
            mList->AddChild(opacity);

            auto nineRow = mmake<HorizontalLayout>();
            nineRow->spacing = 4; nineRow->expandWidth = false; nineRow->expandHeight = true; nineRow->baseCorner = BaseCorner::Left;
            nineRow->layout->minHeight = 20;
            auto nine = MakeCheckbox("9-slice", p.nine);
            nine->layout->minWidth = 80; nine->layout->maxWidth = 80;
            nine->onToggleByUser = [patch, weakThis](bool v) { patch([v](ComposerLayerPlacement& np) { np.nine = v; }, true); if (auto self = weakThis.Lock()) self->RebuildLayersPanel(); };
            nineRow->AddChild(nine);
            if (p.nine && natural.x > 0)
            {
                auto autoBtn = MakeButton("auto");
                autoBtn->layout->minWidth = 44; autoBtn->layout->maxWidth = 44;
                autoBtn->onClick = [patch, natural, weakThis]()
                {
                    patch([natural](ComposerLayerPlacement& np)
                    {
                        np.slice = { natural.x / 4, natural.y / 4, natural.x / 4, natural.y / 4 };
                    }, true);
                    if (auto self = weakThis.Lock()) self->RebuildLayersPanel();
                };
                nineRow->AddChild(autoBtn);
            }
            mList->AddChild(nineRow);

            if (p.nine)
            {
                auto sliceRow = mmake<HorizontalLayout>();
                sliceRow->spacing = 2; sliceRow->expandWidth = false; sliceRow->expandHeight = true; sliceRow->baseCorner = BaseCorner::Left;
                sliceRow->layout->minHeight = 22;
                struct Field { const char* label; int value; int which; };
                Field fields[] = { { "L", p.slice.l, 0 }, { "T", p.slice.t, 1 }, { "R", p.slice.r, 2 }, { "B", p.slice.b, 3 } };
                for (auto& f : fields)
                {
                    auto label = MakeLabel(f.label, true); label->layout->minWidth = 12; label->layout->maxWidth = 12;
                    sliceRow->AddChild(label);
                    int which = f.which;
                    sliceRow->AddChild(MakeNumberEdit((float)f.value, 40, [patch, which](float v)
                    {
                        patch([v, which](ComposerLayerPlacement& np)
                        {
                            int value = Math::Max(0, (int)Math::Round(v));
                            if (which == 0) np.slice.l = value; else if (which == 1) np.slice.t = value; else if (which == 2) np.slice.r = value; else np.slice.b = value;
                        }, true);
                    }));
                }
                mList->AddChild(sliceRow);

                auto corners = mmake<PipelineSlider>();
                corners->layout->minHeight = 20;
                corners->Setup("Corners", 10, 300, 5, Math::Round(p.sliceScale * 100.0f), "%");
                corners->onChanged = [patch](float v, bool completed) { patch([v](ComposerLayerPlacement& np) { np.sliceScale = v / 100.0f; }, completed); };
                mList->AddChild(corners);

                String hint = "Insets in source px";
                if (natural.x > 0) hint += " - source " + (String)natural.x + "x" + (String)natural.y;
                auto hintLabel = MakeLabel(hint, true);
                hintLabel->layout->minHeight = 18;
                mList->AddChild(hintLabel);
            }
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
            RebuildLayersPanel();
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

        // Writes every layer as its own PNG asset, the way a finish node writes its result
        void SaveLayersToAssets()
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
        }
    };

    Ref<PipelineNodeBody> CreateComposerNodeBody(const String& type)
    {
        if (type == "composer")
            return mmake<ComposerBody>();

        return nullptr;
    }
}
