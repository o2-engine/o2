#include "o2/stdafx.h"
#include "Window.h"

#include "o2/Application/Application.h"
#include "o2/Render/Render.h"
#include "o2/Render/Sprite.h"
#include "o2/Render/Text.h"
#include "o2/Scene/UI/UIManager.h"
#include "o2/Scene/UI/WidgetLayer.h"
#include "o2/Scene/UI/WidgetLayout.h"
#include "o2/Scene/UI/WidgetState.h"
#include "o2/Scene/UI/Widgets/Button.h"
#include "o2/Utils/Debug/Debug.h"

namespace o2
{
    Window::Window(RefCounter* refCounter):
        ScrollArea(refCounter)
    {
        InitializeHandles();
        InitializeContextMenu();
    }

    Window::Window(RefCounter* refCounter, const Window& other):
        ScrollArea(refCounter, other), mHeadDragAreaLayout(other.mHeadDragAreaLayout),
        mTopDragAreaLayout(other.mTopDragAreaLayout), mBottomDragAreaLayout(other.mBottomDragAreaLayout),
        mLeftDragAreaLayout(other.mLeftDragAreaLayout), mRightDragAreaLayout(other.mRightDragAreaLayout),
        mLeftTopDragAreaLayout(other.mLeftTopDragAreaLayout), mRightTopDragAreaLayout(other.mRightTopDragAreaLayout),
        mLeftBottomDragAreaLayout(other.mLeftBottomDragAreaLayout), mRightBottomDragAreaLayout(other.mRightBottomDragAreaLayout)
    {
        RestoreControls();
        RetargetStatesAnimations();
        SetLayoutDirty();
        InitializeHandles();
    }

    Window::~Window()
    {}

    Window& Window::operator=(const Window& other)
    {
        ScrollArea::operator=(other);

        mHeadDragAreaLayout = other.mHeadDragAreaLayout;
        mTopDragAreaLayout = other.mTopDragAreaLayout;
        mBottomDragAreaLayout = other.mBottomDragAreaLayout;
        mLeftDragAreaLayout = other.mLeftDragAreaLayout;
        mRightDragAreaLayout = other.mRightDragAreaLayout;
        mLeftTopDragAreaLayout = other.mLeftTopDragAreaLayout;
        mRightTopDragAreaLayout = other.mRightTopDragAreaLayout;
        mLeftBottomDragAreaLayout = other.mLeftBottomDragAreaLayout;
        mRightBottomDragAreaLayout = other.mRightBottomDragAreaLayout;

        RestoreControls();
        RetargetStatesAnimations();
        BindHandlesInteractableToVisibility();
        SetLayoutDirty();

        return *this;
    }

    void Window::Draw()
    {
        PROFILE_SAMPLE_FUNC();

        if (!mResEnabledInHierarchy || mIsClipped) 
        {
            for (auto& child : mChildrenInheritedDepth)
                child->Draw();

            return;
        }

        mBackCursorArea->OnDrawn();

        for (auto& layer : mDrawingLayers)
            layer->Draw();

        IDrawable::OnDrawn();
        mWindowCursorArea->OnDrawn();

        o2Render.EnableScissorTest(mAbsoluteClipArea);

        for (auto& child : mChildrenInheritedDepth)
            child->Draw();

        o2Render.DisableScissorTest();

        ScrollArea::OnDrawn();

        mHeadDragHandle->OnDrawn();
        mTopDragHandle->OnDrawn();
        mBottomDragHandle->OnDrawn();
        mLeftDragHandle->OnDrawn();
        mRightDragHandle->OnDrawn();
        mLeftTopDragHandle->OnDrawn();
        mRightTopDragHandle->OnDrawn();
        mLeftBottomDragHandle->OnDrawn();
        mRightBottomDragHandle->OnDrawn();

        for (auto& child : mInternalWidgets)
            child->Draw();

        for (auto& layer : mTopDrawingLayers)
            layer->Draw();

        DrawDebugFrame();
    }

    void Window::ShowModal()
    {
        Show();
        SetModal(true);
    }

