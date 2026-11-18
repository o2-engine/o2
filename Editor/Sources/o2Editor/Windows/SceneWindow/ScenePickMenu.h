#pragma once

#include "o2/Scene/UI/Widgets/ContextMenu.h"
#include "o2/Utils/Editor/SceneEditableObject.h"

using namespace o2;

namespace Editor
{
    // ----------------------------------------------------------------------------------------------
    // An object of the scene pick menu: its depth in the menu's tree and the ancestors the tree skips
    // ----------------------------------------------------------------------------------------------
    struct ScenePickEntry
    {
        Ref<SceneEditableObject> object; // The object under the cursor
        int depth = 0;                   // The number of its ancestors that are in the menu too

        Vector<Ref<SceneEditableObject>> hiddenParents; // Its ancestors that are not in the menu, from the root down
        Vector<int>                      hiddenDepths;  // Their depths in the scene hierarchy

        bool operator==(const ScenePickEntry& other) const { return object == other.object; }
    };

    // ---------------------------------------------------------------------------------------------------------
    // Menu of the scene objects under the cursor, opened by a right click in the scene. The objects make a tree
    // of their own: the parent of an object in the menu is its nearest ancestor that is under the cursor too.
    // The object of the hovered item gets the hover outline in the scene, and the ancestors the tree skips are
    // listed by the parents menu on the left of it, where they can be hovered and picked too. A click selects
    // the object
    // ---------------------------------------------------------------------------------------------------------
    class ScenePickMenu: public ContextMenu
    {
    public:
        Function<void(const Ref<SceneEditableObject>&)> onHoverChanged; // Called with the object to outline, null when none
        Function<void(const Ref<SceneEditableObject>&)> onPicked;       // Called with the object of the clicked item

        static constexpr float indentStep = 14.0f;     // Indent of an item for each level of the menu's tree
        static constexpr float parentsMenuGap = 2.0f;  // Gap between the menu and its parents menu

    public:
        // Default constructor
        explicit ScenePickMenu(RefCounter* refCounter);

        // Constructor with the look of a context menu
        ScenePickMenu(RefCounter* refCounter, const ContextMenu& style);

        // Copy-constructor
        ScenePickMenu(RefCounter* refCounter, const ScenePickMenu& other);

        using ContextMenu::Show;

        // Fills the menu with the objects and shows it at the position
        void Show(const Vector<Ref<SceneEditableObject>>& objects, const Vec2F& position);

        // Returns the entries of the menu in the order of its items
        const Vector<ScenePickEntry>& GetEntries() const;

        // Returns the index of the entry of the hovered item, -1 when none
        int GetHoveredEntryIndex() const;

        // Hovers the item of the entry, as the cursor does; -1 hovers none
        void SetHoveredEntry(int index);

        // Picks the object of the entry, as a click on its item does, and closes the menu
        void PickEntry(int index);

        // Sets the menu that lists the skipped ancestors of the hovered item
        void SetParentsMenu(const Ref<ScenePickMenu>& menu);

        // Returns the menu of the skipped ancestors of the hovered item
        const Ref<ScenePickMenu>& GetParentsMenu() const;

        // Returns the object outlined in the scene: of the hovered item of the parents menu, else of this one's
        Ref<SceneEditableObject> GetHoverObject() const;

        // Updates the hover
        void Update(float dt) override;

        // Builds the entries for the objects: depth-first in the order of the scene hierarchy
        static Vector<ScenePickEntry> BuildEntries(const Vector<Ref<SceneEditableObject>>& objects);

        // Builds the entries of the parents menu for an entry: its skipped ancestors, indented by their depths
        static Vector<ScenePickEntry> BuildParentEntries(const ScenePickEntry& entry);

        // Returns the icon of an object in the menu: the one of its actor type, or of a widget layer
        static String GetObjectIcon(const Ref<SceneEditableObject>& object);

        // Returns create menu category in editor
        static String GetCreateMenuCategory();

        SERIALIZABLE(ScenePickMenu);
        CLONEABLE_REF(ScenePickMenu);

    protected:
        Vector<ScenePickEntry> mEntries; // The objects of the items

        int mHoveredEntry = -1; // Index of the entry of the hovered item

        Ref<ScenePickMenu> mParentsMenu;            // Menu of the skipped ancestors of the hovered item, shown on the left
        bool               mIsParentsMenu = false;  // True when this one lists the skipped ancestors for its parent menu
        WeakRef<SceneEditableObject> mHoverObject;  // Object reported by onHoverChanged last

