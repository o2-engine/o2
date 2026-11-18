#include "o2Editor/stdafx.h"
#include "ScenePickMenu.h"

#include "o2/Application/Input.h"
#include "o2/Assets/Types/VectorImageAsset.h"
#include "o2/Render/Render.h"
#include "o2/Scene/Actor.h"
#include "o2/Scene/Scene.h"
#include "o2/Scene/UI/Widget.h"
#include "o2/Scene/UI/WidgetLayer.h"
#include "o2/Utils/Editor/EditorScope.h"
#include "o2/Utils/Editor/SceneEditableObject.h"

namespace Editor
{
    ScenePickMenu::ScenePickMenu(RefCounter* refCounter):
        ContextMenu(refCounter)
    {}

    ScenePickMenu::ScenePickMenu(RefCounter* refCounter, const ContextMenu& style):
        ContextMenu(refCounter, style)
    {
        RemoveAllItems();
        SetSearchEnabled(false);
    }

    ScenePickMenu::ScenePickMenu(RefCounter* refCounter, const ScenePickMenu& other):
        ContextMenu(refCounter, other)
    {}

    void ScenePickMenu::Show(const Vector<Ref<SceneEditableObject>>& objects, const Vec2F& position)
    {
        mHoveredEntry = -1;
        mEntries = BuildEntries(objects);

        RebuildEntryItems();
        ContextMenu::Show(position);
    }

    const Vector<ScenePickEntry>& ScenePickMenu::GetEntries() const
    {
        return mEntries;
    }

    int ScenePickMenu::GetHoveredEntryIndex() const
    {
        return mHoveredEntry;
    }

    void ScenePickMenu::SetHoveredEntry(int index)
    {
        if (index >= mEntries.Count())
            index = -1;

        if (index == mHoveredEntry)
            return;

        mHoveredEntry = index;
        UpdateParentsMenu();
        UpdateHoverObject();
    }

    void ScenePickMenu::PickEntry(int index)
    {
        if (index < 0 || index >= mEntries.Count())
            return;

        auto object = mEntries[index].object;
        HideWithChild();
        HideWithParent();
        onPicked(object);
    }

    void ScenePickMenu::SetParentsMenu(const Ref<ScenePickMenu>& menu)
    {
        mParentsMenu = menu;
        if (!menu)
            return;

        menu->mIsParentsMenu = true;
        menu->onPicked = [this](const Ref<SceneEditableObject>& object) { onPicked(object); };
    }

    const Ref<ScenePickMenu>& ScenePickMenu::GetParentsMenu() const
    {
        return mParentsMenu;
    }

    Ref<SceneEditableObject> ScenePickMenu::GetHoverObject() const
    {
        return mHoverObject.Lock();
    }

    void ScenePickMenu::Update(float dt)
    {
        ContextMenu::Update(dt);

        if (!mResEnabledInHierarchy || mIsParentsMenu)
            return;

        // Fading out after a pick or a click outside: nothing is hovered any more
        if (!mEnabled)
        {
            mHoveredEntry = -1;
            UpdateHoverObject();
            return;
        }

        // The item stays hovered while the cursor is out of the menu, so the cursor can go to the parents menu
        int hovered = GetEntryUnderCursor();
        if (hovered >= 0)
            SetHoveredEntry(hovered);

        UpdateHoverObject();
    }

    int ScenePickMenu::GetEntryUnderCursor() const
    {
        if (!mSelectedItem || !layout->IsPointInside(o2Input.GetCursorPos()))
            return -1;

        for (int i = 0; i < mItems.Count() && i < mEntries.Count(); i++)
        {
            if (mItems[i]->widget.Lock() == mSelectedItem)
                return i;
        }

        return -1;
    }

    void ScenePickMenu::UpdateHoverObject()
    {
        Ref<SceneEditableObject> object;
        if (mParentsMenu && mParentsMenu->IsEnabled())
        {
            int parentIndex = mParentsMenu->GetEntryUnderCursor();
            if (parentIndex >= 0)
                object = mParentsMenu->mEntries[parentIndex].object;
        }

        if (!object && mHoveredEntry >= 0 && mHoveredEntry < mEntries.Count())
            object = mEntries[mHoveredEntry].object;

        if (object == mHoverObject.Lock())
            return;

        mHoverObject = object;
        onHoverChanged(object);
    }

