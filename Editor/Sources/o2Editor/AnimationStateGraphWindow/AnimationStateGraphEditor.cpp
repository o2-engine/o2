#include "o2Editor/stdafx.h"
#include "AnimationStateGraphEditor.h"

#include "o2/Application/Application.h"
#include "o2/Render/Render.h"
#include "o2/Render/Sprite.h"
#include "o2/Scene/UI/UIManager.h"
#include "o2/Scene/UI/WidgetLayer.h"
#include "o2/Scene/UI/WidgetLayout.h"
#include "o2/Scene/UI/WidgetState.h"
#include "o2/Scene/UI/Widgets/ContextMenu.h"
#include "o2/Scene/UI/Widgets/EditBox.h"
#include "o2/Scene/UI/Widgets/HorizontalLayout.h"
#include "o2/Scene/UI/Widgets/HorizontalScrollBar.h"
#include "o2/Scene/UI/Widgets/Label.h"
#include "o2/Scene/UI/Widgets/VerticalLayout.h"
#include "o2/Scene/UI/Widgets/VerticalScrollBar.h"
#include "o2/Scene/UI/Widgets/Window.h"
#include "o2/Utils/Editor/EditorScope.h"
#include "o2/Utils/Math/Interpolation.h"
#include "o2/Utils/System/Clipboard.h"
#include "o2Editor/Core/Dialogs/KeyEditDlg.h"
#include "o2Editor/Core/Properties/Properties.h"
#include "o2Editor/Core/UI/CurveEditor/CurveActions.h"
#include "o2Editor/Core/UIRoot.h"
#include "o2Editor/SceneWindow/SceneEditScreen.h"
#include "o2Editor/AnimationWindow/AnimationWindow.h"

namespace Editor
{
    AnimationStateGraphEditor::AnimationStateGraphEditor(RefCounter* refCounter):
        FrameScrollView(refCounter), SelectableDragHandlesGroup(refCounter)
    {
        mReady = false;

        mSelectionSprite = mmake<Sprite>();
        InitializeContextMenu();
        InitializeStateContextMenu();

        mBackColor = Color4(225, 232, 232, 255);
        mViewCameraMinScale = 1.0f;
        mReady = true;
    }

    AnimationStateGraphEditor::~AnimationStateGraphEditor()
    {}

	void AnimationStateGraphEditor::SetGraph(const Ref<AnimationStateGraphAsset>& graph, 
                                             const Ref<AnimationStateGraphComponent>& component)
	{
		mGraph = graph;
		mComponent = component;

		InitializeStates();

		if (mComponent)
			mComponent.Lock()->Reset();
	}

	void AnimationStateGraphEditor::InitializeStates()
	{
		for (auto& state : mStatesWidgets)
			state->RemoveWidget();

		mStatesWidgets.Clear();
		mStatesWidgetsMap.Clear();

		if (auto lastComponent = mComponent.Lock())
		{
			lastComponent->onStateStarted -= THIS_FUNC(OnStateGraphStateStarted);
			lastComponent->onStateFinished -= THIS_FUNC(OnStateGraphStateFinished);
			lastComponent->onTransitionStarted -= THIS_FUNC(OnStateGraphTransitionStarted);
			lastComponent->onTransitionFinished -= THIS_FUNC(OnStateGraphTransitionFinished);
			lastComponent->onTransitionsPlanned -= THIS_FUNC(OnStateGraphTransitionsPlanned);
		}

		if (!mGraph)
			return;

		auto graph = mGraph.Lock();
		for (auto& state : graph->GetStates())
		{
			auto stateWidget = mmake<StateWidget>(Ref(this), state);
			mStatesWidgets.Add(stateWidget);
			mStatesWidgetsMap[state] = stateWidget;
		}

		for (auto& state : mStatesWidgets)
			state->InitializeTransitions();

		if (auto component = mComponent.Lock())
		{
			component->onStateStarted += THIS_FUNC(OnStateGraphStateStarted);
			component->onStateFinished += THIS_FUNC(OnStateGraphStateFinished);
			component->onTransitionStarted += THIS_FUNC(OnStateGraphTransitionStarted);
			component->onTransitionFinished += THIS_FUNC(OnStateGraphTransitionFinished);
			component->onTransitionsPlanned += THIS_FUNC(OnStateGraphTransitionsPlanned);
		}
	}

