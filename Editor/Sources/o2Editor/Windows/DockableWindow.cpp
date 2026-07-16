#include "o2Editor/stdafx.h"
#include "DockableWindow.h"

#include "o2/Animation/Tracks/AnimationColor4Track.h"
#include "o2/Animation/Tracks/AnimationFloatTrack.h"
#include "o2/Animation/Tracks/AnimationVec2FTrack.h"
#include "o2/Events/CursorAreaEventsListener.h"
#include "o2/Events/EventSystem.h"
#include "o2/Render/Render.h"
#include "o2/Render/Text.h"
#include "o2/Scene/UI/UIManager.h"
#include "o2/Scene/UI/WidgetLayer.h"
#include "o2/Scene/UI/WidgetLayout.h"
#include "o2/Scene/UI/WidgetState.h"
#include "o2/Utils/Editor/EditorScope.h"
#include "o2Editor/UIRoot.h"
#include "o2Editor/Windows/DockWindowPlace.h"

namespace Editor
{
    DockableWindow::DockableWindow(RefCounter* refCounter):
        Window(refCounter)
    {
        InitializeDragHandles();
        SetDocked(false);
        mDockingFrameSample = mmake<Sprite>();

        RetargetStatesAnimations();
    }

    DockableWindow::DockableWindow(RefCounter* refCounter, const DockableWindow& other):
        Window(refCounter, other)
    {
        PushEditorScopeOnStack scope;

        InitializeDragHandles();
        SetDocked(false);
        mDockingFrameSample = other.mDockingFrameSample->CloneAsRef<Sprite>();
        InitializeDockFrameAppearanceAnim();

        if (mVisibleState)
            mVisibleState->onStateFullyFalse += THIS_FUNC(Undock);

        RetargetStatesAnimations();
    }

    DockableWindow::~DockableWindow()
    {}

    DockableWindow& DockableWindow::operator=(const DockableWindow& other)
    {
        Window::operator=(other);

        mDockingFrameSample = other.mDockingFrameSample->CloneAsRef<Sprite>();

        if (mVisibleState)
            mVisibleState->onStateFullyFalse += THIS_FUNC(Undock);

        return *this;
    }

    void DockableWindow::Update(float dt)
    {
        Window::Update(dt);

        const float lerpFrameCoef = 10.0f;
        if (mDockingFrameTarget != mDockingFrameCurrent)
        {
            mDockingFrameCurrent = Math::Lerp(mDockingFrameCurrent, mDockingFrameTarget, dt*lerpFrameCoef);
            mDockingFrameSample->rect = mDockingFrameCurrent;
        }

        mDockingFrameAppearance->Update(dt);
    }

    void DockableWindow::Draw()
    {
        if (!mResEnabledInHierarchy)
            return;

        if (!mResEnabledInHierarchy || mIsClipped)
            return;

        for (auto& layer : mDrawingLayers)
            layer->Draw();

        mBackCursorArea->OnDrawn();
        //ScrollArea::OnDrawn();

        if (!mTabState || mTabActive)
        {
            o2Render.EnableScissorTest(mAbsoluteClipArea);

            DrawInheritedDepthChildren();

            o2Render.DisableScissorTest();

            mTopDragHandle->OnDrawn();
            mBottomDragHandle->OnDrawn();
            mLeftDragHandle->OnDrawn();
            mRightDragHandle->OnDrawn();
            mLeftTopDragHandle->OnDrawn();
            mRightTopDragHandle->OnDrawn();
            mLeftBottomDragHandle->OnDrawn();
            mRightBottomDragHandle->OnDrawn();
            mHeadDragHandle->OnDrawn();

            CursorAreaEventsListener::OnDrawn();

            for (auto& child : mInternalWidgets)
                child->Draw();

            for (auto& layer : mTopDrawingLayers)
                layer->Draw();
        }
        else
            mHeadDragHandle->OnDrawn();

        DrawDebugFrame();

        if (mDockingFrameSample->GetTransparency() > 0.001f)
            mDockingFrameSample->Draw();
    }

