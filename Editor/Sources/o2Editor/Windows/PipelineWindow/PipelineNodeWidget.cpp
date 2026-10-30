#include "o2Editor/stdafx.h"
#include "PipelineNodeWidget.h"

#include "o2/Render/Render.h"
#include "o2/Render/Sprite.h"
#include "o2/Assets/Types/FontAsset.h"
#include "o2/Render/Text.h"
#include "o2/Scene/UI/UIManager.h"
#include "o2/Scene/UI/WidgetLayer.h"
#include "o2/Scene/UI/WidgetLayout.h"
#include "o2/Scene/UI/Widgets/Button.h"
#include "o2/Scene/UI/Widgets/EditBox.h"
#include "o2/Scene/UI/Widgets/Label.h"
#include "o2/Utils/Editor/EditorScope.h"
#include "o2/Utils/System/Time/Time.h"
#include "o2Editor/Windows/PipelineWindow/PipelineControls.h"
#include "o2Editor/Windows/PipelineWindow/PipelineDraw.h"
#include "o2Editor/Windows/PipelineWindow/PipelineTheme.h"
#include "o2Editor/Windows/PipelineWindow/PipelineEditor.h"
#include "o2Editor/Windows/PipelineWindow/PipelineNodeBody.h"
#include "o2/Render/VectorSprite.h"

namespace Editor
{
    // AssetsLine card metrics (frontend constants.ts, NodeView.tsx)
    const float PipelineNodeWidget::headerHeight = 36.0f;
    const float PipelineNodeWidget::padTop = 8.0f;
    const float PipelineNodeWidget::portRow = 20.0f;
    const float PipelineNodeWidget::padBottom = 11.0f;
    const float PipelineNodeWidget::portRadius = 6.0f;
    const float PipelineNodeWidget::portHitRadius = 14.0f;
    const float PipelineNodeWidget::defaultWidth = 312.0f;
    const float PipelineNodeWidget::minWidth = 200.0f;
    const float PipelineNodeWidget::minHeight = 80.0f;
    static const int farUpdateFrames = 3;
    static const float cornerRadius = 13.0f;
    static const float runWidth = 40.0f;
    static const float runHeight = 22.0f;
    static const float headerInset = 8.0f;

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

        // The card art is drawn from the theme palette; the style's art layers of older builds go
        for (auto name : { "shadow", "back", "header", "kindTint", "headerLine" })
        {
            if (GetLayer(name))
                RemoveLayer(String(name));
        }

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

        // Handles drawn later take the cursor first, so the edges win over the card drag. Zoomed far out the edges are
        // a pixel wide: their handles are not drawn, so the cursor does not test eight of them per card every frame
        onDraw = [this]()
        {
            dragHandle->Draw();
            if (!mDetailed)
                return;

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
        if (t.StartsWith("finish")) return "ui/pipeline/node_finish.svg";
        if (t == "sourceText" || t == "textCompose" || t == "textConcat") return "ui/pipeline/node_text.svg";
        if (t == "sourceImage") return "ui/pipeline/node_image.svg";
        if (t == "removeBackground" || t == "aiRemoveBg") return "ui/pipeline/node_removebg.svg";
        if (t == "sourceAudio" || t == "sfxGen") return "ui/pipeline/node_audio.svg";
        if (t == "audioProcess") return "ui/pipeline/node_process.svg";
        if (t == "ttsSpeech") return "ui/pipeline/node_voice.svg";
        if (t == "musicGen") return "ui/pipeline/node_music.svg";
        if (t == "videoGen") return "ui/pipeline/node_video.svg";
        if (t == "composer") return "ui/pipeline/node_composer.svg";
        if (t == "drawImage") return "ui/pipeline/node_draw.svg";
        if (t == "imageExtract") return "ui/pipeline/node_extract.svg";
        if (t == "imageEdit") return "ui/pipeline/node_edit.svg";
        if (t == "promptGen") return "ui/pipeline/node_prompt.svg";
        if (t == "aiUpscale") return "ui/pipeline/node_upscale.svg";
        if (t == "imageToUi") return "ui/pipeline/node_ui.svg";
        if (t == "imageOutline" || t == "imageShadow" || t == "imageGradient" || t == "imageColor") return "ui/pipeline/node_effect.svg";
        return "ui/pipeline/node_ai.svg";
    }

