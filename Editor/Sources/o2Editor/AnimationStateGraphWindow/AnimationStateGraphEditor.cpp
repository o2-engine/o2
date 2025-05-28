#include "o2Editor/stdafx.h"
#include "AnimationStateGraphEditor.h"
#include "GraphAnimationStateViewer.h"

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
#include "o2Editor/AnimationWindow/AnimationWindow.h"
#include "o2Editor/Core/Dialogs/KeyEditDlg.h"
#include "o2Editor/Core/Properties/Properties.h"
#include "o2Editor/Core/UI/CurveEditor/CurveActions.h"
#include "o2Editor/Core/UIRoot.h"
#include "o2Editor/PropertiesWindow/PropertiesWindow.h"
#include "o2Editor/SceneWindow/SceneEditScreen.h"

namespace Editor
{
    AnimationStateGraphEditor::AnimationStateGraphEditor(RefCounter* refCounter):
        FrameScrollView(refCounter), SelectableDragHandlesGroup(refCounter)
    {
        mSelectionSprite = mmake<Sprite>();
		mStateWidgetsContainer = mmake<Widget>();
        mBackColor = Color4(225, 232, 232, 255);
        mViewCameraMinScale = 1.0f;

		InitializeContextMenus();

        mReady = true;
    }

    AnimationStateGraphEditor::~AnimationStateGraphEditor()
    {}

	void AnimationStateGraphEditor::SetGraph(const Ref<AnimationStateGraphAsset>& graph, 
                                             const Ref<AnimationStateGraphComponent>& component)
	{
		if (auto lastComponent = mComponent.Lock())
		{
			lastComponent->onStateStarted -= THIS_FUNC(OnStateGraphStateStarted);
			lastComponent->onStateFinished -= THIS_FUNC(OnStateGraphStateFinished);
			lastComponent->onTransitionStarted -= THIS_FUNC(OnStateGraphTransitionStarted);
			lastComponent->onTransitionFinished -= THIS_FUNC(OnStateGraphTransitionFinished);
			lastComponent->onTransitionsPlanned -= THIS_FUNC(OnStateGraphTransitionsPlanned);
			lastComponent->onTransitionCancelled -= THIS_FUNC(OnStateGraphTransitionCancelled);
		}

		mGraph = graph;
		mComponent = component;

		if (auto component = mComponent.Lock())
		{
			component->onStateStarted += THIS_FUNC(OnStateGraphStateStarted);
			component->onStateFinished += THIS_FUNC(OnStateGraphStateFinished);
			component->onTransitionStarted += THIS_FUNC(OnStateGraphTransitionStarted);
			component->onTransitionFinished += THIS_FUNC(OnStateGraphTransitionFinished);
			component->onTransitionsPlanned += THIS_FUNC(OnStateGraphTransitionsPlanned);
			component->onTransitionCancelled += THIS_FUNC(OnStateGraphTransitionCancelled);
		}

		InitializeStates();

		if (mComponent)
			mComponent.Lock()->Reset();

		mNeedAdjustView = true;
	}

	void AnimationStateGraphEditor::InitializeStates()
	{
		auto stateWidgetsCache = mStatesWidgetsMap;

		mStatesWidgets.Clear();
		mStatesWidgetsMap.Clear();
		mStateHandlesMap.Clear();

		if (mGraph)
		{
			auto graph = mGraph.Lock();
			for (auto& state : graph->GetStates())
			{
				Ref<StateWidget> stateWidget;
				if (stateWidgetsCache.TryGetValue(state, stateWidget))
					stateWidgetsCache.Remove(state);
				else
					stateWidget = mmake<StateWidget>(Ref(this), state);

				mStatesWidgets.Add(stateWidget);
				mStatesWidgetsMap[state] = stateWidget;
				mStateHandlesMap[stateWidget->dragHandle] = stateWidget;
			}

			for (auto& state : mStatesWidgets)
				state->InitializeTransitions();
		}

		for (auto kv : stateWidgetsCache)
			kv.second->RemoveWidget();

		RecalculateViewArea();
	}

	void AnimationStateGraphEditor::Draw()
	{
		ScrollView::Draw();
		DrawSelection();
	}

	void AnimationStateGraphEditor::RedrawContent()
	{
		DrawGrid();
		DrawTransitions();
		mStateWidgetsContainer->Draw();
	}