	void AnimationStateGraphEditor::Draw()
	{
		if (!mResEnabledInHierarchy || mIsClipped)
			return;

		DrawLayers();

		Widget::OnDrawn();

		DrawInternalChildren();
		DrawTopLayers();

		if (!mReady)
			return;

		RedrawRenderTarget();
		mRenderTargetSprite->Draw();

		mListenersLayer->OnDrawn(mRenderTargetSprite->GetBasis());

		CursorAreaEventsListener::OnDrawn();

		mHorScrollbar->Draw();
		mVerScrollbar->Draw();

        o2Render.EnableScissorTest(layout->GetWorldRect());

        DrawHandles();
        DrawSelection();

		o2Render.DisableScissorTest();

		DrawDebugFrame();
	}

	void AnimationStateGraphEditor::RedrawContent()
	{
		mListenersLayer->OnBeginDraw();
		mListenersLayer->camera = o2Render.GetCamera();

		DrawGrid();

		DrawTransitions();
		DrawInheritedDepthChildren();

		mListenersLayer->OnEndDraw();
	}

	void AnimationStateGraphEditor::DrawHandles()
	{
	}

	void AnimationStateGraphEditor::DrawSelection()
	{
		if (mIsPressed && false)
		{
			mSelectionSprite->rect = RectF(LocalToScreenPoint(mSelectingPressedPoint), o2Input.cursorPos);
			mSelectionSprite->Draw();
		}
	}

	void AnimationStateGraphEditor::DrawTransitions()
	{
		for (auto& state : mStatesWidgets)
			state->DrawTransitions();
	}

    void AnimationStateGraphEditor::Update(float dt)
    {
        FrameScrollView::Update(dt);

        if (mReady && mResEnabledInHierarchy && !mIsClipped && mNeedAdjustView)
        {
            mNeedAdjustView = false;
            mViewCameraTargetScale = mAvailableArea.Size()/mViewCamera.GetSize();
            mViewCamera.center = mAvailableArea.Center();
            mViewCameraTargetPos = mViewCamera.position;
        }

		if (mComponent)
			mComponent.Lock()->GetActor()->Update(dt);
    }

    void AnimationStateGraphEditor::UpdateSelfTransform()
    {
        FrameScrollView::UpdateSelfTransform();

        UpdateLocalScreenTransforms();
        OnCameraTransformChanged();
    }

    const Ref<ContextMenu>& AnimationStateGraphEditor::GetContextMenu() const
    {
        return mContextMenu;
    }

    Ref<RefCounterable> AnimationStateGraphEditor::CastToRefCounterable(const Ref<AnimationStateGraphEditor>& ref)
    {
        return DynamicCast<FrameScrollView>(ref);
    }

    void AnimationStateGraphEditor::SetSelectionSpriteImage(const AssetRef<ImageAsset>& image)
    {
        mSelectionSprite->LoadFromImage(image);
    }

    void AnimationStateGraphEditor::OnEnabled()
    {
        FrameScrollView::OnEnabled();

        mContextMenu->SetItemsMaxPriority();
        mStateContextMenu->SetItemsMaxPriority();
    }

    void AnimationStateGraphEditor::OnDisabled()
    {
        FrameScrollView::OnDisabled();

        mContextMenu->SetItemsMinPriority();
        mStateContextMenu->SetItemsMinPriority();
    }

