#include "o2Editor/stdafx.h"
#include "PipelineNodeWidget.h"

#include "o2/Render/Render.h"
#include "o2/Render/Sprite.h"
#include "o2/Render/Text.h"
#include "o2/Scene/UI/UIManager.h"
#include "o2/Scene/UI/WidgetLayer.h"
#include "o2/Scene/UI/WidgetLayout.h"
#include "o2/Scene/UI/Widgets/Button.h"
#include "o2/Scene/UI/Widgets/EditBox.h"
#include "o2/Scene/UI/Widgets/Label.h"
#include "o2/Utils/Editor/EditorScope.h"
#include "o2Editor/Windows/PipelineWindow/PipelineControls.h"
#include "o2Editor/Windows/PipelineWindow/PipelineEditor.h"
#include "o2Editor/Windows/PipelineWindow/PipelineNodeBody.h"

namespace Editor
{
    const float PipelineNodeWidget::headerHeight = 28.0f;
    const float PipelineNodeWidget::padTop = 8.0f;
    const float PipelineNodeWidget::portRow = 22.0f;
    const float PipelineNodeWidget::padBottom = 10.0f;
    const float PipelineNodeWidget::portRadius = 8.0f;
    const float PipelineNodeWidget::portHitRadius = 14.0f;
    const float PipelineNodeWidget::defaultWidth = 260.0f;
    const float PipelineNodeWidget::minWidth = 180.0f;
    const float PipelineNodeWidget::minHeight = 80.0f;
    static const int farUpdateFrames = 3;

    static const Widget& PipelineNodeStyle()
    {
        static Ref<Widget> fallback;
        if (auto style = o2UI.GetWidgetStyle<Widget>("pipeline node"))
            return *style;

        if (!fallback)
            fallback = mmake<Widget>();

        return *fallback;
    }

    PipelineNodeWidget::PipelineNodeWidget(RefCounter* refCounter):
        Widget(refCounter)
    {}

    PipelineNodeWidget::PipelineNodeWidget(RefCounter* refCounter, const Ref<PipelineEditor>& editor, const Ref<PipelineNode>& node):
        Widget(refCounter, PipelineNodeStyle()), mEditor(editor), mNode(node)
    {
        PushEditorScopeOnStack scope;
        mSchema = PipelineNodeRegistry::GetSchema(node->nodeType);

        BuildHeader();

        auto weakThis = WeakRef(this);

        dragHandle = mmake<DragHandle>();
        dragHandle->SetSelectionGroup(editor);
        dragHandle->messageFallDownListener = editor.Get();
        dragHandle->isPointInside = [weakThis](const Vec2F& p)
        {
            auto self = weakThis.Lock();
            if (!self || !self->IsUnderPoint(p))
                return false;

            // Ports and the add button belong to the editor's connection gesture
            if (self->FindPortAt(p) || self->IsAddInputAt(p))
                return false;

            // Only the header and the port rows drag the card; the body owns its controls
            RectF rect = self->GetCardRect();
            float top = rect.top;
            float bodyTop = top - headerHeight - padTop - self->GetPortsHeight();
            return p.y >= bodyTop;
        };
        dragHandle->onChangedPos = THIS_FUNC(OnDragged);
        dragHandle->onChangeCompleted = THIS_FUNC(OnDragCompleted);
        dragHandle->onRightButtonReleased = [this](const Input::Cursor&) { OpenContextMenu(); };
        dragHandle->onSelected = [weakThis]() { if (auto s = weakThis.Lock()) s->SetSelected(true); };
        dragHandle->onDeselected = [weakThis]() { if (auto s = weakThis.Lock()) s->SetSelected(false); };
        dragHandle->onPressed = [weakThis]() { if (auto s = weakThis.Lock()) if (auto e = s->mEditor.Lock()) e->OnNodePressed(s); };

        BuildResizeHandles(editor);

        // Handles drawn later take the cursor first, so the edges win over the card drag
        onDraw = [this]()
        {
            dragHandle->Draw();
            for (auto& resize : mResizeHandles)
                resize.handle->Draw();
        };

        Rebuild();
        UpdateFromNode();
    }

    PipelineNodeWidget::~PipelineNodeWidget()
    {}

    Ref<PipelineEditor> PipelineNodeWidget::GetEditor() const
    {
        return mEditor.Lock();
    }