	void AnimationStateGraphEditor::OnSelectionChanged()
	{
		if (!mSelectedHandles.IsEmpty())
		{
			if (auto state = mStateHandlesMap[mSelectedHandles[0]])
			{
				o2EditorPropertiesWindow.SetTarget(state->state.Lock().Get());
			}
		}
	}

	void AnimationStateGraphEditor::DeselectAll()
	{
		SelectableDragHandlesGroup::DeselectAll();

    	for (auto& state : mStatesWidgets)
    	{
    		for (auto& transition : state->transitions)
    			transition->SetSelected(false);
    	}

    	mSelectedTransition = nullptr;
	}

	void AnimationStateGraphEditor::DrawHandles()
	{}

	void AnimationStateGraphEditor::DrawSelection()
	{
		if (mIsPressed)
		{
			mSelectionSprite->rect = RectF(LocalToScreenPoint(mSelectingPressedPoint), o2Input.cursorPos);
			mSelectionSprite->Draw();
		}
	}

	void AnimationStateGraphEditor::DrawTransitions()
	{
		for (auto& state : mStatesWidgets)
			state->DrawTransitions();

		if (mCreatingTransition)
		{
			Vec2F from = mContextMenuState->state.Lock()->GetPosition();
			Vec2F to = ScreenToLocalPoint(o2Input.cursorPos);

			DrawTransition(from, to, StateTransition::TransitionStatus::None, 0.0f);
		}
	}

	void AnimationStateGraphEditor::DrawTransition(Vec2F from, Vec2F to, StateTransition::TransitionStatus status, float progress, bool selected)
	{
		Vec2F dir = (to - from).Normalized();
		Vec2F norm = dir.Perpendicular();
		Vec2F center = (from + to)/2.0f;

		Color4 colorRegular(126, 149, 160);
		Color4 colorPlanned(159, 190, 254);
		Color4 colorFinished(249, 93, 72);
		Color4 colorSelected(0, 150, 136);
		
		float width = selected ? 6.0f : 4.0f; 
		float arrowSize = 10.0f;
		float offset = 7.0f;

		from += norm*offset;
		to += norm*offset;

		static Vector<Vec2F> arrowLocal = { Vec2F(-0.5f, 0.5f), Vec2F(0.0f, 1.5f), Vec2F(0.5f, 0.5f), Vec2F(-0.5f, 0.5f), Vec2F(0.0f, 1.5f) };

		Basis arrowBasis(center, norm*arrowSize, dir*arrowSize);
		Vector<Vec2F> arrowWorld = arrowLocal.Convert<Vec2F>([&](const Vec2F& p) { return arrowBasis.Transform(p) + norm*offset; });

		if (status == StateTransition::TransitionStatus::None)
		{
			Color4 lineColor = selected ? colorSelected : colorRegular;
			o2Render.DrawAALine(from, to, lineColor, width);
			o2Render.DrawAALine(arrowWorld, lineColor, width);
		}
		else
		{
			Vec2F progressPoint = from;

			if (status == StateTransition::TransitionStatus::Started)
				progressPoint = Math::Lerp(from, to, progress);
			else
				progress = 0.0f;

			Color4 plannedColor = selected ? colorSelected : colorPlanned;
			o2Render.DrawAALine(from, to, plannedColor, width);
			o2Render.DrawAALine(from, progressPoint, colorFinished, width);

			auto arrowColor = progress < 0.5f ? plannedColor : colorFinished;
			o2Render.DrawAALine(arrowWorld, arrowColor, width);
		}
	}

	void AnimationStateGraphEditor::Update(float dt)
    {
		mStateWidgetsContainer->Update(dt);
		mStateWidgetsContainer->UpdateChildren(dt);

        FrameScrollView::Update(dt);

        if (mReady && mResEnabledInHierarchy && !mIsClipped && mNeedAdjustView)
        {
            mNeedAdjustView = false;
			mViewCameraTargetScale = Vec2F(1, 1);
            mViewCamera.center = mAvailableArea.Center();
            mViewCameraTargetPos = mViewCamera.position;
        }

		if (mComponent)
			mComponent.Lock()->GetActor()->Update(dt);

		CheckRefreshViewersTimer(dt);
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

		// Deselect all transitions when clicking on empty space
		bool clickedOnTransition = false;
		for (auto& state : mStatesWidgets)
		{
			for (auto& transition : state->transitions)
			{
				if (transition->IsUnderPoint(cursor.position))
				{
					clickedOnTransition = true;
					break;
				}
			}
			if (clickedOnTransition)
				break;
		}

		if (!clickedOnTransition)
		{
			// Deselect all transitions
			for (auto& state : mStatesWidgets)
			{
				for (auto& transition : state->transitions)
					transition->SetSelected(false);
			}
		}

		mSelectingPressedPoint = cursor.position;
		BeginPreSelect();
	}

