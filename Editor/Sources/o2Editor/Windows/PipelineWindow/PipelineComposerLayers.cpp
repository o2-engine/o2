#include "o2Editor/stdafx.h"
#include "PipelineComposerLayers.h"

#include "o2/Events/CursorAreaEventsListener.h"
#include "o2/Render/Render.h"
#include "o2/Render/Sprite.h"
#include "o2/Render/Text.h"
#include "o2/Scene/UI/UIManager.h"
#include "o2/Scene/UI/WidgetLayer.h"
#include "o2/Scene/UI/WidgetLayout.h"
#include "o2/Scene/UI/Widgets/Button.h"
#include "o2/Scene/UI/Widgets/EditBox.h"
#include "o2/Scene/UI/Widgets/HorizontalLayout.h"
#include "o2/Scene/UI/Widgets/Label.h"
#include "o2/Scene/UI/Widgets/Toggle.h"
#include "o2/Utils/Bitmap/Bitmap.h"
#include "o2/Utils/Editor/EditorScope.h"
#include "o2Editor/Windows/PipelineWindow/PipelineControls.h"
#include "o2/Render/VectorSprite.h"

namespace Editor
{
    using namespace PipelineControls;

    // ------------------------------------------------------------------------
    // Six dots at the right end of a layer row: pressing and dragging moves the
    // row to another place in the list
    // ------------------------------------------------------------------------
    class PipelineComposerGrip : public Widget, public CursorAreaEventsListener
    {
    public:
        Function<void()>             onBegin; // The grip was pressed
        Function<void(const Vec2F&)> onDrag;  // The cursor moved while pressed, canvas space
        Function<void(bool)>         onEnd;   // Released (true) or the press was broken (false)

    public:
        explicit PipelineComposerGrip(RefCounter* refCounter): Widget(refCounter) {}

        void Draw() override
        {
            if (!mResEnabledInHierarchy || mIsClipped)
                return;

            Widget::Draw();
            Vec2F c = layout->GetWorldRect().Center();
            Color4 color = IsPressed() ? accentColor : dimTextColor;
            for (int col = 0; col < 2; col++)
            {
                for (int row = 0; row < 3; row++)
                    o2Render.DrawFilledCircle(c + Vec2F(col == 0 ? -2.5f : 2.5f, (row - 1)*5.0f), 1.4f, color, 10);
            }
            CursorAreaEventsListener::OnDrawn();
        }

        bool IsUnderPoint(const Vec2F& point) override { return layout->IsPointInside(point); }

    protected:
        void OnCursorPressed(const Input::Cursor& cursor) override { if (onBegin) onBegin(); }
        void OnCursorStillDown(const Input::Cursor& cursor) override { if (onDrag) onDrag(cursor.position); }
        void OnCursorReleased(const Input::Cursor& cursor) override { if (onEnd) onEnd(true); }
        void OnCursorPressBreak(const Input::Cursor& cursor) override { if (onEnd) onEnd(false); }

        REF_COUNTERABLE_IMPL(Widget);
    };

    PipelineComposerLayersPanel::PipelineComposerLayersPanel(RefCounter* refCounter):
        Widget(refCounter)
    {
        AddLayer("frame", mmake<VectorSprite>("ui/UI4_Editbox_regular.svg"), Layout::BothStretch(-9, -9, -9, -9), -1.0f);
    }

    void PipelineComposerLayersPanel::Setup(const Ref<PipelineNode>& node, const Ref<PipelineComposerStage>& stage)
    {
        mNode = node;
        mStage = stage;
    }

    Ref<Button> PipelineComposerLayersPanel::MakeAddRow(PipelineLayerPlace place, float top)
    {
        WeakRef<PipelineComposerLayersPanel> weakThis(this);
        bool front = place == PipelineLayerPlace::Front;
        auto row = MakeButton(front ? "+ layer on top" : "+ layer below");
        row->name = front ? "add layer on top" : "add layer below";
        *row->layout = WidgetLayout::HorStretch(VerAlign::Top, 4, 4, addRowHeight, top);
        row->onClick = [weakThis, place]() { if (auto self = weakThis.Lock()) if (self->onAddInput) self->onAddInput(place); };
        AddChild(row);
        return row;
    }

    bool PipelineComposerLayersPanel::GetPortCenter(const String& portId, float& y) const
    {
        int index = mDisplayPorts.IndexOf(portId);
        if (index < 0 || portId.IsEmpty())
            return false;

        y = mRowTops[index] + rowHeight*0.5f;
        return true;
    }