    void Window::SetIcon(const Ref<Sprite>& icon)
    {
        if (auto iconLayer = GetLayer(mIconLayerPath))
            iconLayer->SetDrawable(icon);
    }

    Ref<Sprite> Window::GetIcon() const
    {
        auto iconLayer = GetLayer(mIconLayerPath);
        if (iconLayer)
            return DynamicCast<Sprite>(iconLayer->GetDrawable());

        return nullptr;
    }

    void Window::SetIconLayout(const Layout& layout)
    {
        auto iconLayer = GetLayer(mIconLayerPath);
        if (iconLayer)
            iconLayer->layout = layout;
    }

    Layout Window::GetIconLayout() const
    {
        auto iconLayer = GetLayer(mIconLayerPath);
        if (iconLayer)
            return iconLayer->layout;

        return Layout();
    }

    void Window::SetCaption(const WString& caption)
    {
        auto captionLayer = GetLayer(mCaptionLayerPath);
        if (captionLayer)
        {
            if (auto textDrawable = DynamicCast<Text>(captionLayer->GetDrawable()))
                textDrawable->SetText(caption);
        }
    }

    WString Window::GetCaption() const
    {
        auto captionLayer = GetLayer(mCaptionLayerPath);
        if (captionLayer)
        {
            if (auto textDrawable = DynamicCast<Text>(captionLayer->GetDrawable()))
                return textDrawable->GetText();
        }

        return WString();
    }

    const Ref<ContextMenu>& Window::GetOptionsMenu() const
    {
        return mOptionsMenu;
    }

    void Window::SetDragAreaLayouts(const Layout& head, const Layout& top, const Layout&bottom, const Layout&left,
                                      const Layout& right, const Layout& leftTop, const Layout& rightTop,
                                      const Layout& leftBottom, const Layout& rightBottom)
    {
        mHeadDragAreaLayout = head;
        mTopDragAreaLayout = top;
        mBottomDragAreaLayout = bottom;
        mLeftDragAreaLayout = left;
        mRightDragAreaLayout = right;
        mLeftTopDragAreaLayout = leftTop;
        mRightTopDragAreaLayout = rightTop;
        mLeftBottomDragAreaLayout = leftBottom;
        mRightBottomDragAreaLayout = rightBottom;
    }

    bool Window::IsFocusable() const
    {
        return true;
    }

    void Window::SetModal(bool isModal)
    {
        mBackCursorArea->interactable = isModal;
    }

    bool Window::IsModal() const
    {
        return mBackCursorArea->IsInteractable();
    }

    void Window::UpdateSelfTransform()
    {
        ScrollArea::UpdateSelfTransform();

        mHeadDragAreaRect        = mHeadDragAreaLayout.Calculate(GetLayoutData().mWorldBox.ToRect());
        mTopDragAreaRect         = mTopDragAreaLayout.Calculate(GetLayoutData().mWorldBox.ToRect());
        mBottomDragAreaRect      = mBottomDragAreaLayout.Calculate(GetLayoutData().mWorldBox.ToRect());
        mLeftDragAreaRect        = mLeftDragAreaLayout.Calculate(GetLayoutData().mWorldBox.ToRect());
        mRightDragAreaRect       = mRightDragAreaLayout.Calculate(GetLayoutData().mWorldBox.ToRect());
        mLeftTopDragAreaRect     = mLeftTopDragAreaLayout.Calculate(GetLayoutData().mWorldBox.ToRect());
        mRightTopDragAreaRect    = mRightTopDragAreaLayout.Calculate(GetLayoutData().mWorldBox.ToRect());
        mLeftBottomDragAreaRect  = mLeftBottomDragAreaLayout.Calculate(GetLayoutData().mWorldBox.ToRect());
        mRightBottomDragAreaRect = mRightBottomDragAreaLayout.Calculate(GetLayoutData().mWorldBox.ToRect());
    }

    CursorEventsArea& Window::GetBackCursorListener()
    {
        return *mBackCursorArea;
    }

