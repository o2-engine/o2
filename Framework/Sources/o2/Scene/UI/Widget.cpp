#include "o2/stdafx.h"
#include "Widget.h"

#include "o2/Application/Input.h"
#include "o2/Render/Render.h"
#include "o2/Scene/Component.h"
#include "o2/Scene/Scene.h"
#include "o2/Scene/SceneLayer.h"
#include "o2/Scene/UI/UIManager.h"
#include "o2/Scene/UI/WidgetLayer.h"
#include "o2/Scene/UI/WidgetLayout.h"
#include "o2/Scene/UI/WidgetState.h"
#include "o2/Utils/System/Time/Time.h"

namespace o2
{
    Widget::Widget(RefCounter* refCounter, ActorCreateMode mode /*= ActorCreateMode::Default*/) :
        Actor(refCounter, mnew WidgetLayout(), mode), layout(dynamic_cast<WidgetLayout*>(transform))
    {
#if IS_EDITOR
        InitEditables();
#endif

        if (IsFocusable())
            UIManager::RegisterFocusableWidget(this);
    }

    Widget::Widget(RefCounter* refCounter, const AssetRef<ActorAsset>& prototype, ActorCreateMode mode /*= ActorCreateMode::Default*/) :
        Actor(refCounter, mnew WidgetLayout(), prototype, mode), layout(dynamic_cast<WidgetLayout*>(transform))
    {
#if IS_EDITOR
        InitEditables();
#endif

        if (IsFocusable())
            UIManager::RegisterFocusableWidget(this);
    }

    Widget::Widget(RefCounter* refCounter, Vector<Ref<Component>> components, ActorCreateMode mode /*= ActorCreateMode::Default*/) :
        Actor(refCounter, mnew WidgetLayout(), components, mode), layout(dynamic_cast<WidgetLayout*>(transform))
    {
#if IS_EDITOR
        InitEditables();
#endif

        if (IsFocusable())
            UIManager::RegisterFocusableWidget(this);
    }

    Widget::Widget(RefCounter* refCounter, const Widget& other) :
        Actor(refCounter, mnew WidgetLayout(*other.layout), other), layout(dynamic_cast<WidgetLayout*>(transform)),
        mTransparency(other.mTransparency), transparency(this), resTransparency(this),
        childrenWidgets(this), layers(this), states(this), childWidget(this), layer(this), state(this)
    {
#if IS_EDITOR
        InitEditables();
#endif


        // Layers
        {
            Ref<WidgetLayer::ICopyVisitor> layerCopyVisitor;

            if (DynamicCast<InstantiatePrototypeCloneVisitor>(other.mCopyVisitor) || other.mIsAsset)
                layerCopyVisitor = mmake<WidgetLayer::InstantiatePrototypeCloneVisitor>();

#if IS_EDITOR
            if (DynamicCast<MakePrototypeCloneVisitor>(other.mCopyVisitor))
                layerCopyVisitor = mmake<WidgetLayer::MakePrototypeCloneVisitor>();

            if (DynamicCast<InstantiatePrototypeCloneVisitor>(other.mCopyVisitor) || other.mIsAsset)
            {
                layersEditable->prototypeLink = other.layersEditable;
                internalChildrenEditable->prototypeLink = other.internalChildrenEditable;
            }
#endif

            for (auto& layer : other.mLayers)
            {
                layer->mCopyVisitor = layerCopyVisitor;

                auto newLayer = mmake<WidgetLayer>(*layer);
                newLayer->SetOwnerWidget(Ref(this));
                mLayers.Add(newLayer);
                OnLayerAdded(newLayer);
            }

            layerCopyVisitor = nullptr;
        }

        // Children
        {
            for (auto& child : mChildren)
            {
                auto childWidget = DynamicCast<Widget>(child);
                if (childWidget)
                {
                    childWidget->mParentWidget = Ref(this);
                    mChildWidgets.Add(childWidget);
                    OnChildAdded(childWidget);
                }

                child->OnParentChanged(nullptr);
            }

            for (auto& child : other.mInternalWidgets)
            {
                auto newChild = child->CloneAsRef<Widget>();
                newChild->RemoveFromScene();
                newChild->SetInternalParent(Ref(this), false);
            }
        }

        // States
        {
            for (auto& state : other.mStates)
            {
                auto newState = state->CloneAsRef<WidgetState>();
                AddState(newState, false);
            }
        }

        if (IsFocusable())
            UIManager::RegisterFocusableWidget(this);

        UpdateLayersDrawingSequence();
        RetargetStatesAnimations();
    }

    Widget::~Widget()
    {
        if (mParent)
            mParent.Lock()->OnChildRemoved(Ref(this));

        for (auto& layer : mLayers)
            layer->mOwnerWidget = nullptr;

        mLayers.Clear();

        mStates.Clear();

        for (auto& child : mInternalWidgets)
        {
            child->OnBeforeDestroy();

            child->mParent = nullptr;
            child->mParentWidget = nullptr;
        }

        mInternalWidgets.Clear();

        if (IsFocusable())
            UIManager::UnregisterFocusableWidget(this);
    }

    Widget& Widget::operator=(const Widget& other)
    {
        mInternalWidgets.Clear();
        mLayers.Clear();
        mStates.Clear();
        mVisibleState = nullptr;
        mFocusedState = nullptr;

        layout->CopyFrom(*other.layout);
        mTransparency = other.mTransparency;
        mIsFocusable = other.mIsFocusable;

        auto thisRef = Ref(this);

        for (auto& layer : other.mLayers)
        {
            auto newLayer = mmake<WidgetLayer>(*layer);
            newLayer->SetOwnerWidget(thisRef);
            mLayers.Add(newLayer);
            OnLayerAdded(newLayer);
        }

        mChildWidgets.Clear();
        for (auto& child : mChildren)
        {
            auto childWidget = DynamicCast<Widget>(child);
            if (childWidget)
            {
                mChildWidgets.Add(childWidget);
                OnChildAdded(childWidget);
            }
        }

        for (auto& child : other.mInternalWidgets)
        {
            auto newChild = child->CloneAsRef<Widget>();
            newChild->RemoveFromScene();
            newChild->SetInternalParent(thisRef, false);
        }

        for (auto& state : other.mStates)
        {
            auto newState = state->CloneAsRef<WidgetState>();
            AddState(newState, false);
        }

        UpdateLayersDrawingSequence();

        return *this;
    }

