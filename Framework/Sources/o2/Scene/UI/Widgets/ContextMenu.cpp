#include "o2/stdafx.h"
#include "ContextMenu.h"

#include "o2/Assets/Types/ImageAsset.h"
#include "o2/Render/Render.h"
#include "o2/Render/Sprite.h"
#include "o2/Render/Text.h"
#include "o2/Scene/UI/UIManager.h"
#include "o2/Scene/UI/WidgetLayer.h"
#include "o2/Scene/UI/WidgetLayout.h"
#include "o2/Scene/UI/WidgetState.h"
#include "o2/Scene/UI/Widgets/Button.h"
#include "o2/Scene/UI/Widgets/VerticalLayout.h"
#include "o2/Utils/Editor/EditorScope.h"
#include "o2/Utils/FileSystem/FileSystem.h"

namespace o2
{
    FORWARD_REF_IMPL(VerticalLayout);

    bool ContextMenu::Item::operator==(const Item& other) const
    {
        return text == other.text && mShortcut == other.mShortcut && icon == other.icon;
    }

    ContextMenu::Item::Item(RefCounter* refCounter) :
        RefCounterable(refCounter), checked(false), checkable(false)
    {}

    ContextMenu::Item::Item(RefCounter* refCounter, const WString& text, const Function<void()> onClick,
                            const WString& group /*= ""*/, const AssetRef<ImageAsset>& icon /*= AssetRef<ImageAsset>()*/,
                            const ShortcutKeys& shortcut /*= ShortcutKeys()*/) :
        RefCounterable(refCounter), text(text), group(group), onClick(onClick), mShortcut(shortcut), icon(icon), checked(false), checkable(false)
    {
        SetShortcut(shortcut);
    }

    ContextMenu::Item::Item(RefCounter* refCounter, const WString& text, const Vector<Ref<Item>>& subItems,
                            const WString& group /*= ""*/, const AssetRef<ImageAsset>& icon /*= AssetRef<ImageAsset>()*/) :
        RefCounterable(refCounter), text(text), group(group), subItems(subItems), icon(icon), checked(false), checkable(false)
    {}

    ContextMenu::Item::Item(RefCounter* refCounter, const WString& text, bool checked,
                            Function<void(bool)> onChecked /*= Function<void(bool)>()*/,
                            const WString& group /*= ""*/, const AssetRef<ImageAsset>& icon /*= AssetRef<ImageAsset>()*/,
                            const ShortcutKeys& shortcut /*= ShortcutKeys()*/) :
        RefCounterable(refCounter), text(text), group(group), checked(checked), onChecked(onChecked), checkable(true), mShortcut(shortcut), icon(icon)
    {
        SetShortcut(shortcut);
    }

    ContextMenu::Item::Item(RefCounter* refCounter, const Item& other):
        RefCounterable(refCounter), text(other.text), group(other.group), icon(other.icon), checked(other.checked), 
        checkable(other.checkable), onClick(other.onClick), onChecked(other.onChecked), mShortcut(other.mShortcut)
    {
        SetShortcut(mShortcut);
    }

    ContextMenu::Item::~Item()
    {}

    void ContextMenu::Item::SetShortcut(const ShortcutKeys& shortcut)
    {
        mShortcut = shortcut;
        ShortcutKeysListener::SetShortcut(shortcut);
    }

    const ShortcutKeys& ContextMenu::Item::GetShortcut() const
    {
        return mShortcut;
    }

    void ContextMenu::Item::SetMaxPriority()
    {
        ShortcutKeysListener::SetMaxPriority();

        for (auto item : subItems)
            item->SetMaxPriority();
    }

    void ContextMenu::Item::SetMinPriority()
    {
        ShortcutKeysListener::SetMinPriority();

        for (auto item : subItems)
            item->SetMinPriority();
    }