    void Window::InitializeHandles()
    {
        mBackCursorArea = mmake<CursorEventsArea>();
        mBackCursorArea->isUnderPoint = [&](const Vec2F& point) { return true; };
        mBackCursorArea->interactable = false;

        mWindowCursorArea = mmake<CursorEventsArea>();
        mWindowCursorArea->isUnderPoint = [&](const Vec2F& point) { return IsUnderPoint(point); };

        mHeadDragHandle = mmake<CursorEventsArea>();
        mHeadDragHandle->isUnderPoint = [&](const Vec2F& point) { return mHeadDragAreaRect.IsInside(point); };
        mHeadDragHandle->onMoved = [&](const Input::Cursor& cursor) { layout->position2D += cursor.delta; };
        mHeadDragHandle->onCursorPressed = [&](const Input::Cursor& cursor) { OnFocused(); };

        mTopDragHandle = mmake<CursorEventsArea>();
        mTopDragHandle->isUnderPoint = [&](const Vec2F& point) { return mTopDragAreaRect.IsInside(point); };
        mTopDragHandle->onMoved = [&](const Input::Cursor& cursor) { layout->worldTop += cursor.delta.y; };
        mTopDragHandle->onCursorPressed = [&](const Input::Cursor& cursor) { OnFocused(); };
        mTopDragHandle->cursorType = CursorType::SizeNS;

        mBottomDragHandle = mmake<CursorEventsArea>();
        mBottomDragHandle->isUnderPoint = [&](const Vec2F& point) { return mBottomDragAreaRect.IsInside(point); };
        mBottomDragHandle->onMoved = [&](const Input::Cursor& cursor) { layout->worldBottom += cursor.delta.y; };
        mBottomDragHandle->onCursorPressed = [&](const Input::Cursor& cursor) { OnFocused(); };
        mBottomDragHandle->cursorType = CursorType::SizeNS;

        mLeftDragHandle = mmake<CursorEventsArea>();
        mLeftDragHandle->isUnderPoint = [&](const Vec2F& point) { return mLeftDragAreaRect.IsInside(point); };
        mLeftDragHandle->onMoved = [&](const Input::Cursor& cursor) { layout->left += cursor.delta.x; };
        mLeftDragHandle->onCursorPressed = [&](const Input::Cursor& cursor) { OnFocused(); };
        mLeftDragHandle->cursorType = CursorType::SizeWE;

        mRightDragHandle = mmake<CursorEventsArea>();
        mRightDragHandle->isUnderPoint = [&](const Vec2F& point) { return mRightDragAreaRect.IsInside(point); };
        mRightDragHandle->onMoved = [&](const Input::Cursor& cursor) { layout->right += cursor.delta.x; };
        mRightDragHandle->onCursorPressed = [&](const Input::Cursor& cursor) { OnFocused(); };
        mRightDragHandle->cursorType = CursorType::SizeWE;

        mLeftTopDragHandle = mmake<CursorEventsArea>();
        mLeftTopDragHandle->isUnderPoint = [&](const Vec2F& point) { return mLeftTopDragAreaRect.IsInside(point); };
        mLeftTopDragHandle->onMoved = [&](const Input::Cursor& cursor) { layout->worldLeftTop += cursor.delta; };
        mLeftTopDragHandle->onCursorPressed = [&](const Input::Cursor& cursor) { OnFocused(); };
        mLeftTopDragHandle->cursorType = CursorType::SizeNwSe;

        mLeftBottomDragHandle = mmake<CursorEventsArea>();
        mLeftBottomDragHandle->isUnderPoint = [&](const Vec2F& point) { return mLeftBottomDragAreaRect.IsInside(point); };
        mLeftBottomDragHandle->onMoved = [&](const Input::Cursor& cursor) { layout->leftBottom += cursor.delta; };
        mLeftBottomDragHandle->onCursorPressed = [&](const Input::Cursor& cursor) { OnFocused(); };
        mLeftBottomDragHandle->cursorType = CursorType::SizeNeSw;

        mRightTopDragHandle = mmake<CursorEventsArea>();
        mRightTopDragHandle->isUnderPoint = [&](const Vec2F& point) { return mRightTopDragAreaRect.IsInside(point); };
        mRightTopDragHandle->onMoved = [&](const Input::Cursor& cursor) { layout->rightTop += cursor.delta; };
        mRightTopDragHandle->onCursorPressed = [&](const Input::Cursor& cursor) { OnFocused(); };
        mRightTopDragHandle->cursorType = CursorType::SizeNeSw;

        mRightBottomDragHandle = mmake<CursorEventsArea>();
        mRightBottomDragHandle->isUnderPoint = [&](const Vec2F& point) { return mRightBottomDragAreaRect.IsInside(point); };
        mRightBottomDragHandle->onMoved = [&](const Input::Cursor& cursor) { layout->rightBottom += cursor.delta; };
        mRightBottomDragHandle->onCursorPressed = [&](const Input::Cursor& cursor) { OnFocused(); };
        mRightBottomDragHandle->cursorType = CursorType::SizeNwSe;

        BindHandlesInteractableToVisibility();
    }

