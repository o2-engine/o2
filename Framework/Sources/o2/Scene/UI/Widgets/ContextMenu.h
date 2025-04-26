#pragma once

#include "o2/Assets/Types/ImageAsset.h"
#include "o2/Events/ShortcutKeysListener.h"
#include "o2/Render/Sprite.h"
#include "o2/Scene/UI/Widgets/EditBox.h"
#include "o2/Scene/UI/Widgets/PopupWidget.h"
#include "o2/Scene/UI/Widgets/ScrollArea.h"
#include "o2/Utils/System/ShortcutKeys.h"

namespace o2
{
    class Sprite;
    class Button;
    class ContextMenu;
    class ContextMenuItem;

    FORWARD_CLASS_REF(VerticalLayout);

    // -----------------------
    // Context menu ui element
    // -----------------------
    class ContextMenu : public PopupWidget
    {
    public:
        // ---------
        // Menu item
        // ---------
        class Item : public RefCounterable, public ISerializable, public ShortcutKeysListener
        {
        public:
            static WString separatorText;

        public:
            PROPERTIES(Item);
            PROPERTY(ShortcutKeys, shortcut, SetShortcut, GetShortcut);

        public:
            WString              text;  // Item text @SERIALIZABLE
            WString              group; // Item group id @SERIALIZABLE
            AssetRef<ImageAsset> icon;  // Icon image @SERIALIZABLE

            Vector<Ref<Item>> subItems; // Children items @SERIALIZABLE

            bool checkable = false; // Is item checkable @SERIALIZABLE
            bool checked = false;   // Is item checked @SERIALIZABLE

        public:
            Function<void()>     onClick;   // On click event    
            Function<void(bool)> onChecked; // On checked event    

            WeakRef<ContextMenuItem> widget; // Widget of this item

        public:
            // Default constructor
            Item(RefCounter* refCounter);

            // Copy constructor
            Item(RefCounter* refCounter, const Item& other);

            // Constructor from text
            Item(RefCounter* refCounter, const WString& text, const Vector<Ref<Item>>& subItems, const WString& group = "",
                 const AssetRef<ImageAsset>& icon = AssetRef<ImageAsset>());

            // Constructor from text and click event
            Item(RefCounter* refCounter, const WString& text, const Function<void()> onClick, const WString& group = "",
                 const AssetRef<ImageAsset>& icon = AssetRef<ImageAsset>(), const ShortcutKeys& shortcut = ShortcutKeys());

            // Constructor from text and checked event
            Item(RefCounter* refCounter, const WString& text, bool checked, Function<void(bool)> onChecked = Function<void(bool)>(),
                 const WString& group = "", const AssetRef<ImageAsset>& icon = AssetRef<ImageAsset>(),
                 const ShortcutKeys& shortcut = ShortcutKeys());

            // Destructor
            ~Item();

            // Copy-operator
            bool operator==(const Item& other) const;

            // Sets shortcut
            void SetShortcut(const ShortcutKeys& shortcut);

            // Returns shortcut
            const ShortcutKeys& GetShortcut() const;

            // Sets maximum priority
            void SetMaxPriority();

            // Sets minimum priority
            void SetMinPriority();

            // Recursive search sub items by text
            void RecursiveSearchSubItems(const WString& text, Vector<Ref<Item>>& result);

            // Returns new separator item
            static Ref<Item> Separator();

            SERIALIZABLE(Item);

        private:
            ShortcutKeys mShortcut; // Keys shortcut @SERIALIZABLE

        private:
            // This event calling when shortcut hit and this listener has max priority
            void OnShortcutPressed() override;

            REF_COUNTERABLE_IMPL(RefCounterable);
        };

    public:
        // Default constructor
        explicit ContextMenu(RefCounter* refCounter);

        // Constructor from items
        ContextMenu(RefCounter* refCounter, const Vector<Ref<Item>>& items);

        // Copy-constructor
        ContextMenu(RefCounter* refCounter, const ContextMenu& other);

        // Destructor
        ~ContextMenu();

        // Copy operator
        ContextMenu& operator=(const ContextMenu& other);

        // Updates drawables, states and widget
        void Update(float dt) override;

        // Show from parent context
        void Show(const Ref<PopupWidget>& parent, const Vec2F& position = o2Input.GetCursorPos()) override;

        // Shows context
        void Show(const Vec2F& position = o2Input.GetCursorPos());