    void AnimationStateGraphEditor::OnScrolled(float scroll)
    {
        Vec2F newScale = mViewCameraTargetScale;

        if (o2Input.IsKeyDown(VK_CONTROL))
            newScale.x *= 1.0f - (scroll*mViewCameraScaleSence);
        else if (o2Input.IsKeyDown(VK_SHIFT))
            newScale.y *= 1.0f - (scroll*mViewCameraScaleSence);
        else
            newScale *= 1.0f - (scroll*mViewCameraScaleSence);

        ChangeCameraScaleRelativeToCursor(newScale);
    }

	void AnimationStateGraphEditor::OnCursorPressed(const Input::Cursor& cursor)
	{
		Focus();

		mSelectingPressedPoint = cursor.position;
	}

	void AnimationStateGraphEditor::OnCursorReleased(const Input::Cursor& cursor)
	{

	}

	void AnimationStateGraphEditor::OnCursorStillDown(const Input::Cursor& cursor)
	{

	}

	void AnimationStateGraphEditor::OnCursorRightMouseStayDown(const Input::Cursor& cursor)
	{
		FrameScrollView::OnCursorRightMouseStayDown(cursor);
	}

	void AnimationStateGraphEditor::OnCursorRightMouseReleased(const Input::Cursor& cursor)
	{
		FrameScrollView::OnCursorRightMouseReleased(cursor);
	}

	void AnimationStateGraphEditor::InitializeContextMenu()
    {
        mContextMenu = o2UI.CreateWidget<ContextMenu>();

        onShow = [&]() { mContextMenu->SetItemsMaxPriority(); };
        onHide = [&]() { mContextMenu->SetItemsMinPriority(); };

        AddChild(mContextMenu);
    }

    void AnimationStateGraphEditor::InitializeStateContextMenu()
    {
        mStateContextMenu = o2UI.CreateWidget<ContextMenu>();
        
        // Add state-specific menu items
        mStateContextMenu->AddItem(WString("Set as default"), [this]() {
            if (auto stateWidget = mContextMenuStateTarget.Lock()) {
                auto stateGraphAsset = mGraph.Lock();
                if (stateGraphAsset && stateWidget->state) {
                    // Use state's name to set as initial state
                    stateGraphAsset->SetInitialState(stateWidget->state.Lock()->name);
                }
            }
        });

        mStateContextMenu->AddItem(WString("Add animation"), [this]() {
            if (auto stateWidget = mContextMenuStateTarget.Lock()) {
                auto statePtr = stateWidget->state.Lock();
                if (statePtr) {
                    // Create a new animation with a default name
                    String newAnimName = "New Animation";
                    auto newAnimation = statePtr->AddAnimation(newAnimName);

                    // Add to UI
                    Ref<StateAnimation> stateAnimation = mmake<StateAnimation>();
                    stateAnimation->name = newAnimation->name;
                    stateAnimation->animation = newAnimation;
                    stateAnimation->owner = stateWidget;
                    stateWidget->animations.Add(stateAnimation);

                    // Refresh the animations list property
                    if (stateWidget->animationsListProperty)
                        stateWidget->animationsListProperty->Refresh();
                }
            }
        });

        mStateContextMenu->AddItem(WString("Add transition"), [this]() {
            if (auto stateWidget = mContextMenuStateTarget.Lock()) {
                // Find another state to transition to
                auto thisState = stateWidget->state.Lock();
                if (!thisState || !mGraph)
                    return;
                
                auto stateGraphAsset = mGraph.Lock();
                if (!stateGraphAsset)
                    return;
                
                auto states = stateGraphAsset->GetStates();
                
                // Find another state to transition to
                Ref<AnimationGraphState> targetState;
                for (auto& otherState : states) {
                    if (otherState != thisState) {
                        targetState = otherState;
                        break;
                    }
                }

                if (targetState) {
                    // Create transition
                    auto transition = thisState->AddTransition(targetState);

                    // Add visual representation
                    Ref<StateTransition> stateTransition = mmake<StateTransition>();
                    stateTransition->owner = stateWidget;
                    stateTransition->destination = mStatesWidgetsMap[targetState];
                    stateWidget->transitions.Add(stateTransition);
                    stateWidget->transitionsMap[WeakRef<AnimationGraphTransition>(transition)] = stateTransition;
                }
            }
        });

        mStateContextMenu->AddItem(WString("Delete state"), [this]() {
            if (auto stateWidget = mContextMenuStateTarget.Lock()) {
                auto stateGraphAsset = mGraph.Lock();
                if (!stateGraphAsset)
                    return;

                auto thisState = stateWidget->state.Lock();
                if (thisState) {
                    stateGraphAsset->RemoveState(thisState);
                    InitializeStates();
                }
            }
        });

        AddChild(mStateContextMenu);
    }