    Color4 PipelineNodeWidget::ColorOfPortType(PipelinePortType type) const
    {
        return PipelineEditor::GetPortColor(type);
    }

    String PipelineNodeWidget::IconForType() const
    {
        return IconForType(mNode->nodeType);
    }

    String PipelineNodeWidget::IconForType(const String& t)
    {
        if (t.StartsWith("finish")) return "ui/pipeline/node_finish.png";
        if (t == "sourceText" || t == "textCompose" || t == "textConcat") return "ui/pipeline/node_text.png";
        if (t == "sourceImage") return "ui/pipeline/node_image.png";
        if (t == "removeBackground") return "ui/pipeline/node_removebg.png";
        if (t == "sourceAudio" || t == "sfxGen") return "ui/pipeline/node_audio.png";
        if (t == "audioProcess") return "ui/pipeline/node_process.png";
        if (t == "ttsSpeech") return "ui/pipeline/node_voice.png";
        if (t == "musicGen") return "ui/pipeline/node_music.png";
        if (t == "videoGen") return "ui/pipeline/node_video.png";
        if (t == "composer") return "ui/pipeline/node_composer.png";
        if (t == "drawImage") return "ui/pipeline/node_draw.png";
        if (t == "imageExtract") return "ui/pipeline/node_extract.png";
        if (t == "imageEdit") return "ui/pipeline/node_edit.png";
        if (t == "promptGen") return "ui/pipeline/node_prompt.png";
        if (t == "imageOutline" || t == "imageShadow" || t == "imageGradient" || t == "imageColor") return "ui/pipeline/node_effect.png";
        return "ui/pipeline/node_ai.png";
    }

    void PipelineNodeWidget::BuildHeader()
    {
        auto icon = mmake<Sprite>(IconForType());
        icon->color = Color4(96, 125, 139, 255);
        mIconLayer = AddLayer("icon", icon, Layout(Vec2F(0, 1), Vec2F(0, 1), Vec2F(6, -23), Vec2F(24, -5)));

        auto title = mmake<Text>("stdFont.ttf");
        title->text = mSchema ? mSchema->label : mNode->nodeType;
        title->horAlign = HorAlign::Left;
        title->verAlign = VerAlign::Middle;
        title->dotsEngings = true;
        title->color = Color4(96, 125, 139, 255);
        mTitleLayer = AddLayer("title", title, Layout(Vec2F(0, 1), Vec2F(1, 1), Vec2F(28, -headerHeight), Vec2F(-80, 0)));

        // Category tint of the header: sources bluish, finish nodes greenish
        if (auto tint = GetLayer("kindTint"))
        {
            if (auto drawable = tint->GetDrawable())
            {
                if (mNode->nodeType.StartsWith("source")) { drawable->color = Color4(33, 150, 243, 255); tint->transparency = 0.12f; }
                else if (mNode->nodeType.StartsWith("finish")) { drawable->color = Color4(76, 175, 80, 255); tint->transparency = 0.14f; }
                else tint->transparency = 0.0f;
            }
            tint->layout = Layout::HorStretch(VerAlign::Top, 0, 0, headerHeight + 4, -4);
        }

        mPlayButton = o2UI.CreateWidget<Button>("pipeline icon");
        mPlayButton->name = "play";
        *mPlayButton->layout = WidgetLayout(Vec2F(1, 1), Vec2F(1, 1), Vec2F(-36, -24), Vec2F(-4, -4));
        mPlayButton->onClick = THIS_FUNC(OnPlayPressed);
        AddChild(mPlayButton);

        mErrorButton = o2UI.CreateWidget<Button>("pipeline icon");
        mErrorButton->name = "error";
        *mErrorButton->layout = WidgetLayout(Vec2F(1, 1), Vec2F(1, 1), Vec2F(-60, -24), Vec2F(-40, -4));
        if (auto ic = mErrorButton->GetLayerDrawable<Sprite>("icon")) { ic->imageName = "ui/pipeline/btn_error.png"; ic->color = Color4(229, 83, 61, 255); }
        mErrorButton->onClick = [this]() { if (auto e = mEditor.Lock()) e->ShowNodeError(Ref(this)); };
        mErrorButton->enabled = false;
        AddChild(mErrorButton);

        mRetryLabel = o2UI.CreateLabel("");
        mRetryLabel->name = "retry";
        *mRetryLabel->layout = WidgetLayout(Vec2F(1, 1), Vec2F(1, 1), Vec2F(-100, -24), Vec2F(-62, -4));
        mRetryLabel->horAlign = HorAlign::Middle;
        auto retryBack = mmake<Sprite>("ui/UI4_button_regular.png");
        retryBack->color = Color4(255, 213, 79, 255);
        mRetryLabel->AddLayer("back", retryBack, Layout::BothStretch(-9, -9, -10, -10), -1.0f);
        mRetryLabel->enabled = false;
        AddChild(mRetryLabel);

        mAddInputButton = o2UI.CreateWidget<Button>("add small");
        mAddInputButton->name = "add input";
        mAddInputButton->onClick = THIS_FUNC(OnAddInputPressed);
        mAddInputButton->enabled = false;
        AddChild(mAddInputButton);

        mBodyHost = mmake<Widget>();
        mBodyHost->name = "body";
        AddChild(mBodyHost);
    }