    void DockableWindow::SetDocked(bool docked)
    {
        mDocked = docked;

        auto dockedState = state["docked"];
        if (dockedState)
            *dockedState = docked;

        mTopDragHandle->interactable         = !docked;
        mBottomDragHandle->interactable      = !docked;
        mLeftDragHandle->interactable        = !docked;
        mRightDragHandle->interactable       = !docked;
        mLeftTopDragHandle->interactable     = !docked;
        mLeftBottomDragHandle->interactable  = !docked;
        mRightTopDragHandle->interactable    = !docked;
        mRightBottomDragHandle->interactable = !docked;
    }

    void DockableWindow::RecalculateTabWidth()
    {
        float expand = 35;
        if (auto textLayer = GetLayer(mTabCaptionLayerPath))
        {
            if (auto textDrawable = DynamicCast<Text>(textLayer->GetDrawable()))
            {
                Text::SymbolsSet symbolsSet;
                symbolsSet.Initialize(textDrawable->GetFont(), textDrawable->GetFontStyle(), textDrawable->GetText(),
                                      textDrawable->GetFontHeight(),
                                      Vec2F(), Vec2F(), HorAlign::Left, VerAlign::Bottom, false, false, 1.0f, 1.0f);

                SetTabWidth(symbolsSet.mRealSize.x + textLayer->layout.offsetLeft - textLayer->layout.offsetRight + expand);
            }
        }
    }

    bool DockableWindow::IsDocked() const
    {
        return mDocked;
    }

    const Ref<Sprite>& DockableWindow::GetDockingFrameSample() const
    {
        return mDockingFrameSample;
    }

    void DockableWindow::SetIcon(const Ref<Sprite>& icon)
    {
        auto iconLayer = GetLayer(mIconLayerPath);
        if (iconLayer)
        {
            iconLayer->SetDrawable(icon);
        }

        auto tabIconLayer = GetLayer(mTabIconLayerPath);
        if (tabIconLayer)
        {
            tabIconLayer->SetDrawable(icon->CloneAsRef<Sprite>());
        }
    }

    Ref<Sprite> DockableWindow::GetIcon() const
    {
        auto iconLayer = GetLayer(mIconLayerPath);
        if (iconLayer)
        {
            if (iconLayer->GetDrawable())
                return DynamicCast<Sprite>(iconLayer->GetDrawable());
        }

        return nullptr;
    }

    void DockableWindow::SetIconLayout(const Layout& layout)
    {
        auto iconLayer = GetLayer(mIconLayerPath);
        if (iconLayer)
            iconLayer->layout = layout;

        auto tabIconLayer = GetLayer(mTabIconLayerPath);
        if (tabIconLayer)
            tabIconLayer->layout = layout;
    }

    Layout DockableWindow::GetIconLayout() const
    {
        auto iconLayer = GetLayer(mIconLayerPath);
        if (iconLayer)
            return iconLayer->layout;

        return Layout();
    }

    void DockableWindow::SetCaption(const WString& caption)
    {
        auto captionLayer = GetLayer(mCaptionLayerPath);
        if (captionLayer)
        {
            if (auto textDrawable = DynamicCast<Text>(captionLayer->GetDrawable()))
                textDrawable->SetText(caption);
        }

        auto tabCaptionLayer = GetLayer(mTabCaptionLayerPath);
        if (tabCaptionLayer)
        {
            if (auto textDrawable = DynamicCast<Text>(tabCaptionLayer->GetDrawable()))
                textDrawable->SetText(caption);
        }

        if (mAutoCalculateTabWidth)
        {
            RecalculateTabWidth();

            if (auto parentDock = DynamicCast<DockWindowPlace>(mParent.Lock()))
                parentDock->ArrangeChildWindows();
        }
    }

    WString DockableWindow::GetCaption() const
    {
        auto captionLayer = GetLayer(mCaptionLayerPath);
        if (captionLayer)
        {
            if (auto textDrawable = DynamicCast<Text>(captionLayer->GetDrawable()))
                return textDrawable->GetText();
        }

        return WString();
    }

    void DockableWindow::SetTabWidth(float width)
    {
        mTabWidth = width;

        if (auto tabLayer = GetLayer(mTabLayerPath))
            tabLayer->layout.offsetRight = tabLayer->layout.offsetLeft + width;
    }

    float DockableWindow::GetTabWidth() const
    {
        return mTabWidth;
    }