    void Widget::Update(float dt)
    {
        PROFILE_SAMPLE_FUNC();
        PROFILE_INFO(mName);

        if (mResEnabledInHierarchy)
        {
            if (GetLayoutData().updateFrame == 0)
            {
                for (auto& child : mChildren)
                    child->transform->SetDirty(true);

                for (auto& child : mInternalWidgets)
                    child->transform->SetDirty(true);

                UpdateSelfTransform();
            }

            if (!mIsClipped)
            {
                for (auto& state : mStates)
                {
                    if (state)
                        state->Update(dt);
                }
            }

            for (auto& comp : mComponents)
                comp->OnUpdate(dt);
        }
    }

    void Widget::UpdateChildren(float dt)
    {
        PROFILE_SAMPLE_FUNC();
        PROFILE_INFO(mName);

        for (auto& child : mChildren)
            child->Update(dt);

        RectF childrenWorldRect = GetLayoutData().childrenWorldRect;
        GetLayoutData().childrenWorldRect = GetLayoutData().worldRectangle;

        for (auto& child : mInternalWidgets)
            child->Update(dt);

        GetLayoutData().childrenWorldRect = childrenWorldRect;

        for (auto& child : mChildren)
            child->UpdateChildren(dt);

        GetLayoutData().childrenWorldRect = GetLayoutData().worldRectangle;

        for (auto& child : mInternalWidgets)
            child->UpdateChildren(dt);

        GetLayoutData().childrenWorldRect = childrenWorldRect;
    }

    void Widget::UpdateTransform()
    {
        if (GetLayoutData().drivenByParent && mParentWidget)
            mParentWidget.Lock()->UpdateTransform();

        UpdateSelfTransform();
        UpdateChildrenTransforms();
    }

    void Widget::UpdateChildrenTransforms()
    {
        Actor::UpdateChildrenTransforms();

        RectF childrenWorldRect = GetLayoutData().childrenWorldRect;
        GetLayoutData().childrenWorldRect = GetLayoutData().worldRectangle;

        for (auto& child : mInternalWidgets)
            child->UpdateSelfTransform();

        for (auto& child : mInternalWidgets)
            child->UpdateChildrenTransforms();

        GetLayoutData().childrenWorldRect = childrenWorldRect;
    }

    void Widget::SetLayoutDirty()
    {
        layout->SetDirty(false);
    }

    void Widget::Draw()
    {
        PROFILE_SAMPLE_FUNC();

        if (!mResEnabledInHierarchy || mIsClipped)
        {
            if (mIsClipped)
                DrawInheritedDepthChildren();

            return;
        }

        DrawLayers();

        OnDrawn();

		DrawInheritedDepthChildren();
        DrawInternalChildren();
        DrawTopLayers();

        DrawDebugFrame();
	}

	void Widget::DrawLayers()
	{
		for (auto& layer : mDrawingLayers)
			layer->Draw();
	}

	void Widget::DrawTopLayers()
	{
		for (auto& layer : mTopDrawingLayers)
			layer->Draw();
	}

	void Widget::DrawInternalChildren()
	{
		for (auto& child : mInternalWidgets)
			child->Draw();
	}

    void Widget::DrawDebugFrame()
    {
        if (!IsUIDebugEnabled() && !o2Input.IsKeyDown(VK_F2))
            return;

        static int colr = 0;
        static int lastFrame = 0;

        if (lastFrame != o2Time.GetCurrentFrame())
            colr = 0;

        lastFrame = o2Time.GetCurrentFrame();

        o2Render.DrawRectFrame(mBoundsWithChilds, Color4::SomeColor(colr++));
    }

    void Widget::SerializeRaw(DataValue& node) const
    {
        Actor::SerializeRaw(node);

        node["InternalWidgets"] = mInternalWidgets;
        node["Layers"] = mLayers;
        node["States"] = mStates;
    }

    void Widget::DeserializeRaw(const DataValue& node)
    {
        Actor::DeserializeRaw(node);

        if (auto internalWidgetsNode = node.FindMember("InternalWidgets"))
            mInternalWidgets = *internalWidgetsNode;

        if (auto layersNode = node.FindMember("Layers"))
            mLayers = *layersNode;

        for (auto& layer : mLayers)
            layer->SetOwnerWidget(Ref(this));

        for (auto& layer : mLayers)
            OnLayerAdded(layer);

        if (auto statesNode = node.FindMember("States"))
            mStates = *statesNode;
    }

    void Widget::SerializeWithProto(DataValue& node) const
    {
        Actor::SerializeWithProto(node);

        if (!mInternalWidgets.IsEmpty())
        {
            auto& internalWidgetsNode = node.AddMember("InternalWidgets");
            internalWidgetsNode.SetArray();
            for (auto& widget : mInternalWidgets)
            {
                auto& widgetNode = internalWidgetsNode.AddElement();
                widgetNode.AddMember("Type") = widget->GetType().GetName();
                widget->Serialize(widgetNode.AddMember("Data"));
            }
        }

        if (!mLayers.IsEmpty())
        {
            auto& layersNode = node.AddMember("Layers");
            layersNode.SetArray();
            for (auto& layer : mLayers)
                layer->Serialize(layersNode.AddElement());
        }

        if (!mStates.IsEmpty())
        {
            const Widget* proto = dynamic_cast<Widget*>(mPrototypeLink.Lock().Get());
            auto& statesNode = node.AddMember("States");
            for (auto& state : mStates)
            {
                if (auto protoState = proto->GetStateObject(state->name))
                {
                    auto& stateNode = statesNode.AddElement();
                    stateNode["name"] = state->name;
                    stateNode.SetDelta(*state, *protoState);
                }
                else
                    statesNode.AddElement().Set(*state);
            }
        }
    }