    Ref<Widget> PipelineComposerLayersPanel::FindRow(const String& layerId) const
    {
        int index = mDisplayIds.IndexOf(layerId);
        return index >= 0 ? mRows[index] : nullptr;
    }

    Ref<Widget> PipelineComposerLayersPanel::FindGrip(const String& layerId) const
    {
        int index = mDisplayIds.IndexOf(layerId);
        return index >= 0 ? mGrips[index] : nullptr;
    }

    void PipelineComposerLayersPanel::Rebuild()
    {
        PushEditorScopeOnStack scope;
        if (!mStage || !mNode)
            return;

        RemoveAllChildren();
        mDisplayIds.Clear();
        mDisplayPorts.Clear();
        mRowTops.Clear();
        mRows.Clear();
        mGrips.Clear();
        mDragging = "";

        auto layers = mStage->GetLayers();
        String selected = mStage->GetSelectedLayer();
        String open = mNode->GetConfigString("openLayerSettings", "");

        auto head = MakeLabel("Layers \xC2\xB7 " + (String)layers.Count(), true);
        head->name = "layers head";
        *head->layout = WidgetLayout::HorStretch(VerAlign::Top, 8, 8, headHeight, border);
        AddChild(head);

        int openRow = -1;
        for (int i = layers.Count() - 1; i >= 0; i--)
        {
            if (open == layers[i].id)
                openRow = layers.Count() - 1 - i;
        }
        bool openNine = openRow >= 0 && mStage->GetPlacement(layers[layers.Count() - 1 - openRow]).nine;
        auto rows = PipelineComposerLayout::Rows(layers.Count(), openRow, openNine);

        mAddTopRow = MakeAddRow(PipelineLayerPlace::Front, rows.addTopRow);
        if (layers.IsEmpty())
        {
            auto empty = MakeLabel("Add image inputs and connect sprites", true);
            empty->name = "empty";
            *empty->layout = WidgetLayout::HorStretch(VerAlign::Top, 8, 8, rowHeight, rows.emptyTop);
            AddChild(empty);
        }

        for (int i = layers.Count() - 1; i >= 0; i--)
        {
            auto& layer = layers[i];
            int display = layers.Count() - 1 - i;
            float y = rows.rowTops[display];
            mDisplayIds.Add(layer.id);
            mDisplayPorts.Add(layer.dup ? String() : layer.portId);
            mRowTops.Add(y);

            auto row = MakeRow(layer, i, layers.Count(), layer.id == selected, display == openRow);
            *row->layout = WidgetLayout::HorStretch(VerAlign::Top, 4, 4, rowHeight, y);
            AddChild(row);
            mRows.Add(row);

            if (display == openRow)
                AddSettings(layer, rows.settingsTop);
        }

        mAddTop = rows.addBottomRow;
        mAddRow = MakeAddRow(PipelineLayerPlace::Back, rows.addBottomRow);
        mContentHeight = rows.height;
    }