    Vector<ScenePickEntry> ScenePickMenu::BuildEntries(const Vector<Ref<SceneEditableObject>>& objects)
    {
        Vector<Ref<SceneEditableObject>> picked;
        for (auto& object : objects)
        {
            if (object && !picked.Contains(object))
                picked.Add(object);
        }

        // Actors, widgets and layers are the nodes of the scene tree; the "layers" and "internal children"
        // groups of a widget are only containers
        auto isNode = [](const Ref<SceneEditableObject>& object) {
            return DynamicCast<Actor>(object) || DynamicCast<WidgetLayer>(object);
        };

        auto rootIndex = [](const Ref<SceneEditableObject>& object) {
            if (Scene::IsSingletonInitialzed())
            {
                if (auto actor = DynamicCast<Actor>(object))
                    return o2Scene.GetRootActors().IndexOf(actor);
            }

            return -1;
        };

        struct Ordered
        {
            Vector<Ref<SceneEditableObject>> chain; // From the root down to the object itself
            Vector<int>                      key;   // Indices in the siblings along the chain
            int                              index; // Index in the objects

            bool operator==(const Ordered& other) const { return index == other.index; }
        };

        Vector<Ordered> ordered;
        for (int i = 0; i < picked.Count(); i++)
        {
            Ordered item;
            item.index = i;

            for (auto it = picked[i]; it; it = it->GetEditableParent())
                item.chain.Insert(it, 0);

            for (int k = 0; k < item.chain.Count(); k++)
            {
                if (k == 0)
                    item.key.Add(rootIndex(item.chain[k]));
                else
                    item.key.Add(item.chain[k - 1]->GetEditableChildren().IndexOf(item.chain[k]));
            }

            ordered.Add(item);
        }

        ordered.Sort([](const Ordered& a, const Ordered& b) {
            int count = Math::Min(a.key.Count(), b.key.Count());
            for (int i = 0; i < count; i++)
            {
                if (a.key[i] != b.key[i])
                    return a.key[i] < b.key[i];
            }

            if (a.key.Count() != b.key.Count())
                return a.key.Count() < b.key.Count();

            return a.index < b.index;
        });

        Vector<ScenePickEntry> result;
        for (auto& item : ordered)
        {
            ScenePickEntry entry;
            entry.object = item.chain.Last();

            int nodeDepth = 0;
            for (int k = 0; k < item.chain.Count() - 1; k++)
            {
                auto& ancestor = item.chain[k];
                if (picked.Contains(ancestor))
                    entry.depth++;
                else if (isNode(ancestor))
                {
                    entry.hiddenParents.Add(ancestor);
                    entry.hiddenDepths.Add(nodeDepth);
                }

                if (isNode(ancestor))
                    nodeDepth++;
            }

            result.Add(entry);
        }

        return result;
    }

    Vector<ScenePickEntry> ScenePickMenu::BuildParentEntries(const ScenePickEntry& entry)
    {
        int minDepth = entry.hiddenDepths.IsEmpty() ? 0 : entry.hiddenDepths[0];
        for (int depth : entry.hiddenDepths)
            minDepth = Math::Min(minDepth, depth);

        Vector<ScenePickEntry> result;
        for (int i = 0; i < entry.hiddenParents.Count(); i++)
        {
            ScenePickEntry parentEntry;
            parentEntry.object = entry.hiddenParents[i];
            parentEntry.depth = i < entry.hiddenDepths.Count() ? entry.hiddenDepths[i] - minDepth : 0;
            result.Add(parentEntry);
        }

        return result;
    }

    String ScenePickMenu::GetObjectIcon(const Ref<SceneEditableObject>& object)
    {
        if (DynamicCast<WidgetLayer>(object))
            return "ui/UI4_node_layer_icon.svg";

        // The icon of the actor's type, as the create menu shows it
        if (auto actor = DynamicCast<Actor>(object))
        {
            String icon = actor->GetType().InvokeStatic<String>("GetEditorIcon");
            if (!icon.IsEmpty())
                return icon;
        }

        return "ui/UI4_node_actor_icon.svg";
    }

    String ScenePickMenu::GetCreateMenuCategory()
    {
        return "UI/Editor";
    }