	void AnimationStateGraphEditor::OnCursorReleased(const Input::Cursor& cursor)
	{
		EndPreSelect();
	}

	void AnimationStateGraphEditor::OnCursorStillDown(const Input::Cursor& cursor)
	{
		Vector<Ref<DragHandle>> preSelectedHandles;

		RectF selectionRect = RectF(mSelectingPressedPoint, ScreenToLocalPoint(o2Input.cursorPos));

		for (auto& state : mStatesWidgets)
		{
			RectF stateRect = state->widget->layout->worldAABB;
			if (selectionRect.IsIntersects(stateRect))
			{
				preSelectedHandles.Add(state->dragHandle);
				state->widget->SetState("focused", true);
			}
			else
				state->widget->SetState("focused", mSelectedHandles.Contains(state->dragHandle));
		}

		UpdatePreSelect(preSelectedHandles);
	}

	void AnimationStateGraphEditor::OnCursorRightMouseStayDown(const Input::Cursor& cursor)
	{
		FrameScrollView::OnCursorRightMouseStayDown(cursor);
	}

	void AnimationStateGraphEditor::OnCursorRightMouseReleased(const Input::Cursor& cursor)
	{
		if (!mViewCameraMoved)
		{
			mContextMenuPos = cursor.position;
			o2Debug.Log("Right mouse button released at: " + String(mContextMenuPos));
			mContextMenu->Show();
		}

		FrameScrollView::OnCursorRightMouseReleased(cursor);
	}

	void AnimationStateGraphEditor::DrawInheritedDepthChildren()
	{
		mContextMenu->Draw();
		mStateContextMenu->Draw();
		mTransitionContextMenu->Draw();
	}

	void AnimationStateGraphEditor::InitializeContextMenus()
    {
        mContextMenu = o2UI.CreateWidget<ContextMenu>();
		mContextMenu->AddItem("Add state", THIS_FUNC(CreateState));

		mStateContextMenu = o2UI.CreateWidget<ContextMenu>();
		mStateContextMenu->AddItem("Set as initial", THIS_FUNC(SetCurrentStateInitial));
		mStateContextMenu->AddItem("Add transition", THIS_FUNC(StartAddingTransition));
		mStateContextMenu->AddItem("Remove state", THIS_FUNC(RemoveCurrentStates));

		mTransitionContextMenu = o2UI.CreateWidget<ContextMenu>();
		mTransitionContextMenu->AddItem("Remove transition", THIS_FUNC(RemoveCurrentTransition));

		AddChild(mContextMenu);
		AddChild(mStateContextMenu);
		AddChild(mTransitionContextMenu);

		onShow = [&]() { mContextMenu->SetItemsMaxPriority(); };
		onHide = [&]() { mContextMenu->SetItemsMinPriority(); };
    }

    void AnimationStateGraphEditor::RecalculateViewArea()
    {
        // Initialize with first state position if available
        if (!mStatesWidgets.IsEmpty())
            mAvailableArea = mStatesWidgets[0]->widget->layout->worldAABB;
        else
            mAvailableArea = RectF(Vec2F(), Vec2F());

        // Calculate bounds from all state positions
        for (auto& state : mStatesWidgets)
			mAvailableArea.Expand(state->widget->layout->worldAABB);

		// Add borders
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
				stateTransition->SetStatus(StateTransition::TransitionStatus::Started);
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
				stateTransition->SetStatus(StateTransition::TransitionStatus::None);
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
					stateTransition->SetStatus(StateTransition::TransitionStatus::Planned);
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
				stateTransition->SetStatus(StateTransition::TransitionStatus::None);
		}