    Ref<Widget> PipelineComposerLayersPanel::MakeRow(const ComposerLayerRef& layer, int stackIndex, int count, bool selected, bool open)
    {
        WeakRef<PipelineComposerLayersPanel> weakThis(this);
        auto p = mStage->GetPlacement(layer);
        String id = layer.id, portId = layer.portId;
        bool dup = layer.dup;

        auto container = mmake<Widget>();
        container->name = "layer " + id;
        container->AddLayer("select", mmake<Sprite>(accentColor), Layout::BothStretch(0, 0, 0, 0))->transparency = selected ? selectedTint : 0.0f;

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
        *row->layout = WidgetLayout::BothStretch(2, 2, 2, 2);
        container->AddChild(row);

        auto action = [&](const String& actionName, const String& icon, float angle, bool enabled, const Function<void()>& onClick)
        {
            auto button = MakeIconButton(icon, textColor, Color4(0, 0, 0, 0));
            button->name = actionName;
            button->layout->minWidth = 18; button->layout->maxWidth = 18;
            if (auto ic = button->GetLayerDrawableBasedOn<IRectDrawable>("icon"))
            {
                ic->angleDegree = angle;
                if (auto iconLayer = button->GetLayer("icon")) iconLayer->layout = Layout::Based(BaseCorner::Center, Vec2F(13, 13));
            }
            button->interactable = enabled;
            button->transparency = enabled ? 1.0f : 0.35f;
            button->onClick = onClick;
            row->AddChild(button);
            return button;
        };

        auto expand = o2UI.CreateWidget<Button>("expand");
        expand->name = "settings";
        expand->layout->minWidth = 16; expand->layout->maxWidth = 16;
        expand->SetStateForcible("expanded", open);
        expand->onClick = [weakThis, id]()
        {
            auto self = weakThis.Lock();
            if (!self)
                return;

            bool wasOpen = self->mNode->GetConfigString("openLayerSettings", "") == id;
            self->mNode->SetConfigString("openLayerSettings", wasOpen ? String() : id);
            if (self->onConfigChanged) self->onConfigChanged("openLayerSettings", true);
            self->mStage->SelectLayer(id);
            self->Rebuild();
            if (self->onLayoutChanged) self->onLayoutChanged();
        };
        row->AddChild(expand);

        auto layerRef = layer;
        action("visibility", p.hidden ? "ui/UI4_eye_closed_icon.svg" : "ui/UI4_eye_opened_icon.svg", 0, true, [weakThis, layerRef]()
        {
            if (auto self = weakThis.Lock())
            {
                auto np = self->mStage->GetPlacement(layerRef);
                np.hidden = !np.hidden;
                self->mStage->WritePlacement(layerRef.id, np, true);
                self->Rebuild();
            }
        });

        auto thumb = mmake<PipelineImageView>();
        thumb->layout->minSize = Vec2F(22, 22);
        thumb->layout->maxWidth = 22;
        thumb->SetHint("");
        thumb->SetBitmap(mStage->GetLayerImage(layer.portId));
        row->AddChild(thumb);

        auto name = MakeEditBox(layer.name, false);
        name->name = "layer name";
        name->layout->minWidth = 40;
        name->onChangeCompleted = [weakThis, id, portId, dup](const WString& text)
        {
            if (auto self = weakThis.Lock()) if (self->onRename) self->onRename(id, portId, dup, (String)text);
        };
        // The field takes the press of the row: focusing it, by click or keyboard, selects the layer too
        name->onFocused = [weakThis, id]() { if (auto self = weakThis.Lock()) self->mStage->SelectLayer(id); };
        row->AddChild(name);

        if (dup)
        {
            auto tag = MakeLabel("copy", true);
            tag->layout->minWidth = 30; tag->layout->maxWidth = 30;
            row->AddChild(tag);
        }

        action("duplicate", "ui/pipeline/btn_copy.svg", 0, true, [weakThis, id]() { if (auto self = weakThis.Lock()) if (self->onDuplicate) self->onDuplicate(id); });
        action("forward", "ui/UI4_Down_icn.svg", 180, stackIndex < count - 1, [weakThis, stackIndex]() { if (auto self = weakThis.Lock()) if (self->onMove) self->onMove(stackIndex, 1); });
        action("backward", "ui/UI4_Down_icn.svg", 0, stackIndex > 0, [weakThis, stackIndex]() { if (auto self = weakThis.Lock()) if (self->onMove) self->onMove(stackIndex, -1); });
        action("reset", "ui/UI4_revert.svg", 0, true, [weakThis, layerRef]()
        {
            if (auto self = weakThis.Lock())
            {
                auto np = self->mStage->DefaultPlacement(layerRef.portId);
                np.hidden = self->mStage->GetPlacement(layerRef).hidden;
                self->mStage->WritePlacement(layerRef.id, np, true);
            }
        });
        action("remove", "ui/UI4_small_trash_icon.svg", 0, true, [weakThis, id, portId, dup]()
        {
            if (auto self = weakThis.Lock()) if (self->onRemove) self->onRemove(id, portId, dup);
        });

        auto grip = mmake<PipelineComposerGrip>();
        grip->name = "grip";
        grip->layout->minWidth = 14; grip->layout->maxWidth = 14;
        grip->onBegin = [weakThis, id]() { if (auto self = weakThis.Lock()) self->BeginRowDrag(id); };
        grip->onDrag = [weakThis](const Vec2F& point) { if (auto self = weakThis.Lock()) self->DragRowTo(point); };
        grip->onEnd = [weakThis](bool commit) { if (auto self = weakThis.Lock()) self->EndRowDrag(commit); };
        row->AddChild(grip);
        mGrips.Add(grip);

        return container;
    }