    void Widget::DeserializeWithProto(const DataValue& node)
    {
        for (auto& layer : mLayers)
            layer->mOwnerWidget = nullptr;

        for (auto& child : mInternalWidgets)
        {
            child->mParent = nullptr;
            child->mParentWidget = nullptr;
        }

        mLayers.Clear();
        mStates.Clear();
        mInternalWidgets.Clear();

        Actor::DeserializeWithProto(node);

        auto internalWidgetsNode = node.FindMember("InternalWidgets");
        if (internalWidgetsNode && internalWidgetsNode->IsArray())
        {
            for (auto& widgetNode : *internalWidgetsNode)
            {
                const DataValue* typeNode = widgetNode.FindMember("Type");
                const DataValue* dataValue = widgetNode.FindMember("Data");
                if (typeNode && dataValue)
                {
                    const ObjectType* type = dynamic_cast<const ObjectType*>(o2Reflection.GetType(*typeNode));
                    if (type)
                    {
                        auto widget = DynamicCast<Widget>(type->CreateSampleRef());
                        AddInternalWidget(widget);
                        widget->mCopyVisitor = mCopyVisitor;
                        widget->Deserialize(*dataValue);
                        widget->mCopyVisitor = nullptr;
                    }
                }
            }
        }

        auto layersNode = node.FindMember("Layers");
        if (layersNode && layersNode->IsArray())
        {
            for (auto& layerNode : *layersNode)
            {
                auto layer = mmake<WidgetLayer>();
                AddLayer(layer);
                layer->Deserialize(layerNode);
            }
        }

        auto statesNode = node.FindMember("States");
        if (statesNode && statesNode->IsArray())
        {
            const Widget* proto = dynamic_cast<const Widget*>(mPrototypeLink.Lock().Get());

            for (auto& stateNode : *statesNode)
            {
                auto state = mmake<WidgetState>();
                if (auto nameNode = stateNode.FindMember("name"))
                    nameNode->Get(state->name);

                bool delta = false;
                if (proto)
                {
                    if (auto protoState = proto->GetStateObject(state->name))
                    {
                        stateNode.GetDelta(*state, *protoState);
                        delta = true;
                    }
                }

                if (!delta)
                    stateNode.Get(*state);

                AddState(state, false);
            }
        }
    }

    void Widget::OnDeserialized(const DataValue& node)
    {
        auto thisRef = Ref(this);

        mChildWidgets.Clear();
        for (auto& child : mChildren)
        {
            auto childWidget = DynamicCast<Widget>(child);
            if (childWidget)
            {
                childWidget->mParentWidget = thisRef;
                mChildWidgets.Add(childWidget);
                OnChildAdded(childWidget);
            }
        }

        for (auto& child : mInternalWidgets)
        {
            child->mParent = thisRef;
            child->mParentWidget = thisRef;

            child->RemoveFromScene();
        }

        RetargetStatesAnimations();
        SetEnabledForcible(mEnabled);
        UpdateLayersDrawingSequence();
    }

    void Widget::OnDeserializedDelta(const DataValue& node, const IObject& origin)
    {
        OnDeserialized(node);
    }

    void Widget::OnTransformUpdated()
    {
        mIsClipped = false;
        Actor::OnTransformUpdated();
        UpdateLayersLayouts();
        onLayoutUpdated();
    }

    void Widget::OnEnabled()
    {
        Actor::OnEnabled();

        layout->SetDirty(false);
        onShow();
    }

    void Widget::OnDisabled()
	{
		Actor::OnDisabled();
        onHide();
    }

    void Widget::OnFocused()
    {
        onFocused();
    }

    void Widget::OnUnfocused()
    {
        onUnfocused();
    }

    const WeakRef<Widget>& Widget::GetParentWidget() const
    {
        return mParentWidget;
    }

    const RectF& Widget::GetChildrenWorldRect() const
    {
        return GetLayoutData().childrenWorldRect;
    }

    const Vector<Ref<Widget>>& Widget::GetChildWidgets() const
    {
        return mChildWidgets;
    }

    Ref<Actor> Widget::FindActorById(SceneUID id)
    {
        if (auto res = Actor::FindActorById(id))
            return res;

        for (auto& widget : mInternalWidgets)
        {
            if (auto res = widget->FindActorById(id))
                return res;
        }

        return nullptr;
    }

    String Widget::GetCreateMenuCategory()
    {
        return "UI";
    }

    Ref<WidgetLayer> Widget::AddLayer(const Ref<WidgetLayer>& layer)
    {
        if (layer->mParent)
            layer->mParent.Lock()->RemoveChild(layer);
        else if (layer->mOwnerWidget)
            layer->mOwnerWidget.Lock()->RemoveLayer(layer);

        mLayers.Add(layer);
        layer->SetOwnerWidget(Ref(this));
        UpdateLayersDrawingSequence();
        OnLayerAdded(layer);

#if IS_EDITOR
        if (Scene::IsSingletonInitialzed() && IsOnScene())
        {
            o2Scene.OnObjectChanged(layersEditable);
            o2Scene.onChildrenHierarchyChanged(layersEditable);
        }
#endif

        return layer;
    }