    void ScenePickMenu::FitPosition(const Vec2F& position, Vec2F size)
    {
        int maxDepth = 0;
        for (auto& entry : mEntries)
            maxDepth = Math::Max(maxDepth, entry.depth);

        size.x += indentStep*(float)maxDepth;

        Vec2F place = position;
        auto parent = mParentPopup.Lock();
        if (mIsParentsMenu && parent)
        {
            // On the left of the parent menu, or on its right where the left has no room. position.y is where its last
            // item ends: the ancestors go up from the hovered item, the root on top
            RectF parentRect = parent->layout->GetWorldRect();
            place.x = parentRect.left - parentsMenuGap - size.x;
            if (place.x < -(float)o2Render.GetResolution().x*0.5f)
                place.x = parentRect.right + parentsMenuGap;

            place.y = position.y + size.y - mViewAreaLayout.offsetMin.y;
        }

        ContextMenu::FitPosition(place, size);
    }

    void ScenePickMenu::OnDisabled()
    {
        ContextMenu::OnDisabled();

        if (mParentsMenu && mParentsMenu->IsEnabled())
            mParentsMenu->Hide(true);

        mHoveredEntry = -1;
        if (!mIsParentsMenu && mHoverObject.Lock())
        {
            mHoverObject = nullptr;
            onHoverChanged(nullptr);
        }
    }

    void ScenePickMenu::HideWithParent()
    {
        if (mIsParentsMenu)
        {
            // A press on the parent menu is its own: a click on its item closes both menus
            auto parent = mParentPopup.Lock();
            bool pressed = o2Input.IsCursorPressed() || o2Input.IsRightMousePressed();
            if (pressed && parent && parent->layout->IsPointInside(o2Input.GetCursorPos()))
                return;
        }

        ContextMenu::HideWithParent();
    }

    void ScenePickMenu::ShowAsParentsMenu(const Vector<ScenePickEntry>& entries, const Ref<ScenePickMenu>& parent,
                                          const Vec2F& position)
    {
        mHoveredEntry = -1;
        mEntries = entries;

        RebuildEntryItems();
        ContextMenu::Show(parent, position);
    }

    void ScenePickMenu::RebuildEntryItems()
    {
        PushEditorScopeOnStack scope;

        mItems.Clear();
        for (int i = 0; i < mEntries.Count(); i++)
        {
            auto& entry = mEntries[i];
            WString name = entry.object->GetName();
            if (name.IsEmpty())
                name = "(no name)";

            WeakRef<SceneEditableObject> object = entry.object;
            auto item = mmake<Item>(name, Function<void()>([this, object]() {
                                        if (auto locked = object.Lock())
                                            onPicked(locked);
                                    }), WString(),
                                    AssetRef<Asset>(AssetRef<VectorImageAsset>(GetObjectIcon(entry.object))));
            mItems.Add(item);
        }

        RebuildItems();

        // The whole content of an item lies on its "basic" layer, which is shifted by the depth
        for (int i = 0; i < mItems.Count(); i++)
        {
            if (auto widget = mItems[i]->widget.Lock())
            {
                if (auto basic = widget->FindLayer("basic"))
                    basic->layout = Layout::BothStretch(indentStep*(float)mEntries[i].depth, 0, 0, 0);
            }
        }
    }

    void ScenePickMenu::UpdateParentsMenu()
    {
        if (!mParentsMenu || mIsParentsMenu)
            return;

        auto hideParents = [&]() {
            if (mParentsMenu->IsEnabled())
                mParentsMenu->Hide(true);

            if (mChildPopup.Lock() == mParentsMenu)
                mChildPopup = nullptr;
        };

        if (mHoveredEntry < 0 || mHoveredEntry >= mEntries.Count() || mEntries[mHoveredEntry].hiddenParents.IsEmpty() ||
            !mResEnabledInHierarchy)
        {
            hideParents();
            return;
        }

        auto item = mHoveredEntry < mItems.Count() ? mItems[mHoveredEntry]->widget.Lock() : Ref<ContextMenuItem>();
        if (!item)
        {
            hideParents();
            return;
        }

        // The last item of the parents menu ends where the hovered one begins
        Vec2F position(layout->GetWorldRect().left, item->layout->GetWorldRect().top);
        mParentsMenu->ShowAsParentsMenu(BuildParentEntries(mEntries[mHoveredEntry]), Ref(this), position);
    }
}

DECLARE_TEMPLATE_CLASS(o2::LinkRef<Editor::ScenePickMenu>);
// --- META ---

DECLARE_CLASS(Editor::ScenePickMenu, Editor__ScenePickMenu);
// --- END META ---