    void ContextMenu::Item::RecursiveSearchSubItems(const WString& text, Vector<Ref<Item>>& result)
    {
        if (subItems.IsEmpty())
        {
            if (this->text.ToLowerCase().Contains(text))
                result.Add(mmake<Item>(*this));
        }
        else
        {
            for (auto item : subItems)
                item->RecursiveSearchSubItems(text, result);
        }
    }

    Ref<ContextMenu::Item> ContextMenu::Item::Separator()
    {
        return mmake<Item>(separatorText, Function<void()>());
    }

    WString ContextMenu::Item::separatorText = "---";

    void ContextMenu::Item::OnShortcutPressed()
    {
        onClick();
        onChecked(checked);
    }

    ContextMenu::ContextMenu(RefCounter* refCounter) :
        PopupWidget(refCounter)
    {
        mItemSample = mmake<ContextMenuItem>();
        mItemSample->RemoveFromScene();
        mItemSample->layout->minHeight = 20.0f;
        auto basicLayer = mItemSample->AddLayer("basic", nullptr);
        basicLayer->AddChildLayer("icon", nullptr, Layout(Vec2F(0.0f, 0.5f), Vec2F(0.0f, 0.5f), Vec2F(10, 0), Vec2F(10, 0)));
        basicLayer->AddChildLayer("subIcon", nullptr, Layout(Vec2F(1.0f, 0.5f), Vec2F(1.0f, 0.5f), Vec2F(-10, 0), Vec2F(-10, 0)));
        basicLayer->AddChildLayer("caption", nullptr, Layout(Vec2F(0.0f, 0.0f), Vec2F(1.0f, 1.0f), Vec2F(20, 0), Vec2F(0, 0)));
        basicLayer->AddChildLayer("shortcut", nullptr, Layout(Vec2F(0.0f, 0.0f), Vec2F(1.0f, 1.0f), Vec2F(20, 0), Vec2F(0, 0)));

        mSeparatorSample = mmake<Widget>();
        mSeparatorSample->RemoveFromScene();
        mSeparatorSample->layout->minHeight = 3.0f;
        mSeparatorSample->layout->height = 3.0f;

        mSelectionDrawable = mmake<Sprite>();

        mItemsLayout = mmake<VerticalLayout>();
        AddChild(mItemsLayout);

        mItemsLayout->expandHeight = false;
        mItemsLayout->expandWidth = true;
        mItemsLayout->baseCorner = BaseCorner::LeftTop;
        mItemsLayout->fitByChildren = true;
        *mItemsLayout->layout = WidgetLayout::BothStretch();
    }

    ContextMenu::ContextMenu(RefCounter* refCounter, const Vector<Ref<Item>>& items) :
        ContextMenu(refCounter)
    {
        AddItems(items);
    }

    ContextMenu::ContextMenu(RefCounter* refCounter, const ContextMenu& other) :
        PopupWidget(refCounter, other), mMaxVisibleItems(other.mMaxVisibleItems)
    {
        mItemSample = other.mItemSample->CloneAsRef<ContextMenuItem>();
        mItemSample->RemoveFromScene();

        mSeparatorSample = other.mSeparatorSample->CloneAsRef<Widget>();
        mSeparatorSample->RemoveFromScene();

        mSelectionDrawable = other.mSelectionDrawable->CloneAsRef<Sprite>();
        mSelectionLayout = other.mSelectionLayout;
        mItemsLayout = FindChildByType<VerticalLayout>();

        mSearchPanel = mItemsLayout->GetChildByType<Widget>("search panel");
        if (mSearchPanel)
        {
            mSearchEditBox = mSearchPanel->FindChildByType<EditBox>();
            if (mSearchEditBox)
                mSearchEditBox->onChanged = THIS_FUNC(OnSearchChanged);
        }

        RetargetStatesAnimations();
    }

    ContextMenu::~ContextMenu()
    {}