    Ref<WidgetLayer> Widget::AddLayer(const String& name, const Ref<IRectDrawable>& drawable,
                                      const Layout& layout /*= Layout::Both()*/, float depth /*= 0.0f*/)
    {
        if (Math::Equals(depth, 0.0f))
            depth = (float)mDrawingLayers.Count();

        auto layer = mmake<WidgetLayer>();
        layer->depth = depth;
        layer->name = name;
        layer->SetDrawable(drawable);
        layer->layout = layout;

        AddLayer(layer);

        return layer;
    }

    Ref<WidgetLayer> Widget::GetLayer(const String& path) const
    {
        int delPos = path.Find("/");
        String pathPart = path.SubStr(0, delPos);

        for (auto& layer : mLayers)
        {
            if (layer->name == pathPart)
            {
                if (delPos == -1)
                    return layer;
                else
                    return layer->GetChild(path.SubStr(delPos + 1));
            }
        }

        return nullptr;
    }

    Ref<WidgetLayer> Widget::FindLayer(const String& name) const
    {
        for (auto& childLayer : mLayers)
        {
            if (childLayer->name == name)
                return childLayer;

            auto layer = childLayer->FindChild(name);
            if (layer)
                return layer;
        }

        return nullptr;
    }

    void Widget::RemoveLayer(const Ref<WidgetLayer>& layer)
    {
        layer->SetOwnerWidget(nullptr);

        mLayers.Remove(layer);

        UpdateLayersDrawingSequence();

#if IS_EDITOR
        OnChildrenChanged();
#endif
    }

    void Widget::RemoveLayer(const String& path)
    {
        auto layer = GetLayer(path);

        if (!layer)
            return;

        if (layer->GetParent())
        {
            layer->GetParent().Lock()->RemoveChild(layer);
            return;
        }

        mLayers.Remove(layer);
        UpdateLayersDrawingSequence();
    }

    void Widget::RemoveAllLayers()
    {
        mDrawingLayers.Clear();
        mLayers.Clear();
    }

    const Vector<Ref<WidgetLayer>>& Widget::GetLayers() const
    {
        return mLayers;
    }

    Ref<WidgetState> Widget::AddState(const String& name)
    {
        auto newState = mmake<WidgetState>();
        newState->name = name;

        return AddState(newState);
    }

    Ref<WidgetState> Widget::AddState(const String& name, const Ref<AnimationClip>& animation)
    {
        auto newState = mmake<WidgetState>();
        newState->name = name;
        newState->animationClip = animation;

        return AddState(newState);
    }

    Ref<WidgetState> Widget::AddState(const String& name, const AssetRef<AnimationAsset>& animation)
    {
        auto newState = mmake<WidgetState>();
        newState->name = name;
        newState->animationAsset = animation;

        return AddState(newState);
    }

    Ref<WidgetState> Widget::AddState(const Ref<WidgetState>& state, bool showAnimErrors /*= true*/)
    {
        mStates.Add(state);
        state->SetOwner(Ref(this), showAnimErrors);

        if (state->name == "visible")
        {
            mVisibleState = state;
            state->SetStateForcible(mEnabled);

            state->onStateBecomesTrue += [&]()
                {
                    mResEnabled = true;
                    UpdateResEnabledInHierarchy();
                };

            state->onStateFullyFalse += [&]()
                {
                    mResEnabled = false;
                    UpdateResEnabledInHierarchy();
                };
        }

        if (state->name == "focused")
            mFocusedState = state;

        OnStateAdded(state);

        return state;
    }

    bool Widget::RemoveState(const String& name)
    {
        int idx = mStates.IndexOf([&](const Ref<WidgetState>& state) { return state->name == name; });
        if (idx < 0)
            return false;

        if (mStates[idx] == mVisibleState)
            mVisibleState = nullptr;

        if (mStates[idx] == mFocusedState)
            mFocusedState = nullptr;

        mStates.RemoveAt(idx);

        return true;
    }

    bool Widget::RemoveState(const Ref<WidgetState>& state)
    {
        int idx = mStates.IndexOf(state);
        if (idx < 0)
            return false;

        if (state == mVisibleState)
            mVisibleState = nullptr;

        mStates.RemoveAt(idx);

        return true;
    }

    void Widget::RemoveAllStates()
    {
        mVisibleState = nullptr;
        mFocusedState = nullptr;

        mStates.Clear();
    }

    void Widget::SetState(const String& name, bool state)
    {
        auto stateObj = GetStateObject(name);

        if (stateObj)
            stateObj->SetState(state);
    }

    void Widget::SetStateForcible(const String& name, bool state)
    {
        auto stateObj = GetStateObject(name);

        if (stateObj)
            stateObj->SetStateForcible(state);
    }

    bool Widget::GetState(const String& name) const
    {
        auto state = GetStateObject(name);

        if (state)
            return state->GetState();

        return false;
    }

    Ref<WidgetState> Widget::GetStateObject(const String& name) const
    {
        return mStates.FindOrDefault([&](auto state) { return state->name == name; });
    }

    const Vector<Ref<WidgetState>>& Widget::GetStates() const
    {
        return mStates;
    }

    void Widget::SetTransparency(float transparency)
    {
        mTransparency = transparency;
        UpdateTransparency();
    }

    float Widget::GetTransparency() const
    {
        return mTransparency;
    }

    float Widget::GetResTransparency() const
    {
        return mResTransparency;
    }

    void Widget::SetEnabledForcible(bool visible)
    {
        if (mVisibleState)
            mVisibleState->SetStateForcible(visible);

        mEnabled = visible;
        Widget::UpdateResEnabled();
        Widget::UpdateTransparency();
    }

    void Widget::Show(bool forcible /*= false*/)
    {
        if (forcible)
            SetEnabledForcible(true);
        else
            SetEnabled(true);
    }