    void AnimationStateGraphEditor::RecalculateViewArea()
    {
        // Initialize with first state position if available
        if (!mStatesWidgets.IsEmpty())
            mAvailableArea = RectF(mStatesWidgets[0]->widget->layout->position, 
                                  mStatesWidgets[0]->widget->layout->position);
        else
            mAvailableArea = RectF(Vec2F(), Vec2F());

        // Calculate bounds from all state positions
        for (auto& state : mStatesWidgets)
        {
            Vec2F pos = state->widget->layout->position;
            mAvailableArea.left = Math::Min(mAvailableArea.left, pos.x);
            mAvailableArea.right = Math::Max(mAvailableArea.right, pos.x);
            mAvailableArea.top = Math::Max(mAvailableArea.top, pos.y);
            mAvailableArea.bottom = Math::Min(mAvailableArea.bottom, pos.y);
        }

        float bordersCoef = 1.5f;
        Vec2F size = mAvailableArea.Size();
        mAvailableArea.left -= size.x*bordersCoef;
        mAvailableArea.right += size.x*bordersCoef;
        mAvailableArea.top += size.y*bordersCoef;
        mAvailableArea.bottom -= size.y*bordersCoef;

        mHorScrollbar->SetValueRange(mAvailableArea.left, mAvailableArea.right);
        mVerScrollbar->SetValueRange(mAvailableArea.bottom, mAvailableArea.top);
    }

	void AnimationStateGraphEditor::OnStateGraphStateStarted(const Ref<AnimationStateGraphComponent::StatePlayer>& player)
	{
		Ref<StateWidget> widget;
		if (mStatesWidgetsMap.TryGetValue(player->GetState(), widget))
		{
			widget->UpdateState(StateWidget::TransitionState::Planned);
			widget->SetPlayer(player);
		}
	}

	void AnimationStateGraphEditor::OnStateGraphStateFinished(const Ref<AnimationStateGraphComponent::StatePlayer>& player)
	{
		Ref<StateWidget> widget;
		if (mStatesWidgetsMap.TryGetValue(player->GetState(), widget))
		{
			widget->UpdateState(StateWidget::TransitionState::Finished);
		}
	}

	void AnimationStateGraphEditor::OnStateGraphTransitionStarted(const Ref<AnimationGraphTransition>& transition)
	{
		Ref<StateWidget> widget;
		if (mStatesWidgetsMap.TryGetValue(transition->GetSourceState(), widget))
		{
			widget->UpdateState(StateWidget::TransitionState::Finished);

			Ref<StateTransition> stateTransition;
			if (widget->transitionsMap.TryGetValue(transition, stateTransition))
				stateTransition->SetStatus(StateTransition::Status::Started);
		}
	}

	void AnimationStateGraphEditor::OnStateGraphTransitionFinished(const Ref<AnimationGraphTransition>& transition)
	{
		Ref<StateWidget> widget;
		if (mStatesWidgetsMap.TryGetValue(transition->GetSourceState(), widget))
		{
			widget->UpdateState(StateWidget::TransitionState::None);

			Ref<StateTransition> stateTransition;
			if (widget->transitionsMap.TryGetValue(transition, stateTransition))
				stateTransition->SetStatus(StateTransition::Status::None);
		}
	}