    protected:
        // Widens the menu for the indents of the tree; the parents menu stands on the left of its parent menu, its last
        // item ending at position.y
        void FitPosition(const Vec2F& position, Vec2F size) override;

        // Clears the hover and closes the parents menu when the menu closes
        void OnDisabled() override;

        // Hides the menu with its parent; the parents menu stays for a press on its parent menu, which closes both
        // itself when its item is clicked
        void HideWithParent() override;

        // Shows the entries as the parents menu of the parent menu: its last item ends at position.y
        void ShowAsParentsMenu(const Vector<ScenePickEntry>& entries, const Ref<ScenePickMenu>& parent,
                               const Vec2F& position);

        // Builds the items of the entries and indents them by their depths
        void RebuildEntryItems();

        // Shows the parents menu for the hovered item, or hides it when the item has no skipped ancestors
        void UpdateParentsMenu();

        // Reports the object to outline when it changes
        void UpdateHoverObject();

        // Returns the index of the entry of the item under the cursor, -1 when none
        int GetEntryUnderCursor() const;
    };
}
// --- META ---

CLASS_BASES_META(Editor::ScenePickMenu)
{
    BASE_CLASS(o2::ContextMenu);
}
END_META;
CLASS_FIELDS_META(Editor::ScenePickMenu)
{
    FIELD().PUBLIC().NAME(onHoverChanged);
    FIELD().PUBLIC().NAME(onPicked);
    FIELD().PROTECTED().NAME(mEntries);
    FIELD().PROTECTED().DEFAULT_VALUE(-1).NAME(mHoveredEntry);
    FIELD().PROTECTED().NAME(mParentsMenu);
    FIELD().PROTECTED().DEFAULT_VALUE(false).NAME(mIsParentsMenu);
    FIELD().PROTECTED().NAME(mHoverObject);
}
END_META;
CLASS_METHODS_META(Editor::ScenePickMenu)
{

    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*);
    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*, const ContextMenu&);
    FUNCTION().PUBLIC().CONSTRUCTOR(RefCounter*, const ScenePickMenu&);
    FUNCTION().PUBLIC().SIGNATURE(void, Show, const Vector<Ref<SceneEditableObject>>&, const Vec2F&);
    FUNCTION().PUBLIC().SIGNATURE(const Vector<ScenePickEntry>&, GetEntries);
    FUNCTION().PUBLIC().SIGNATURE(int, GetHoveredEntryIndex);
    FUNCTION().PUBLIC().SIGNATURE(void, SetHoveredEntry, int);
    FUNCTION().PUBLIC().SIGNATURE(void, PickEntry, int);
    FUNCTION().PUBLIC().SIGNATURE(void, SetParentsMenu, const Ref<ScenePickMenu>&);
    FUNCTION().PUBLIC().SIGNATURE(const Ref<ScenePickMenu>&, GetParentsMenu);
    FUNCTION().PUBLIC().SIGNATURE(Ref<SceneEditableObject>, GetHoverObject);
    FUNCTION().PUBLIC().SIGNATURE(void, Update, float);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(Vector<ScenePickEntry>, BuildEntries, const Vector<Ref<SceneEditableObject>>&);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(Vector<ScenePickEntry>, BuildParentEntries, const ScenePickEntry&);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(String, GetObjectIcon, const Ref<SceneEditableObject>&);
    FUNCTION().PUBLIC().SIGNATURE_STATIC(String, GetCreateMenuCategory);
    FUNCTION().PROTECTED().SIGNATURE(void, FitPosition, const Vec2F&, Vec2F);
    FUNCTION().PROTECTED().SIGNATURE(void, OnDisabled);
    FUNCTION().PROTECTED().SIGNATURE(void, HideWithParent);
    FUNCTION().PROTECTED().SIGNATURE(void, ShowAsParentsMenu, const Vector<ScenePickEntry>&, const Ref<ScenePickMenu>&, const Vec2F&);
    FUNCTION().PROTECTED().SIGNATURE(void, RebuildEntryItems);
    FUNCTION().PROTECTED().SIGNATURE(void, UpdateParentsMenu);
    FUNCTION().PROTECTED().SIGNATURE(void, UpdateHoverObject);
    FUNCTION().PROTECTED().SIGNATURE(int, GetEntryUnderCursor);
}
END_META;
// --- END META ---