    void Window::SetHandlesInteractable(bool interactable)
    {
        mHeadDragHandle->interactable        = interactable;
        mTopDragHandle->interactable         = interactable;
        mBottomDragHandle->interactable      = interactable;
        mLeftDragHandle->interactable        = interactable;
        mRightDragHandle->interactable       = interactable;
        mLeftTopDragHandle->interactable     = interactable;
        mRightTopDragHandle->interactable    = interactable;
        mLeftBottomDragHandle->interactable  = interactable;
        mRightBottomDragHandle->interactable = interactable;
    }

    void Window::BindHandlesInteractableToVisibility()
    {
        if (mVisibleState)
        {
            mVisibleState->onStateBecomesTrue += [&]() { SetHandlesInteractable(true); };
            mVisibleState->onStateFullyFalse += [&]() { SetHandlesInteractable(false); };
        }
    }

    void Window::OnFocused()
    {
        if (mParent)
            SetIndexInSiblings(mParent.Lock()->GetChildren().Count() - 1);

        SetLastOnCurrentDepth();

        onFocused();
    }

    void Window::OnEnabled()
    {
        ScrollArea::OnEnabled();

        SetInteractable(true);
        Focus();
        onOpened();
    }
     
    void Window::OnDisabled()
    {
        ScrollArea::OnDisabled();

        SetInteractable(false);
        onClosed();
    }

    void Window::OnChildFocused(const Ref<Widget>& child)
    {
        OnFocused();
    }

    void Window::OnCursorPressed(const Input::Cursor& cursor)
    {
        o2UI.FocusWidget(Ref(this));
    }

    void Window::OnStateAdded(const Ref<WidgetState>& state)
    {
        BindHandlesInteractableToVisibility();
    }

    void Window::InitializeContextMenu()
    {
        mOptionsMenu = o2UI.CreateWidget<ContextMenu>("standard");

        if (!mOptionsMenu)
            return;

        mOptionsMenu->name = "options context";
        InitializeContextItems();
        AddInternalWidget(mOptionsMenu);

        auto optionsBtn = DynamicCast<Button>(mInternalWidgets.FindOrDefault(
            [](auto& x) { return x->GetName() == "optionsButton" && x->GetType() == TypeOf(Button); }));

        if (optionsBtn)
            optionsBtn->onClick += [=]() { mOptionsMenu->Show(optionsBtn->transform->worldCenter); };
    }

    void Window::InitializeContextItems()
    {
        mOptionsMenu->AddItem("Close", [&]() { Hide(); });
    }

    void Window::RestoreControls()
    {
        auto closeBtn = DynamicCast<Button>(mInternalWidgets.FindOrDefault(
            [](auto& x) { return x->GetName() == "closeButton" && x->GetType() == TypeOf(Button); }));

        if (closeBtn)
            closeBtn->onClick += [&]() { Hide(); };

        for (auto& element : mInternalWidgets)
        {
            if (element->GetName() == "options context" && element->GetType() == TypeOf(ContextMenu))
            {
                element->SetInternalParent(nullptr);
                break;
            }
        }

        InitializeContextMenu();
    }

    String Window::GetCreateMenuGroup()
    {
        return "Window";
    }
}

DECLARE_TEMPLATE_CLASS(o2::LinkRef<o2::Window>);
// --- META ---

DECLARE_CLASS(o2::Window, o2__Window);
// --- END META ---