    void DockableWindow::SetTabActive()
    {
        if (mTabState)
        {
            if (auto parentDock = DynamicCast<DockWindowPlace>(mParent.Lock()))
                parentDock->SetActiveTab(Ref(this));
        }
    }

    bool DockableWindow::IsTabActive() const
    {
        return mDocked && mTabActive;
    }

    void DockableWindow::SetAutoCalcuclatingTabWidth(bool enable)
    {
        mAutoCalculateTabWidth = enable;

        if (mAutoCalculateTabWidth)
            RecalculateTabWidth();
    }

    bool DockableWindow::IsAutoCalcuclatingTabWidth() const
    {
        return mAutoCalculateTabWidth;
    }

    void DockableWindow::UpdateSelfTransform()
    {
        Window::UpdateSelfTransform();

        if (auto tabLayer = GetLayer(mTabLayerPath))
            mHeadDragAreaRect = mHeadDragAreaLayout.Calculate(tabLayer->GetRect());
    }

    bool DockableWindow::IsUnderPoint(const Vec2F& point)
    {
        return !mTabState && Widget::IsUnderPoint(point);
    }

    String DockableWindow::GetCreateMenuCategory()
    {
        return "UI/Editor";
    }

    void DockableWindow::OnEnabled()
    {
        Window::OnEnabled();
    }

    void DockableWindow::OnDisabled()
    {
        Window::OnDisabled();
        //Undock();
    }

    void DockableWindow::OnFocused()
    {
        Window::OnFocused();
        SetTabActive();
    }

    void DockableWindow::InitializeDockFrameAppearanceAnim()
    {
        mDockingFrameAppearance->SetClip(mmake<AnimationClip>());
        mDockingFrameAppearance->SetTarget(this);

        *mDockingFrameAppearance->GetClip()->AddTrack<float>("mDockingFrameSample/transparency") =
            AnimationTrack<float>::EaseInOut(0, 1, 0.3f);

        *mDockingFrameAppearance->GetClip()->AddTrack<float>("transparency") =
            AnimationTrack<float>::EaseInOut(1, 0, 0.15f);

        mDockingFrameAppearance->GoToBegin();
    }

    void DockableWindow::InitializeDragHandles()
    {
        mHeadDragHandle->onMoved          = THIS_FUNC(OnMoved);
        mHeadDragHandle->onDblClicked     = THIS_FUNC(OnHeadDblCKicked);
        mHeadDragHandle->onCursorPressed  = THIS_FUNC(OnMoveBegin);
        mHeadDragHandle->onCursorReleased = THIS_FUNC(OnMoveCompleted);
    }

    void DockableWindow::OnHeadDblCKicked(const Input::Cursor& cursor)
    {
        if (IsDocked())
        {
            Undock();

            Vec2F size = layout->GetSize2D();

            if (auto headLayer = GetLayer(mTabLayerPath))
                layout->worldLeftTop = o2Input.GetCursorPos() - headLayer->GetRect().Size().InvertedY()*0.5f;
            else
                layout->worldLeftTop = o2Input.GetCursorPos();

            layout->worldRightBottom = layout->worldLeftTop + size.InvertedY();
        }
        else
        {
            auto listenersUnderCursor = o2Events.GetAllCursorListenersUnderCursor(0);
            auto dockPlaceListener = listenersUnderCursor.FindOrDefault([](const Ref<CursorAreaEventsListener>& x) {
                return DynamicCast<DockWindowPlace>(x) != nullptr;
            });

            if (dockPlaceListener)
                PlaceDock(DynamicCast<DockWindowPlace>(dockPlaceListener));
        }
    }

    void DockableWindow::OnMoved(const Input::Cursor& cursor)
    {
        if (mDocked)
        {
            if (!layout->IsPointInside(cursor.position))
            {
                Undock();

                SetLayoutDirty();

                Vec2F anchor = (layout->worldLeftTop + layout->worldRightTop.Get())*0.5f;

                if (auto headLayer = GetLayer(mTabLayerPath))
                    anchor.y -= headLayer->GetRect().Height()*0.5f;

                layout->worldPosition2D += o2Input.GetCursorPos() - anchor;

                mDragOffset = Vec2F();
            }

            return;
        }

        layout->worldPosition2D += cursor.delta;

        Ref<DockWindowPlace> targetDock;
        Side dockPosition = Side::None;
        RectF dockZoneRect;

        bool tracedDock = TraceDock(targetDock, dockPosition, dockZoneRect);

        if (dockPosition != Side::None && tracedDock)
        {
            mDockingFrameAppearance->PlayForward();
            mDockingFrameTarget = dockZoneRect;
        }
        else
        {
            mDockingFrameAppearance->PlayBack();
            mDockingFrameTarget = layout->worldRect;
        }
    }