    void Widget::Hide(bool forcible /*= false*/)
    {
        if (forcible)
            SetEnabledForcible(false);
        else
            SetEnabled(false);
    }

    void Widget::Focus()
    {
        o2UI.FocusWidget(Ref(this));
    }

    void Widget::Unfocus()
    {
        o2UI.FocusWidget(nullptr);
    }

    bool Widget::IsFocused() const
    {
        return mIsFocused;
    }

    bool Widget::IsFocusable() const
    {
        return mIsFocusable;
    }

    void Widget::SetFocusable(bool selectable)
    {
        mIsFocusable = selectable;
    }

    bool Widget::IsUnderPoint(const Vec2F& point)
    {
        return mDrawingScissorRect.IsInside(point) && layout->IsPointInside(point);
    }

    void Widget::SetIndexInSiblings(int index)
    {
        Actor::SetIndexInSiblings(index);

        if (mParentWidget)
            mParentWidget.Lock()->UpdateChildWidgetsList();
    }

    float Widget::GetMinWidthWithChildren() const
    {
        return GetLayoutData().minSize.x;
    }

    float Widget::GetMinHeightWithChildren() const
    {
        return GetLayoutData().minSize.y;
    }

    float Widget::GetWidthWeightWithChildren() const
    {
        return GetLayoutData().weight.x;
    }

    float Widget::GetHeightWeightWithChildren() const
    {
        return GetLayoutData().weight.y;
    }

    void Widget::UpdateBoundsWithChilds()
    {
        if ((!mResEnabledInHierarchy || mIsClipped) && GetLayoutData().dirtyFrame != o2Time.GetCurrentFrame())
            return;

        mBoundsWithChilds = mBounds;

        for (auto& child : mChildWidgets)
            mBoundsWithChilds.Expand(child->mBoundsWithChilds);

        if (mParentWidget)
            mParentWidget.Lock()->UpdateBoundsWithChilds();
    }

    void Widget::CheckClipping(const RectF& clipArea)
    {
        mIsClipped = !mBoundsWithChilds.IsIntersects(clipArea);

        for (auto& child : mChildWidgets)
            child->CheckClipping(clipArea);
    }

    void Widget::UpdateTransparency()
    {
        if (mParentWidget)
            mResTransparency = mTransparency * mParentWidget.Lock()->mResTransparency;
        else
            mResTransparency = mTransparency;

        for (auto& layer : mLayers)
            layer->UpdateResTransparency();

        for (auto& child : mChildWidgets)
            child->UpdateTransparency();

        for (auto& child : mInternalWidgets)
            child->UpdateTransparency();
    }

    void Widget::UpdateVisibility(bool updateLayout /*= true*/)
    {}

    void Widget::OnChildFocused(const Ref<Widget>& child)
    {
        if (mParentWidget)
            mParentWidget.Lock()->OnChildFocused(child);
    }

    void Widget::RetargetStatesAnimations()
    {
        for (auto& state : mStates)
        {
            state->mPlayer->SetTarget(this, false);
            state->mPlayer->relTime = state->GetState() ? 1.0f : 0.0f;
        }
    }

    void Widget::UpdateLayersLayouts()
    {
        for (auto& layer : mLayers)
            layer->UpdateLayout();

        UpdateBounds();
    }

    void Widget::UpdateBounds()
    {
        if ((!mResEnabledInHierarchy || mIsClipped) && GetLayoutData().dirtyFrame != o2Time.GetCurrentFrame())
            return;

        mBounds = GetLayoutData().worldRectangle;
        mBoundsWithChilds = mBounds;

        for (auto& layer : mDrawingLayers)
            mBounds.Expand(layer->GetRect());

        bool anyEnabled = false;
        for (auto& child : mChildWidgets)
        {
            if (child->mResEnabledInHierarchy)
            {
                anyEnabled = true;
                break;
            }
        }

        if (!anyEnabled)
            UpdateBoundsWithChilds();
    }

    void Widget::UpdateLayersDrawingSequence()
    {
        const float topLayersDepth = 1000.0f;

        mDrawingLayers.Clear();
        mTopDrawingLayers.Clear();

        for (auto& layer : mLayers)
        {
            if (layer->GetDrawable())
            {
                if (layer->mDepth < topLayersDepth)
                    mDrawingLayers.Add(layer);
                else
                    mTopDrawingLayers.Add(layer);
            }

            auto childLayers = layer->GetAllChilds();
            for (auto& childLayer : childLayers)
            {
                if (childLayer->GetDrawable())
                {
                    if (childLayer->mDepth < topLayersDepth)
                        mDrawingLayers.Add(childLayer);
                    else
                        mTopDrawingLayers.Add(childLayer);
                }
            }
        }

        mDrawingLayers.Sort([](auto& a, auto& b) { return a->mDepth < b->mDepth; });
        mTopDrawingLayers.Sort([](auto& a, auto& b) { return a->mDepth < b->mDepth; });
    }

    void Widget::SetParentWidget(const Ref<Widget>& widget)
    {
        SetParent(widget);
    }

    Ref<Widget> Widget::GetChildWidget(const String& path) const
    {
        auto actor = GetChild(path);
        return DynamicCast<Widget>(actor);
    }

    Ref<Actor> Widget::AddChild(const Ref<Actor>& actor)
    {
        return Actor::AddChild(actor);
    }

    Ref<Actor> Widget::AddChild(const Ref<Actor>& actor, int index)
    {
        Actor::AddChild(actor, index);
        UpdateChildWidgetsList();
        return actor;
    }

    Ref<Widget> Widget::AddChildWidget(const Ref<Widget>& widget)
    {
        return DynamicCast<Widget>(AddChild(widget));
    }

