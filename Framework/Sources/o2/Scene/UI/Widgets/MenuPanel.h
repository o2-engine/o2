#pragma once

#include "o2/Events/DrawableCursorEventsListener.h"
#include "o2/Scene/UI/Widgets/ContextMenu.h"
#include "o2/Scene/UI/Widget.h"
#include "o2/Utils/Function/Function.h"

namespace o2
{
    class HorizontalLayout;

    // -------------
    // UI Menu panel
    // -------------
    class MenuPanel: public Widget, public DrawableCursorEventsListener
    {
    public:
        // --------------------
        // Menu panel text item
        // --------------------
        class Item: public ISerializable, public RefCounterable, public ICloneableRef
        {
        public:
            WString text; // Item text @SERIALIZABLE

            Vector<Ref<ContextMenu::Item>> subItems; // Children sub items @SERIALIZABLE

            Function<void()> onClick; // Click function 

        public:
            // Default constructor
            Item();

            // Constructor with text and sub items
            Item(const WString& text, const Vector<Ref<ContextMenu::Item>>& subItems);

            // Constructor with text and click function
            Item(const WString& text, const Function<void()> onClick);

            // Copy operator
            bool operator==(const Item& other) const;

            SERIALIZABLE(Item);
            CLONEABLE_REF(Item);
        };

    public:
        // Default constructor
        explicit MenuPanel(RefCounter* refCounter);

        // Copy-constructor
        MenuPanel(RefCounter* refCounter, const MenuPanel& other);

        // Destructor
        ~MenuPanel();

        // Copy-operator
        MenuPanel& operator=(const MenuPanel& other);

        // Updates drawables, states and widget
        void Update(float dt) override;

        // Draws widget
        void Draw() override;

        // Add item
        Ref<Widget> AddItem(const Item& item);

        // Adds item by path ("node/sub node/target")
        void AddItem(const WString& path, const Function<void()>& clickFunc = Function<void()>(),
                     const AssetRef<Asset>& icon = AssetRef<Asset>(),
                     const ShortcutKeys& shortcut = ShortcutKeys());

        // Adds toggle item by path ("node/sub node/target")
        void AddToggleItem(const WString& path, bool value,
                           const Function<void(bool)>& clickFunc = Function<void(bool)>(),
                           const AssetRef<Asset>& icon = AssetRef<Asset>(),
                           const ShortcutKeys& shortcut = ShortcutKeys());

        // Inserts item at position
        void InsertItem(const Item& item, int position);

        // Adds array of items
        void AddItems(Vector<Item> items);

        // Inserts array of items at position
        void InsertItems(Vector<Item> items, int position);

        // Returns item at position
        Item GetItem(int position);

        // Returns array of all items
        Vector<Item> GetItems() const;

        // Removes item at position
        void RemoveItem(int position);

        // Removes item by path
        void RemoveItem(const WString& path);

        // Removes all items
        void RemoveAllItems();

        // Returns items vertical layout
        const Ref<HorizontalLayout>& GetItemsLayout() const;

        // Returns item sample
        const Ref<Widget>& GetItemSample() const;

        // Sets item sample
        void SetItemSample(const Ref<Widget>& sample);

        // Sets selection drawable
        void SetSelectionDrawable(const Ref<IRectDrawable>& drawable);

        // Returns selection drawable as sprite, null when it is a drawable of another type
        Ref<Sprite> GetSelectionDrawable() const;

        // Returns selection drawable
        const Ref<IRectDrawable>& GetSelectionRectDrawable() const;

        // Sets selection drawable layout
        void SetSelectionDrawableLayout(const Layout& layout);

        // Returns selection drawable layout
        Layout GetSelectionDrawableLayout() const;

        // Returns create menu group in editor
        static String GetCreateMenuGroup();

        SERIALIZABLE(MenuPanel);
        CLONEABLE_REF(MenuPanel);