    void PipelineNodeWidget::BuildResizeHandles(const Ref<PipelineEditor>& editor)
    {
        struct Side { bool left, right, top, bottom; CursorType cursor; };
        Side sides[] = {
            { true, false, false, false, CursorType::SizeWE },
            { false, true, false, false, CursorType::SizeWE },
            { false, false, true, false, CursorType::SizeNS },
            { false, false, false, true, CursorType::SizeNS },
            { true, false, true, false, CursorType::SizeNwSe },
            { false, true, false, true, CursorType::SizeNwSe },
            { false, true, true, false, CursorType::SizeNeSw },
            { true, false, false, true, CursorType::SizeNeSw },
        };

        auto weakThis = WeakRef(this);
        for (auto& side : sides)
        {
            ResizeHandle resize;
            resize.left = side.left;
            resize.right = side.right;
            resize.top = side.top;
            resize.bottom = side.bottom;
            resize.handle = mmake<DragHandle>();
            resize.handle->cursorType = side.cursor;
            resize.handle->messageFallDownListener = editor.Get();

            int index = mResizeHandles.Count();
            resize.handle->isPointInside = [weakThis, index](const Vec2F& p)
            {
                auto self = weakThis.Lock();
                return self && index < self->mResizeHandles.Count() && self->IsResizeHandleAt(self->mResizeHandles[index], p);
            };
            resize.handle->onBeganDragging = [weakThis]()
            {
                if (auto self = weakThis.Lock())
                {
                    self->mResizing = true;
                    self->mResizeStartRect = self->GetCardRect();
                }
            };
            resize.handle->onChangedPos = [weakThis, index](const Vec2F& position)
            {
                if (auto self = weakThis.Lock())
                    self->OnResizeDragged(self->mResizeHandles[index], position);
            };
            resize.handle->onChangeCompleted = THIS_FUNC(OnResizeCompleted);
            mResizeHandles.Add(resize);
        }
    }

    bool PipelineNodeWidget::IsResizeHandleAt(const ResizeHandle& handle, const Vec2F& p) const
    {
        if (FindPortAt(p) || IsAddInputAt(p))
            return false;

        const float band = 6.0f;
        const float corner = 12.0f;
        RectF rect = GetCardRect();
        bool nearLeft = Math::Abs(p.x - rect.left) <= band;
        bool nearRight = Math::Abs(p.x - rect.right) <= band;
        bool nearTop = Math::Abs(p.y - rect.top) <= band;
        bool nearBottom = Math::Abs(p.y - rect.bottom) <= band;
        bool insideX = p.x >= rect.left - band && p.x <= rect.right + band;
        bool insideY = p.y >= rect.bottom - band && p.y <= rect.top + band;
        bool cornerX = p.x <= rect.left + corner || p.x >= rect.right - corner;
        bool cornerY = p.y >= rect.top - corner || p.y <= rect.bottom + corner;

        bool isCorner = (handle.left || handle.right) && (handle.top || handle.bottom);
        if (isCorner)
        {
            bool x = handle.left ? p.x >= rect.left - band && p.x <= rect.left + corner : p.x <= rect.right + band && p.x >= rect.right - corner;
            bool y = handle.top ? p.y <= rect.top + band && p.y >= rect.top - corner : p.y >= rect.bottom - band && p.y <= rect.bottom + corner;
            return x && y;
        }

        if (handle.left)
            return nearLeft && insideY && !cornerY;

        if (handle.right)
            return nearRight && insideY && !cornerY;

        if (handle.top)
            return nearTop && insideX && !cornerX;

        return nearBottom && insideX && !cornerX;
    }