    Ref<Widget> Widget::AddChildWidget(const Ref<Widget>& widget, int position)
    {
        return DynamicCast<Widget>(AddChild(widget, position));
    }

    Vector<Ref<Widget>>& Widget::GetChildrenNonConst()
    {
        return mChildWidgets;
    }

    Vector<Ref<WidgetLayer>>& Widget::GetLayersNonConst()
    {
        return mLayers;
    }

    Vector<Ref<WidgetState>>& Widget::GetStatesNonConst()
    {
        return mStates;
    }

    Map<String, Ref<WidgetLayer>> Widget::GetAllLayers()
    {
        Map<String, Ref<WidgetLayer>> res;
        for (auto& layer : mLayers)
            res.Add(layer->name, layer);

        return res;
    }

    Map<String, Ref<Widget>> Widget::GetAllChilds()
    {
        Map<String, Ref<Widget>> res;
        for (auto& child : mChildWidgets)
            res.Add(child->GetName(), child);

        return res;
    }

    Map<String, Ref<Widget>> Widget::GetAllInternalWidgets()
    {
        Map<String, Ref<Widget>> res;
        for (auto& child : mInternalWidgets)
            res.Add(child->GetName(), child);

        return res;
    }

    Map<String, Ref<WidgetState>> Widget::GetAllStates()
    {
        Map<String, Ref<WidgetState>> res;
        for (auto& state : mStates)
            res.Add(state->name, state);

        return res;
    }

    void Widget::OnLayerAdded(const Ref<WidgetLayer>& layer)
    {}

    void Widget::OnStateAdded(const Ref<WidgetState>& state)
    {}

    void Widget::OnStatesListChanged()
    {
        auto statesCopy = mStates;
        mStates.Clear();

        for (auto& state : statesCopy)
            AddState(state);
    }

    void Widget::OnParentChanged(const Ref<Actor>& oldParent)
    {
        if (mState == Actor::State::Default)
            layout->SetDirty();

        mParentWidget = DynamicCast<Widget>(mParent.Lock());

        if (mParent && mParent.Lock()->IsOnScene())
            AddToScene();

        OnDrawbleParentChanged();
    }

    void Widget::OnChildrenChanged()
    {
        SortInheritedDrawables();
    }

    void Widget::OnChildAdded(const Ref<Actor>& child)
    {
        layout->SetDirty(false);

        Ref<Widget> widget = DynamicCast<Widget>(child);
        if (widget)
        {
            UpdateChildWidgetsList();

            OnChildAdded(widget);
        }

        Actor::OnChildAdded(child);
    }

    void Widget::OnChildAdded(const Ref<Widget>& child)
    {}

    void Widget::OnChildRemoved(const Ref<Actor>& child)
    {
        layout->SetDirty();

        auto widget = DynamicCast<Widget>(child);
        if (widget)
        {
            mChildWidgets.Remove(widget);
            mInternalWidgets.Remove(widget);

            OnChildRemoved(widget);
        }

        Actor::OnChildRemoved(child);
    }

    void Widget::OnChildRemoved(const Ref<Widget>& child)
    {}

    void Widget::OnRemoveFromScene()
    {
        Actor::OnRemoveFromScene();

#if IS_EDITOR
        o2Scene.mEditableObjects.Remove(DynamicCast<SceneEditableObject>(layersEditable));
        o2Scene.mEditableObjects.Remove(DynamicCast<SceneEditableObject>(internalChildrenEditable));
#endif

        for (auto& layer : mLayers)
            layer->OnRemoveFromScene();

        for (auto& child : mInternalWidgets)
            child->OnRemoveFromScene();

    }

	void Widget::OnAddToScene()
    {
        Actor::OnAddToScene();

        for (auto& child : mInternalWidgets)
            child->OnAddToScene();

        for (auto& layer : mLayers)
            layer->OnAddToScene();

#if IS_EDITOR
        o2Scene.mEditableObjects.Add(DynamicCast<SceneEditableObject>(layersEditable));
        o2Scene.mEditableObjects.Add(DynamicCast<SceneEditableObject>(internalChildrenEditable));
#endif
    }

    void Widget::UpdateChildWidgetsList()
    {
        mChildWidgets.Clear();
        for (auto& child : mChildren)
        {
            if (auto widget = DynamicCast<Widget>(child))
                mChildWidgets.Add(widget);
        }
    }

    WidgetLayoutData& Widget::GetLayoutData()
    {
        return *layout->mData;
    }

    const WidgetLayoutData& Widget::GetLayoutData() const
    {
        return *layout->mData;
    }

    void Widget::SetChildrenWorldRect(const RectF& childrenWorldRect)
    {
        layout->mData->childrenWorldRect = childrenWorldRect;
    }

    void Widget::ForceDraw(const RectF& area, float transparency)
    {
        Vec2F oldLayoutOffsetMin = GetLayoutData().offsetMin;
        Vec2F oldLayoutOffsetMax = GetLayoutData().offsetMax;
        float oldTransparency = mTransparency;
        auto oldParent = mParent;
        auto oldParentWidget = mParentWidget;
        bool oldResEnabledInHierarchy = mEnabled;

        GetLayoutData().offsetMin = area.LeftBottom();
        GetLayoutData().offsetMax = area.RightTop();
        mTransparency = transparency;
        mParent = nullptr;
        mParentWidget = nullptr;
        mIsClipped = false;

        SetEnabledForcible(true);

        UpdateSelfTransform();
        UpdateChildrenTransforms();
        UpdateTransparency();

        Draw();

        GetLayoutData().offsetMin = oldLayoutOffsetMin;
        GetLayoutData().offsetMax = oldLayoutOffsetMax;
        mTransparency = oldTransparency;
        mParent = oldParent;
        mParentWidget = oldParentWidget;
        mIsClipped = false;

        SetEnabledForcible(oldResEnabledInHierarchy);

        UpdateSelfTransform();
        UpdateChildrenTransforms();

        GetLayoutData().dirtyFrame = o2Time.GetCurrentFrame();

        UpdateBounds();
        UpdateBoundsWithChilds();
        UpdateTransparency();
    }