    protected:
        Ref<HorizontalLayout> mLayout; // Items layout

        Ref<Widget> mItemSample; // Item sample @SERIALIZABLE

        Vector<Function<void()>> mClickFunctions; // Items click functions

        Ref<IRectDrawable> mSelectionDrawable; // Selection drawable @SERIALIZABLE
        Layout      mSelectionLayout;   // Selection layout, result selection area depends on selected item @SERIALIZABLE

        RectF mCurrentSelectionRect;  // Current selection rectangle (for smoothing)
        RectF mTargetSelectionRect;   // Target selection rectangle (over selected item)
        Vec2F mLastSelectCheckCursor; // Last cursor position on selection check

        int   mSelectedItem = -1;            // Index of selected item
        float mSelectSubContextTime = -1.0f; // Time to appearing selected sub context

        WeakRef<ContextMenu> mOpenedContext; // Last opened context in menu

    protected:
        // Called when visible was changed
        void OnEnabled() override;

        // Called when visible was changed
        void OnDisabled() override;

        // Creates sub context menus by path
        Ref<ContextMenu> CreateSubContext(WString& path);

        // Creates item widget
        Ref<Widget> CreateItem(const Item& item);

        // Returns item info
        Item GetItemDef(int idx) const;

        // Returns item widget under point and stores index in idxPtr, if not null
        Ref<Widget> GetItemUnderPoint(const Vec2F& point, int* idxPtr);

        // Updates hover
        void UpdateHover(const Vec2F& point);

        // Called when cursor pressed on this
        void OnCursorPressed(const Input::Cursor& cursor) override;

        // Called when cursor stay down during frame
        void OnCursorStillDown(const Input::Cursor& cursor) override;

        // Called when cursor released (only when cursor pressed this at previous time)
        void OnCursorReleased(const Input::Cursor& cursor) override;

        // Called when cursor pressing was broken (when scrolled scroll area or some other)
        void OnCursorPressBreak(const Input::Cursor& cursor) override;

        // Called when cursor moved on this (or moved outside when this was pressed)
        void OnCursorMoved(const Input::Cursor& cursor) override;

        // Called when cursor exits this object
        void OnCursorExit(const Input::Cursor& cursor) override;

        REF_COUNTERABLE_IMPL(Widget);
    };
}
// --- META ---