    void PipelineNodeWidget::Rebuild()
    {
        PushEditorScopeOnStack scope;
        mFarUpdateFrames = farUpdateFrames;
        mSchema = PipelineNodeRegistry::GetSchema(mNode->nodeType);
        BuildPorts();
        BuildBody();
        UpdateHeaderButtons();
        LayoutPorts();
        LayoutBody();
    }

    void PipelineNodeWidget::UpdateHeaderButtons()
    {
        bool runnable = mSchema && mSchema->category != PipelineNodeCategory::Source && (!mSchema->inputs.IsEmpty() || !mSchema->outputs.IsEmpty());
        bool selfApplying = mSchema && mSchema->instant && mSchema->category != PipelineNodeCategory::Output;
        mPlayButton->enabled = runnable && !selfApplying;

        bool busy = mRuntime.state == "running" || mRuntime.state == "queued";
        if (auto icon = mPlayButton->GetLayerDrawable<Sprite>("icon"))
        {
            icon->imageName = busy ? "ui/pipeline/btn_stop.png" : "ui/pipeline/btn_play.png";
            icon->color = busy ? Color4(229, 83, 61, 255) : Color4(0, 150, 136, 255);
        }

        bool hasError = mRuntime.state == "error" && !mRuntime.error.IsEmpty();
        mErrorButton->enabled = hasError;

        bool retry = mRuntime.retryAttempt > 0 && busy;
        mRetryLabel->enabled = retry;
        if (retry)
            mRetryLabel->text = (String)mRuntime.retryAttempt + "/" + (String)mRuntime.retryMax;
        float retryX = hasError ? -100.0f : -76.0f;
        *mRetryLabel->layout = WidgetLayout(Vec2F(1, 1), Vec2F(1, 1), Vec2F(retryX, -24), Vec2F(retryX + 36, -4));
    }

    void PipelineNodeWidget::BuildPorts()
    {
        for (auto& view : mInputs)
        {
            if (view.label) RemoveLayer(view.label);
            if (view.nameEdit) RemoveChild(view.nameEdit);
            if (view.deleteButton) RemoveChild(view.deleteButton);
        }
        for (auto& view : mOutputs)
        {
            if (view.label) RemoveLayer(view.label);
        }
        mInputs.Clear();
        mOutputs.Clear();

        auto makeCircle = [&](const PipelinePort& port)
        {
            auto sprite = mmake<Sprite>("ui/pipeline/port_fill.png");
            sprite->color = ColorOfPortType(port.portType);
            sprite->size = Vec2F(portRadius * 2, portRadius * 2);
            return sprite;
        };

        auto makeLabel = [&](const PipelinePort& port, bool input)
        {
            auto text = mmake<Text>("stdFont.ttf");
            text->text = port.name;
            text->horAlign = input ? HorAlign::Left : HorAlign::Right;
            text->verAlign = VerAlign::Middle;
            text->dotsEngings = true;
            text->color = ColorOfPortType(port.portType);
            return AddLayer("portLabel_" + port.id, text, Layout::Based(BaseCorner::LeftTop, Vec2F(100, portRow)), 2.0f);
        };

        for (auto& port : mNode->inputs)
        {
            PortView view;
            view.port = port;
            view.input = true;
            view.circle = makeCircle(port);
            if (port.custom)
            {
                view.nameEdit = o2UI.CreateEditBox("singleline");
                view.nameEdit->name = "portName_" + port.id;
                view.nameEdit->SetText(port.name);
                view.nameEdit->layout->minSize = Vec2F(20, 18);
                String portId = port.id;
                auto weakThis = WeakRef(this);
                view.nameEdit->onChangeCompleted = [weakThis, portId](const WString& text)
                {
                    if (auto self = weakThis.Lock()) self->OnCustomInputRenamed(portId, (String)text);
                };
                if (auto textDrawable = view.nameEdit->GetTextDrawable())
                    textDrawable->color = ColorOfPortType(port.portType);
                AddChild(view.nameEdit);

                view.deleteButton = o2UI.CreateWidget<Button>("remove small");
                view.deleteButton->name = "portDelete_" + port.id;
                view.deleteButton->onClick = [weakThis, portId]() { if (auto self = weakThis.Lock()) self->OnCustomInputDeleted(portId); };
                AddChild(view.deleteButton);
            }
            else
                view.label = makeLabel(port, true);

            mInputs.Add(view);
        }

        for (auto& port : mNode->outputs)
        {
            PortView view;
            view.port = port;
            view.input = false;
            view.circle = makeCircle(port);
            view.label = makeLabel(port, false);
            mOutputs.Add(view);
        }

        mAddInputButton->enabled = mSchema && !mSchema->addableInputs.IsEmpty();
    }