    void Widget::UpdateResEnabled(bool withChildren /*= true*/)
    {
        if (mVisibleState)
            mVisibleState->SetState(mEnabled);
        else
            Actor::UpdateResEnabled(withChildren);
    }

    void Widget::UpdateResEnabledInHierarchy(bool withChildren /*= true*/)
    {
        Actor::UpdateResEnabledInHierarchy(withChildren);

        for (auto& child : mInternalWidgets)
            child->UpdateResEnabledInHierarchy(withChildren);
    }

    void Widget::SetInternalParent(const Ref<Widget>& parent, bool worldPositionStays /*= false*/)
    {
        SetParent(parent, worldPositionStays);

        if (parent)
        {
            auto thisPtr = Ref<Widget>(this);
            parent->mChildren.Remove(thisPtr);
            parent->mChildWidgets.Remove(thisPtr);
            parent->mInternalWidgets.Add(thisPtr);
        }

        SetDrawingDepthInheritFromParent(false);
    }

    void Widget::AddInternalWidget(const Ref<Widget>& widget, bool worldPositionStays /*= false*/)
    {
        widget->SetInternalParent(Ref(this), worldPositionStays);
    }

	void Widget::RemoveInternalWidget(const Ref<Widget>& widget)
	{
		auto oldParent = widget->mParent.Lock();

        widget->mParent = nullptr;
		mInternalWidgets.RemoveFirst([&](auto& x) { return x == widget; });

		widget->OnParentChanged(oldParent);

		OnChildRemoved(widget);

		if (widget->mState != State::Destroyed)
		{
            widget->transform->SetDirty();
            widget->UpdateResEnabledInHierarchy();
		}

	}

	Ref<Widget> Widget::GetInternalWidget(const String& path) const
    {
        int delPos = path.Find("/");
        String pathPart = path.SubStr(0, delPos);

        if (pathPart == "..")
        {
            if (mParent)
            {
                if (delPos == -1)
                    return mParentWidget.Lock();
                else
                    return mParent.Lock()->GetChildByType<Widget>(path.SubStr(delPos + 1));
            }

            return nullptr;
        }

        for (auto& child : mInternalWidgets)
        {
            if (child->mName == pathPart)
            {
                if (delPos == -1)
                    return child;
                else
                    return child->GetChildByType<Widget>(path.SubStr(delPos + 1));
            }
        }

        return nullptr;
    }

    Ref<Widget> Widget::FindInternalWidget(const String& name) const
    {
        for (auto& widget : mInternalWidgets)
        {
            if (widget->GetName() == name)
                return widget;

            if (Ref<Widget> res = widget->FindChildByTypeAndName<Widget>(name))
                return res;
        }

        return nullptr;
    }

    void Widget::MoveAndCheckClipping(const Vec2F& delta, const RectF& clipArea)
    {
        mBoundsWithChilds += delta;
        mIsClipped = !mBoundsWithChilds.IsIntersects(clipArea);

        if (!mIsClipped)
            UpdateSelfTransform();

        for (auto& child : mChildWidgets)
            child->MoveAndCheckClipping(delta, clipArea);

        RectF childrenWorldRect = GetLayoutData().childrenWorldRect;
        GetLayoutData().childrenWorldRect = GetLayoutData().worldRectangle;

        for (auto& child : mInternalWidgets)
            child->MoveAndCheckClipping(delta, clipArea);

        GetLayoutData().childrenWorldRect = childrenWorldRect;
    }

#if IS_SCRIPTING_SUPPORTED
    WidgetLayout* Widget::GetLayoutScript()
    {
        return layout;
    }
#endif

#if IS_EDITOR

    bool Widget::isEditorLayersVisible = true;
    bool Widget::isEditorInternalChildrenVisible = true;

    void Widget::SetEditableParent(const Ref<SceneEditableObject>& object, int idx /*= -1*/)
    {
        if (auto inter = DynamicCast<InternalChildrenEditableEditable>(object))
            SetInternalParent(inter->widget.Lock());
        else
            Actor::SetEditableParent(object, idx);
    }

    Ref<SceneEditableObject> Widget::GetEditableParent() const
    {
        if (mParentWidget && mParentWidget.Lock()->mInternalWidgets.Contains(Ref(const_cast<Widget*>(this))))
        {
            return mParentWidget.Lock()->internalChildrenEditable;
        }

        return Actor::GetEditableParent();
    }

    Vector<Ref<SceneEditableObject>> Widget::GetEditableChildren() const
    {
        Vector<Ref<SceneEditableObject>> res = Actor::GetEditableChildren();

        if (isEditorInternalChildrenVisible)
            res.Insert(DynamicCast<SceneEditableObject>(internalChildrenEditable), 0);

        if (isEditorLayersVisible)
            res.Insert(DynamicCast<SceneEditableObject>(layersEditable), 0);

        return res;
    }

    void Widget::AddEditableChild(const Ref<SceneEditableObject>& object, int idx /*= -1*/)
    {
        idx -= 2; // Because of internal editable objects layers and internal children

        if (auto actor = DynamicCast<Actor>(object))
        {
            if (idx < 0)
                AddChild(actor);
            else
                AddChild(actor, idx);
        }
        else if (auto layer = DynamicCast<WidgetLayer>(object))
            AddLayer(layer);
    }

    bool Widget::IsSupportsTransforming() const
    {
        return true;
    }

    Basis Widget::GetTransform() const
    {
        return layout->GetWorldBasis();
    }