	void AnimationStateGraphEditor::OnStateGraphTransitionsPlanned(const Vector<Ref<AnimationGraphTransition>>& path)
	{
		for (auto& transition : path)
		{
			Ref<StateWidget> widget;
			if (mStatesWidgetsMap.TryGetValue(transition->GetDestinationState(), widget))
				widget->UpdateState(StateWidget::TransitionState::Planned);

			if (mStatesWidgetsMap.TryGetValue(transition->GetSourceState(), widget))
			{
				Ref<StateTransition> stateTransition;
				if (widget->transitionsMap.TryGetValue(transition, stateTransition))
					stateTransition->SetStatus(StateTransition::Status::Planned);
			}
		}
	}

	void AnimationStateGraphEditor::OnStateGraphTransitionCancelled(const Ref<AnimationGraphTransition>& transition)
	{
		Ref<StateWidget> widget;
		if (mStatesWidgetsMap.TryGetValue(transition->GetSourceState(), widget))
		{
			widget->UpdateState(StateWidget::TransitionState::None);

			Ref<StateTransition> stateTransition;
			if (widget->transitionsMap.TryGetValue(transition, stateTransition))
				stateTransition->SetStatus(StateTransition::Status::None);
		}

		// Also update destination state if needed
		if (mStatesWidgetsMap.TryGetValue(transition->GetDestinationState(), widget))
			widget->UpdateState(StateWidget::TransitionState::None);
	}

	AnimationStateGraphEditor::StateWidget::StateWidget(RefCounter* refCounter, const Ref<AnimationStateGraphEditor>& owner,
														const Ref<AnimationGraphState>& state):
		RefCounterable(refCounter), editor(owner), state(state)
	{
        widget = o2UI.CreateWidget<VerticalLayout>("ASG state");
		borderLayer = widget->GetLayer("border");

		dragHandle = mmake<DragHandle>();
		dragHandle->isPointInside = [this](const Vec2F& p) { return widget->IsUnderPoint(p); };
		dragHandle->onHoverEnter = [this]() { widget->SetState("hover", true); };
		dragHandle->onHoverExit = [this]() { widget->SetState("hover", false); };
		dragHandle->onPressed = [this]() { widget->SetState("pressed", true); };
		dragHandle->onReleased = [this]() { widget->SetState("pressed", false); };
        dragHandle->messageFallDownListener = owner.Get();
        dragHandle->onChangedPos = [this](const Vec2F& pos)
		{
			widget->layout->position = pos;
			this->state.Lock()->SetPosition(pos); 
		};

		widget->onDraw = [this]() { dragHandle->Draw(); };
        
		for (auto& animation : state->GetAnimations())
		{
			Ref<StateAnimation> stateAnimation = mmake<StateAnimation>();
			stateAnimation->name = animation->name;
			stateAnimation->animation = animation;
			stateAnimation->owner = Ref(this);
			animations.Add(stateAnimation);
		}

		propertiesContext = mmake<PropertiesContext>();
		propertiesContext->Set({ Pair<IObject*, IObject*>(this, nullptr) });
		animationsListProperty = DynamicCast<VectorProperty>(o2EditorProperties.BuildField(widget, GetType(), "animations", "", propertiesContext));
		animationsListProperty->SetHeaderEnabled(false);
		animationsListProperty->SetCaptionIndexesEnabled(false);
		animationsListProperty->SetCountEditBoxEnabled(false);
		animationsListProperty->SetValuePointers<Vector<Ref<StateAnimation>>>({ &animations });
		widget->AddChild(animationsListProperty);

		widget->layout->position = state->GetPosition();
		dragHandle->position = state->GetPosition();

		editor.Lock()->AddChild(widget);
	}

	void AnimationStateGraphEditor::StateWidget::InitializeTransitions()
	{
		for (auto& transition : state.Lock()->GetTransitions())
		{
			Ref<StateTransition> stateTransition = mmake<StateTransition>();
			stateTransition->owner = Ref(this);
			stateTransition->destination = editor.Lock()->mStatesWidgetsMap[transition->GetDestinationState()];
			transitions.Add(stateTransition);
			transitionsMap[WeakRef(transition)] = stateTransition;
		}
	}