    void PipelineComposerLayersPanel::AddSettings(const ComposerLayerRef& layer, float top)
    {
        WeakRef<PipelineComposerLayersPanel> weakThis(this);
        auto p = mStage->GetPlacement(layer);
        auto image = mStage->GetLayerImage(layer.portId);
        Vec2I natural = image ? image->GetSize() : Vec2I();

        auto settings = mmake<Widget>();
        settings->name = "layer settings";
        *settings->layout = WidgetLayout::HorStretch(VerAlign::Top, 22, 6, PipelineComposerLayout::SettingsHeight(p.nine), top);
        AddChild(settings);

        // The export line stands between the size and the rest
        const float line = PipelineComposerLayout::settingsLine;
        auto place = [&](const Ref<Widget>& widget, float y, float height)
        {
            *widget->layout = WidgetLayout::HorStretch(VerAlign::Top, 0, 0, height, y);
            settings->AddChild(widget);
        };

        auto patch = [weakThis, layer](const Function<void(ComposerLayerPlacement&)>& change, bool completed)
        {
            if (auto self = weakThis.Lock())
            {
                auto np = self->mStage->GetPlacement(layer);
                change(np);
                self->mStage->WritePlacement(layer.id, np, completed);
            }
        };
        auto rebuild = [weakThis](bool layoutChanged)
        {
            if (auto self = weakThis.Lock())
            {
                self->Rebuild();
                if (layoutChanged && self->onLayoutChanged) self->onLayoutChanged();
            }
        };
        auto numberEdit = [](float value, float width, const Function<void(float)>& onChange)
        {
            auto edit = MakeEditBox(FormatNumber(value, 1), false);
            edit->SetFilterInteger();
            edit->layout->minWidth = width;
            edit->layout->maxWidth = width;
            edit->onChangeCompleted = [onChange](const WString& text) { onChange((float)atof(((String)text).Data())); };
            return edit;
        };
        auto makeLine = []()
        {
            auto line = mmake<HorizontalLayout>();
            line->spacing = 3; line->expandWidth = false; line->expandHeight = true; line->baseCorner = BaseCorner::Left;
            return line;
        };

        auto sizeRow = makeLine();
        auto sizeLabel = MakeLabel("Size", true); sizeLabel->layout->minWidth = 34; sizeLabel->layout->maxWidth = 34;
        sizeRow->AddChild(sizeLabel);
        bool lock = p.lockAspect;
        sizeRow->AddChild(numberEdit(Math::Round(p.w), 48, [patch, lock](float v)
        {
            patch([v, lock](ComposerLayerPlacement& np)
            {
                float w = Math::Max(1.0f, Math::Round(v));
                float h = lock && np.w > 0 ? Math::Round(w*np.h/np.w) : np.h;
                np.w = w; np.h = Math::Max(1.0f, h);
            }, true);
        }));
        auto lockToggle = MakeSegment("", p.lockAspect);
        lockToggle->layout->minWidth = 22; lockToggle->layout->maxWidth = 22;
        auto lockIcon = mmake<VectorSprite>("ui/pipeline/btn_link.svg");
        lockIcon->color = textColor;
        lockToggle->AddLayer("icon", lockIcon, Layout::Based(BaseCorner::Center, Vec2F(14, 14)));
        lockToggle->onToggleByUser = [patch, rebuild](bool v) { patch([v](ComposerLayerPlacement& np) { np.lockAspect = v; }, true); rebuild(false); };
        sizeRow->AddChild(lockToggle);
        sizeRow->AddChild(numberEdit(Math::Round(p.h), 48, [patch, lock](float v)
        {
            patch([v, lock](ComposerLayerPlacement& np)
            {
                float h = Math::Max(1.0f, Math::Round(v));
                float w = lock && np.h > 0 ? Math::Round(h*np.w/np.h) : np.w;
                np.h = h; np.w = Math::Max(1.0f, w);
            }, true);
        }));
        if (natural.x > 0)
        {
            auto one = MakeButton("1:1");
            one->layout->minWidth = 34; one->layout->maxWidth = 34;
            one->onClick = [patch, natural, rebuild]() { patch([natural](ComposerLayerPlacement& np) { np.w = (float)natural.x; np.h = (float)natural.y; }, true); rebuild(false); };
            sizeRow->AddChild(one);
        }
        place(sizeRow, 4, 22);
        AddExportRow(settings, layer, p);

        auto opacity = mmake<PipelineSlider>();
        opacity->Setup("Alpha", 0, 100, 1, Math::Round(p.opacity*100.0f), "%");
        opacity->onChanged = [patch](float v, bool completed) { patch([v](ComposerLayerPlacement& np) { np.opacity = v/100.0f; }, completed); };
        place(opacity, 29 + line, 20);

        auto nineRow = makeLine();
        auto nine = MakeCheckbox("9-slice", p.nine);
        nine->name = "nine slice";
        nine->layout->minWidth = 80; nine->layout->maxWidth = 80;
        nine->onToggleByUser = [patch, rebuild](bool v) { patch([v](ComposerLayerPlacement& np) { np.nine = v; }, true); rebuild(true); };
        nineRow->AddChild(nine);
        if (p.nine && natural.x > 0)
        {
            auto autoButton = MakeButton("auto");
            autoButton->layout->minWidth = 44; autoButton->layout->maxWidth = 44;
            autoButton->onClick = [patch, natural, rebuild]()
            {
                patch([natural](ComposerLayerPlacement& np) { np.slice = { natural.x/4, natural.y/4, natural.x/4, natural.y/4 }; }, true);
                rebuild(false);
            };
            nineRow->AddChild(autoButton);
        }
        place(nineRow, 52 + line, 20);

        if (!p.nine)
            return;

        auto sliceRow = makeLine();
        sliceRow->spacing = 2;
        struct Field { const char* label; int value; int which; };
        Field fields[] = { { "L", p.slice.l, 0 }, { "T", p.slice.t, 1 }, { "R", p.slice.r, 2 }, { "B", p.slice.b, 3 } };
        for (auto& f : fields)
        {
            auto label = MakeLabel(f.label, true); label->layout->minWidth = 12; label->layout->maxWidth = 12;
            sliceRow->AddChild(label);
            int which = f.which;
            sliceRow->AddChild(numberEdit((float)f.value, 40, [patch, which](float v)
            {
                patch([v, which](ComposerLayerPlacement& np)
                {
                    int value = Math::Max(0, (int)Math::Round(v));
                    if (which == 0) np.slice.l = value; else if (which == 1) np.slice.t = value; else if (which == 2) np.slice.r = value; else np.slice.b = value;
                }, true);
            }));
        }
        place(sliceRow, 75 + line, 22);

        auto corners = mmake<PipelineSlider>();
        corners->Setup("Corners", 10, 300, 5, Math::Round(p.sliceScale*100.0f), "%");
        corners->onChanged = [patch](float v, bool completed) { patch([v](ComposerLayerPlacement& np) { np.sliceScale = v/100.0f; }, completed); };
        place(corners, 100 + line, 20);

        String hint = "Insets in source px";
        if (natural.x > 0)
            hint += " - source " + (String)natural.x + "x" + (String)natural.y;
        place(MakeLabel(hint, true), 123 + line, 18);
    }