    ContextMenu& ContextMenu::operator=(const ContextMenu& other)
    {
        RemoveAllItems();

        mItemSample = other.mItemSample->CloneAsRef<ContextMenuItem>();
        mSeparatorSample = other.mSeparatorSample->CloneAsRef<Widget>();
        mSelectionDrawable = other.mSelectionDrawable->CloneAsRef<Sprite>();
        mSelectionLayout = other.mSelectionLayout;

        PopupWidget::operator=(other);

        mItemsLayout = FindChildByType<VerticalLayout>();
        mMaxVisibleItems = other.mMaxVisibleItems;

        mItemSample->RemoveFromScene();
        mSeparatorSample->RemoveFromScene();

        RetargetStatesAnimations();
        SetLayoutDirty();

        return *this;
    }

    void ContextMenu::Update(float dt)
    {
        PROFILE_SAMPLE_FUNC();

        if (!mResEnabledInHierarchy)
            return;

        PopupWidget::Update(dt);

        const float rectLerpCoef = 20.0f;
        if (mCurrentSelectionRect != mTargetSelectionRect) {
            mCurrentSelectionRect = Math::Lerp(mCurrentSelectionRect, mTargetSelectionRect, dt * rectLerpCoef);
            mSelectionDrawable->SetRect(mCurrentSelectionRect);
        }

        bool cursorPressed = o2Input.IsCursorPressed() || o2Input.IsRightMousePressed();
        if (cursorPressed) {
            if (!mChildPopup && (cursorPressed || Math::Abs(o2Input.GetMouseWheelDelta()) > 0.1f) &&
                !layout->IsPointInside(o2Input.GetCursorPos()) && !mShownAtFrame && mEnabled) {
                HideWithParent();
            }
        }

        if (mSelectSubContextTime >= 0.0f) {
            mSelectSubContextTime -= dt;

            if (mSelectSubContextTime < 0.0f && GetItemUnderPoint(o2Input.GetCursorPos()) == mSelectedItem) {
                if (mChildPopup)
                    mChildPopup.Lock()->HideWithChild();

                mChildPopup = nullptr;

                if (mSelectedItem && mSelectedItem->GetSubMenu())
                    mSelectedItem->GetSubMenu()->Show(Ref(this), mSelectedItem->layout->worldRightTop);
            }
        }

        mShownAtFrame = false;
    }

    void ContextMenu::Show(const Ref<PopupWidget>& parent, const Vec2F& position /*= o2Input.GetCursorPos()*/)
    {
        if (mSearchEnabled)
        {
            mSearchEditBox->text = "";
            mSearchEditBox->Focus();
        }

        if (!mItemsBuilt)
            RebuildItems();

        PopupWidget::Show(parent, position);

        auto hoverState = state["hover"];
        if (hoverState) 
        {
            mSelectionDrawable->SetEnabled(true);
            *hoverState = false;
        }
        else 
            mSelectionDrawable->SetEnabled(false);
    }

    void ContextMenu::Show(const Vec2F& position /*= o2Input.GetCursorPos()*/)
    {
        Show(nullptr, position);
    }

    void ContextMenu::SetSearchEnabled(bool enabled)
    {
        mSearchEnabled = enabled;

        if (mSearchPanel)
            mSearchPanel->enabled = enabled;
    }

    bool ContextMenu::IsSearchEnabled() const
    {
        return mSearchEnabled;
    }

    void ContextMenu::AddItem(const Ref<Item>& item)
    {
        mItems.Add(item);
        ForceRebuildItems();
    }

    Ref<ContextMenu::Item>  ContextMenu::AddItem(const WString& path,
                              const Function<void()>& clickFunc /*= Function<void()>()*/,
                              const AssetRef<ImageAsset>& icon /*= AssetRef<ImageAsset>()*/,
                              const ShortcutKeys& shortcut /*= ShortcutKeys()*/)
    {
        WString targetPath = path;
        auto& itemsList = CreateItemsByPath(targetPath);
        int groupDelPos = targetPath.FindLast(".");

        auto item = mmake<Item>(targetPath.SubStr(0, groupDelPos), clickFunc,
                                groupDelPos < 0 ? "" : targetPath.SubStr(groupDelPos + 1),
                                icon, shortcut);

        itemsList.Add(item);
        ForceRebuildItems();

        return item;
    }