	void AnimationStateGraphEditor::StateWidget::RemoveWidget()
	{
        if (!widget)
			return;

		auto parent = widget->GetParent();
		if (parent)
			parent.Lock()->RemoveChild(widget);

        widget = nullptr;
	}

	void AnimationStateGraphEditor::StateWidget::DrawTransitions()
	{
		for (auto& transition : transitions)
			transition->Draw();
	}

	void AnimationStateGraphEditor::StateWidget::UpdateState(TransitionState state)
	{
		widget->SetState("finished", state == TransitionState::Finished);
		widget->SetState("planned", state == TransitionState::Planned);
	}

	void AnimationStateGraphEditor::StateWidget::SetPlayer(const Ref<AnimationStateGraphComponent::StatePlayer>& player)
	{
		this->player = player;

		if (!player)
			return;

		auto& statePlayers = player->GetPlayers();
		for (auto& animation : animations)
		{
			auto animationState = statePlayers.FindOrDefault([animation](const Pair<Ref<AnimationGraphState::Animation>, Ref<IAnimationState>>& p) 
													         { return p.first == animation->animation; }).second;

			animation->state = animationState; 
		}

		animationsListProperty->Refresh();
	}

	void AnimationStateGraphEditor::StateTransition::Draw()
	{
		auto from = owner.Lock();
		auto to = destination.Lock();
		if (!from || !to)
			return;

		Vec2F fromPoint = from->widget->layout->GetWorldCenter();
		Vec2F toPoint = to->widget->layout->GetWorldCenter();
		Vec2F dir = (toPoint - fromPoint).Normalized();
		Vec2F norm = dir.Perpendicular();
		Vec2F center = (fromPoint + toPoint)/2.0f;

		Color4 colorRegular(126, 149, 160);
		Color4 colorPlanned(159, 190, 254);
		Color4 colorFinished(249, 93, 72);
		float width = 4.0f;
		float arrowSize = 10.0f;
		float offset = 7.0f;

		fromPoint += norm*offset;
		toPoint += norm*offset;

		static Vector<Vec2F> arrowLocal = { Vec2F(-0.5f, 0.5f), Vec2F(0.0f, 1.5f), Vec2F(0.5f, 0.5f), Vec2F(-0.5f, 0.5f), Vec2F(0.0f, 1.5f) };

		Basis arrowBasis(center, norm*arrowSize, dir*arrowSize);
		Vector<Vec2F> arrowWorld = arrowLocal.Convert<Vec2F>([&](const Vec2F& p) { return arrowBasis.Transform(p) + norm*offset; });

		if (status == Status::None)
		{
			o2Render.DrawAALine(fromPoint, toPoint, colorRegular, width);
			o2Render.DrawAALine(arrowWorld, colorRegular, width);
		}
		else
		{
			float currentProgress = 0.0f;
			Vec2F progressPoint = fromPoint;

			if (status == Status::Started)
			{
				if (auto editor = owner.Lock()->editor.Lock())
				{
					if (auto component = editor->mComponent.Lock())
					{
						if (auto currentTransition = component->GetCurrentTransition())
						{
							currentProgress = component->GetCurrentTransitionTime() / currentTransition->duration;
							progressPoint = Math::Lerp(fromPoint, toPoint, currentProgress);
						}
					}
				}
			}

			o2Render.DrawAALine(fromPoint, toPoint, colorPlanned, width);
			o2Render.DrawAALine(fromPoint, progressPoint, colorFinished, width);

			auto arrowColor = currentProgress < 0.5f ? colorPlanned : colorFinished;
			o2Render.DrawAALine(arrowWorld, arrowColor, width);
		}
	}

	void AnimationStateGraphEditor::StateTransition::SetStatus(Status status)
	{
    	this->status = status;
	}

	const Type* GraphAnimationStateViewer::GetViewingObjectType() const
	{
		if (mRealObjectType)
			return mRealObjectType;

		return GetViewingObjectTypeStatic();
	}