    void PipelineComposerLayersPanel::AddExportRow(const Ref<Widget>& settings, const ComposerLayerRef& layer, const ComposerLayerPlacement& p)
    {
        WeakRef<PipelineComposerLayersPanel> weakThis(this);
        bool exportOn = PipelineComposerLayout::HasExportSize(p.exportW, p.exportH);

        auto row = mmake<HorizontalLayout>();
        row->name = "export size";
        row->spacing = 3; row->expandWidth = false; row->expandHeight = true; row->baseCorner = BaseCorner::Left;
        *row->layout = WidgetLayout::HorStretch(VerAlign::Top, 0, 0, 22, 4 + PipelineComposerLayout::settingsLine);
        settings->AddChild(row);

        auto label = MakeLabel("Export", true);
        label->layout->minWidth = 34; label->layout->maxWidth = 34;
        row->AddChild(label);

        // An empty field shows the layer's own size, dimmed: that is what the file gets
        auto sideEdit = [&](bool horizontal)
        {
            float value = horizontal ? p.exportW : p.exportH;
            auto edit = MakeEditBox(exportOn ? FormatNumber(Math::Round(value), 1) : String(), false);
            edit->name = horizontal ? "export width" : "export height";
            edit->SetFilterInteger();
            edit->layout->minWidth = 48; edit->layout->maxWidth = 48;

            auto hint = mmake<Text>("stdFont.ttf");
            hint->text = FormatNumber(Math::Round(horizontal ? p.w : p.h), 1);
            hint->color = dimTextColor;
            hint->horAlign = HorAlign::Left;
            hint->verAlign = VerAlign::Middle;
            hint->height = 11;
            auto hintLayer = edit->AddLayer("placeholder", hint, Layout::BothStretch(6, 0, 6, 0), 1.0f);
            hintLayer->transparency = exportOn ? 0.0f : 1.0f;
            WeakRef<WidgetLayer> weakHint(hintLayer);
            edit->onChanged = [weakHint](const WString& text) { if (auto h = weakHint.Lock()) h->transparency = text.IsEmpty() ? 1.0f : 0.0f; };

            edit->onChangeCompleted = [weakThis, layer, horizontal](const WString& text)
            {
                auto self = weakThis.Lock();
                if (!self)
                    return;

                auto np = self->mStage->GetPlacement(layer);
                float exportW = np.exportW, exportH = np.exportH;
                PipelineComposerLayout::TypeExportSide(np.w, np.h, np.lockAspect, horizontal, Math::Round((float)atof(((String)text).Data())),
                                                       np.exportW, np.exportH);
                if (np.exportW == exportW && np.exportH == exportH)
                    return;

                self->mStage->WritePlacement(layer.id, np, true);
                self->Rebuild();
            };
            return edit;
        };

        row->AddChild(sideEdit(true));
        auto cross = MakeLabel("x", true);
        cross->horAlign = HorAlign::Middle;
        cross->layout->minWidth = 22; cross->layout->maxWidth = 22;
        row->AddChild(cross);
        row->AddChild(sideEdit(false));

        if (!exportOn)
            return;

        auto autoButton = MakeButton("auto");
        autoButton->name = "export auto";
        autoButton->layout->minWidth = 34; autoButton->layout->maxWidth = 34;
        autoButton->onClick = [weakThis, layer]()
        {
            if (auto self = weakThis.Lock())
            {
                auto np = self->mStage->GetPlacement(layer);
                np.exportW = np.exportH = 0.0f;
                self->mStage->WritePlacement(layer.id, np, true);
                self->Rebuild();
            }
        };
        row->AddChild(autoButton);
    }