    Ref<ContextMenu::Item>  ContextMenu::AddToggleItem(const WString& path, bool value,
                                    const Function<void(bool)>& clickFunc /*= Function<void(bool)>()*/,
                                    const AssetRef<ImageAsset>& icon /*= AssetRef<ImageAsset>()*/,
                                    const ShortcutKeys& shortcut /*= ShortcutKeys()*/)
    {
        WString targetPath = path;
        auto& itemsList = CreateItemsByPath(targetPath);
        int groupDelPos = targetPath.FindLast(".");

        auto item = mmake<Item>(targetPath.SubStr(0, groupDelPos), value, clickFunc,
                                groupDelPos < 0 ? "" : targetPath.SubStr(groupDelPos + 1),
                                icon, shortcut);
        itemsList.Add(item);
        ForceRebuildItems();

        return item;
    }

    Vector<Ref<ContextMenu::Item>>& ContextMenu::CreateItemsByPath(WString& path)
    {
        Vector<Ref<Item>>* items = &mItems;

        while (true) {
            int slashPos = path.Find("/");
            if (slashPos < 0)
                break;

            WString subMenu = path.SubStr(0, slashPos);

            auto subItem = items->Find([&](auto& x) { return x->text == subMenu; });

            if (!subItem) {
                items->Add(mmake<Item>(subMenu, Function<void()>(), o2FileSystem.GetFileExtension(subMenu)));
                subItem = &items->Last();
            }

            items = &(*subItem)->subItems;
            path = path.SubStr(slashPos + 1);
        }

        return *items;
    }

    void ContextMenu::InsertItem(const Ref<Item>& item, int position)
    {
        mItems.Insert(item, position);
        ForceRebuildItems();
    }

    void ContextMenu::AddItems(const Vector<Ref<Item>>& items)
    {
        mItems.Add(items);
        ForceRebuildItems();
    }

    void ContextMenu::InsertItems(const Vector<Ref<Item>>& items, int position)
    {
        int i = 0;
        for (auto item : items) {
            InsertItem(item, position + i);
            i++;
        }
    }

    Ref<ContextMenuItem> ContextMenu::GetItemWidget(int position)
    {
        if (position > 0 && position < mItemsLayout->GetChildren().Count())
            return DynamicCast<ContextMenuItem>(mItemsLayout->GetChildren()[position]);

        return nullptr;
    }

    int ContextMenu::FindItem(const WString& text) const
    {
        int idx = 0;
        for (auto& child : mItemsLayout->GetChildWidgets()) 
        {
            auto item = DynamicCast<ContextMenuItem>(child);
            if (item && item->GetText() == text)
                return idx;

            idx++;
        }

        return -1;
    }

    Ref<ContextMenuItem> ContextMenu::FindItemWidget(const WString& path) const
    {
        int delPos = path.Find("/");
        WString pathPart = path.SubStr(0, delPos);

        for (auto& child : mItemsLayout->mChildren) 
        {
            auto item = DynamicCast<ContextMenuItem>(child);
            if (item && item->GetText() == pathPart) {
                if (delPos == -1)
                    return item;
                else if (auto subContext = item->GetSubMenu())
                    return subContext->FindItemWidget(path.SubStr(delPos + 1));
            }
        }

        return nullptr;
    }

    void ContextMenu::SetItem(int position, const Ref<Item>& item)
    {
        if (position < 0 || position >= mItems.Count())
            return;

        mItems[position] = item;
        ForceRebuildItems();
    }