        // Enables or disables search panel
        void SetSearchEnabled(bool enabled);

        // Returns is search enabled
        bool IsSearchEnabled() const;

        // Add item
        void AddItem(const Ref<Item>& item);

        // Adds item by path ("node/sub node/target")
        Ref<Item> AddItem(const WString& path, const Function<void()>& clickFunc = Function<void()>(),
                          const AssetRef<ImageAsset>& icon = AssetRef<ImageAsset>(), const ShortcutKeys& shortcut = ShortcutKeys());

        // Adds item by path ("node/sub node/target")
        Ref<Item> AddToggleItem(const WString& path, bool value,
                                const Function<void(bool)>& clickFunc = Function<void(bool)>(),
                                const AssetRef<ImageAsset>& icon = AssetRef<ImageAsset>(),
                                const ShortcutKeys& shortcut = ShortcutKeys());

        // Inserts item at position
        void InsertItem(const Ref<Item>& item, int position);

        // Adds array of items
        void AddItems(const Vector<Ref<Item>>& items);

        // Inserts array of items at position
        void InsertItems(const Vector<Ref<Item>>& items, int position);

        // Returns item's widget at position
        Ref<ContextMenuItem> GetItemWidget(int position);

        // Returns index of item by path
        int FindItem(const WString& text) const;

        // Returns item's widget by path
        Ref<ContextMenuItem> FindItemWidget(const WString& path) const;

        // Sets item at position
        void SetItem(int position, const Ref<Item>& item);

        // Returns array of all items
        const Vector<Ref<Item>>& GetItems() const;

        // Removes item at position
        void RemoveItem(int position);

        // Removes item by path
        void RemoveItem(const WString& path);

        // Removes all items
        void RemoveAllItems();

        // Sets item at position checked (if it checkable)
        void SetItemChecked(int position, bool checked);

        // Returns is item checked
        bool IsItemChecked(int position) const;

        // Returns items vertical layout
        const Ref<VerticalLayout>& GetItemsLayout() const;

        // Returns item sample
        const Ref<ContextMenuItem>& GetItemSample() const;

        // Returns items separator sample
        const Ref<Widget>& GetSeparatorSample() const;

        // Returns selection drawable
        const Ref<Sprite>& GetSelectionDrawable() const;

        // Sets selection drawable layout
        void SetSelectionDrawableLayout(const Layout& layout);

        // Returns selection drawable layout
        Layout GetSelectionDrawableLayout() const;

        // Sets maximum visible items count
        void SetMaxItemsVisible(int count);

        // Sets context menu items shortcut maximum priority
        void SetItemsMaxPriority();

        // Sets context menu items shortcut minimum priority
        void SetItemsMinPriority();

        // Returns create menu group in editor
        static String GetCreateMenuGroup();

        SERIALIZABLE(ContextMenu);
        CLONEABLE_REF(ContextMenu);

    protected:
        const float mOpenSubMenuDelay = 0.4f; // Sub menu opening delay when cursor hover it

        int mMaxVisibleItems = 100; // Maximum visible items @SERIALIZABLE

        bool         mSearchEnabled = false; // Is search enabled
        Ref<Widget>  mSearchPanel;           // Widget with search edit box and icon
        Ref<EditBox> mSearchEditBox;         // Search input text box
        String       mSearchText;            // Search text used to filter items

        Vector<Ref<Item>>   mItems;              // Items list
        Ref<VerticalLayout> mItemsLayout;        // Items layout; builds when opening context
        bool                mItemsBuilt = false; // Is items layout is actual and built

        Ref<ContextMenuItem> mItemSample;      // Item sample @SERIALIZABLE
        Ref<Widget>          mSeparatorSample; // Items separator sample @SERIALIZABLE

        Ref<Sprite> mSelectionDrawable; // Selection sprite @SERIALIZABLE
        Layout      mSelectionLayout;   // Selection layout, result selection area depends on selected item @SERIALIZABLE

        RectF mCurrentSelectionRect;  // Current selection rectangle (for smoothing)
        RectF mTargetSelectionRect;   // Target selection rectangle (over selected item)
        Vec2F mLastSelectCheckCursor; // Last cursor position on selection check

        Ref<ContextMenuItem> mSelectedItem;                 // Index of selected item
        float                mSelectSubContextTime = -1.0f; // Time to appearing selected sub context