    void PipelineComposerLayersPanel::UpdateSelection()
    {
        String selected = mStage ? mStage->GetSelectedLayer() : String();
        for (int i = 0; i < mRows.Count(); i++)
        {
            if (auto layer = mRows[i]->FindLayer("select"))
                layer->transparency = mDisplayIds[i] == selected ? selectedTint : 0.0f;
        }
    }

    void PipelineComposerLayersPanel::BeginRowDrag(const String& layerId)
    {
        int index = mDisplayIds.IndexOf(layerId);
        if (index < 0)
            return;

        mDragging = layerId;
        mDropIndex = index;
        mRows[index]->SetTransparency(0.5f);
    }

    void PipelineComposerLayersPanel::DragRowTo(const Vec2F& point)
    {
        if (mDragging.IsEmpty())
            return;

        // The row's own middle decides, not its open settings
        int before = 0;
        for (auto& row : mRows)
        {
            if (row->layout->GetWorldRect().Center().y > point.y)
                before++;
        }
        mDropIndex = before;
    }

    void PipelineComposerLayersPanel::EndRowDrag(bool commit)
    {
        if (mDragging.IsEmpty())
            return;

        int from = mDisplayIds.IndexOf(mDragging);
        int to = mDropIndex;
        if (from >= 0 && from < mRows.Count())
            mRows[from]->SetTransparency(1.0f);
        mDragging = "";

        if (!commit || from < 0 || to == from || to == from + 1)
            return;

        auto ids = mDisplayIds;
        String moved = ids[from];
        ids.RemoveAt(from);
        ids.Insert(moved, to > from ? to - 1 : to);

        Vector<String> stack;
        for (int i = ids.Count() - 1; i >= 0; i--)
            stack.Add(ids[i]);

        if (onReorder)
            onReorder(stack);
    }

    void PipelineComposerLayersPanel::Draw()
    {
        if (!mResEnabledInHierarchy || mIsClipped)
            return;

        Widget::Draw();
        if (mDragging.IsEmpty() || PipelineControls::IsFarView())
            return;

        // The landing place: above the row the dragged one would go before, or above "+ layer below" for the end
        RectF panel = layout->GetWorldRect();
        float y = mDropIndex < mRows.Count() ? mRows[mDropIndex]->layout->GetWorldRect().top : mAddRow->layout->GetWorldRect().top;
        y += rowGap*0.5f;
        o2Render.DrawFilledPolygon({ Vec2F(panel.left + 4, y - 1), Vec2F(panel.left + 4, y + 1), Vec2F(panel.right - 4, y + 1),
                                     Vec2F(panel.right - 4, y - 1) }, accentColor);
    }
}
// --- META ---

DECLARE_CLASS(Editor::PipelineComposerLayersPanel, Editor__PipelineComposerLayersPanel);
// --- END META ---