    void PipelineNodeWidget::BuildBody()
    {
        if (mBody)
        {
            mBodyHost->RemoveChild(mBody);
            mBody = nullptr;
        }

        mBody = PipelineNodeBodies::Create(mNode->nodeType, Ref(this));
        if (mBody)
        {
            *mBody->layout = WidgetLayout::BothStretch(0, 0, 0, 0);
            mBodyHost->AddChild(mBody);
            mBody->Build();
        }
    }

    float PipelineNodeWidget::GetPortsHeight() const
    {
        int extra = (mSchema && !mSchema->addableInputs.IsEmpty()) ? 1 : 0;
        int rows = Math::Max(mNode->inputs.Count() + extra, mNode->outputs.Count());
        return rows * portRow;
    }

    float PipelineNodeWidget::GetBodyHeight() const
    {
        return mBody ? mBody->GetPreferredHeight(GetCardWidth()) : 0.0f;
    }

    float PipelineNodeWidget::GetAutoHeight() const
    {
        return headerHeight + padTop + GetPortsHeight() + GetBodyHeight() + padBottom;
    }

    float PipelineNodeWidget::GetCardWidth() const
    {
        float w = mNode->size.x > 0 ? mNode->size.x : (mSchema && mSchema->defaultSize.x > 0 ? mSchema->defaultSize.x : defaultWidth);
        return Math::Max(w, minWidth);
    }

    Vec2F PipelineNodeWidget::GetCardSize() const
    {
        float h = Math::Max(mNode->size.y, GetAutoHeight());
        return Vec2F(GetCardWidth(), Math::Max(h, minHeight));
    }

    RectF PipelineNodeWidget::GetCardRect() const
    {
        Vec2F size = GetCardSize();
        Vec2F leftTop = PipelineEditor::NodeToCanvas(mNode->position);
        return RectF(leftTop.x, leftTop.y, leftTop.x + size.x, leftTop.y - size.y);
    }

    void PipelineNodeWidget::UpdateFromNode()
    {
        Vec2F size = GetCardSize();
        Vec2F leftTop = PipelineEditor::NodeToCanvas(mNode->position);
        *layout = WidgetLayout::Based(BaseCorner::LeftTop, size, leftTop);
        dragHandle->position = leftTop;

        RectF rect(leftTop.x, leftTop.y, leftTop.x + size.x, leftTop.y - size.y);
        for (auto& resize : mResizeHandles)
        {
            float x = resize.right ? rect.right : rect.left;
            float y = resize.bottom ? rect.bottom : rect.top;
            resize.handle->position = Vec2F(x, y);
        }

        LayoutPorts();
        LayoutBody();
    }