    void DockableWindow::OnMoveCompleted(const Input::Cursor& cursor)
    {
        if (mDocked)
            return;

        Ref<DockWindowPlace> targetDock;
        Side dockPosition = Side::None;
        RectF dockZoneRect;

        if (!TraceDock(targetDock, dockPosition, dockZoneRect) || targetDock == nullptr)
            return;

        bool allOnLine = targetDock->mParent && targetDock->mParentWidget.Lock()->GetChildWidgets().All([&](auto& x) {

            if (x->GetType() != TypeOf(DockWindowPlace))
                return false;

            auto dock = DynamicCast<DockWindowPlace>(x);
            TwoDirection rd = dock->GetResizibleDir();

            if (dockPosition == Side::Left || dockPosition == Side::Right)
                return rd == TwoDirection::Horizontal;

            return rd == TwoDirection::Vertical;
        });

        if (allOnLine)
            PlaceLineDock(targetDock, dockPosition, dockZoneRect);
        else
            PlaceNonLineDock(targetDock, dockPosition);
    }

    void DockableWindow::OnMoveBegin(const Input::Cursor& cursor)
    {
        OnFocused();

        if (mDocked)
            mDragOffset = (Vec2F)o2Input.cursorPos - layout->worldLeftTop;
    }

    bool DockableWindow::TraceDock(Ref<DockWindowPlace>& targetDock, Side& dockPosition, RectF& dockZoneRect)
    {
        Vec2F cursorPos = o2Input.cursorPos;
        auto listenersUnderCursor = o2Events.GetAllCursorListenersUnderCursor(0);
        auto dockPlaceListener = listenersUnderCursor.FindOrDefault([](const Ref<CursorAreaEventsListener>& x) {
            return DynamicCast<DockWindowPlace>(x) != nullptr;
        });

        if (dockPlaceListener)
        {
            auto dockPlace = DynamicCast<DockWindowPlace>(dockPlaceListener);

            RectF dockPlaceRect = dockPlace->layout->worldRect;

            RectF leftZone(dockPlaceRect.left, dockPlaceRect.bottom,
                           Math::Lerp(dockPlaceRect.left, dockPlaceRect.right, mDockSizeCoef), dockPlaceRect.top);

            RectF rightZone(Math::Lerp(dockPlaceRect.right, dockPlaceRect.left, mDockSizeCoef), dockPlaceRect.bottom,
                            dockPlaceRect.right, dockPlaceRect.top);

            RectF bottomZone(dockPlaceRect.left, dockPlaceRect.bottom,
                             dockPlaceRect.right, Math::Lerp(dockPlaceRect.bottom, dockPlaceRect.top, mDockSizeCoef));

            if (bottomZone.IsInside(cursorPos))
            {
                targetDock = dockPlace;
                dockPosition = Side::Bottom;
                dockZoneRect = bottomZone;
            }
            else if (rightZone.IsInside(cursorPos))
            {
                targetDock = dockPlace;
                dockPosition = Side::Right;
                dockZoneRect = rightZone;
            }
            else if (leftZone.IsInside(cursorPos))
            {
                targetDock = dockPlace;
                dockPosition = Side::Left;
                dockZoneRect = leftZone;
            }
        }

        return targetDock != nullptr;
    }

    void DockableWindow::PlaceDock(const Ref<DockWindowPlace>& targetDock)
    {
        PushEditorScopeOnStack scope;

        mNonDockSize = layout->size2D;

        mTabPosition = targetDock->mChildren.Count();

        targetDock->AddChild(Ref(this));
        *layout = WidgetLayout::BothStretch();
        SetDocked(true);

        mDockingFrameAppearance->PlayBack();
        mDockingFrameTarget = layout->GetWorldRect();

        targetDock->ArrangeChildWindows();
        targetDock->SetLayoutDirty();
    }