    Ref<ContextMenuItem> ContextMenu::GetItemUnderPoint(const Vec2F& point)
    {
        if (!mItemsLayout)
            return nullptr;

        for (auto& child : mItemsLayout->mChildWidgets) {
            if (child->layout->IsPointInside(point) && child->GetType() == TypeOf(ContextMenuItem))
                return DynamicCast<ContextMenuItem>(child);
        }

        return nullptr;
    }

    void ContextMenu::UpdateHover(const Vec2F& point)
    {
        auto itemUnderCursor = GetItemUnderPoint(point);

        if (!itemUnderCursor)
        {
            auto hoverState = state["hover"];
            if (hoverState) {
                mSelectionDrawable->SetEnabled(true);
                *hoverState = false;
            }
            else
                mSelectionDrawable->SetEnabled(false);

            mSelectedItem = itemUnderCursor;
        }
        else 
        {
            mTargetSelectionRect = mSelectionLayout.Calculate(itemUnderCursor->GetLayoutData().mWorldBox.ToRect());

            auto hoverState = state["hover"];
            if (hoverState)
            {
                mSelectionDrawable->SetEnabled(true);
                *hoverState = true;
            }
            else
                mSelectionDrawable->SetEnabled(true);

            if (itemUnderCursor != mSelectedItem) 
            {
                mSelectedItem = itemUnderCursor;

                if (mSelectedItem && mSelectedItem->GetSubMenu())
                    mSelectSubContextTime = mOpenSubMenuDelay;
            }
        }
    }

    void ContextMenu::OnCursorPressed(const Input::Cursor& cursor)
    {}

    void ContextMenu::OnCursorStillDown(const Input::Cursor& cursor)
    {}

    void ContextMenu::OnCursorReleased(const Input::Cursor& cursor)
    {
        auto itemUnderCursor = GetItemUnderPoint(cursor.position);

        if (!itemUnderCursor)
            return;

        if (!itemUnderCursor->IsEnabled())
            return;

        if (itemUnderCursor)
        {
            itemUnderCursor->onClick();

            if (itemUnderCursor->IsCheckable())
            {
                itemUnderCursor->SetChecked(!itemUnderCursor->IsChecked());
                itemUnderCursor->onChecked(itemUnderCursor->IsChecked());
            }
        }

        if (itemUnderCursor && itemUnderCursor->FindChildByType<ContextMenu>() == nullptr) {
            HideWithParent();
            HideWithChild();
        }
    }

    void ContextMenu::OnCursorMoved(const Input::Cursor& cursor)
    {
        const float checkDeltaThreshold = 2.0f;
        if ((cursor.position - mLastSelectCheckCursor).Length() < checkDeltaThreshold)
            return;

        mLastSelectCheckCursor = cursor.position;

        UpdateHover(cursor.position);
    }

    void ContextMenu::HideWithParent()
    {
        PopupWidget::HideWithParent();
        mSelectedItem = nullptr;
    }

    void ContextMenu::HideWithChild()
    {
        PopupWidget::HideWithChild();
        mSelectedItem = nullptr;
    }

    const Vector<Ref<ContextMenu::Item>>& ContextMenu::GetItems() const
    {
        return mItems;
    }

    void ContextMenu::RemoveItem(int position)
    {
        if (position < 0 || position >= mItems.Count())
            return;

        mItems.RemoveAt(position);
        ForceRebuildItems();
    }

    void ContextMenu::RemoveItem(const WString& path)
    {
        auto idx = FindItem(path);
        if (idx < 0)
            return;

        RemoveItem(idx);
    }

    void ContextMenu::RemoveAllItems()
    {
        mItems.Clear();
        ForceRebuildItems();
    }

    void ContextMenu::SetItemChecked(int position, bool checked)
    {
        if (position < 0 || position >= mItemsLayout->mChildren.Count())
            return;

        auto item = DynamicCast<ContextMenuItem>(mItemsLayout->mChildren[position]);
        if (item && item->IsCheckable())
            item->SetChecked(checked);
    }

