#include "o2/stdafx.h"
#include "MenuPanel.h"

#include "o2/Render/Sprite.h"
#include "o2/Render/Text.h"
#include "o2/Scene/UI/Widgets/HorizontalLayout.h"
#include "o2/Scene/UI/UIManager.h"
#include "o2/Scene/UI/WidgetLayer.h"
#include "o2/Scene/UI/WidgetLayout.h"
#include "o2/Scene/UI/WidgetState.h"

namespace o2
{
    MenuPanel::MenuPanel(RefCounter* refCounter):
        Widget(refCounter), DrawableCursorEventsListener(this)
    {
        mItemSample = mmake<Widget>();
        mItemSample->AddLayer("text", nullptr, Layout(Vec2F(0.0f, 0.0f), Vec2F(1.0f, 1.0f), Vec2F(200, 0), Vec2F(0, 0)));

        mSelectionDrawable = mmake<Sprite>();

        mLayout = mmake<HorizontalLayout>();
        AddChild(mLayout);

        mLayout->expandHeight = true;
        mLayout->expandWidth = false;
        mLayout->baseCorner = BaseCorner::LeftTop;
        mLayout->fitByChildren = true;
        *mLayout->layout = WidgetLayout::BothStretch();
    }

    MenuPanel::MenuPanel(RefCounter* refCounter, const MenuPanel& other):
        Widget(refCounter, other), DrawableCursorEventsListener(this)
    {
        mItemSample = other.mItemSample->CloneAsRef<Widget>();
        mSelectionDrawable = other.mSelectionDrawable->CloneAsRef<Sprite>();
        mSelectionLayout = other.mSelectionLayout;
        mLayout = FindChildByType<HorizontalLayout>();

        RetargetStatesAnimations();
    }

    MenuPanel::~MenuPanel()
    {}

    MenuPanel& MenuPanel::operator=(const MenuPanel& other)
    {
        Widget::operator=(other);

        mItemSample = other.mItemSample->CloneAsRef<Widget>();
        mSelectionDrawable = other.mSelectionDrawable->CloneAsRef<Sprite>();
        mSelectionLayout = other.mSelectionLayout;
        mLayout = FindChildByType<HorizontalLayout>();

        RetargetStatesAnimations();
        SetLayoutDirty();

        return *this;
    }

    void MenuPanel::Update(float dt)
    {
        PROFILE_SAMPLE_FUNC();

        Widget::Update(dt);

        if (!mResEnabledInHierarchy || mIsClipped)
            return;

        const float rectLerpCoef = 20.0f;
        if (mCurrentSelectionRect != mTargetSelectionRect)
        {
            mCurrentSelectionRect = Math::Lerp(mCurrentSelectionRect, mTargetSelectionRect, dt*rectLerpCoef);
            mSelectionDrawable->SetRect(mCurrentSelectionRect);
        }

        if (mSelectSubContextTime >= 0.0f)
        {
            mSelectSubContextTime -= dt;

            if (mSelectSubContextTime < 0.0f)
            {
                if (mOpenedContext)
                {
                    mOpenedContext.Lock()->HideWithChild();

                    if (mSelectedItem >= 0)
                    {
                        if (auto contextMenu = mLayout->mChildWidgets[mSelectedItem]->FindChildByType<ContextMenu>())
                        {
                            contextMenu->Show(mLayout->mChildWidgets[mSelectedItem]->layout->worldLeftBottom);
                            contextMenu->onHide = [=]() { mOpenedContext = nullptr; };
                            mOpenedContext = contextMenu;
                        }
                    }
                }
            }
        }
    }

    void MenuPanel::Draw()
    {
        PROFILE_SAMPLE_FUNC();

        if (!mResEnabledInHierarchy)
            return;

        for (auto& layer : mDrawingLayers)
            layer->Draw();

        IDrawable::OnDrawn();

        DrawInheritedDepthChildren();

        mSelectionDrawable->Draw();

        for (auto& layer : mTopDrawingLayers)
            layer->Draw();

        DrawDebugFrame();
    }

    Ref<Widget> MenuPanel::AddItem(const Item& item)
    {
        auto newItem = CreateItem(item);
        mLayout->AddChild(newItem);
        mClickFunctions.Add(item.onClick);

        return newItem;
    }

    Ref<ContextMenu> MenuPanel::CreateSubContext(WString& path)
    {
        int slashPos = path.Find("/");
        if (slashPos < 0)
            return nullptr;

        WString subMenu = path.SubStr(0, slashPos);

        auto subChild = mLayout->mChildWidgets.FindOrDefault([&](auto& x) {
            if (auto text = x->template GetLayerDrawable<Text>("text"))
                return text->text == subMenu;

            return false;
        });

        if (!subChild)
            subChild = AddItem(Item(subMenu, Function<void()>()));

        auto subContext = subChild->FindChildByType<ContextMenu>();
        if (!subContext)
        {
            subContext = o2UI.CreateWidget<ContextMenu>();
            subChild->AddChild(subContext);
        }

        path = path.SubStr(slashPos + 1);

        return subContext;
    }