    void DockableWindow::PlaceNonLineDock(const Ref<DockWindowPlace>& targetDock, Side dockPosition)
    {
        PushEditorScopeOnStack scope;

        mNonDockSize = layout->size2D;

        auto windowDock = mmake<DockWindowPlace>();
        windowDock->name = "window dock";
        *windowDock->layout = WidgetLayout::BothStretch();

        auto windowNeighborDock = mmake<DockWindowPlace>();
        windowNeighborDock->name = "empty dock";
        *windowNeighborDock->layout = WidgetLayout::BothStretch();

        for (auto& child : targetDock->GetChildWidgets())
            windowNeighborDock->AddChild(child);

        targetDock->AddChild(windowNeighborDock);
        targetDock->AddChild(windowDock);
        targetDock->interactable = false;

        if (dockPosition == Side::Bottom)
        {
            windowNeighborDock->layout->anchorBottom = mDockSizeCoef;
            windowDock->layout->anchorTop = mDockSizeCoef;
            windowDock->layout->offsetTop = -mDockBorder;

            windowDock->SetResizibleDir(TwoDirection::Vertical, mDockBorder, nullptr, windowNeighborDock);
            windowNeighborDock->SetResizibleDir(TwoDirection::Vertical, mDockBorder, windowDock, nullptr);
        }
        else if (dockPosition == Side::Right)
        {
            windowNeighborDock->layout->anchorRight = 1.0f - mDockSizeCoef;
            windowDock->layout->anchorLeft = 1.0f - mDockSizeCoef;
            windowDock->layout->offsetLeft = mDockBorder;

            windowDock->SetResizibleDir(TwoDirection::Horizontal, mDockBorder, windowNeighborDock, nullptr);
            windowNeighborDock->SetResizibleDir(TwoDirection::Horizontal, mDockBorder, nullptr, windowDock);
        }
        else if (dockPosition == Side::Left)
        {
            windowNeighborDock->layout->anchorLeft = mDockSizeCoef;
            windowDock->layout->anchorRight = mDockSizeCoef;
            windowDock->layout->offsetRight = -mDockBorder;

            windowDock->SetResizibleDir(TwoDirection::Horizontal, mDockBorder, nullptr, windowNeighborDock);
            windowNeighborDock->SetResizibleDir(TwoDirection::Horizontal, mDockBorder, windowDock, nullptr);
        }

        windowDock->AddChild(Ref(this));
        *layout = WidgetLayout::BothStretch();
        SetDocked(true);

        mDockingFrameAppearance->PlayBack();
        mDockingFrameTarget = layout->GetWorldRect();
    }

