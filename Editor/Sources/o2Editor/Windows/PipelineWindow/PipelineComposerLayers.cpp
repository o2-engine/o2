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
#include "o2Editor/Windows/PipelineWindow/PipelineComposerTheme.h"
#include "o2Editor/Windows/PipelineWindow/PipelineControls.h"
#include "o2Editor/Windows/PipelineWindow/PipelineDraw.h"
#include "o2Editor/Windows/PipelineWindow/PipelineTheme.h"
#include "o2/Render/VectorSprite.h"
#include "o2/Assets/Types/FontAsset.h"

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

            // .cmp-grip: six dots in --fg-3, a bg-2 wash and --fg under the cursor, the accent while dragged
            auto& theme = PipelineTheme::Get();
            Widget::Draw();
            RectF rect = layout->GetWorldRect();
            bool hover = IsPressed() || IsUnderPoint(o2Input.GetCursorPos());
            if (hover)
                PipelineDraw::Fill(rect, PipelineDraw::Corners(PipelineComposerTheme::boxRadius), theme.bg2);
            if (!mDots)
                mDots = mmake<VectorSprite>("ui/pipeline/al_cmp_grip.svg");
            Vec2F c = rect.Center();
            mDots->rect = RectF(c.x - 6.0f, c.y + 10.5f, c.x + 6.0f, c.y - 10.5f);
            mDots->color = IsPressed() ? theme.accent : hover ? theme.fg : theme.fg3;
            mDots->Draw();
            CursorAreaEventsListener::OnDrawn();
        }

    private:
        Ref<VectorSprite> mDots; // Six dots of the grip

    public:

        bool IsUnderPoint(const Vec2F& point) override { return layout->IsPointInside(point); }

    protected:
        void OnCursorPressed(const Input::Cursor& cursor) override { if (onBegin) onBegin(); }
        void OnCursorStillDown(const Input::Cursor& cursor) override { if (onDrag) onDrag(cursor.position); }
        void OnCursorReleased(const Input::Cursor& cursor) override { if (onEnd) onEnd(true); }
        void OnCursorPressBreak(const Input::Cursor& cursor) override { if (onEnd) onEnd(false); }

        REF_COUNTERABLE_IMPL(Widget);
    };

    // --------------------------------------------------------------------------------------------------------------
    // Thumbnail of a layer row (.cmp-thumb): 26 px, the checker of 4 px squares, the picture fitted, a 1 px border
    // --------------------------------------------------------------------------------------------------------------
    class PipelineComposerThumb : public Widget
    {
    public:
        explicit PipelineComposerThumb(RefCounter* refCounter): Widget(refCounter) {}

        void SetBitmap(const Ref<Bitmap>& bitmap)
        {
            if (mSprite && bitmap && mShown.Lock() == bitmap)
                return;

            mShown = bitmap;
            mSprite = nullptr;
            mSize = Vec2I();
            if (!bitmap)
                return;

            // A thumbnail needs no more than a few dozen pixels: a small copy keeps the texture light
            Ref<Bitmap> display = bitmap;
            Vec2I size = bitmap->GetSize();
            const int maxSide = 64;
            if (size.x > maxSide || size.y > maxSide)
            {
                float k = (float)maxSide / Math::Max(size.x, size.y);
                display = bitmap->Resized(Vec2I(Math::Max(1, (int)(size.x * k)), Math::Max(1, (int)(size.y * k))));
            }
            mSize = display->GetSize();
            mSprite = mmake<Sprite>();
            mSprite->SetTexture(TextureRef(*display));
            mSprite->SetTextureSrcRect(RectI(Vec2I(), mSize));
        }

        void Draw() override
        {
            if (!mResEnabledInHierarchy || mIsClipped)
                return;

            auto& theme = PipelineTheme::Get();
            RectF rect = layout->GetWorldRect();
            if (!PipelineControls::IsFarView())
                PipelineComposerTheme::Checker(rect, 4.0f, theme.checkerA, theme.checkerB);
            if (mSprite && mSize.x > 0 && mSize.y > 0)
            {
                RectF inner(rect.left + 1, rect.top - 1, rect.right - 1, rect.bottom + 1);
                float k = Math::Min(inner.Width() / mSize.x, inner.Height() / mSize.y);
                Vec2F size(mSize.x * k, mSize.y * k), c = inner.Center();
                mSprite->rect = RectF(c - size * 0.5f, c + size * 0.5f);
                mSprite->Draw();
            }
            PipelineDraw::Stroke(rect, PipelineDraw::Corners(PipelineComposerTheme::boxRadius), 0.0f, 1.0f, PipelineDraw::Gradient(theme.border));
            Widget::Draw();
        }

    private:
        Ref<Sprite>     mSprite; // Picture of the layer
        Vec2I           mSize;   // Size of the picture texture
        WeakRef<Bitmap> mShown;  // Picture the texture was made from
    };

    PipelineComposerLayersPanel::PipelineComposerLayersPanel(RefCounter* refCounter):
        Widget(refCounter)
    {
        // .cmp-layers: bg-2 plate, --border frame, 3 px; the head is parted from the list by a --border line
        auto& theme = PipelineTheme::Get();
        AddFillLayer(*this, "back", theme.bg2, PipelineComposerTheme::boxRadius, Layout::BothStretch(), -3.0f);
        AddBorderLayer(*this, "frame", theme.border, PipelineComposerTheme::boxRadius, 1.0f, Layout::BothStretch(), -2.0f);
        auto line = mmake<PipelineRoundedRect>();
        line->radius = 0.0f;
        line->color = Color4(theme.border.r, theme.border.g, theme.border.b, 255);
        AddLayer("headLine", line, Layout::HorStretch(VerAlign::Top, border, border, 1.0f, border + headHeight - 1.0f), -1.0f)
            ->transparency = theme.border.a / 255.0f;
    }

    void PipelineComposerLayersPanel::Setup(const Ref<PipelineNode>& node, const Ref<PipelineComposerStage>& stage)
    {
        mNode = node;
        mStage = stage;
    }

    Ref<Button> PipelineComposerLayersPanel::MakeAddRow(PipelineLayerPlace place, float top)
    {
        // .cmp-add-row: a dashed --border frame, the hollow dot 7 px and the caption 7 px after it, 10 px dim; the accent and
        // --fg under the cursor
        WeakRef<PipelineComposerLayersPanel> weakThis(this);
        auto& theme = PipelineTheme::Get();
        bool front = place == PipelineLayerPlace::Front;
        auto row = mmake<Button>();
        row->name = front ? "add layer on top" : "add layer below";
        auto text = MakeThemeText(front ? "+ layer on top" : "+ layer below", 10.0f, 400, theme.fgDim, HorAlign::Left);
        row->AddLayer("caption", text, Layout::BothStretch(23, 0, 8, 0));
        row->caption = front ? "+ layer on top" : "+ layer below";
        // An empty text carries the hover state the frame is drawn by
        row->AddLayer("hoverMark", mmake<Text>(PipelineTheme::Font(400)), Layout::BothStretch())->transparency = 0.0f;
        row->AddState("hover", AnimationClip::EaseInOut("layer/hoverMark/transparency", 0.0f, 1.0f, 0.12f))->offStateAnimationSpeed = 0.5f;
        WeakRef<Button> weakRow(row);
        row->onDraw = [weakRow]()
        {
            auto self = weakRow.Lock();
            if (!self || PipelineControls::IsFarView())
                return;

            auto& theme = PipelineTheme::Get();
            bool hover = self->GetLayer("hoverMark")->transparency > 0.5f;
            Color4 color = hover ? theme.accent : theme.border;
            RectF rect = self->layout->GetWorldRect();
            PipelineComposerTheme::DashedFrame(rect, color);
            Vec2F dot(rect.left + 9.0f + 3.5f, rect.Center().y);
            PipelineDraw::Ring(dot, 2.75f, 1.5f, hover ? theme.fg : theme.fgDim);
            if (auto caption = self->GetLayerDrawable<Text>("caption"))
                caption->color = hover ? theme.fg : theme.fgDim;
        };
        *row->layout = WidgetLayout::HorStretch(VerAlign::Top, border + listPadding, border + listPadding, addRowHeight, top);
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
        mThumbs.Clear();
        mNameEdits.Clear();
        mDragging = "";
        mBuiltKey = RowsKey();

        auto layers = mStage->GetLayers();
        String selected = mStage->GetSelectedLayer();
        String open = mNode->GetConfigString("openLayerSettings", "");

        auto head = PipelineComposerTheme::MakeText("Layers \xC2\xB7 " + (String)layers.Count(), 10.0f, PipelineTheme::Get().fgDim);
        head->name = "layers head";
        head->layout->minWidth = 0; head->layout->maxWidth = 10000;
        *head->layout = WidgetLayout::HorStretch(VerAlign::Top, border + 8, 8, headHeight - 1.0f, border);
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
            auto empty = PipelineComposerTheme::MakeText("Add image inputs and connect sprites", 10.0f, PipelineTheme::Get().fgDim);
            empty->name = "empty";
            empty->horAlign = HorAlign::Middle;
            empty->layout->minWidth = 0; empty->layout->maxWidth = 10000;
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
            *row->layout = WidgetLayout::HorStretch(VerAlign::Top, border + listPadding, border + listPadding, rowHeight, y);
            AddChild(row);
            mRows.Add(row);

            if (display == openRow)
                AddSettings(layer, rows.settingsTop);
        }

        mAddTop = rows.addBottomRow;
        mAddRow = MakeAddRow(PipelineLayerPlace::Back, rows.addBottomRow);
        mContentHeight = rows.height;
    }

    String PipelineComposerLayersPanel::RowsKey() const
    {
        if (!mStage || !mNode)
            return "";

        String key = mNode->GetConfigString("openLayerSettings", "") + "|";
        for (auto& layer : mStage->GetLayers())
        {
            auto p = mStage->GetPlacement(layer);
            key += layer.id + ":" + layer.portId + (layer.dup ? ":d" : "") + (p.hidden ? ":h" : "") + ";";
        }

        // The open settings show the placement and the source size
        String open = mNode->GetConfigString("openLayerSettings", "");
        for (auto& layer : mStage->GetLayers())
        {
            if (layer.id != open)
                continue;

            auto p = mStage->GetPlacement(layer);
            Vec2I natural = mStage->GetNaturalSize(layer.portId);
            key += "|" + (String)p.w + "x" + (String)p.h + ":" + (String)p.opacity + ":" + (String)(p.nine ? 1 : 0) + ":" +
                (String)p.slice.l + "," + (String)p.slice.t + "," + (String)p.slice.r + "," + (String)p.slice.b + ":" +
                (String)p.sliceScale + ":" + (String)(p.lockAspect ? 1 : 0) + ":" + (String)p.exportW + "x" + (String)p.exportH + ":" +
                (String)natural.x + "x" + (String)natural.y;
        }
        return key;
    }

    void PipelineComposerLayersPanel::Refresh()
    {
        if (!mStage || !mNode)
            return;

        if (RowsKey() != mBuiltKey || mThumbs.Count() != mDisplayIds.Count())
        {
            Rebuild();
            return;
        }

        auto layers = mStage->GetLayers();
        for (int i = 0; i < mDisplayIds.Count(); i++)
        {
            auto layer = layers.FindOrDefault([&](const ComposerLayerRef& l) { return l.id == mDisplayIds[i]; });
            if (auto thumb = DynamicCast<PipelineComposerThumb>(mThumbs[i]))
                thumb->SetBitmap(mStage->GetLayerImage(layer.portId));
            if (i < mNameEdits.Count() && !mNameEdits[i]->IsFocused() && (String)mNameEdits[i]->GetText() != layer.name)
                mNameEdits[i]->SetText(layer.name);
        }
        UpdateSelection();
    }

    Ref<Widget> PipelineComposerLayersPanel::MakeRow(const ComposerLayerRef& layer, int stackIndex, int count, bool selected, bool open)
    {
        // .cmp-layer-row: 32 tall, padding 2 / 3 in a 1 px frame; the parts at the web's places from the row's left edge:
        // disclosure 4 (14), eye 22 (20), thumbnail 46 (26), name 76 to the actions, actions 94, grip 12
        WeakRef<PipelineComposerLayersPanel> weakThis(this);
        auto& theme = PipelineTheme::Get();
        auto p = mStage->GetPlacement(layer);
        String id = layer.id, portId = layer.portId;
        bool dup = layer.dup;

        auto container = mmake<Widget>();
        container->name = "layer " + id;
        auto select = AddFillLayer(*container, "select", theme.bg3, PipelineComposerTheme::boxRadius, Layout::BothStretch(), -2.0f);
        mSelectStrength = select->transparency;
        select->transparency = selected ? mSelectStrength : 0.0f;
        AddBorderLayer(*container, "selectBorder", theme.accent, PipelineComposerTheme::boxRadius, 1.0f, Layout::BothStretch(), -1.0f)
            ->transparency = selected ? 1.0f : 0.0f;

        auto selectButton = mmake<Button>();
        auto hover = AddFillLayer(*selectButton, "hover", theme.bg3, PipelineComposerTheme::boxRadius, Layout::BothStretch(), -1.0f);
        float hoverStrength = hover->transparency;
        hover->transparency = 0.0f;
        selectButton->AddState("hover", AnimationClip::EaseInOut("layer/hover/transparency", 0.0f, hoverStrength, 0.12f))->offStateAnimationSpeed = 0.5f;
        *selectButton->layout = WidgetLayout::BothStretch(0, 0, 0, 0);
        selectButton->onClick = [weakThis, id]() { if (auto self = weakThis.Lock()) self->mStage->SelectLayer(id); };
        container->AddChild(selectButton);

        auto place = [&](const Ref<Widget>& widget, float x, float width, float height)
        {
            *widget->layout = WidgetLayout(Vec2F(0, 0.5f), Vec2F(0, 0.5f), Vec2F(x, -height * 0.5f), Vec2F(x + width, height * 0.5f));
            container->AddChild(widget);
        };

        auto expand = PipelineComposerTheme::MakeRowAction(open ? "ui/pipeline/al_cmp_disclosure_open.svg" : "ui/pipeline/al_cmp_disclosure.svg", 14.0f, 14.0f, 20.0f);
        expand->name = "settings";
        if (open)
            if (auto icon = expand->GetLayerDrawable<VectorSprite>("icon"))
                icon->color = theme.fg;
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
        place(expand, 4.0f, 14.0f, 20.0f);

        auto layerRef = layer;
        auto eye = PipelineComposerTheme::MakeRowAction(p.hidden ? "ui/pipeline/al_cmp_eye_off.svg" : "ui/pipeline/al_cmp_eye.svg", 20.0f, 20.0f, 20.0f);
        eye->name = "visibility";
        eye->onClick = [weakThis, layerRef]()
        {
            if (auto self = weakThis.Lock())
            {
                auto np = self->mStage->GetPlacement(layerRef);
                np.hidden = !np.hidden;
                self->mStage->WritePlacement(layerRef.id, np, true);
                self->Rebuild();
            }
        };
        place(eye, 22.0f, 20.0f, 20.0f);

        auto thumb = mmake<PipelineComposerThumb>();
        thumb->name = "thumb";
        thumb->SetBitmap(mStage->GetLayerImage(layer.portId));
        place(thumb, 46.0f, 26.0f, 26.0f);
        mThumbs.Add(thumb);

        // The actions and the grip keep to the right edge; the name takes what is left
        const float actionsWidth = 5 * 18.0f + 4 * 1.0f, gripWidth = 12.0f;
        float right = 3.0f + 1.0f + gripWidth + 1.0f + 4.0f + actionsWidth + 4.0f;
        float tagWidth = 0.0f;
        if (dup)
        {
            auto tag = PipelineComposerTheme::MakeText("COPY", 8.0f, theme.bg);
            tag->name = "copy tag";
            tag->horAlign = HorAlign::Middle;
            tagWidth = tag->layout->minWidth + 6.0f;
            AddFillLayer(*tag, "back", theme.fgDim, 2.0f, Layout::BothStretch(), -1.0f);
            *tag->layout = WidgetLayout(Vec2F(1, 0.5f), Vec2F(1, 0.5f), Vec2F(-right - tagWidth, -6), Vec2F(-right, 6));
            container->AddChild(tag);
            tagWidth += 4.0f;
        }

        auto name = MakeEditBox(layer.name, false, "layer");
        PipelineComposerTheme::SkinField(name, 10.0f, false);
        name->name = "layer name";
        *name->layout = WidgetLayout(Vec2F(0, 0.5f), Vec2F(1, 0.5f), Vec2F(76.0f, -8.5f), Vec2F(-right - tagWidth, 8.5f));
        name->onChangeCompleted = [weakThis, id, portId, dup](const WString& text)
        {
            if (auto self = weakThis.Lock()) if (self->onRename) self->onRename(id, portId, dup, (String)text);
        };
        // The field takes the press of the row: focusing it, by click or keyboard, selects the layer too
        name->onFocused = [weakThis, id]() { if (auto self = weakThis.Lock()) self->mStage->SelectLayer(id); };
        container->AddChild(name);
        mNameEdits.Add(name);

        float x = -(3.0f + 1.0f + gripWidth + 1.0f + 4.0f + actionsWidth);
        auto action = [&](const String& actionName, const String& icon, bool enabled, const Function<void()>& onClick)
        {
            // .canvas-host svg fills its button: the 24 px icons are drawn 18 px wide
            auto button = PipelineComposerTheme::MakeRowAction(icon, 18.0f);
            button->name = actionName;
            button->interactable = enabled;
            button->transparency = enabled ? 1.0f : 0.3f;
            button->onClick = onClick;
            *button->layout = WidgetLayout(Vec2F(1, 0.5f), Vec2F(1, 0.5f), Vec2F(x, -10), Vec2F(x + 18, 10));
            container->AddChild(button);
            x += 19.0f;
            return button;
        };

        action("duplicate", "ui/pipeline/al_cmp_copy.svg", true, [weakThis, id]() { if (auto self = weakThis.Lock()) if (self->onDuplicate) self->onDuplicate(id); });
        action("forward", "ui/pipeline/al_cmp_up.svg", stackIndex < count - 1, [weakThis, stackIndex]() { if (auto self = weakThis.Lock()) if (self->onMove) self->onMove(stackIndex, 1); });
        action("backward", "ui/pipeline/al_cmp_down.svg", stackIndex > 0, [weakThis, stackIndex]() { if (auto self = weakThis.Lock()) if (self->onMove) self->onMove(stackIndex, -1); });
        action("reset", "ui/pipeline/al_cmp_reset.svg", true, [weakThis, layerRef]()
        {
            if (auto self = weakThis.Lock())
            {
                auto np = self->mStage->DefaultPlacement(layerRef.portId);
                np.hidden = self->mStage->GetPlacement(layerRef).hidden;
                self->mStage->WritePlacement(layerRef.id, np, true);
            }
        });
        auto remove = action("remove", "ui/pipeline/al_cmp_trash.svg", true, [weakThis, id, portId, dup]()
        {
            if (auto self = weakThis.Lock()) if (self->onRemove) self->onRemove(id, portId, dup);
        });
        WeakRef<Button> weakRemove(remove);
        remove->onDraw = [weakRemove]()
        {
            // .cmp-del: red under the cursor
            if (auto self = weakRemove.Lock())
                if (auto icon = self->GetLayerDrawable<VectorSprite>("icon"))
                    icon->color = self->GetLayer("hover")->transparency > 0.5f ? PipelineTheme::Get().red : PipelineTheme::Get().fgDim;
        };

        auto grip = mmake<PipelineComposerGrip>();
        grip->name = "grip";
        *grip->layout = WidgetLayout(Vec2F(1, 0.5f), Vec2F(1, 0.5f), Vec2F(-4.0f - gripWidth, -11), Vec2F(-4.0f, 11));
        grip->onBegin = [weakThis, id]() { if (auto self = weakThis.Lock()) self->BeginRowDrag(id); };
        grip->onDrag = [weakThis](const Vec2F& point) { if (auto self = weakThis.Lock()) self->DragRowTo(point); };
        grip->onEnd = [weakThis](bool commit) { if (auto self = weakThis.Lock()) self->EndRowDrag(commit); };
        container->AddChild(grip);
        mGrips.Add(grip);

        return container;
    }

    void PipelineComposerLayersPanel::AddSettings(const ComposerLayerRef& layer, float top)
    {
        WeakRef<PipelineComposerLayersPanel> weakThis(this);
        auto p = mStage->GetPlacement(layer);
        auto image = mStage->GetLayerImage(layer.portId);
        Vec2I natural = image ? image->GetSize() : Vec2I();

        // .cmp-layer-settings: field fill and --border, 3 px, under the row 24 px in
        auto& theme = PipelineTheme::Get();
        auto settings = mmake<Widget>();
        settings->name = "layer settings";
        *settings->layout = WidgetLayout::HorStretch(VerAlign::Top, 22, 6, PipelineComposerLayout::SettingsHeight(p.nine) - 2.0f, top + 1.0f);
        AddFillLayer(*settings, "back", theme.field, PipelineComposerTheme::boxRadius, Layout::BothStretch(-4, -2, -4, -2), -2.0f);
        AddBorderLayer(*settings, "border", theme.border, PipelineComposerTheme::boxRadius, 1.0f, Layout::BothStretch(-4, -2, -4, -2), -1.0f);
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
            auto edit = PipelineComposerTheme::MakeNumberField(FormatNumber(value, 1), width);
            edit->onChangeCompleted = [onChange](const WString& text) { onChange((float)atof(((String)text).Data())); };
            return edit;
        };
        auto caption = [&](const String& text, float width)
        {
            auto label = PipelineComposerTheme::MakeText(text, 10.0f, theme.fgDim);
            label->layout->minWidth = width; label->layout->maxWidth = width;
            return label;
        };
        auto makeLine = []()
        {
            auto line = mmake<HorizontalLayout>();
            line->spacing = 3; line->expandWidth = false; line->expandHeight = true; line->baseCorner = BaseCorner::Left;
            return line;
        };

        auto sizeRow = makeLine();
        sizeRow->AddChild(caption("Size", 34));
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
        auto lockToggle = PipelineComposerTheme::MakeIconToggle("ui/pipeline/al_cmp_link.svg", p.lockAspect);
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
            auto one = PipelineComposerTheme::MakeToolButton("1:1", "", 10.0f, 34.0f);
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
        float nineWidth = 0.0f;
        auto nine = PipelineComposerTheme::MakeCheck("9-slice", p.nine, nineWidth);
        nine->name = "nine slice";
        nine->onToggleByUser = [patch, rebuild](bool v) { patch([v](ComposerLayerPlacement& np) { np.nine = v; }, true); rebuild(true); };
        nineRow->AddChild(nine);
        if (p.nine && natural.x > 0)
        {
            auto autoButton = PipelineComposerTheme::MakeToolButton("auto", "", 10.0f, 44.0f);
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
            sliceRow->AddChild(caption(f.label, 12));
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
        auto hintLabel = PipelineComposerTheme::MakeText(hint, 9.5f, theme.fg3);
        hintLabel->layout->minWidth = 0; hintLabel->layout->maxWidth = 10000;
        hintLabel->horOverflow = Label::HorOverflow::Dots;
        place(hintLabel, 123 + line, 18);
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

        auto& theme = PipelineTheme::Get();
        auto label = PipelineComposerTheme::MakeText("Export", 10.0f, theme.fgDim);
        label->layout->minWidth = 34; label->layout->maxWidth = 34;
        row->AddChild(label);

        // An empty field shows the layer's own size, dimmed: that is what the file gets
        auto sideEdit = [&](bool horizontal)
        {
            float value = horizontal ? p.exportW : p.exportH;
            auto edit = PipelineComposerTheme::MakeNumberField(exportOn ? FormatNumber(Math::Round(value), 1) : String(), 48);
            edit->name = horizontal ? "export width" : "export height";

            auto hint = MakeThemeText(FormatNumber(Math::Round(horizontal ? p.w : p.h), 1), 11.0f, 400, theme.fg3, HorAlign::Left);
            hint->SetFontAsset(AssetRef<FontAsset>(PipelineTheme::MonoFont()));
            hint->height = PipelineTheme::Size(11.0f);
            auto hintLayer = edit->AddLayer("placeholder", hint, Layout::BothStretch(5, 0, 5, 0), 1.0f);
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
        auto cross = PipelineComposerTheme::MakeText("\xC3\x97", 11.5f, theme.fgDim);
        cross->horAlign = HorAlign::Middle;
        cross->layout->minWidth = 18; cross->layout->maxWidth = 18;
        row->AddChild(cross);
        row->AddChild(sideEdit(false));

        if (!exportOn)
            return;

        auto autoButton = PipelineComposerTheme::MakeToolButton("auto", "", 10.0f, 34.0f);
        autoButton->name = "export auto";
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
            bool on = mDisplayIds[i] == selected;
            if (auto layer = mRows[i]->FindLayer("select"))
                layer->transparency = on ? mSelectStrength : 0.0f;
            if (auto layer = mRows[i]->FindLayer("selectBorder"))
                layer->transparency = on ? 1.0f : 0.0f;
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
        PipelineDraw::Fill(RectF(panel.left + border + listPadding + 2, y + 1, panel.right - border - listPadding - 2, y - 1),
                           PipelineDraw::Corners(1.0f), PipelineTheme::Get().accent);
    }
}
// --- META ---

DECLARE_CLASS(Editor::PipelineComposerLayersPanel, Editor__PipelineComposerLayersPanel);
// --- END META ---