    void MenuPanel::AddItem(const WString& path,
                            const Function<void()>& clickFunc /*= Function<void()>()*/,
                            const AssetRef<ImageAsset>& icon /*= AssetRef<ImageAsset>()*/,
                            const ShortcutKeys& shortcut /*= ShortcutKeys()*/)
    {
        WString itemPath = path;
        auto subContext = CreateSubContext(itemPath);
        if (!subContext)
        {
            AddItem(Item(path, clickFunc));
            return;
        }

        subContext->AddItem(itemPath, clickFunc, icon, shortcut);
    }

    void MenuPanel::AddToggleItem(const WString& path, bool value,
                                  const Function<void(bool)>& clickFunc /*= Function<void(bool)>()*/,
                                  const AssetRef<ImageAsset>& icon /*= AssetRef<ImageAsset>()*/,
                                  const ShortcutKeys& shortcut /*= ShortcutKeys()*/)
    {
        WString itemPath = path;
        auto subContext = CreateSubContext(itemPath);
        if (!subContext)
            return;

        subContext->AddToggleItem(itemPath, value, clickFunc, icon, shortcut);
    }

    void MenuPanel::InsertItem(const Item& item, int position)
    {
        auto newItem = CreateItem(item);
        mLayout->AddChild(newItem, position);

        if (item.subItems.Count() > 0)
        {
            mClickFunctions.Insert([=]()
            {
                newItem->FindChildByType<ContextMenu>()->Show(newItem->layout->worldRightBottom);
            }, position);
        }
        else mClickFunctions.Insert(item.onClick, position);
    }

    void MenuPanel::AddItems(Vector<Item> items)
    {
        for (auto item : items)
            AddItem(item);
    }

    void MenuPanel::InsertItems(Vector<Item> items, int position)
    {
        int i = 0;
        for (auto item : items)
        {
            InsertItem(item, position + i);
            i++;
        }
    }

    MenuPanel::Item MenuPanel::GetItem(int position)
    {
        if (position > 0 && position < mLayout->GetChildren().Count())
            return GetItemDef(position);

        return Item();
    }

    Vector<MenuPanel::Item> MenuPanel::GetItems() const
    {
        Vector<Item> res;
        for (int i = 0; i < mLayout->GetChildren().Count(); i++)
            res.Add(GetItemDef(i));

        return res;
    }

    void MenuPanel::RemoveItem(int position)
    {
        if (position > 0 && position < mLayout->GetChildren().Count())
            mLayout->RemoveChild(mLayout->GetChildren()[position]);
    }

    void MenuPanel::RemoveItem(const WString& path)
    {
        int slashPos = path.Find("/");
        if (slashPos < 0)
        {
            auto removingItem = mLayout->mChildWidgets.FindOrDefault([&](auto& x) {
                if (auto text = x->template GetLayerDrawable<Text>("text"))
                    return text->text == path;

                return false;
            });

            if (!removingItem)
            {
                o2Debug.LogError("Failed to remove menu item " + path);
                return;
            }

            mLayout->RemoveChild(removingItem);

            return;
        }

        WString subMenu = path.SubStr(0, slashPos);

        auto subChild = mLayout->mChildWidgets.FindOrDefault([&](auto& x) {
            if (auto text = x->template GetLayerDrawable<Text>("text"))
                return text->text == subMenu;

            return false;
        });

        if (!subChild)
        {
            o2Debug.LogError("Failed to remove menu item " + path);
            return;
        }

        auto subContext = subChild->FindChildByType<ContextMenu>();
        if (!subContext)
        {
            o2Debug.LogError("Failed to remove menu item " + path);
            return;
        }

        subContext->RemoveItem(path.SubStr(slashPos + 1));
    }

    void MenuPanel::RemoveAllItems()
    {
        mLayout->RemoveAllChildren();
    }

    const Ref<HorizontalLayout>& MenuPanel::GetItemsLayout() const
    {
        return mLayout;
    }

    const Ref<Widget>& MenuPanel::GetItemSample() const
    {
        return mItemSample;
    }

    void MenuPanel::SetItemSample(const Ref<Widget>& sample)
    {
        mItemSample = sample;
    }

    const Ref<Sprite>& MenuPanel::GetSelectionDrawable() const
    {
        return mSelectionDrawable;
    }

    void MenuPanel::SetSelectionDrawableLayout(const Layout& layout)
    {
        mSelectionLayout = layout;
    }

    Layout MenuPanel::GetSelectionDrawableLayout() const
    {
        return mSelectionLayout;
    }