    void DockableWindow::PlaceLineDock(const Ref<DockWindowPlace>& targetDock, Side dockPosition, RectF dockZoneRect)
    {
        PushEditorScopeOnStack scope;

        mNonDockSize = layout->size2D;

        auto windowDock = mmake<DockWindowPlace>();
        windowDock->name = "window dock";
        *windowDock->layout = WidgetLayout::BothStretch();

        auto windowNeighborDock = targetDock;
        targetDock->mParent.Lock()->AddChild(windowDock);

        if (dockPosition == Side::Bottom)
        {
            windowDock->layout->anchorBottom = windowNeighborDock->layout->anchorBottom;

            windowNeighborDock->layout->anchorBottom +=
                windowNeighborDock->layout->height*mDockSizeCoef / windowNeighborDock->mParentWidget.Lock()->layout->height;

            windowDock->layout->anchorTop = windowNeighborDock->layout->anchorBottom;

            if (targetDock->mNeighborMin)
            {
                auto nmin = windowNeighborDock->mNeighborMin.Lock();

                nmin->SetResizibleDir(TwoDirection::Vertical, mDockBorder, nmin->mNeighborMin.Lock(), windowDock);
                windowDock->SetResizibleDir(TwoDirection::Vertical, mDockBorder, nmin, windowNeighborDock);
                windowNeighborDock->SetResizibleDir(TwoDirection::Vertical, mDockBorder, windowDock, windowNeighborDock->mNeighborMax.Lock());
            }
            else
            {
                windowDock->SetResizibleDir(TwoDirection::Vertical, mDockBorder, nullptr, windowNeighborDock);
                windowNeighborDock->SetResizibleDir(TwoDirection::Vertical, mDockBorder, windowDock, windowNeighborDock->mNeighborMax.Lock());
            }
        }
        else if (dockPosition == Side::Right)
        {
            windowDock->layout->anchorRight = windowNeighborDock->layout->anchorRight;

            windowNeighborDock->layout->anchorRight -=
                windowNeighborDock->layout->width*mDockSizeCoef / windowNeighborDock->mParentWidget.Lock()->layout->width;

            windowDock->layout->anchorLeft = windowNeighborDock->layout->anchorRight;

            if (targetDock->mNeighborMax)
            {
                auto nmax = windowNeighborDock->mNeighborMax.Lock();

                nmax->SetResizibleDir(TwoDirection::Horizontal, mDockBorder, windowDock, nmax->mNeighborMax.Lock());
                windowDock->SetResizibleDir(TwoDirection::Horizontal, mDockBorder, windowNeighborDock, nmax);
                windowNeighborDock->SetResizibleDir(TwoDirection::Horizontal, mDockBorder, windowNeighborDock->mNeighborMin.Lock(), windowDock);
            }
            else
            {
                windowDock->SetResizibleDir(TwoDirection::Horizontal, mDockBorder, windowNeighborDock, nullptr);
                windowNeighborDock->SetResizibleDir(TwoDirection::Horizontal, mDockBorder, windowNeighborDock->mNeighborMin.Lock(), windowDock);
            }
        }
        else if (dockPosition == Side::Left)
        {
            windowDock->layout->anchorLeft = windowNeighborDock->layout->anchorLeft;

            windowNeighborDock->layout->anchorLeft +=
                windowNeighborDock->layout->width*mDockSizeCoef / windowNeighborDock->mParentWidget.Lock()->layout->width;

            windowDock->layout->anchorRight = windowNeighborDock->layout->anchorLeft;

            if (targetDock->mNeighborMin)
            {
                auto nmin = windowNeighborDock->mNeighborMin.Lock();

                nmin->SetResizibleDir(TwoDirection::Horizontal, mDockBorder, nmin->mNeighborMin.Lock(), windowDock);
                windowDock->SetResizibleDir(TwoDirection::Horizontal, mDockBorder, nmin, windowNeighborDock);
                windowNeighborDock->SetResizibleDir(TwoDirection::Horizontal, mDockBorder, windowDock, windowNeighborDock->mNeighborMax.Lock());
            }
            else
            {
                windowDock->SetResizibleDir(TwoDirection::Horizontal, mDockBorder, nullptr, windowNeighborDock);
                windowNeighborDock->SetResizibleDir(TwoDirection::Horizontal, mDockBorder, windowDock, windowNeighborDock->mNeighborMax.Lock());
            }
        }

        windowDock->AddChild(Ref(this));
        *layout = WidgetLayout::BothStretch();
        SetDocked(true);

        mDockingFrameAppearance->PlayBack();
        mDockingFrameTarget = layout->GetWorldRect();
    }

    void DockableWindow::SetTabState(float offset, int position, bool isFirst)
    {
        SetStateForcible("tab", true);

        if (mAutoCalculateTabWidth)
            RecalculateTabWidth();

        if (auto tabFirstState = GetStateObject("tabFirst"))
            tabFirstState->SetState(isFirst);

        mTabPosition = position;

        if (auto tabMainLayer = GetLayer(mTabLayerPath))
        {
            tabMainLayer->layout.offsetLeft = offset;
            tabMainLayer->layout.offsetRight = offset + mTabWidth;
            tabMainLayer->layout.anchorLeft = 0;
            tabMainLayer->layout.anchorRight = 0;
        }

        mTabState = true;
    }

    void DockableWindow::SetNonTabState()
    {
        SetStateForcible("tab", false);

        mTabActive = false;

        if (auto state = GetStateObject("tabActive"))
            state->SetState(false);

        if (auto tabMainLayer = GetLayer(mTabLayerPath))
        {
            tabMainLayer->layout.offsetLeft = 0;
            tabMainLayer->layout.offsetRight = 0;
            tabMainLayer->layout.anchorLeft = 0;
            tabMainLayer->layout.anchorRight = 1;
        }

        mTabState = false;
    }