    void Widget::SetTransform(const Basis& transform)
    {
        layout->SetWorldBasis(transform);
    }

    bool Widget::IsSupportsLayout() const
    {
        return true;
    }

    Layout Widget::GetLayout() const
    {
        return Layout(layout->GetAnchorMin(), layout->GetAnchorMax(), layout->GetOffsetMin(), layout->GetOffsetMax());
    }

    void Widget::SetLayout(const Layout& layout)
    {
        this->layout->SetAnchorMin(layout.anchorMin);
        this->layout->SetAnchorMax(layout.anchorMax);
        this->layout->SetOffsetMin(layout.offsetMin);
        this->layout->SetOffsetMax(layout.offsetMax);
    }

    Ref<SceneEditableObject> Widget::GetEditableOwner()
    {
        return Ref(this);
    }

    void Widget::InitEditables()
    {
        layersEditable = mmake<LayersEditable>(Ref(this));
        internalChildrenEditable = mmake<InternalChildrenEditableEditable>(Ref(this));
    }

    Widget::LayersEditable::LayersEditable(RefCounter* refCounter) :
        SceneEditableObject(refCounter)
    {}

    Widget::LayersEditable::LayersEditable(RefCounter* refCounter, const Ref<Widget>& widget) :
        SceneEditableObject(refCounter), widget(widget)
    {}

    SceneUID Widget::LayersEditable::GetID() const
    {
        return UID;
    }

    void Widget::LayersEditable::GenerateNewID(bool childs /*= true*/)
    {
        UID = Math::Random();
    }

    const String& Widget::LayersEditable::GetName() const
    {
        static String name = "layers";
        return name;
    }

    void Widget::LayersEditable::SetName(const String& name)
    {}

    Vector<Ref<SceneEditableObject>> Widget::LayersEditable::GetEditableChildren() const
    {
        return widget.Lock()->mLayers.Convert<Ref<SceneEditableObject>>([](const Ref<WidgetLayer>& x) { return DynamicCast<SceneEditableObject>(x); });
    }

    Ref<SceneEditableObject> Widget::LayersEditable::GetEditableParent() const
    {
        return DynamicCast<SceneEditableObject>(widget.Lock());
    }

    void Widget::LayersEditable::SetEditableParent(const Ref<SceneEditableObject>& object, int idx /*= -1*/)
    {}

    void Widget::LayersEditable::AddEditableChild(const Ref<SceneEditableObject>& object, int idx /*= -1*/)
    {
        if (auto layer = DynamicCast<WidgetLayer>(object))
            widget.Lock()->AddLayer(layer);
    }

    void Widget::LayersEditable::SetIndexInSiblings(int idx)
    {}

    bool Widget::LayersEditable::IsSupportsDeleting() const
    {
        return false;
    }

    Basis Widget::LayersEditable::GetTransform() const
    {
        return widget.Lock()->GetTransform();
    }

    Ref<SceneEditableObject> Widget::LayersEditable::GetEditableLink() const
    {
        return prototypeLink.Lock();
    }

    Widget::InternalChildrenEditableEditable::InternalChildrenEditableEditable(RefCounter* refCounter) :
        SceneEditableObject(refCounter)
    {}

    Widget::InternalChildrenEditableEditable::InternalChildrenEditableEditable(RefCounter* refCounter, const Ref<Widget>& widget) :
        SceneEditableObject(refCounter), widget(widget)
    {}

    SceneUID Widget::InternalChildrenEditableEditable::GetID() const
    {
        return UID;
    }

    void Widget::InternalChildrenEditableEditable::GenerateNewID(bool childs /*= true*/)
    {
        UID = Math::Random();
    }

    const String& Widget::InternalChildrenEditableEditable::GetName() const
    {
        static String name = "internal children";
        return name;
    }

    void Widget::InternalChildrenEditableEditable::SetName(const String& name)
    {}

    Vector<Ref<SceneEditableObject>> Widget::InternalChildrenEditableEditable::GetEditableChildren() const
    {
        return widget.Lock()->mInternalWidgets.Convert<Ref<SceneEditableObject>>([](const Ref<Widget>& x) { return DynamicCast<SceneEditableObject>(x); });
    }

    Ref<SceneEditableObject> Widget::InternalChildrenEditableEditable::GetEditableParent() const
    {
        return DynamicCast<SceneEditableObject>(widget.Lock());
    }

    void Widget::InternalChildrenEditableEditable::SetEditableParent(const Ref<SceneEditableObject>& object, int idx /*= -1*/)
    {}

    void Widget::InternalChildrenEditableEditable::AddEditableChild(const Ref<SceneEditableObject>& object, int idx /*= -1*/)
    {
        if (auto widget = DynamicCast<Widget>(object))
            widget->SetInternalParent(widget);
    }

    void Widget::InternalChildrenEditableEditable::SetIndexInSiblings(int idx)
    {}

    bool Widget::InternalChildrenEditableEditable::IsSupportsDeleting() const
    {
        return false;
    }

    Basis Widget::InternalChildrenEditableEditable::GetTransform() const
    {
        return widget.Lock()->GetTransform();
    }

    Ref<SceneEditableObject> Widget::InternalChildrenEditableEditable::GetEditableLink() const
    {
        return prototypeLink.Lock();
    }

#endif // IS_EDITOR

}

DECLARE_TEMPLATE_CLASS(o2::LinkRef<o2::Widget>);
// --- META ---

DECLARE_CLASS(o2::Widget, o2__Widget);

#if  IS_EDITOR
DECLARE_CLASS(o2::Widget::LayersEditable, o2__Widget__LayersEditable);
#endif

#if  IS_EDITOR
DECLARE_CLASS(o2::Widget::InternalChildrenEditableEditable, o2__Widget__InternalChildrenEditableEditable);
#endif
// --- END META ---