    protected:
        // Fits size by items
        void FitSizeAndPosition(const Vec2F& position) override;

        // Hides context with his parent
        void HideWithParent() override;

        // Hides context with his child
        void HideWithChild() override;

        // Special drawing for contexts
        void SpecialDraw() override;

        // Build items layout
        void RebuildItems();

        // Force rebuild items - clears cache and rebuilds items if enabled
        void ForceRebuildItems();

        // Called when search edit box has changed, updates items list
        void OnSearchChanged(const WString& text);

        // Creates context items by path ("node/sub node/target")
        Vector<Ref<Item>>& CreateItemsByPath(WString& path);

        // Returns item info
        Ref<Item> GetItem(int idx) const;

        // Returns item widget under point and stores index in idxPtr, if not null
        Ref<ContextMenuItem> GetItemUnderPoint(const Vec2F& point);

        // Updates hover
        void UpdateHover(const Vec2F& point);

        // Called when cursor pressed on this
        void OnCursorPressed(const Input::Cursor& cursor) override;

        // Called when cursor stay down during frame
        void OnCursorStillDown(const Input::Cursor& cursor) override;

        // Called when cursor released (only when cursor pressed this at previous time)
        void OnCursorReleased(const Input::Cursor& cursor) override;

        // Called when cursor moved on this (or moved outside when this was pressed)
        void OnCursorMoved(const Input::Cursor& cursor) override;

        friend class UIManager;
        friend class MenuPanel;
    };

    // -----------------
    // Context menu item
    // -----------------
    class ContextMenuItem : public Widget
    {
    public:
        PROPERTIES(ContextMenuItem);
        PROPERTY(WString, text, SetText, GetText); // Text

    public:
        Function<void()>     onClick;   // Click function
        Function<void(bool)> onChecked; // Checked function, calls when check was changed and item is checkable

    public:
        // Default constructor
        explicit ContextMenuItem(RefCounter* refCounter);

        // Copy-constructor
        ContextMenuItem(RefCounter* refCounter, const ContextMenuItem& other);

        // Destructor
        ~ContextMenuItem();

        // Copy-operator
        ContextMenuItem& operator=(const ContextMenuItem& other);

        // Sets item
        void Setup(const Ref<ContextMenu::Item>& item);

        // Sets text
        void SetText(const WString& text);

        // Returns text
        WString GetText() const;

        // Returns sub context menu
        const Ref<ContextMenu>& GetSubMenu() const;

        // Sets checked icon
        void SetChecked(bool checked);

        // Returns is menu checked
        bool IsChecked() const;

        // Sets item checkable
        void SetCheckable(bool checkable);

        // Returns is menu item can be checked
        bool IsCheckable() const;

        // Sets item enabled
        void SetEnabled(bool enabled) override;

        // Returns is item enabled
        bool IsEnabled() const OPTIONAL_OVERRIDE;

        // Enables item
        void Enable();

        // DIsables item
        void Disable();

        // Returns create menu group in editor
        static String GetCreateMenuGroup();

        SERIALIZABLE(ContextMenuItem);
        CLONEABLE_REF(ContextMenuItem);

    protected:
        Ref<ContextMenu> mSubMenu; // Context sub menu

        bool mChecked = false;   // Is menu item checked
        bool mCheckable = false; // Is menu item can be checked

    protected:
        // Called when child widget was added
        void OnChildAdded(const Ref<Widget>& child) override;

        friend class ContextMenu;
    };
}
// --- META ---