    void PipelineNodeWidget::LayoutPorts()
    {
        Vec2F size = GetCardSize();
        for (int i = 0; i < mInputs.Count(); i++)
        {
            auto& view = mInputs[i];
            float cy = headerHeight + padTop + (i + 0.5f) * portRow;
            view.localPos = Vec2F(0, cy);
            float rightReserve = i < mOutputs.Count() ? 60.0f : 16.0f;
            if (view.label)
                view.label->layout = Layout(Vec2F(0, 1), Vec2F(1, 1), Vec2F(14, -cy - portRow / 2), Vec2F(-rightReserve, -cy + portRow / 2));
            if (view.nameEdit)
            {
                float avail = Math::Max(40.0f, size.x - 14 - rightReserve);
                float inputW = Math::Clamp((view.port.name.Length() + 2) * 7.0f + 12.0f, 40.0f, avail - 24.0f);
                *view.nameEdit->layout = WidgetLayout(Vec2F(0, 1), Vec2F(0, 1), Vec2F(14, -cy - 9), Vec2F(14 + inputW, -cy + 9));
                *view.deleteButton->layout = WidgetLayout(Vec2F(0, 1), Vec2F(0, 1), Vec2F(16 + inputW, -cy - 9), Vec2F(34 + inputW, -cy + 9));
            }
        }

        for (int i = 0; i < mOutputs.Count(); i++)
        {
            auto& view = mOutputs[i];
            float cy = headerHeight + padTop + (i + 0.5f) * portRow;
            view.localPos = Vec2F(size.x, cy);
            view.label->layout = Layout(Vec2F(0, 1), Vec2F(1, 1), Vec2F(14, -cy - portRow / 2), Vec2F(-14, -cy + portRow / 2));
        }

        if (mAddInputButton->enabled)
        {
            float cy = headerHeight + padTop + (mInputs.Count() + 0.5f) * portRow;
            *mAddInputButton->layout = WidgetLayout(Vec2F(0, 1), Vec2F(0, 1), Vec2F(-8, -cy - 8), Vec2F(8, -cy + 8));
        }
    }

    void PipelineNodeWidget::LayoutBody()
    {
        mFarUpdateFrames = farUpdateFrames;
        float bodyTop = headerHeight + padTop + GetPortsHeight();
        *mBodyHost->layout = WidgetLayout(Vec2F(0, 0), Vec2F(1, 1), Vec2F(0, padBottom), Vec2F(0, -bodyTop));
        if (mBody)
            mBody->Relayout(GetCardWidth(), GetCardSize().y - bodyTop - padBottom);
    }

    Vec2F PipelineNodeWidget::GetPortPosition(const String& portId, bool input) const
    {
        RectF rect = GetCardRect();
        auto& views = input ? mInputs : mOutputs;
        for (auto& view : views)
        {
            if (view.port.id == portId)
                return Vec2F(rect.left + view.localPos.x, rect.top - view.localPos.y);
        }
        return input ? Vec2F(rect.left, rect.top) : Vec2F(rect.right, rect.top);
    }

    const PipelineNodeWidget::PortView* PipelineNodeWidget::FindPortAt(const Vec2F& p) const
    {
        RectF rect = GetCardRect();
        for (auto& view : mInputs)
        {
            Vec2F c(rect.left + view.localPos.x, rect.top - view.localPos.y);
            if ((c - p).Length() <= portHitRadius)
                return &view;
        }
        for (auto& view : mOutputs)
        {
            Vec2F c(rect.left + view.localPos.x, rect.top - view.localPos.y);
            if ((c - p).Length() <= portHitRadius)
                return &view;
        }
        return nullptr;
    }

    bool PipelineNodeWidget::IsAddInputAt(const Vec2F& p) const
    {
        if (!mAddInputButton->enabled)
            return false;

        RectF rect = GetCardRect();
        float cy = headerHeight + padTop + (mInputs.Count() + 0.5f) * portRow;
        Vec2F c(rect.left, rect.top - cy);
        return (c - p).Length() <= 12.0f;
    }

    bool PipelineNodeWidget::IsHeaderAt(const Vec2F& p) const
    {
        RectF rect = GetCardRect();
        return p.x >= rect.left && p.x <= rect.right && p.y <= rect.top && p.y >= rect.top - headerHeight;
    }

    void PipelineNodeWidget::SetSelected(bool selected)
    {
        mSelected = selected;
        SetState("selected", selected);
    }

    void PipelineNodeWidget::ApplyRuntime()
    {
        const String& s = mRuntime.state;
        SetState("running", s == "running");
        SetState("queued", s == "queued");
        SetState("error", s == "error");
        SetState("done", s != "running" && s != "queued" && s != "error" && mRuntime.fresh);
        UpdateHeaderButtons();
    }

    void PipelineNodeWidget::OnOutputChanged()
    {
        mFarUpdateFrames = farUpdateFrames;
        if (mBody)
            mBody->OnOutputChanged();
    }

    void PipelineNodeWidget::OnConfigChanged()
    {
        mFarUpdateFrames = farUpdateFrames;
        if (mBody)
            mBody->OnConfigChanged();
    }