CLASS_BASES_META(o2::MenuPanel)
{
    BASE_CLASS(o2::Widget);
    BASE_CLASS(o2::DrawableCursorEventsListener);
}
END_META;
CLASS_FIELDS_META(o2::MenuPanel)
{
    FIELD().PROTECTED().NAME(mLayout);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().NAME(mItemSample);
    FIELD().PROTECTED().NAME(mClickFunctions);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().NAME(mSelectionDrawable);
    FIELD().PROTECTED().SERIALIZABLE_ATTRIBUTE().NAME(mSelectionLayout);
    FIELD().PROTECTED().NAME(mCurrentSelectionRect);
    FIELD().PROTECTED().NAME(mTargetSelectionRect);
    FIELD().PROTECTED().NAME(mLastSelectCheckCursor);
    FIELD().PROTECTED().DEFAULT_VALUE(-1).NAME(mSelectedItem);
    FIELD().PROTECTED().DEFAULT_VALUE(-1.0f).NAME(mSelectSubContextTime);
    FIELD().PROTECTED().NAME(mOpenedContext);
}
END_META;
CLASS_METHODS_META(o2::MenuPanel)
{

    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*);
    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*, const MenuPanel&);
    FUNCTION().PUBLIC().SIGNATURE(void, Update, float);
    FUNCTION().PUBLIC().SIGNATURE(void, Draw);
    FUNCTION().PUBLIC().SIGNATURE(Ref<Widget>, AddItem, const Item&);
    FUNCTION().PUBLIC().SIGNATURE(void, AddItem, const WString&, const Function<void()>&, const AssetRef<Asset>&, const ShortcutKeys&);
    FUNCTION().PUBLIC().SIGNATURE(void, AddToggleItem, const WString&, bool, const Function<void(bool)>&, const AssetRef<Asset>&, const ShortcutKeys&);
    FUNCTION().PUBLIC().SIGNATURE(void, InsertItem, const Item&, int);
    FUNCTION().PUBLIC().SIGNATURE(void, AddItems, Vector<Item>);
    FUNCTION().PUBLIC().SIGNATURE(void, InsertItems, Vector<Item>, int);
    FUNCTION().PUBLIC().SIGNATURE(Item, GetItem, int);
    FUNCTION().PUBLIC().SIGNATURE(Vector<Item>, GetItems);
    FUNCTION().PUBLIC().SIGNATURE(void, RemoveItem, int);
    FUNCTION().PUBLIC().SIGNATURE(void, RemoveItem, const WString&);
    FUNCTION().PUBLIC().SIGNATURE(void, RemoveAllItems);
    FUNCTION().PUBLIC().SIGNATURE(const Ref<HorizontalLayout>&, GetItemsLayout);
    FUNCTION().PUBLIC().SIGNATURE(const Ref<Widget>&, GetItemSample);
    FUNCTION().PUBLIC().SIGNATURE(void, SetItemSample, const Ref<Widget>&);
    FUNCTION().PUBLIC().SIGNATURE(void, SetSelectionDrawable, const Ref<IRectDrawable>&);
    FUNCTION().PUBLIC().SIGNATURE(Ref<Sprite>, GetSelectionDrawable);
    FUNCTION().PUBLIC().SIGNATURE(const Ref<IRectDrawable>&, GetSelectionRectDrawable);
    FUNCTION().PUBLIC().SIGNATURE(void, SetSelectionDrawableLayout, const Layout&);
    FUNCTION().PUBLIC().SIGNATURE(Layout, GetSelectionDrawableLayout);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(String, GetCreateMenuGroup);
    FUNCTION().PROTECTED().SIGNATURE(void, OnEnabled);
    FUNCTION().PROTECTED().SIGNATURE(void, OnDisabled);
    FUNCTION().PROTECTED().SIGNATURE(Ref<ContextMenu>, CreateSubContext, WString&);
    FUNCTION().PROTECTED().SIGNATURE(Ref<Widget>, CreateItem, const Item&);
    FUNCTION().PROTECTED().SIGNATURE(Item, GetItemDef, int);
    FUNCTION().PROTECTED().SIGNATURE(Ref<Widget>, GetItemUnderPoint, const Vec2F&, int*);
    FUNCTION().PROTECTED().SIGNATURE(void, UpdateHover, const Vec2F&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCursorPressed, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCursorStillDown, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCursorReleased, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCursorPressBreak, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCursorMoved, const Input::Cursor&);
    FUNCTION().PROTECTED().SIGNATURE(void, OnCursorExit, const Input::Cursor&);
}
END_META;

CLASS_BASES_META(o2::MenuPanel::Item)
{
    BASE_CLASS(o2::ISerializable);
    BASE_CLASS(o2::RefCounterable);
    BASE_CLASS(o2::ICloneableRef);
}
END_META;
CLASS_FIELDS_META(o2::MenuPanel::Item)
{
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().NAME(text);
    FIELD().PUBLIC().SERIALIZABLE_ATTRIBUTE().NAME(subItems);
    FIELD().PUBLIC().NAME(onClick);
}
END_META;
CLASS_METHODS_META(o2::MenuPanel::Item)
{

    FUNCTION().PUBLIC().CONSTRUCTOR();
    FUNCTION().PUBLIC().CONSTRUCTOR(const WString&, const Vector<Ref<ContextMenu::Item>>&);
    FUNCTION().PUBLIC().CONSTRUCTOR(const WString&, const Function<void()>);
}
END_META;
// --- END META ---