CLASS_BASES_META(o2::ContextMenu)
{
    BASE_CLASS(o2::PopupWidget);
}
END_META;
CLASS_FIELDS_META(o2::ContextMenu)
{
    FIELD().PROTECTED().DEFAULT_VALUE(0.4f).NAME(mOpenSubMenuDelay);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(100).NAME(mMaxVisibleItems);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mSearchEnabled);
    FIELD().PROTECTED().NAME(mSearchPanel);
    FIELD().PROTECTED().NAME(mSearchEditBox);
    FIELD().PROTECTED().NAME(mSearchText);
    FIELD().PROTECTED().NAME(mItems);
    FIELD().PROTECTED().NAME(mItemsLayout);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mItemsBuilt);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().NAME(mItemSample);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().NAME(mSeparatorSample);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().NAME(mSelectionDrawable);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().NAME(mSelectionLayout);
    FIELD().PROTECTED().NAME(mCurrentSelectionRect);
    FIELD().PROTECTED().NAME(mTargetSelectionRect);
    FIELD().PROTECTED().NAME(mLastSelectCheckCursor);
    FIELD().PROTECTED().NAME(mSelectedItem);
    FIELD().PROTECTED().DEFAULT_VALUE(-1.0f).NAME(mSelectSubContextTime);
}
END_META;
CLASS_METHODS_META(o2::ContextMenu)
{

    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*);
    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*, const Vector<Ref<Item>>&);
    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*, const ContextMenu&);
    FUNCTION().PUBLIC().SIGNATURE(void, Update, float);
    FUNCTION().PUBLIC().SIGNATURE(void, Show, const Ref<PopupWidget>&, const Vec2F&);
    FUNCTION().PUBLIC().SIGNATURE(void, Show, const Vec2F&);
    FUNCTION().PUBLIC().SIGNATURE(void, SetSearchEnabled, bool);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsSearchEnabled);
    FUNCTION().PUBLIC().SIGNATURE(void, AddItem, const Ref<Item>&);
    FUNCTION().PUBLIC().SIGNATURE(Ref<Item>, AddItem, const WString&, const Function<void()>&, const AssetRef<ImageAsset>&, const ShortcutKeys&);
    FUNCTION().PUBLIC().SIGNATURE(Ref<Item>, AddToggleItem, const WString&, bool, const Function<void(bool)>&, const AssetRef<ImageAsset>&, const ShortcutKeys&);
    FUNCTION().PUBLIC().SIGNATURE(void, InsertItem, const Ref<Item>&, int);
    FUNCTION().PUBLIC().SIGNATURE(void, AddItems, const Vector<Ref<Item>>&);
    FUNCTION().PUBLIC().SIGNATURE(void, InsertItems, const Vector<Ref<Item>>&, int);
    FUNCTION().PUBLIC().SIGNATURE(Ref<ContextMenuItem>, GetItemWidget, int);
    FUNCTION().PUBLIC().SIGNATURE(int, FindItem, const WString&);
    FUNCTION().PUBLIC().SIGNATURE(Ref<ContextMenuItem>, FindItemWidget, const WString&);
    FUNCTION().PUBLIC().SIGNATURE(void, SetItem, int, const Ref<Item>&);
    FUNCTION().PUBLIC().SIGNATURE(const Vector<Ref<Item>>&, GetItems);
    FUNCTION().PUBLIC().SIGNATURE(void, RemoveItem, int);
    FUNCTION().PUBLIC().SIGNATURE(void, RemoveItem, const WString&);
    FUNCTION().PUBLIC().SIGNATURE(void, RemoveAllItems);
    FUNCTION().PUBLIC().SIGNATURE(void, SetItemChecked, int, bool);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsItemChecked, int);
    FUNCTION().PUBLIC().SIGNATURE(const Ref<VerticalLayout>&, GetItemsLayout);
    FUNCTION().PUBLIC().SIGNATURE(const Ref<ContextMenuItem>&, GetItemSample);
    FUNCTION().PUBLIC().SIGNATURE(const Ref<Widget>&, GetSeparatorSample);
    FUNCTION().PUBLIC().SIGNATURE(const Ref<Sprite>&, GetSelectionDrawable);
    FUNCTION().PUBLIC().SIGNATURE(void, SetSelectionDrawableLayout, const Layout&);
    FUNCTION().PUBLIC().SIGNATURE(Layout, GetSelectionDrawableLayout);
    FUNCTION().PUBLIC().SIGNATURE(void, SetMaxItemsVisible, int);
    FUNCTION().PUBLIC().SIGNATURE(void, SetItemsMaxPriority);
    FUNCTION().PUBLIC().SIGNATURE(void, SetItemsMinPriority);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(String, GetCreateMenuGroup);
    FUNCTION().PROTECTED().SIGNATURE(void, FitSizeAndPosition, const Vec2F&);
    FUNCTION().PROTECTED().SIGNATURE(void, HideWithParent);
    FUNCTION().PROTECTED().SIGNATURE(void, HideWithChild);
    FUNCTION().PROTECTED().SIGNATURE(void, SpecialDraw);
    FUNCTION().PROTECTED().SIGNATURE(void, RebuildItems);
    FUNCTION().PROTECTED().SIGNATURE(void, ForceRebuildItems);
    FUNCTION().PROTECTED().SIGNATURE(void, OnSearchChanged, const WString&);
    FUNCTION().PROTECTED().SIGNATURE(Vector<Ref<Item>>&, CreateItemsByPath, WString&);
    FUNCTION().PROTECTED().SIGNATURE(Ref<Item>, GetItem, int);
    FUNCTION().PROTECTED().SIGNATURE(Ref<ContextMenuItem>, GetItemUnderPoint, const Vec2F&);
    FUNCTION().PROTECTED().SIGNATURE(void, UpdateHover, const Vec2F&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCursorPressed, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCursorStillDown, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCursorReleased, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCursorMoved, const Input::Cursor&);
}
END_META;