    String PipelineNodeWidget::MenuIconForType(const String& nodeType)
    {
        // The dark menu tints the white card icons; the light one has its own teal ones
        if (PipelineTheme::Get().dark)
            return IconForType(nodeType);

        return IconForType(nodeType).ReplacedAll("/node_", "/menu_node_");
    }

    void PipelineNodeWidget::BuildHeader()
    {
        auto& theme = PipelineTheme::Get();

        auto icon = mmake<VectorSprite>(IconForType());
        icon->color = theme.fgDim;
        mIconLayer = AddLayer("icon", icon, Layout(Vec2F(0, 1), Vec2F(0, 1), Vec2F(11, -25.5f), Vec2F(25, -11.5f)));

        auto title = mmake<Text>(PipelineTheme::Font(600));
        title->text = mSchema ? mSchema->label : mNode->nodeType;
        title->height = PipelineTheme::Size(12.0f);
        title->horAlign = HorAlign::Left;
        title->verAlign = VerAlign::Middle;
        title->dotsEngings = true;
        title->color = theme.fg;
        mTitleLayer = AddLayer("title", title, Layout(Vec2F(0, 1), Vec2F(1, 1), Vec2F(32, -headerHeight), Vec2F(-headerInset, 0)));

        // ▶ pill: accent fill and ink glyph; while running an outlined stop
        mPlayButton = mmake<Button>();
        mPlayButton->name = "play";
        *mPlayButton->layout = WidgetLayout(Vec2F(1, 1), Vec2F(1, 1), Vec2F(-headerInset - runWidth, -7.5f - runHeight), Vec2F(-headerInset, -7.5f));
        auto pill = [&](const Color4& color)
        {
            auto rect = mmake<PipelineRoundedRect>();
            rect->radius = runHeight * 0.5f;
            rect->roundBottom = true;
            rect->color = color;
            return rect;
        };
        mPlayButton->AddLayer("back", pill(theme.accent), Layout::BothStretch());
        mPlayButton->AddLayer("hover", pill(theme.accentHover), Layout::BothStretch())->transparency = 0.0f;
        mPlayButton->AddLayer("stopHover", pill(theme.accentSoft), Layout::BothStretch())->transparency = 0.0f;
        auto stopFrame = mmake<PipelineRoundedFrame>();
        stopFrame->radius = runHeight * 0.5f;
        stopFrame->width = 1.5f;
        stopFrame->color = theme.accent;
        mPlayButton->AddLayer("stopFrame", stopFrame, Layout::BothStretch(-0.75f, -0.75f, -0.75f, -0.75f));
        auto glyph = mmake<VectorSprite>("ui/pipeline/al_play.svg");
        glyph->color = theme.accentInk;
        mPlayButton->AddLayer("icon", glyph, Layout(Vec2F(0.5f, 0.5f), Vec2F(0.5f, 0.5f), Vec2F(-4.5f, -5.5f), Vec2F(6.5f, 5.5f)));
        mPlayButton->AddState("hover", AnimationClip::EaseInOut("layer/hover/transparency", 0.0f, 1.0f, 0.12f))->offStateAnimationSpeed = 0.5f;
        mPlayButton->onClick = THIS_FUNC(OnPlayPressed);
        AddChild(mPlayButton);

        // The dot carries the result state; the outline is left to selection, running and errors
        auto dot = mmake<PipelineRoundedRect>();
        dot->radius = 3.5f;
        dot->roundBottom = true;
        mDotLayer = AddLayer("dot", dot, Layout(Vec2F(1, 1), Vec2F(1, 1), Vec2F(-62.0f, -22.0f), Vec2F(-55.0f, -15.0f)), 1.0f);
        auto ring = mmake<PipelineRoundedFrame>();
        ring->radius = 4.25f;
        ring->width = 1.5f;
        mDotRingLayer = AddLayer("dotRing", ring, Layout(Vec2F(1, 1), Vec2F(1, 1), Vec2F(-62.75f, -22.75f), Vec2F(-54.25f, -14.25f)), 1.0f);

        mErrorButton = mmake<Button>();
        mErrorButton->name = "error";
        *mErrorButton->layout = WidgetLayout(Vec2F(1, 1), Vec2F(1, 1), Vec2F(-76, -28), Vec2F(-56, -8));
        auto errorBack = mmake<PipelineRoundedRect>();
        errorBack->radius = 10.0f;
        errorBack->roundBottom = true;
        errorBack->color = theme.red;
        mErrorButton->AddLayer("back", errorBack, Layout::BothStretch());
        auto errorIcon = mmake<VectorSprite>("ui/pipeline/btn_error.svg");
        errorIcon->color = Color4::White();
        mErrorButton->AddLayer("icon", errorIcon, Layout::BothStretch(3, 3, 3, 3));
        mErrorButton->onClick = [this]() { if (auto e = mEditor.Lock()) e->ShowNodeError(Ref(this)); };
        mErrorButton->enabled = false;
        AddChild(mErrorButton);

        mRetryLabel = mmake<Label>();
        mRetryLabel->name = "retry";
        mRetryLabel->SetFontAsset(AssetRef<FontAsset>(PipelineTheme::Font(600)));
        mRetryLabel->height = PipelineTheme::Size(10.0f);
        mRetryLabel->color = Color4(26, 26, 26, 255);
        mRetryLabel->horAlign = HorAlign::Right;
        mRetryLabel->verAlign = VerAlign::Middle;
        auto retryBack = mmake<PipelineRoundedRect>();
        retryBack->radius = 3.0f;
        retryBack->roundBottom = true;
        retryBack->color = theme.yellow;
        mRetryLabel->AddLayer("back", retryBack, Layout::BothStretch(), -1.0f);
        auto retryIcon = mmake<VectorSprite>("ui/pipeline/btn_loop.svg");
        retryIcon->color = Color4(0, 0, 0, 255);
        mRetryLabel->AddLayer("icon", retryIcon, Layout(Vec2F(0, 0.5f), Vec2F(0, 0.5f), Vec2F(4, -6), Vec2F(16, 6)));
        mRetryLabel->enabled = false;
        AddChild(mRetryLabel);

        // Only the click area: the button is drawn with the ports, over the card frame
        mAddInputButton = mmake<Button>();
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
        mWarm = false;
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
        auto& theme = PipelineTheme::Get();

        // A finish node writes what reaches it: it is run from its own Save button or from Run all, not from a header play
        bool runnable = mSchema && mSchema->category != PipelineNodeCategory::Source &&
            !PipelineNodeRegistry::IsFinishType(mNode->nodeType) && (!mSchema->inputs.IsEmpty() || !mSchema->outputs.IsEmpty());
        bool selfApplying = mSchema && mSchema->instant && mSchema->category != PipelineNodeCategory::Output;
        mPlayButton->enabled = runnable && !selfApplying;

        bool busy = mRuntime.state == "running" || mRuntime.state == "queued";
        mPlayButton->GetLayer("back")->enabled = !busy;
        mPlayButton->GetLayer("hover")->enabled = !busy;
        mPlayButton->GetLayer("stopFrame")->enabled = busy;
        mPlayButton->GetLayer("stopHover")->enabled = busy;
        if (auto icon = mPlayButton->SetLayerImage("icon", busy ? "ui/pipeline/al_stop.svg" : "ui/pipeline/al_play.svg"))
            icon->color = busy ? theme.accent : theme.accentInk;
        if (auto iconLayer = mPlayButton->GetLayer("icon"))
            iconLayer->layout = busy ? Layout(Vec2F(0.5f, 0.5f), Vec2F(0.5f, 0.5f), Vec2F(-5, -5), Vec2F(5, 5))
                                     : Layout(Vec2F(0.5f, 0.5f), Vec2F(0.5f, 0.5f), Vec2F(-4.5f, -5.5f), Vec2F(6.5f, 5.5f));

        bool hasError = mRuntime.state == "error" && !mRuntime.error.IsEmpty();
        bool retry = mRuntime.retryAttempt > 0 && busy;

        // Header controls right to left, each slot taken only when its control is shown (NodeView.tsx)
        float cursor = -headerInset;
        if (mPlayButton->enabled)
            cursor -= runWidth + 8.0f;

        bool showDot = mPlayButton->enabled && !hasError && !retry;
        float dotCx = cursor - 2.5f;
        if (showDot)
            cursor -= 15.0f;

        if (mDotLayer)
        {
            mDotLayer->enabled = false;
            mDotRingLayer->enabled = false;
            const String& state = mRuntime.state;
            bool hollow = state == "queued" || (state != "running" && !mRuntime.fresh && !mRuntime.output.IsValid() && state != "error");
            Color4 color = state == "running" ? theme.accent
                : state == "queued" ? theme.accent
                : state == "error" ? theme.red
                : mRuntime.fresh ? theme.green
                : mRuntime.output.IsValid() ? theme.yellow : theme.fg3;
            if (showDot)
            {
                auto& layer = hollow ? mDotRingLayer : mDotLayer;
                layer->enabled = true;
                layer->GetDrawable()->color = color;
                float half = hollow ? 4.25f : 3.5f;
                layer->layout = Layout(Vec2F(1, 1), Vec2F(1, 1), Vec2F(dotCx - half, -18.5f - half), Vec2F(dotCx + half, -18.5f + half));
            }
        }

        mErrorButton->enabled = hasError;
        float errorX = cursor - 20.0f;
        if (hasError)
        {
            *mErrorButton->layout = WidgetLayout(Vec2F(1, 1), Vec2F(1, 1), Vec2F(errorX, -28), Vec2F(errorX + 20, -8));
            cursor -= 26.0f;
        }

        mRetryLabel->enabled = retry;
        float retryX = cursor - 32.0f;
        if (retry)
        {
            mRetryLabel->text = (String)mRuntime.retryAttempt + "/" + (String)mRuntime.retryMax;
            *mRetryLabel->layout = WidgetLayout(Vec2F(1, 1), Vec2F(1, 1), Vec2F(retryX, -28), Vec2F(retryX + 32, -8));
            cursor -= 38.0f;
        }

        if (mTitleLayer)
            mTitleLayer->layout = Layout(Vec2F(0, 1), Vec2F(1, 1), Vec2F(32, -headerHeight), Vec2F(cursor - 4.0f, 0));
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

        auto makeCircle = [&](const PipelinePort& port) -> Ref<IRectDrawable>
        {
            auto marker = mmake<PipelinePortMarker>();
            marker->portType = port.portType;
            marker->color = ColorOfPortType(port.portType);
            marker->size = Vec2F(portRadius * 2, portRadius * 2);
            return marker;
        };

        auto makeLabel = [&](const PipelinePort& port, bool input)
        {
            auto text = mmake<Text>(PipelineTheme::Font(400));
            text->text = port.name;
            text->height = PipelineTheme::Size(11.0f);
            text->horAlign = input ? HorAlign::Left : HorAlign::Right;
            text->verAlign = VerAlign::Middle;
            text->dotsEngings = true;
            text->color = PipelineTheme::Get().fgDim;
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
                // .port-name-input: 11 px dim text, a border only while hovered or edited; the remove button shows on hover
                auto& theme = PipelineTheme::Get();
                view.nameEdit = PipelineControls::MakeEditBox(port.name, false);
                view.nameEdit->name = "portName_" + port.id;
                view.nameEdit->layout->minSize = Vec2F(20, 16);
                for (auto layerName : { "back", "border" })
                {
                    if (auto layer = view.nameEdit->GetLayer(layerName))
                        layer->transparency = 0.0f;
                }
                if (auto focus = view.nameEdit->GetLayer("focus"))
                    if (auto frame = DynamicCast<PipelineRoundedFrame>(focus->GetDrawable()))
                        frame->radius = 3.0f;
                PipelineControls::AddBorderLayer(*view.nameEdit, "hover", theme.border, 3.0f, 1.0f, Layout::BothStretch(), -0.6f)->transparency = 0.0f;
                view.nameEdit->AddState("hover", AnimationClip::EaseInOut("layer/hover/transparency", 0.0f, theme.border.a / 255.0f, 0.12f))
                    ->offStateAnimationSpeed = 0.5f;
                view.nameEdit->SetViewLayout(Layout::BothStretch(3, 0, 3, 0));
                if (auto textDrawable = view.nameEdit->GetTextDrawable())
                {
                    textDrawable->height = PipelineTheme::Size(11.0f);
                    textDrawable->color = theme.fgDim;
                }
                String portId = port.id;
                auto weakThis = WeakRef(this);
                view.nameEdit->onChangeCompleted = [weakThis, portId](const WString& text)
                {
                    if (auto self = weakThis.Lock()) self->OnCustomInputRenamed(portId, (String)text);
                };
                AddChild(view.nameEdit);

                view.deleteButton = PipelineControls::MakeIconButton("ui/pipeline/al_cmp_trash.svg", theme.fg3, Color4(0, 0, 0, 0));
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

        if (mAddLabelLayer)
        {
            RemoveLayer(mAddLabelLayer);
            mAddLabelLayer = nullptr;
        }
        if (mAddInputButton->enabled)
        {
            auto text = mmake<Text>(PipelineTheme::Font(400));
            text->text = "+ input";
            text->height = PipelineTheme::Size(10.0f);
            text->horAlign = HorAlign::Left;
            text->verAlign = VerAlign::Middle;
            text->color = PipelineTheme::Get().fg3;
            mAddLabelLayer = AddLayer("addLabel", text, Layout::Based(BaseCorner::LeftTop, Vec2F(80, portRow)), 2.0f);
        }
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

    bool PipelineNodeWidget::IsBodyPort(const String& portId, Vec2F& offset) const
    {
        return mBody && mBody->GetBodyPortOffset(portId, offset);
    }

    float PipelineNodeWidget::GetPortsHeight() const
    {
        bool inputsInBody = mBody && mBody->InputsInBody();
        int extra = (mSchema && !mSchema->addableInputs.IsEmpty() && !inputsInBody) ? 1 : 0;
        int outputs = 0;
        for (auto& port : mNode->outputs)
        {
            if (!mBody || !mBody->HasBodyPort(port.id))
                outputs++;
        }
        int rows = Math::Max(inputsInBody ? 0 : mNode->inputs.Count() + extra, outputs);
        return rows * portRow;
    }

    float PipelineNodeWidget::GetBodyAreaHeight() const
    {
        return GetCardSize().y - (headerHeight + padTop + GetPortsHeight()) - padBottom;
    }

    float PipelineNodeWidget::GetBodyHeight() const
    {
        return mBody ? mBody->GetPreferredHeight(GetCardWidth()) : 0.0f;
    }

    float PipelineNodeWidget::GetAutoHeightForWidth(float width) const
    {
        float body = mBody ? mBody->GetPreferredHeight(width) : 0.0f;
        return headerHeight + padTop + GetPortsHeight() + body + padBottom;
    }

    float PipelineNodeWidget::GetAutoHeight() const
    {
        return GetAutoHeightForWidth(GetCardWidth());
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
        float bodyTop = headerHeight + padTop + GetPortsHeight();
        bool inputsInBody = mBody && mBody->InputsInBody();
        if (inputsInBody)
            mAddInputButton->enabled = false;

        for (int i = 0; i < mInputs.Count(); i++)
        {
            auto& view = mInputs[i];

            // The body's rows carry the inputs: the marker sits on the left edge, the row names and removes it
            Vec2F offset;
            if (inputsInBody)
            {
                view.localPos = IsBodyPort(view.port.id, offset) ? Vec2F(0, bodyTop + offset.y) : Vec2F(0, bodyTop);
                if (view.label)
                    view.label->enabled = false;
                if (view.nameEdit)
                    view.nameEdit->SetEnabledForcible(false);
                if (view.deleteButton)
                    view.deleteButton->SetEnabledForcible(false);
                continue;
            }

            float cy = headerHeight + padTop + (i + 0.5f) * portRow;
            view.localPos = Vec2F(1, cy);
            float rightReserve = i < mOutputs.Count() ? 60.0f : 16.0f;
            if (view.label)
                view.label->layout = Layout(Vec2F(0, 1), Vec2F(1, 1), Vec2F(13, -cy - 1 - portRow / 2), Vec2F(-rightReserve, -cy - 1 + portRow / 2));
            if (view.nameEdit)
            {
                float avail = Math::Max(40.0f, size.x - 14 - rightReserve);
                float inputW = Math::Clamp(view.port.name.Length() * 7.0f + 12.0f, 34.0f, avail - 24.0f);
                *view.nameEdit->layout = WidgetLayout(Vec2F(0, 1), Vec2F(0, 1), Vec2F(9, -cy - 8), Vec2F(9 + inputW, -cy + 8));
                *view.deleteButton->layout = WidgetLayout(Vec2F(0, 1), Vec2F(0, 1), Vec2F(11 + inputW, -cy - 8), Vec2F(27 + inputW, -cy + 8));
            }
        }

        int row = 0;
        for (auto& view : mOutputs)
        {
            // An output the body places (a part of the extract grid) sits in the body, without a row of its own
            Vec2F offset;
            if (IsBodyPort(view.port.id, offset))
            {
                mBodyPortPasses = 2;
                view.localPos = Vec2F(offset.x, bodyTop + offset.y);
                if (view.label)
                    view.label->enabled = false;
                continue;
            }

            float cy = headerHeight + padTop + (row + 0.5f) * portRow;
            row++;
            view.localPos = Vec2F(size.x - 1, cy);
            if (view.label)
            {
                view.label->enabled = true;
                view.label->layout = Layout(Vec2F(0, 1), Vec2F(1, 1), Vec2F(13, -cy - 1 - portRow / 2), Vec2F(-13, -cy - 1 + portRow / 2));
            }
        }

        if (mAddInputButton->enabled)
        {
            float cy = headerHeight + padTop + (mInputs.Count() + 0.5f) * portRow;
            *mAddInputButton->layout = WidgetLayout(Vec2F(0, 1), Vec2F(0, 1), Vec2F(-8, -cy - 8), Vec2F(80, -cy + 8));
        }
        if (mAddLabelLayer)
        {
            float cy = headerHeight + padTop + (mInputs.Count() + 0.5f) * portRow;
            mAddLabelLayer->enabled = mAddInputButton->enabled;
            mAddLabelLayer->layout = Layout(Vec2F(0, 1), Vec2F(0, 1), Vec2F(13, -cy - portRow / 2), Vec2F(93, -cy + portRow / 2));
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
        return GetPortPosition(portId, input, GetCardRect());
    }

    Vec2F PipelineNodeWidget::GetPortPosition(const String& portId, bool input, const RectF& rect) const
    {
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
        if (mBody && mBody->InputsInBody())
            return mSchema && !mSchema->addableInputs.IsEmpty() && mBody->IsAddInputAt(p);

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
    }

    void PipelineNodeWidget::ApplyRuntime()
    {
        PushEditorScopeOnStack scope;
        if (mRuntime.state != "running" && mRuntime.state != "queued")
            mRuntime.portStates.Clear();

        UpdateHeaderButtons();
        if (mBody)
            mBody->OnRunStateChanged();
    }

    void PipelineNodeWidget::OnOutputChanged()
    {
        PushEditorScopeOnStack scope;
        mFarUpdateFrames = farUpdateFrames;
        if (!mBody)
            return;

        // A pair row grows once an image arrives on either side; the ports the body places follow what moved inside it
        float before = GetAutoHeight();
        mBody->OnOutputChanged();
        if (!Math::Equals(before, GetAutoHeight()))
            UpdateFromNode();
        else
            LayoutPorts();
    }

    void PipelineNodeWidget::OnConfigChanged()
    {
        PushEditorScopeOnStack scope;
        mFarUpdateFrames = farUpdateFrames;
        if (mBody)
            mBody->OnConfigChanged();
    }

    void PipelineNodeWidget::Update(float dt)
    {
        if (mCulled)
            return;

        mWarm = true;
        Widget::Update(dt);

        // A custom input's remove button shows while the cursor is on its row (opacity 0 until hover in AssetsLine)
        auto editor = mEditor.Lock();
        if (!editor)
            return;

        Vec2F cursor = editor->ScreenToLocalPoint(o2Input.GetCursorPos());
        for (auto& view : mInputs)
        {
            if (!view.deleteButton || !view.nameEdit)
                continue;

            RectF row = view.nameEdit->layout->GetWorldRect();
            row.right = view.deleteButton->layout->GetWorldRect().right;
            bool hovered = row.IsInside(cursor) || view.deleteButton->IsPressed();
            float target = hovered ? 1.0f : 0.0f;
            if (!Math::Equals(view.deleteButton->transparency, target))
                view.deleteButton->transparency = target;
        }
    }

    void PipelineNodeWidget::WarmUp(float dt)
    {
        PushEditorScopeOnStack scope;
        mWarm = true;
        Widget::Update(dt);
        Widget::UpdateChildren(dt);
    }

    void PipelineNodeWidget::SetParamsOpen(bool open)
    {
        KeepAreaThrough([&]()
        {
            mNode->SetConfigBool("paramsOpen", open);
            Rebuild();
        });
    }

    void PipelineNodeWidget::KeepAreaThrough(const Function<void()>& change)
    {
        PushEditorScopeOnStack scope;
        Ref<PipelineNodeWidget> self(this);
        auto editor = mEditor.Lock();
        bool sized = mNode->size.y > 0.0f;
        float shown = GetCardSize().y, before = GetAutoHeight();
        if (sized && editor)
            editor->OnNodeResized(self, false);

        change();
        if (sized)
            mNode->size.y = Math::Max(minHeight, shown + GetAutoHeight() - before);

        UpdateFromNode();
        if (sized && editor)
            editor->OnNodeResized(self, true);
    }

    void PipelineNodeWidget::UpdateChildren(float dt)
    {
        if (mCulled)
            return;

        if (!mDetailed)
        {
            bool loading = mBody && mBody->IsLoadingMedia();
            if (mFarUpdateFrames <= 0 && !loading)
                return;

            if (mFarUpdateFrames > 0)
                mFarUpdateFrames--;
            mBodyHost->Update(dt);
            mBodyHost->UpdateChildren(dt);
        }
        else
            Widget::UpdateChildren(dt);

        // The body's widgets snap to pixels as they update: the ports placed on them follow for an update or two after a change
        if (mBodyPortPasses > 0)
        {
            int left = mBodyPortPasses - 1;
            LayoutPorts();
            mBodyPortPasses = left;
        }
    }

    void PipelineNodeWidget::Draw()
    {
        if (mCulled || !mResEnabledInHierarchy)
            return;

        DrawCardBack();

        if (!mDetailed)
        {
            DrawLayers();
            OnDrawn();
            PipelineControls::SetFarView(true);
            mBodyHost->Draw();
            PipelineControls::SetFarView(false);
            // AssetsLine keeps ▶ on screen at any zoom
            if (mPlayButton->IsEnabled())
                mPlayButton->Draw();
            DrawTopLayers();
            DrawOutline();
            return;
        }

        Widget::Draw();
        DrawOutline();
        DrawBusyBar();
    }

    bool PipelineNodeWidget::IsBusyInBackground() const
    {
        if (mRuntime.applying)
            return true;

        auto editor = mEditor.Lock();
        return editor && editor->IsWaitingForPictures(mNode->id);
    }

    void PipelineNodeWidget::DrawBusyBar()
    {
        if (!IsBusyInBackground())
            return;

        // A segment sweeping along the header's bottom edge, two pixels tall on screen
        auto editor = mEditor.Lock();
        float scale = editor ? editor->GetCamera().GetScale2D().x : 1.0f;
        RectF rect = layout->GetWorldRect();
        float height = Math::Max(1.0f, 2.0f*scale);
        float y = rect.top - headerHeight;
        float width = rect.Width();
        float phase = Math::Mod((float)o2Time.GetApplicationTime()*0.8f, 1.0f);
        float from = rect.left + width*Math::Max(0.0f, phase*1.4f - 0.4f);
        float to = rect.left + width*Math::Min(1.0f, phase*1.4f);
        Color4 color = PipelineTheme::Get().accent;
        o2Render.DrawFilledPolygon({ Vec2F(from, y - height), Vec2F(from, y), Vec2F(to, y), Vec2F(to, y - height) }, color);
    }

    void PipelineNodeWidget::DrawCardBack()
    {
        auto& theme = PipelineTheme::Get();
        RectF rect = layout->GetWorldRect();
        PipelineDraw::Corners corners(cornerRadius);

        // Zoomed far out the shadow goes, as AssetsLine drops the filter under .lod-far
        if (mDetailed && theme.cardShadow.a > 0)
        {
            RectF shadow(rect.left, rect.top - theme.shadowOffset, rect.right, rect.bottom - theme.shadowOffset);
            PipelineDraw::Shadow(shadow, corners, PipelineTheme::Alpha(theme.cardShadow, theme.card.a / 255.0f), theme.shadowBlur);
        }

        PipelineDraw::Fill(rect, corners, theme.card);

        if (theme.headerBand.a > 0)
        {
            RectF band(rect.left, rect.top, rect.right, rect.top - headerHeight);
            PipelineDraw::Fill(band, PipelineDraw::Corners(cornerRadius, 0.0f), theme.headerBand);
            float px = PipelineDraw::Pixel();
            RectF line(rect.left, rect.top - headerHeight + px, rect.right, rect.top - headerHeight);
            PipelineDraw::Fill(line, PipelineDraw::Corners(0.0f), theme.headerLine);
        }
    }

    void PipelineNodeWidget::DrawOutline()
    {
        auto& theme = PipelineTheme::Get();
        auto editor = mEditor.Lock();
        float scale = editor ? editor->GetCamera().GetScale2D().x : 1.0f;
        float line = Math::Max(1.0f, scale);

        RectF rect = layout->GetWorldRect();
        PipelineDraw::Corners corners(cornerRadius);

        // At rest a glassy edge in the dark theme (white 22% -> 8% -> 6%, node-stroke in Canvas.tsx), a flat border in the light one
        const String& s = mRuntime.state;
        if (theme.dark)
        {
            PipelineDraw::Gradient stroke(theme.cardStrokeTop, 0.45f, theme.cardStrokeMid, theme.cardStrokeBottom);
            PipelineDraw::Stroke(rect, corners, 1.5f - line * 0.5f, 1.5f + line * 0.5f, stroke);
            PipelineDraw::Line(Vec2F(rect.left + cornerRadius + 2.0f, rect.top - 1.5f), Vec2F(rect.right - cornerRadius - 2.0f, rect.top - 1.5f),
                               line, theme.cardHighlight);
        }
        else
            PipelineDraw::Stroke(rect, corners, 1.5f - line * 0.5f, 1.5f + line * 0.5f, PipelineDraw::Gradient(theme.cardStrokeTop));

        // Running pulses the accent, queued is a quiet accent, an error is red (NodeView .node-outline states)
        Color4 state(0, 0, 0, 0);
        float width = Math::Max(1.5f, scale);
        if (s == "error")
            state = theme.red;
        else if (s == "running")
        {
            float t = (float)o2Time.GetApplicationTime();
            float pulse = 0.55f + 0.45f * (0.5f - 0.5f * Math::Cos(t * Math::PI() * 2.0f / 1.4f));
            state = PipelineTheme::Alpha(theme.accent, pulse);
        }
        else if (s == "queued")
        {
            state = PipelineTheme::Alpha(theme.accent, 0.6f);
            width = line;
        }
        if (state.a > 0)
            PipelineDraw::Stroke(rect, corners, 1.5f - width * 0.5f, 1.5f + width * 0.5f, PipelineDraw::Gradient(state));

        if (mSelected)
        {
            RectF ring(rect.left - 4.0f, rect.top + 4.0f, rect.right + 4.0f, rect.bottom - 4.0f);
            PipelineDraw::Corners ringCorners(cornerRadius + 3.0f);
            float w = 1.5f * scale;
            PipelineDraw::Stroke(ring, ringCorners, -5.0f * scale, 5.0f * scale, PipelineDraw::Gradient(PipelineTheme::Alpha(theme.accent, 0.08f)));
            PipelineDraw::Stroke(ring, ringCorners, -2.0f * scale, 2.0f * scale, PipelineDraw::Gradient(PipelineTheme::Alpha(theme.accent, 0.12f)));
            PipelineDraw::Stroke(ring, ringCorners, -w * 0.5f, w * 0.5f, PipelineDraw::Gradient(theme.accent));
        }
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

        if (mAddInputButton->enabled)
        {
            auto& theme = PipelineTheme::Get();
            float cy = headerHeight + padTop + (mInputs.Count() + 0.5f)*portRow;
            Vec2F center(rect.left + 1.0f, rect.top - cy);
            bool hover = mAddInputButton->IsPressed() || mAddInputButton->IsUnderPoint(o2Input.GetCursorPos());
            PipelineDraw::Circle(center, 5.5f, theme.cardSolid);
            PipelineDraw::Ring(center, 5.0f, 1.0f, hover ? theme.accent : theme.fg3);
        }
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
        if (handle.left)
            rect.left = Math::Min(position.x, rect.right - minWidth);
        if (handle.right)
            rect.right = Math::Max(position.x, rect.left + minWidth);
        if (handle.top)
            rect.top = position.y;
        if (handle.bottom)
            rect.bottom = position.y;

        // Never shorter than the content at the new width: a narrower card may wrap its rows taller
        float minH = Math::Max(minHeight, GetAutoHeightForWidth(Math::Round(rect.Width())));
        if (rect.Height() < minH)
        {
            if (handle.top)
                rect.top = rect.bottom + minH;
            else
                rect.bottom = rect.top - minH;
        }

        mNode->position = PipelineEditor::CanvasToNode(Vec2F(Math::Round(rect.left), Math::Round(rect.top)));
        // A side edge sets the width alone: an automatic height stays automatic
        mNode->size = Vec2F(Math::Round(rect.Width()),
                            PipelinePairLayout::HeightAfterResize(handle.top || handle.bottom, mNode->size.y, Math::Round(rect.Height())));
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