		// Also update destination state if needed
		if (mStatesWidgetsMap.TryGetValue(transition->GetDestinationState(), widget))
			widget->UpdateState(StateWidget::TransitionState::None);
	}

	void AnimationStateGraphEditor::CreateState()
	{
		auto graph = mGraph.Lock();
		if (!graph)
			return;

		auto state = graph->AddState("New state", {});
		state->SetPosition(mContextMenuPos);
		state->AddAnimation("");
		InitializeStates();
	}

	void AnimationStateGraphEditor::SetCurrentStateInitial()
	{
		auto graph = mGraph.Lock();
		if (!graph)
			return;

		for (auto& handle : mSelectedHandles)
		{
			if (auto state = mStateHandlesMap[handle])
			{
				graph->SetInitialState(state->state.Lock()->name);
				break;
			}
		}
	}

	void AnimationStateGraphEditor::StartAddingTransition()
	{
		mCreatingTransition = true;
	}

	void AnimationStateGraphEditor::RemoveCurrentStates()
	{
		auto graph = mGraph.Lock();
		if (graph)
		{
			for (auto& handle : mSelectedHandles)
			{
				if (auto state = mStateHandlesMap[handle])
					graph->RemoveState(state->state.Lock());

			}

			InitializeStates();
		}
	}

	void AnimationStateGraphEditor::RemoveCurrentTransition()
	{
		auto graph = mGraph.Lock();
		if (!graph || !mSelectedTransition)
			return;

		auto sourceState = mSelectedTransition->owner.Lock()->state.Lock();
		auto transition = mSelectedTransition->transition;

		if (sourceState && transition)
		{
			sourceState->RemoveTransition(transition);
			mSelectedTransition = nullptr;
			InitializeStates();
		}
	}

	void AnimationStateGraphEditor::OpenStateContextMenu(const Ref<StateWidget>& state)
	{
		mContextMenuState = state;
		mStateContextMenu->Show();
	}

	void AnimationStateGraphEditor::OnStatePressed(const Ref<StateWidget>& state)
	{
		if (mCreatingTransition)
		{
			auto sourceState = mContextMenuState->state.Lock();
			auto destinationState = state->state.Lock();
			if (sourceState && destinationState)
			{
				auto transition = sourceState->AddTransition(state->state.Lock());
				mCreatingTransition = false;
				InitializeStates();
			}
		}
	}

	void AnimationStateGraphEditor::CheckRefreshViewersTimer(float dt)
	{
		mRefreshViewersTimer += dt;

		const float refreshInterval = 0.5f;
		if (mRefreshViewersTimer > refreshInterval)
		{
			mRefreshViewersTimer = 0.0f;

			for (auto& state : mStatesWidgets)
				state->animationsListProperty->Refresh();
		}
	}

	const Vec2F AnimationStateGraphEditor::StateWidget::defaultWidgetSize = Vec2F(300, 50);

	AnimationStateGraphEditor::StateWidget::StateWidget(RefCounter* refCounter, const Ref<AnimationStateGraphEditor>& owner,
														const Ref<AnimationGraphState>& state):
		RefCounterable(refCounter), editor(owner), state(state)
	{
		widget = o2UI.CreateWidget<VerticalLayout>("ASG state");
		auto weakWidget = WeakRef(widget);

		dragHandle = mmake<DragHandle>();
		dragHandle->SetSelectionGroup(owner);
		dragHandle->messageFallDownListener = owner.Get();
		dragHandle->isPointInside = [weakWidget](const Vec2F& p) { return weakWidget ? weakWidget.Lock()->IsUnderPoint(p) : false; };

		dragHandle->onHoverEnter = [weakWidget]() { if (weakWidget) weakWidget.Lock()->SetState("hover", true); };
		dragHandle->onHoverExit = [weakWidget]() { if (weakWidget) weakWidget.Lock()->SetState("hover", false); };
		dragHandle->onPressed = [weakWidget]() { if (weakWidget) weakWidget.Lock()->SetState("pressed", true); };
		dragHandle->onReleased = [weakWidget]() { if (weakWidget) weakWidget.Lock()->SetState("pressed", false); };
		dragHandle->onSelected = [weakWidget]() { if (weakWidget) weakWidget.Lock()->SetState("focused", true); };
		dragHandle->onDeselected = [weakWidget]() { if (weakWidget) weakWidget.Lock()->SetState("focused", false); };

		dragHandle->onPressed = [this]() { OnPressed(); };
		dragHandle->onRightButtonReleased = [this](const Input::Cursor&) { OpenContextMenu(); };

        dragHandle->onChangedPos = [weakWidget, this](const Vec2F& pos)
		{
			if (auto widget = weakWidget.Lock())
			{
				*widget->layout = WidgetLayout::Based(BaseCorner::Center, defaultWidgetSize, pos);
				this->state.Lock()->SetPosition(pos);
				editor.Lock()->RecalculateViewArea();
			}
		};

		widget->onDraw = [this]() { dragHandle->Draw(); };

		dragHandle->position = state->GetPosition();
        
		for (auto& animation : state->GetAnimations())
		{
			Ref<StateAnimation> stateAnimation = mmake<StateAnimation>();

			stateAnimation->owner = Ref(this);
			stateAnimation->name = animation->name;
			stateAnimation->animation = animation;

			if (auto graphComponent = owner->mComponent.Lock())
			{
				if (auto animationComponent = graphComponent->GetAnimationComponent())
					stateAnimation->state = animationComponent->GetState(animation->name);
			}

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

		*widget->layout = WidgetLayout::Based(BaseCorner::Center, defaultWidgetSize, state->GetPosition());

		editor.Lock()->mStateWidgetsContainer->AddChild(widget);
	}

	void AnimationStateGraphEditor::StateWidget::InitializeTransitions()
	{
		transitions.Clear();
		transitionsMap.Clear();

		for (auto& transition : state.Lock()->GetTransitions())
		{
			Ref<StateTransition> stateTransition = mmake<StateTransition>();
			stateTransition->owner = Ref(this);
			stateTransition->destination = editor.Lock()->mStatesWidgetsMap[transition->GetDestinationState()];
			stateTransition->transition = transition;
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
		animationsListProperty->Refresh();
	}

	void AnimationStateGraphEditor::StateWidget::OpenContextMenu()
	{
		editor.Lock()->OpenStateContextMenu(Ref(this));
	}


	void AnimationStateGraphEditor::StateWidget::OnPressed()
	{
		editor.Lock()->OnStatePressed(Ref(this));
	}


	void AnimationStateGraphEditor::StateWidget::OnAnimationsListChanged()
	{
		auto state = this->state.Lock();
		if (!state)
			return;

		Vector<Ref<AnimationGraphState::Animation>> animations;
		for (auto& stateAnimation : this->animations)
		{
			if (stateAnimation->animation)
				animations.Add(stateAnimation->animation.Lock());
			else
			{
				auto newAnimation = mmake<AnimationGraphState::Animation>();
				newAnimation->name = stateAnimation->name;
				newAnimation->weight = stateAnimation->weight;

				stateAnimation->animation = newAnimation;
				stateAnimation->owner = Ref(this);

				animations.Add(newAnimation);
			}

			if (auto editor = this->editor.Lock())
			{
				if (auto graphComponent = editor->mComponent.Lock())
				{
					if (auto animationComponent = graphComponent->GetAnimationComponent())
						stateAnimation->state = animationComponent->GetState(stateAnimation->name);
				}
			}
		}

		state->SetAnimations(animations);
	}

	void AnimationStateGraphEditor::StateTransition::Draw()
	{
		auto from = owner.Lock();
		auto to = destination.Lock();
		if (!from || !to)
			return;

		Vec2F fromPoint = from->widget->layout->GetWorldCenter();
		Vec2F toPoint = to->widget->layout->GetWorldCenter();

		float progress = 0.0f;
		if (auto component = from->editor.Lock()->mComponent.Lock())
		{
			if (auto currentTransition = component->GetCurrentTransition())
				progress = component->GetCurrentTransitionTime() / currentTransition->duration;
		}

		AnimationStateGraphEditor::DrawTransition(fromPoint, toPoint, status, progress, mIsSelected);

		OnDrawn();
	}

	void AnimationStateGraphEditor::StateTransition::SetStatus(TransitionStatus status)
	{
    	this->status = status;
	}

	Vector<String> AnimationStateGraphEditor::StateAnimation::GetAvailableStates() const
	{
		auto owner = this->owner.Lock();
		if (!owner)
			return {};

		auto editor = owner->editor.Lock();
		if (!editor)
			return {};

		auto component = editor->mComponent.Lock();
		if (!component)
			return {};

		auto animationComponent = component->GetAnimationComponent();
		if (!animationComponent)
			return {};

		return animationComponent->GetStatesNames();
	}

	void AnimationStateGraphEditor::StateAnimation::SetName(const String& name)
	{
		auto stateAnimation = animation.Lock();
		if (!stateAnimation)
			return;

		stateAnimation->name = name;

		auto owner = this->owner.Lock();
		if (!owner)
			return;

		auto state = owner->state.Lock();
		if (!state)
			return;

		state->name = name;
	}

	const String& AnimationStateGraphEditor::StateAnimation::GetName() const
	{
		auto stateAnimation = animation.Lock();
		if (!stateAnimation)
			return String::empty;

		return stateAnimation->name;
	}

	void AnimationStateGraphEditor::StateAnimation::SetWeight(float weight)
	{
		auto stateAnimation = animation.Lock();
		if (!stateAnimation)
			return;

		stateAnimation->weight = weight;
	}

	float AnimationStateGraphEditor::StateAnimation::GetWeight() const
	{
		auto stateAnimation = animation.Lock();
		if (!stateAnimation)
			return 0.0f;

		return stateAnimation->weight;
	}

	AnimationStateGraphEditor::StateTransition::StateTransition():
		RefCounterable(nullptr)
	{}

	bool AnimationStateGraphEditor::StateTransition::IsUnderPoint(const Vec2F& point)
	{
		const float lineThickness = 10.0f;
		const float offset = 10.0f;

		auto from = owner.Lock();
		auto to = destination.Lock();
		if (!from || !to)
			return false;

		Vec2F fromPoint = from->widget->layout->GetWorldCenter();
		Vec2F toPoint = to->widget->layout->GetWorldCenter();

		Vec2F line = toPoint - fromPoint;
		float lineLength = line.Length();
		if (lineLength < Math::Epsilon)
			return false;

		Vec2F lineDir = line / lineLength;
		Vec2F norm = lineDir.Perpendicular();

		fromPoint += norm*offset;
		toPoint += norm*offset;
		
		Vec2F pointVector = point - fromPoint;
		float projection = pointVector.Dot(lineDir);
		
		if (projection < 0.0f || projection > lineLength)
			return false;

		Vec2F projectionPoint = fromPoint + lineDir * projection;
		
		float distance = Math::Abs(pointVector.Dot(norm));
		return distance <= lineThickness;
	}

	void AnimationStateGraphEditor::StateTransition::OnCursorPressed(const Input::Cursor& cursor)
	{
		if (auto editorPtr = owner.Lock()->editor.Lock())
			editorPtr->DeselectAll();

		SetSelected(true);
	}

	bool AnimationStateGraphEditor::StateTransition::IsSelected() const
	{
		return mIsSelected;
	}

	void AnimationStateGraphEditor::StateTransition::SetSelected(bool selected)
	{
		mIsSelected = selected;

		if (mIsSelected)
			o2EditorPropertiesWindow.SetTarget(this);

		if (auto editorPtr = owner.Lock()->editor.Lock())
		{
			if (selected)
				editorPtr->mSelectedTransition = Ref(this);
			else if (editorPtr->mSelectedTransition == this)
				editorPtr->mSelectedTransition = nullptr;
		}
	}

	void AnimationStateGraphEditor::StateTransition::OnCursorRightMouseReleased(const Input::Cursor& cursor)
	{
		SetSelected(true);
		
		if (auto ownerPtr = owner.Lock())
		{
			if (auto editorPtr = ownerPtr->editor.Lock())
			{
				editorPtr->mSelectedTransition = Ref(this);
				editorPtr->mTransitionContextMenu->Show();
			}
		}
	}
}

DECLARE_TEMPLATE_CLASS(o2::LinkRef<Editor::AnimationStateGraphEditor>);
// --- META ---

ENUM_META(Editor::AnimationStateGraphEditor::StateTransition::TransitionStatus)
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

DECLARE_CLASS(Editor::AnimationStateGraphEditor::StateAnimation, Editor__AnimationStateGraphEditor__StateAnimation);

DECLARE_CLASS(Editor::AnimationStateGraphEditor::StateTransition, Editor__AnimationStateGraphEditor__StateTransition);

DECLARE_CLASS(Editor::AnimationStateGraphEditor::StateWidget, Editor__AnimationStateGraphEditor__StateWidget);
// --- END META ---