    bool ContextMenu::IsItemChecked(int position) const
    {
        if (position < 0 || position >= mItemsLayout->mChildren.Count())
            return false;

        auto item = DynamicCast<ContextMenuItem>(mItemsLayout->mChildren[position]);
        return item->IsChecked();
    }

    const Ref<VerticalLayout>& ContextMenu::GetItemsLayout() const
    {
        return mItemsLayout;
    }

    const Ref<ContextMenuItem>& ContextMenu::GetItemSample() const
    {
        return mItemSample;
    }

    const Ref<Widget>& ContextMenu::GetSeparatorSample() const
    {
        return mSeparatorSample;
    }

    const Ref<Sprite>& ContextMenu::GetSelectionDrawable() const
    {
        return mSelectionDrawable;
    }

    void ContextMenu::SetSelectionDrawableLayout(const Layout& layout)
    {
        mSelectionLayout = layout;
    }

    Layout ContextMenu::GetSelectionDrawableLayout() const
    {
        return mSelectionLayout;
    }

    void ContextMenu::SetMaxItemsVisible(int count)
    {
        mMaxVisibleItems = count;
    }

    void ContextMenu::SetItemsMaxPriority()
    {
        for (auto item : mItems)
            item->SetMaxPriority();
    }

    void ContextMenu::SetItemsMinPriority()
    {
        for (auto item : mItems)
            item->SetMinPriority();
    }

    String ContextMenu::GetCreateMenuGroup()
    {
        return "Context";
    }

    void ContextMenu::FitSizeAndPosition(const Vec2F& position)
    {
        Vec2F size;
        float maxCaption = 0.0f;
        float maxShortcut = 0.0f;

        if (mSearchPanel)
            mSearchPanel->enabled = mSearchEnabled;

        int i = 0;
        for (auto& child : mItemsLayout->GetChildWidgets()) 
        {
            if (!child->IsEnabled())
                continue;

            if (auto childCaption = child->GetLayerDrawable<Text>("basic/caption"))
                maxCaption = Math::Max(childCaption->GetRealSize().x, maxCaption);

            if (auto shortcutCaption = child->GetLayerDrawable<Text>("basic/shortcut"))
                maxShortcut = Math::Max(shortcutCaption->GetRealSize().x, maxShortcut);

            size.y += child->layout->minHeight;

            i++;
            if (i == mMaxVisibleItems)
                break;
        }

        size.x = mFitSizeMin + maxCaption + maxShortcut;

        if (mSearchEnabled)
            size.x = Math::Max(size.x, mSearchPanel->layout->GetMinWidth());

        size.x += mViewAreaLayout.offsetMin.x - mViewAreaLayout.offsetMax.x;
        size.y += mViewAreaLayout.offsetMin.y - mViewAreaLayout.offsetMax.y;

        // Check if menu is out of screen to the right. Then put to the right of parent context
        auto correctedPosition = position;
        if (position.x + size.x > o2Render.GetResolution().x/2)
        {
            if (mParentPopup)
                correctedPosition.x = mParentPopup.Lock()->layout->worldLeft - size.x;
        }

        FitPosition(correctedPosition, size);
    }

    void ContextMenu::SpecialDraw()
    {
        if (!mResEnabledInHierarchy)
            return;

        for (auto& layer : mDrawingLayers)
            layer->Draw();

        IDrawable::OnDrawn();
        CursorAreaEventsListener::OnDrawn();

        o2Render.EnableScissorTest(mAbsoluteClipArea);

        for (auto& child : mChildWidgets)
            child->Draw();

        mSelectionDrawable->Draw();

        o2Render.DisableScissorTest();

        for (auto& layer : mTopDrawingLayers)
            layer->Draw();

        if (mOwnHorScrollBar)
            mHorScrollBar->Draw();

        if (mOwnVerScrollBar)
            mVerScrollBar->Draw();

        DrawDebugFrame();

        if (mChildPopup)
            mChildPopup.Lock()->SpecialDraw();
    }