CLASS_BASES_META(o2::ContextMenuItem)
{
    BASE_CLASS(o2::Widget);
}
END_META;
CLASS_FIELDS_META(o2::ContextMenuItem)
{
    FIELD().PUBLIC().NAME(text);
    FIELD().PUBLIC().NAME(onClick);
    FIELD().PUBLIC().NAME(onChecked);
    FIELD().PROTECTED().NAME(mSubMenu);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mChecked);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mCheckable);
}
END_META;
CLASS_METHODS_META(o2::ContextMenuItem)
{

    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*);
    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*, const ContextMenuItem&);
    FUNCTION().PUBLIC().SIGNATURE(void, Setup, const Ref<ContextMenu::Item>&);
    FUNCTION().PUBLIC().SIGNATURE(void, SetText, const WString&);
    FUNCTION().PUBLIC().SIGNATURE(WString, GetText);
    FUNCTION().PUBLIC().SIGNATURE(const Ref<ContextMenu>&, GetSubMenu);
    FUNCTION().PUBLIC().SIGNATURE(void, SetChecked, bool);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsChecked);
    FUNCTION().PUBLIC().SIGNATURE(void, SetCheckable, bool);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsCheckable);
    FUNCTION().PUBLIC().SIGNATURE(void, SetEnabled, bool);
    FUNCTION().PUBLIC().SIGNATURE(bool, IsEnabled);
    FUNCTION().PUBLIC().SIGNATURE(void, Enable);
    FUNCTION().PUBLIC().SIGNATURE(void, Disable);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(String, GetCreateMenuGroup);
    FUNCTION().PROTECTED().SIGNATURE(void, OnChildAdded, const Ref<Widget>&);
}
END_META;

CLASS_BASES_META(o2::ContextMenu::Item)
{
    BASE_CLASS(o2::RefCounterable);
    BASE_CLASS(o2::ISerializable);
    BASE_CLASS(o2::ShortcutKeysListener);
}
END_META;
CLASS_FIELDS_META(o2::ContextMenu::Item)
{
    FIELD().PUBLIC().NAME(shortcut);
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().NAME(text);
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().NAME(group);
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().NAME(icon);
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().NAME(subItems);
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(false).NAME(checkable);
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().DEFAULT_VALUE(false).NAME(checked);
    FIELD().PUBLIC().NAME(onClick);
    FIELD().PUBLIC().NAME(onChecked);
    FIELD().PUBLIC().NAME(widget);
    FIELD().PRIVATE().SERIALIZABLE_ATTRIBUTE().NAME(mShortcut);
}
END_META;
CLASS_METHODS_META(o2::ContextMenu::Item)
{

    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*);
    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*, const Item&);
    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*, const WString&, const Vector<Ref<Item>>&, const WString&, const AssetRef<ImageAsset>&);
    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*, const WString&, const Function<void()>, const WString&, const AssetRef<ImageAsset>&, const ShortcutKeys&);
    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*, const WString&, bool, Function<void(bool)>, const WString&, const AssetRef<ImageAsset>&, const ShortcutKeys&);
    FUNCTION().PUBLIC().SIGNATURE(void, SetShortcut, const ShortcutKeys&);
    FUNCTION().PUBLIC().SIGNATURE(const ShortcutKeys&, GetShortcut);
    FUNCTION().PUBLIC().SIGNATURE(void, SetMaxPriority);
    FUNCTION().PUBLIC().SIGNATURE(void, SetMinPriority);
    FUNCTION().PUBLIC().SIGNATURE(void, RecursiveSearchSubItems, const WString&, Vector<Ref<Item>>&);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(Ref<Item>, Separator);
    FUNCTION().PRIVATE().SIGNATURE(void, OnShortcutPressed);
}
END_META;
// --- END META ---