	const Type* GraphAnimationStateViewer::GetViewingObjectTypeStatic()
	{
		return &TypeOf(AnimationStateGraphEditor::StateAnimation);
	}

	Ref<Spoiler> GraphAnimationStateViewer::CreateSpoiler(const Ref<Widget>& parent)
	{
		mSpoiler = IObjectPropertiesViewer::CreateSpoiler(parent);

		mPlayPause = o2UI.CreateWidget<Toggle>("animation state play-stop");
		mPlayPause->name = "play-stop";
		*mPlayPause->layout = WidgetLayout::Based(BaseCorner::LeftTop, Vec2F(20, 20), Vec2F(7, 1));
		mPlayPause->onToggleByUser = THIS_FUNC(OnPlayPauseToggled);
		mSpoiler->AddInternalWidget(mPlayPause);

		mEditBtn = o2UI.CreateWidget<Button>("edit animation state");
		mEditBtn->name = "edit";
		*mEditBtn->layout = WidgetLayout::Based(BaseCorner::RightTop, Vec2F(20, 20), Vec2F(-40, 1));
		mEditBtn->onClick = THIS_FUNC(OnEditPressed);
		mSpoiler->AddInternalWidget(mEditBtn);

		mLooped = o2UI.CreateWidget<Toggle>("animation state loop");
		mLooped->name = "loop";
		*mLooped->layout = WidgetLayout::Based(BaseCorner::RightTop, Vec2F(20, 20), Vec2F(-20, 1));
		mLooped->onToggleByUser = THIS_FUNC(OnLoopToggled);
		mSpoiler->AddInternalWidget(mLooped);

		mTimeProgress = o2UI.CreateWidget<HorizontalProgress>("animation state bar");
		mTimeProgress->name = "bar";
		*mTimeProgress->layout = WidgetLayout::HorStretch(VerAlign::Top, 0, 0, 2, 18);
		mTimeProgress->onChangeByUser = THIS_FUNC(OnTimeProgressChanged);
		mSpoiler->AddInternalWidget(mTimeProgress);

		if (auto textLayer = GetSpoiler()->GetLayer("caption"))
		{
			textLayer->layout.offsetLeft = 27;
			textLayer->layout.offsetBottom = -19;
			textLayer->layout.offsetTop = 1;
		}

		if (auto header = parent->GetChildByType<Widget>("caption/header"))
		{
			auto spacer = mmake<Widget>();
			spacer->layout->maxWidth = 40;
			header->AddChild(spacer, 1);
		}

		return mSpoiler;
	}

	void GraphAnimationStateViewer::OnRefreshed(const Vector<Pair<IObject*, IObject*>>& targetObjets)
	{
		if (mSubscribedPlayer)
			mSubscribedPlayer.Lock()->onUpdate -= THIS_FUNC(OnAnimationUpdated);

		mSubscribedPlayer = nullptr;

		if (!targetObjets.IsEmpty())
		{
			auto state = dynamic_cast<AnimationStateGraphEditor::StateAnimation*>(targetObjets.Last().first);
			if (state && state->state)
				mSubscribedPlayer = Ref(&state->state->GetPlayer());

			if (auto player = mSubscribedPlayer.Lock())
			{
				player->onUpdate += THIS_FUNC(OnAnimationUpdated);
				player->onPlay += THIS_FUNC(OnAnimationStarted);
				player->onStop += THIS_FUNC(OnAnimationFinished);

				mPlayPause->value = player->IsPlaying();
			}
		}
	}

	void GraphAnimationStateViewer::OnFree()
	{
		if (auto player = mSubscribedPlayer.Lock())
		{
			player->onUpdate -= THIS_FUNC(OnAnimationUpdated);
			player->onPlay -= THIS_FUNC(OnAnimationStarted);
			player->onStop -= THIS_FUNC(OnAnimationFinished);
		}

		mSubscribedPlayer = nullptr;
	}