    void ContextMenu::RebuildItems()
    {
#if IS_EDITOR
        PushEditorScopeOnStack scope(KeyboardEventsListener::mIsEditorMode ? 1 : 0);
#endif

        Vector<Ref<ContextMenuItem>> cache;

        // Cache all items
        for (auto item : mItems)
        {
            if (item->widget)
                mItemsLayout->RemoveChild(item->widget.Lock());
        }

        // Cache extra items
        auto children = mItemsLayout->GetChildren();
        for (auto& child : children) 
        {
            if (auto item = DynamicCast<ContextMenuItem>(child)) {
                cache.Add(item);
                mItemsLayout->RemoveChild(item);
            }
            else if (child->name == String("Separator"))
                mItemsLayout->RemoveChild(child);
        }

        // Update visible items with search text
        Vector<Ref<Item>> viewItems;
        if (mSearchText.IsEmpty()) 
        {
            viewItems = mItems;
        }
        else
        {
            for (auto item : mItems) 
            {
                if (item->subItems.IsEmpty())
                {
                    if (item->text.ToLowerCase().Contains(mSearchText))
                        viewItems.Add(mmake<Item>(*item));
                }
                else
                    item->RecursiveSearchSubItems(mSearchText, viewItems);
            }
        }

        // Group items by group
        Map<WString, Vector<Ref<Item>>> groups;
        for (auto item : viewItems)
            groups[item->group].Add(item);

        // Add items to layout
        bool isFirst = true;
        for (auto& kv : groups) 
        {
            if (!isFirst) {
                auto newItem = mSeparatorSample->CloneAsRef<Widget>();
                newItem->name = "Separator";
                mItemsLayout->AddChild(newItem);
            }

            if (isFirst)
                isFirst = false;

            for (auto item : kv.second) 
            {
                if (item->widget)
                    mItemsLayout->AddChild(item->widget.Lock());
                else {
                    if (item->text == Item::separatorText) {
                        auto newItem = mSeparatorSample->CloneAsRef<Widget>();
                        newItem->name = "Separator";
                        mItemsLayout->AddChild(newItem);
                    }
                    else {
                        auto newItem = cache.IsEmpty() ? mItemSample->CloneAsRef<ContextMenuItem>() : cache.PopBack();
                        newItem->Setup(item);
                        mItemsLayout->AddChild(newItem);
                    }
                }
            }
        }

        mItemsBuilt = true;
    }

    void ContextMenu::OnSearchChanged(const WString& text)
    {
        mSearchText = text.ToLowerCase();
        ForceRebuildItems();
    }

    Ref<ContextMenu::Item> ContextMenu::GetItem(int idx) const
    {
        return mItems[idx];
    }

    ContextMenuItem::ContextMenuItem(RefCounter* refCounter) :
        Widget(refCounter), mSubMenu(nullptr)
    {
        RetargetStatesAnimations();
    }

    ContextMenuItem::ContextMenuItem(RefCounter* refCounter, const ContextMenuItem& other) :
        Widget(refCounter, other)
    {
        mSubMenu = FindChildByType<ContextMenu>();
        if (mSubMenu)
            mSubMenu->Hide(true);

        RetargetStatesAnimations();
    }

    ContextMenuItem::~ContextMenuItem()
    {}