    Ref<Widget> MenuPanel::CreateItem(const Item& item)
    {
        auto newItem = mItemSample->CloneAsRef<Widget>();
        newItem->name = (WString)"Menu Item:" + item.text;

        if (auto textLayer = newItem->GetLayerDrawable<Text>("text"))
            textLayer->text = item.text;

        if (item.subItems.Count() > 0)
        {
            auto subMenu = o2UI.CreateWidget<ContextMenu>();
            subMenu->AddItems(item.subItems);
            newItem->AddChild(subMenu);
        }

        return newItem;
    }

    MenuPanel::Item MenuPanel::GetItemDef(int idx) const
    {
        Item res;
        auto item = mLayout->mChildWidgets[idx];

        if (auto textLayer = item->GetLayerDrawable<Text>("text"))
            res.text = textLayer->text;

        if (auto subMenu = item->FindChildByType<ContextMenu>())
            res.subItems = subMenu->GetItems();

        res.onClick = mClickFunctions[idx];

        return res;
    }

    void MenuPanel::OnEnabled()
    {
        Widget::OnEnabled();
        interactable = true;
    }

    void MenuPanel::OnDisabled()
    {
        Widget::OnDisabled();
        interactable = false;
    }

    Ref<Widget> MenuPanel::GetItemUnderPoint(const Vec2F& point, int* idxPtr)
    {
        if (!mLayout)
            return nullptr;

        int idx = 0;
        for (auto& child : mLayout->mChildWidgets)
        {
            if (child->layout->IsPointInside(point))
            {
                if (idxPtr)
                    *idxPtr = idx;

                return child;
            }

            idx++;
        }

        if (idxPtr)
            *idxPtr = -1;

        return nullptr;
    }

    void MenuPanel::UpdateHover(const Vec2F& point)
    {
        int itemIdx = -1;
        auto itemUnderCursor = GetItemUnderPoint(point, &itemIdx);

        if (itemIdx < 0)
        {
            auto hoverState = state["hover"];
            if (hoverState)
            {
                mSelectionDrawable->SetEnabled(true);
                *hoverState = mOpenedContext != nullptr;
            }
            else
                mSelectionDrawable->SetEnabled(false);

            mSelectedItem = itemIdx;
        }
        else
        {
            mTargetSelectionRect = mSelectionLayout.Calculate(itemUnderCursor->layout->worldRect);

            auto hoverState = state["hover"];
            if (hoverState)
            {
                mSelectionDrawable->SetEnabled(true);
                *hoverState = true;
            }
            else
                mSelectionDrawable->SetEnabled(true);

            if (itemIdx != mSelectedItem)
            {
                mSelectSubContextTime = 0.5f;
                mSelectedItem = itemIdx;
            }
        }
    }

    void MenuPanel::OnCursorPressed(const Input::Cursor& cursor)
    {}

    void MenuPanel::OnCursorStillDown(const Input::Cursor& cursor)
    {}

    void MenuPanel::OnCursorReleased(const Input::Cursor& cursor)
    {
        int itemIdx = -1;
        auto itemUnderCursor = GetItemUnderPoint(cursor.position, &itemIdx);

        if (itemIdx >= 0)
        {
            mClickFunctions[itemIdx]();

            if (mOpenedContext)
                mOpenedContext.Lock()->HideWithChild();

            if (auto context = itemUnderCursor->FindChildByType<ContextMenu>())
            {
                context->Show(itemUnderCursor->layout->worldLeftBottom);
                mOpenedContext = context;
                mSelectSubContextTime = -1.0f;
            }
        }
    }

    void MenuPanel::OnCursorPressBreak(const Input::Cursor& cursor)
    {}

    void MenuPanel::OnCursorMoved(const Input::Cursor& cursor)
    {
        const float checkDeltaThreshold = 2.0f;
        if ((cursor.position - mLastSelectCheckCursor).Length() < checkDeltaThreshold)
            return;

        mLastSelectCheckCursor = cursor.position;

        UpdateHover(cursor.position);
    }

    void MenuPanel::OnCursorExit(const Input::Cursor& cursor)
    {
        UpdateHover(cursor.position);
    }

    MenuPanel::Item::Item()
    {}

    MenuPanel::Item::Item(const WString& text, const Vector<Ref<ContextMenu::Item>>& subItems):
        text(text), subItems(subItems)
    {}

    MenuPanel::Item::Item(const WString& text, const Function<void()> onClick) :
        text(text), onClick(onClick)
    {}

    bool MenuPanel::Item::operator==(const Item& other) const
    {
        return text == other.text;
    }

    String MenuPanel::GetCreateMenuGroup()
    {
        return "Dropping";
    }
}

DECLARE_TEMPLATE_CLASS(o2::LinkRef<o2::MenuPanel>);
// --- META ---

DECLARE_CLASS(o2::MenuPanel, o2__MenuPanel);

DECLARE_CLASS(o2::MenuPanel::Item, o2__MenuPanel__Item);
// --- END META ---