	void GraphAnimationStateViewer::OnPlayPauseToggled(bool play)
	{
		if (auto stateWidget = mPropertiesContext->FindOnStack<AnimationStateGraphEditor::StateWidget>())
		{
			if (auto component = stateWidget->editor.Lock()->mComponent.Lock())
			{
				if (play)
					component->GoToState(stateWidget->state.Lock());
			}
		}

		o2Scene.OnObjectChanged(o2EditorSceneScreen.GetSelectedObjects().First());
	}

	void GraphAnimationStateViewer::OnLoopToggled(bool looped)
	{
		for (auto& targets : mTargetObjects)
		{
			if (!targets.first)
				continue;

			auto animationState = dynamic_cast<IAnimationState*>(targets.first);
			if (!animationState)
				continue;

			animationState->SetLooped(looped);
		}

		o2Scene.OnObjectChanged(o2EditorSceneScreen.GetSelectedObjects().First());
	}

	void GraphAnimationStateViewer::OnEditPressed()
	{
		if (mTargetObjects.IsEmpty())
			return;

		auto state = dynamic_cast<AnimationStateGraphEditor::StateAnimation*>(mTargetObjects.Last().first);
		auto animationState = DynamicCast<AnimationState>(state->state);
		if (!animationState)
			return;

		auto animationRef = animationState->GetAnimation();
		if (!animationRef)
		{
			animationRef.CreateInstance();
			animationState->SetAnimation(animationRef);

			GetSpoiler()->Expand();
		}

		if (animationRef)
		{
			o2EditorAnimationWindow.SetAnimation(animationRef->animation);

			if (!o2EditorSceneScreen.GetSelectedObjects().IsEmpty())
				o2EditorAnimationWindow.SetTarget(DynamicCast<Actor>(o2EditorSceneScreen.GetSelectedObjects().Last()));

			o2EditorAnimationWindow.SetAnimationEditable(Ref(mPropertiesContext->FindOnStack<IEditableAnimation>()));
			o2EditorAnimationWindow.GetWindow()->Focus();
		}
	}

	void GraphAnimationStateViewer::OnTimeProgressChanged(float value)
	{
		if (mSubscribedPlayer)
			mSubscribedPlayer.Lock()->SetRelTime(value);
	}

	void GraphAnimationStateViewer::OnAnimationUpdated(float time)
	{
		if (auto subscribedPlayer = mSubscribedPlayer.Lock())
			mTimeProgress->value = subscribedPlayer->GetLoopTime()/ subscribedPlayer->GetDuration();
	}

	void GraphAnimationStateViewer::OnAnimationStarted()
	{
		mPlayPause->value = true;
	}

	void GraphAnimationStateViewer::OnAnimationFinished()
	{
		mPlayPause->value = false;
		mTimeProgress->value = 0.0f;
	}

}

DECLARE_TEMPLATE_CLASS(o2::LinkRef<Editor::AnimationStateGraphEditor>);
// --- META ---

ENUM_META(Editor::AnimationStateGraphEditor::StateTransition::Status)
{
    ENUM_ENTRY(None);
    ENUM_ENTRY(Planned);
    ENUM_ENTRY(Started);
}
END_ENUM_META;

ENUM_META(Editor::AnimationStateGraphEditor::StateWidget::TransitionState)
{
    ENUM_ENTRY(Finished);
    ENUM_ENTRY(None);
    ENUM_ENTRY(Planned);
}
END_ENUM_META;

DECLARE_CLASS(Editor::AnimationStateGraphEditor, Editor__AnimationStateGraphEditor);

DECLARE_CLASS(Editor::GraphAnimationStateViewer, Editor__GraphAnimationStateViewer);

DECLARE_CLASS(Editor::AnimationStateGraphEditor::StateAnimation, Editor__AnimationStateGraphEditor__StateAnimation);

DECLARE_CLASS(Editor::AnimationStateGraphEditor::StateWidget, Editor__AnimationStateGraphEditor__StateWidget);
// --- END META ---