    void ContextMenuItem::Setup(const Ref<ContextMenu::Item>& item)
    {
        item->widget = Ref(this);

        name = (WString)"Context Item " + item->text;

        if (auto iconLayer = FindLayer("icon")) {
            if (item->icon) {
                Vec2F size = item->icon->GetAtlasRect().Size();

                if (size.x > size.y) {
                    size.y *= size.x / layout->height;
                    size.x = layout->height;
                }
                else {
                    size.x *= size.y / layout->height;
                    size.y = layout->height;
                }

                iconLayer->AddChildLayer("sprite", mmake<Sprite>(item->icon),
                                         Layout(Vec2F(), Vec2F(),
                                                Vec2F(-Math::Floor(size.x * 0.5f), Math::Floor(size.y * 0.5f)),
                                                Vec2F(Math::Floor(size.x * 0.5f), -Math::Floor(size.y * 0.5f))));

                UpdateLayersDrawingSequence();
            }
        }

        if (auto textLayer = GetLayerDrawable<Text>("basic/caption"))
            textLayer->text = item->text;

        if (auto shortcutLayer = GetLayerDrawable<Text>("basic/shortcut"))
            shortcutLayer->text = item->GetShortcut().AsString();

        if (auto subIconLayer = FindLayer("subIcon"))
            subIconLayer->transparency = item->subItems.Count() > 0 ? 1.0f : 0.0f;

        if (auto checkLayer = GetLayerDrawable<Sprite>("check"))
            checkLayer->enabled = item->checked;

        SetChecked(item->checked);
        SetCheckable(item->checkable);

        onClick = item->onClick;
        onChecked = item->onChecked;

        RemoveAllChildren();

        if (item->subItems.Count() > 0) {
            auto subMenu = o2UI.CreateWidget<ContextMenu>();
            subMenu->RemoveAllItems();
            subMenu->AddItems(item->subItems);

            AddChild(subMenu);
        }
    }

    void ContextMenuItem::SetText(const WString& text)
    {
        auto textLayer = GetLayerDrawable<Text>("basic/caption");
        if (textLayer)
            textLayer->text = text;
    }

    WString ContextMenuItem::GetText() const
    {
        auto textLayer = GetLayerDrawable<Text>("basic/caption");
        if (textLayer)
            return textLayer->text;

        return "";
    }

    ContextMenuItem& ContextMenuItem::operator=(const ContextMenuItem& other)
    {
        Widget::operator=(other);

        mSubMenu = FindChildByType<ContextMenu>();
        if (mSubMenu)
            mSubMenu->Hide(true);

        return *this;
    }

    const Ref<ContextMenu>& ContextMenuItem::GetSubMenu() const
    {
        return mSubMenu;
    }

    void ContextMenuItem::SetChecked(bool checked)
    {
        if (auto checkLayer = GetLayerDrawable<Sprite>("check"))
            checkLayer->enabled = checked;

        mChecked = checked;
    }

    bool ContextMenuItem::IsChecked() const
    {
        return mChecked;
    }

    void ContextMenuItem::SetCheckable(bool checkable)
    {
        mCheckable = checkable;
    }

    bool ContextMenuItem::IsCheckable() const
    {
        return mCheckable;
    }

    void ContextMenuItem::SetEnabled(bool enabled)
    {
        SetState("enabled", enabled);
    }

    bool ContextMenuItem::IsEnabled() const
    {
        return GetState("enabled");
    }

    void ContextMenuItem::Enable()
    {
        SetEnabled(true);
    }

    void ContextMenuItem::Disable()
    {
        SetEnabled(false);
    }

    String ContextMenuItem::GetCreateMenuGroup()
    {
        return "Context";
    }

    void ContextMenuItem::OnChildAdded(const Ref<Widget>& child)
    {
        if (child->GetType() == TypeOf(ContextMenu))
            mSubMenu = DynamicCast<ContextMenu>(child);
    }

    void ContextMenu::ForceRebuildItems()
    {
        if (mResEnabledInHierarchy)
        {
            RebuildItems();
            FitSizeAndPosition(layout->worldLeftTop);
            SetLayoutDirty();
        }
    }
}

DECLARE_TEMPLATE_CLASS(o2::LinkRef<o2::ContextMenu>);
// --- META ---

DECLARE_CLASS(o2::ContextMenu, o2__ContextMenu);

DECLARE_CLASS(o2::ContextMenuItem, o2__ContextMenuItem);

DECLARE_CLASS(o2::ContextMenu::Item, o2__ContextMenu__Item);
// --- END META ---