    void PipelineNodeWidget::Update(float dt)
    {
        if (mCulled)
            return;

        Widget::Update(dt);
    }

    void PipelineNodeWidget::UpdateChildren(float dt)
    {
        if (mCulled)
            return;

        if (!mDetailed)
        {
            if (mFarUpdateFrames <= 0)
                return;

            mFarUpdateFrames--;
            mBodyHost->Update(dt);
            mBodyHost->UpdateChildren(dt);
            return;
        }

        Widget::UpdateChildren(dt);
    }

    void PipelineNodeWidget::Draw()
    {
        if (mCulled || !mResEnabledInHierarchy)
            return;

        if (!mDetailed)
        {
            DrawLayers();
            OnDrawn();
            PipelineControls::SetFarView(true);
            mBodyHost->Draw();
            PipelineControls::SetFarView(false);
            DrawTopLayers();
            return;
        }

        Widget::Draw();
    }

    void PipelineNodeWidget::DrawPorts()
    {
        if (mCulled || !mResEnabledInHierarchy)
            return;

        RectF rect = GetCardRect();
        auto draw = [&](const Vector<PortView>& views)
        {
            for (auto& view : views)
            {
                if (!view.circle)
                    continue;

                view.circle->size = Vec2F(portRadius * 2, portRadius * 2);
                view.circle->position = Vec2F(rect.left + view.localPos.x, rect.top - view.localPos.y);
                view.circle->Draw();
            }
        };
        draw(mInputs);
        draw(mOutputs);
    }

    void PipelineNodeWidget::OnDragged(const Vec2F& position)
    {
        auto editor = mEditor.Lock();
        if (!editor)
            return;

        editor->OnNodeDragged(Ref(this), position);
    }

    void PipelineNodeWidget::OnDragCompleted()
    {
        if (auto editor = mEditor.Lock())
            editor->OnNodeDragCompleted(Ref(this));
    }

    void PipelineNodeWidget::OnResizeDragged(const ResizeHandle& handle, const Vec2F& position)
    {
        RectF rect = mResizeStartRect;
        float minH = Math::Max(minHeight, GetAutoHeight());
        if (handle.left)
            rect.left = Math::Min(position.x, rect.right - minWidth);
        if (handle.right)
            rect.right = Math::Max(position.x, rect.left + minWidth);
        if (handle.top)
            rect.top = Math::Max(position.y, rect.bottom + minH);
        if (handle.bottom)
            rect.bottom = Math::Min(position.y, rect.top - minH);

        mNode->position = PipelineEditor::CanvasToNode(Vec2F(Math::Round(rect.left), Math::Round(rect.top)));
        mNode->size = Vec2F(Math::Round(rect.Width()), Math::Round(rect.Height()));
        UpdateFromNode();
        if (auto editor = mEditor.Lock())
            editor->OnNodeResized(Ref(this), false);
    }

    void PipelineNodeWidget::OnResizeCompleted()
    {
        mResizing = false;
        if (auto editor = mEditor.Lock())
            editor->OnNodeResized(Ref(this), true);
    }

    void PipelineNodeWidget::OnPlayPressed()
    {
        if (auto editor = mEditor.Lock())
        {
            bool busy = mRuntime.state == "running" || mRuntime.state == "queued";
            if (busy)
                editor->StopRun();
            else
                editor->RunNode(mNode->id, true);
        }
    }

    void PipelineNodeWidget::OnAddInputPressed()
    {
        if (auto editor = mEditor.Lock())
            editor->AddCustomInput(Ref(this));
    }

    void PipelineNodeWidget::OnCustomInputRenamed(const String& portId, const String& name)
    {
        if (auto editor = mEditor.Lock())
            editor->RenameCustomInput(Ref(this), portId, name);
    }

    void PipelineNodeWidget::OnCustomInputDeleted(const String& portId)
    {
        if (auto editor = mEditor.Lock())
            editor->RemoveCustomInput(Ref(this), portId);
    }

    void PipelineNodeWidget::OpenContextMenu()
    {
        if (auto editor = mEditor.Lock())
            editor->OpenNodeContextMenu(Ref(this));
    }
}
// --- META ---

DECLARE_CLASS(Editor::PipelineNodeWidget, Editor__PipelineNodeWidget);
// --- END META ---