    void DockableWindow::SetActiveTab()
    {
        if (!mTabState)
            return;

        mTabActive = true;

        if (auto state = GetStateObject("tabActive"))
            state->SetState(true);
    }

    void DockableWindow::Undock()
    {
        PushEditorScopeOnStack scope;

        if (!IsDocked())
            return;

        auto topDock = DynamicCast<DockWindowPlace>(mParent.Lock()->GetParent().Lock());

        if (!topDock)
        {
            EditorUIRoot.AddWidget(Ref(this));
        }
        else
        {
            auto parent = DynamicCast<DockWindowPlace>(mParent.Lock());
            auto parentNeighbors = topDock->GetChildWidgets().FindAll([&](auto x) { return x != parent; })
                .Convert<Ref<DockWindowPlace>>([](auto& x) { return DynamicCast<DockWindowPlace>(x); });

            EditorUIRoot.AddWidget(Ref(this));

            if (!parent->GetChildWidgets().IsEmpty())
            {
                parent->ArrangeChildWindows();
                parent->SetLayoutDirty();
            }
            else
            {
                auto neighborMin = parent->mNeighborMin.Lock();
                auto neighborMax = parent->mNeighborMax.Lock();

                if (neighborMin)
                {
                    neighborMin->SetResizibleDir(neighborMin->mResizibleDir, mDockBorder,
                                                 neighborMin->mNeighborMin.Lock(), neighborMax);
                }

                if (neighborMax)
                {
                    neighborMax->SetResizibleDir(neighborMax->mResizibleDir, mDockBorder,
                                                 neighborMin, neighborMax->mNeighborMax.Lock());
                }

                if (parent->mResizibleDir == TwoDirection::Horizontal)
                {
                    if (neighborMin && neighborMax)
                    {
                        float anchor = (neighborMin->layout->anchorRight + neighborMax->layout->anchorLeft) / 2.0f;
                        neighborMin->layout->anchorRight = anchor;
                        neighborMax->layout->anchorLeft = anchor;
                    }
                    else if (neighborMin && !neighborMax)
                        neighborMin->layout->anchorRight = 1.0f;
                    else if (!neighborMin && neighborMax)
                        neighborMax->layout->anchorLeft = 0.0f;
                }

                if (parent->mResizibleDir == TwoDirection::Vertical)
                {
                    if (neighborMin && neighborMax)
                    {
                        float anchor = (neighborMin->layout->anchorTop + neighborMax->layout->anchorBottom) / 2.0f;
                        neighborMin->layout->anchorTop = anchor;
                        neighborMax->layout->anchorBottom = anchor;
                    }
                    else if (neighborMin && !neighborMax)
                        neighborMin->layout->anchorTop = 1.0f;
                    else if (!neighborMin && neighborMax)
                        neighborMax->layout->anchorBottom = 0.0f;
                }

                topDock->RemoveChild(parent);

                if (parentNeighbors.Count() == 1)
                {
                    auto children = parentNeighbors[0]->GetChildWidgets();
                    for (auto& child : children)
                        topDock->AddChild(child);

                    topDock->RemoveChild(parentNeighbors[0]);
                }

                // 
                //     for (auto& child : parentNeighbor->GetChilds())
                //         topDock->AddChild(child);
                // 
                //     topDock->RemoveChild(parent);
                //     topDock->RemoveChild(parentNeighbor);
                topDock->CheckInteractable();
            }
        }

        SetDocked(false);
        SetNonTabState();

        Vec2F leftTop = layout->worldLeftTop;
        layout->anchorMin = Vec2F(); layout->anchorMax = Vec2F();
        layout->worldLeftTop = leftTop;
        layout->worldRightBottom = layout->worldLeftTop + mNonDockSize.InvertedY();
    }
}

DECLARE_TEMPLATE_CLASS(o2::LinkRef<Editor::DockableWindow>);
// --- META ---

DECLARE_CLASS(Editor::DockableWindow, Editor__DockableWindow);
// --- END META ---
